################################################
# DEBUG SOURCES
################################################
debug_sources = [
    'debug.cpp',
]

shared_library('debug',
    debug_sources,
    cpp_args : hailo_lib_args,
    include_directories: hailo_general_inc + xtensor_inc,
    dependencies : post_deps,
    gnu_symbol_visibility : 'default',
    install: true,
    install_dir: post_proc_install_dir,
)

set_stream_id_sources = [
    'set_stream_id.cpp',
]

shared_library('stream_id_tool',
    set_stream_id_sources,
    cpp_args : hailo_lib_args,
    include_directories: hailo_general_inc,
    dependencies : post_deps,
    gnu_symbol_visibility : 'default',
    install: true,
    install_dir: post_proc_install_dir,
)
