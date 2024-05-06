project('vs-placebo', ['c', 'cpp'],
  default_options: {
    'buildtype': 'release',
    'b_ndebug': 'if-release',
    'c_std': 'c17',
    'cpp_std': 'c++20',
  },
  meson_version: '>=1.4.0',
  version: '3.0.0'
)

win32 = host_machine.system() == 'windows' or host_machine.system() == 'cygwin'
is_static = get_option('default_library') == 'static'

vapoursynth_dep = dependency('vapoursynth', static: is_static).partial_dependency(includes: true, compile_args: true)
dovi = dependency('dovi', required: false, static: is_static)

use_dovi = dovi.found()

config_vsplacebo = configuration_data()
config_vsplacebo.set('HAVE_DOVI', use_dovi)

configure_file(
  output: 'config_vsplacebo.h',
  configuration: config_vsplacebo,
)

placebo = dependency('libplacebo',
  static: is_static,
  fallback: ['libplacebo', 'libplacebo'],
  default_options: {
    'default_library': is_static ? 'static' : 'both',
    'prefer_static': is_static,
    'demos': false,
    'tests': false,
    'dovi': 'enabled',
    'lcms': 'enabled',
    'libdovi': use_dovi ? 'enabled' : 'disabled',
    'shaderc': 'enabled',
    'vulkan': 'enabled',
    'xxhash': 'enabled'
  }
)

p2p = static_library('p2p', ['libp2p/p2p_api.cpp', 'libp2p/v210.cpp'], pic: true)

sources = []

subdir('src')

shared_module('vs_placebo', sources,
  dependencies: [placebo, vapoursynth_dep, dovi],
  link_with: [p2p],
  name_prefix: 'lib',
  install_dir: join_paths(vapoursynth_dep.get_variable(pkgconfig: 'libdir'), 'vapoursynth'),
  install: true
)
