
encoder_sources = [
    'hailoencoder.c',
    'gopconfig.c',
    'gsthailoenc.c'
]

################################################
# Hailo Gstreamer Shared Library
################################################
shared_library('gsthailoencoder',
    encoder_sources,
    cpp_args : hailo_lib_args + common_args,
    c_args : hailo_lib_args + common_args + sysroot_arg,
    link_args : sysroot_arg + ['-lhantro_vc8000e', '-lm'],
    include_directories: [include_directories('../')],
    dependencies : plugin_deps,
    gnu_symbol_visibility : 'default',
    version: meson.project_version(),
    install: true,
    install_dir: get_option('libdir') + '/gstreamer-1.0/',
)
