#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#include <VapourSynth4.h>

#include "vs-placebo.h"
#include "deband.h"
#include "tonemap.h"
#include "resample.h"
#include "shader.h"

pthread_mutex_t vspl_vulkan_mutex = PTHREAD_MUTEX_INITIALIZER;

static pl_log vspl_log = NULL;
static pl_vulkan vspl_vk = NULL;
static pl_gpu vspl_gpu = NULL;
static int vspl_refcount = 0;

void *VSPlaceboInit(enum pl_log_level log_level) {
    struct priv *p = calloc(1, sizeof(struct priv));
    if (!p)
        return NULL;

    pthread_mutex_lock(&vspl_vulkan_mutex);

    if (vspl_refcount == 0) {
        vspl_log = pl_log_create(PL_API_VER, pl_log_params(
            .log_cb = pl_log_color,
            .log_level = log_level
        ));

        if (!vspl_log) {
            fprintf(stderr, "Failed initializing libplacebo\n");
            pthread_mutex_unlock(&vspl_vulkan_mutex);
            goto error;
        }

        struct pl_vulkan_params vp = pl_vulkan_default_params;
        struct pl_vk_inst_params ip = pl_vk_inst_default_params;
        vp.allow_software = true;
        vp.instance_params = &ip;
        vspl_vk = pl_vulkan_create(vspl_log, &vp);

        if (!vspl_vk) {
            fprintf(stderr, "Failed creating vulkan context\n");
            pl_log_destroy(&vspl_log);
            pthread_mutex_unlock(&vspl_vulkan_mutex);
            goto error;
        }

        vspl_gpu = vspl_vk->gpu;
    }

    vspl_refcount++;
    p->log = vspl_log;
    p->vk = vspl_vk;
    p->gpu = vspl_gpu;

    p->dp = pl_dispatch_create(p->log, p->gpu);
    if (!p->dp) {
        fprintf(stderr, "Failed creating shader dispatch object\n");
        vspl_refcount--;
        if (vspl_refcount == 0) {
            pl_vulkan_destroy(&vspl_vk);
            pl_log_destroy(&vspl_log);
            vspl_gpu = NULL;
        }
        pthread_mutex_unlock(&vspl_vulkan_mutex);
        goto error;
    }

    p->rr = pl_renderer_create(p->log, p->gpu);
    if (!p->rr) {
        fprintf(stderr, "Failed creating renderer\n");
        pl_dispatch_destroy(&p->dp);
        vspl_refcount--;
        if (vspl_refcount == 0) {
            pl_vulkan_destroy(&vspl_vk);
            pl_log_destroy(&vspl_log);
            vspl_gpu = NULL;
        }
        pthread_mutex_unlock(&vspl_vulkan_mutex);
        goto error;
    }

    pthread_mutex_unlock(&vspl_vulkan_mutex);
    return p;

error:
    free(p);
    return NULL;
}

void VSPlaceboUninit(void *priv)
{
    struct priv *p = priv;

    pthread_mutex_lock(&vspl_vulkan_mutex);

    for (int i = 0; i < MAX_PLANES; i++) {
        if (p->tex_in[i])
            pl_tex_destroy(p->gpu, &p->tex_in[i]);
        if (p->tex_out[i])
            pl_tex_destroy(p->gpu, &p->tex_out[i]);
        if (p->el_tex[i])
            pl_tex_destroy(p->gpu, &p->el_tex[i]);
    }

    pl_renderer_destroy(&p->rr);
    pl_shader_obj_destroy(&p->dither_state);
    pl_dispatch_destroy(&p->dp);

    vspl_refcount--;
    if (vspl_refcount == 0) {
        pl_vulkan_destroy(&vspl_vk);
        pl_log_destroy(&vspl_log);
        vspl_gpu = NULL;
    }

    pthread_mutex_unlock(&vspl_vulkan_mutex);

    free(p);
}

VS_EXTERNAL_API(void) VapourSynthPluginInit2(VSPlugin *plugin, const VSPLUGINAPI *vspapi) {
    vspapi->configPlugin(
        "com.vs.placebo",
        "placebo",
        "libplacebo plugin for VapourSynth",
        VS_MAKE_VERSION(2, 0),
        VAPOURSYNTH_API_VERSION,
        0,
        plugin
    );
    vspapi->registerFunction("Deband", "clip:vnode;planes:int:opt;iterations:int:opt;threshold:float:opt;"
                           "radius:float:opt;grain:float:opt;dither:int:opt;dither_algo:int:opt;"
                           "log_level:int:opt;", "clip:vnode;", VSPlaceboDebandCreate, 0, plugin);

    vspapi->registerFunction("Resample", "clip:vnode;width:int;height:int;filter:data:opt;clamp:float:opt;blur:float:opt;"
                             "taper:float:opt;radius:float:opt;param1:float:opt;param2:float:opt;"
                             "src_width:float:opt;src_height:float:opt;sx:float:opt;sy:float:opt;antiring:float:opt;"
                             "sigmoidize:int:opt;sigmoid_center:float:opt;sigmoid_slope:float:opt;linearize:int:opt;trc:int:opt;"
                             "min_luma:float:opt;"
                             "log_level:int:opt;", "clip:vnode;", VSPlaceboResampleCreate, 0, plugin);

    vspapi->registerFunction("Tonemap",
                             "clip:vnode;"
                             "src_csp:int:opt;dst_csp:int:opt;"
                             "dst_prim:int:opt;"
                             "src_max:float:opt;src_min:float:opt;"
                             "dst_max:float:opt;dst_min:float:opt;"
                             "dynamic_peak_detection:int:opt;smoothing_period:float:opt;"
                             "scene_threshold_low:float:opt;scene_threshold_high:float:opt;"
                             "percentile:float:opt;"
                             "gamut_mapping:int:opt;"
                             "tone_mapping_function:int:opt;tone_mapping_function_s:data:opt;"
                             "tone_mapping_param:float:opt;"
                             "metadata:int:opt;"
                             "use_dovi:int:opt;"
                             "dovi_el:vnode:opt;"
                             "visualize_lut:int:opt;show_clipping:int:opt;"
                             "contrast_recovery:float:opt;"
                             "log_level:int:opt;",
                             "clip:vnode;", VSPlaceboTMCreate, 0, plugin);

    vspapi->registerFunction("Shader", "clip:vnode;shader:data:opt;width:int:opt;height:int:opt;chroma_loc:int:opt;matrix:int:opt;trc:int:opt;"
                           "linearize:int:opt;sigmoidize:int:opt;sigmoid_center:float:opt;sigmoid_slope:float:opt;"
                           "antiring:float:opt;"
                           "filter:data:opt;clamp:float:opt;blur:float:opt;taper:float:opt;radius:float:opt;"
                           "param1:float:opt;param2:float:opt;shader_s:data:opt;"
                           "log_level:int:opt;", "clip:vnode;", VSPlaceboShaderCreate, 0, plugin);
}
