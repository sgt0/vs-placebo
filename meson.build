project('vs-placebo', ['c', 'cpp'],
  default_options: ['buildtype=release', 'b_ndebug=if-release', 'c_std=c17', 'cpp_std=c++20'],
  meson_version: '>=1.4.0',
  version: '3.0.2'
)

cc = meson.get_compiler('c')
win32 = host_machine.system() == 'windows' or host_machine.system() == 'cygwin'

if win32 and get_option('win32-overrides')
  # Use static versions of these libraries
  shaderc_combined = dependency('shaderc_combined')
  meson.override_dependency('shaderc', shaderc_combined)
  spirv_cross_c = dependency('spirv-cross-c')
  meson.override_dependency('spirv-cross-c-shared', spirv_cross_c)
endif

if cc.get_argument_syntax() == 'gcc'
  add_project_arguments(
    '-Werror-implicit-function-declaration',
    '-D_FORTIFY_SOURCE=2',
    language: 'c'
  )
endif

placebo = dependency('libplacebo', required: true, version: '>=5.229.0', static: win32)
vapoursynth_dep = dependency('vapoursynth', static: win32).partial_dependency(includes: true, compile_args: true)
dovi = dependency('dovi', required: false, static: win32)

use_dovi = dovi.found()

config_vsplacebo = configuration_data()
config_vsplacebo.set('HAVE_DOVI', use_dovi)

configure_file(
  output: 'config_vsplacebo.h',
  configuration: config_vsplacebo,
)

p2p = static_library('p2p', ['libp2p/p2p_api.cpp', 'libp2p/v210.cpp'], pic: true)

sources = []

subdir('src')

shared_module('vs_placebo', sources,
  dependencies: [dependency('threads'), placebo, vapoursynth_dep, dovi],
  link_with: [p2p],
  name_prefix: 'lib',
  install_dir : join_paths(vapoursynth_dep.get_variable(pkgconfig: 'libdir'), 'vapoursynth'),
  install: true
)
