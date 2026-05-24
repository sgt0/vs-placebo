#ifndef VS_PLACEBO_LIBRARY_H
#define VS_PLACEBO_LIBRARY_H

#include <pthread.h>

#include <libplacebo/dispatch.h>
#include <libplacebo/shaders/sampling.h>
#include <libplacebo/utils/upload.h>
#include <libplacebo/vulkan.h>

#include "config_vsplacebo.h"

extern pthread_mutex_t vspl_vulkan_mutex;

#define MAX_PLANES 4

struct priv {
    pl_log log;
    pl_vulkan vk;
    pl_gpu gpu;
    pl_dispatch dp;
    pl_shader_obj dither_state;

    pl_renderer rr;
    pl_tex tex_in[MAX_PLANES];
    pl_tex tex_out[MAX_PLANES];

    pl_tex el_tex[MAX_PLANES];
};

void *VSPlaceboInit(enum pl_log_level log_level);
void VSPlaceboUninit(void *priv);

#endif //VS_PLACEBO_LIBRARY_H
