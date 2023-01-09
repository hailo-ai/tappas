################################################
# RE-ID OVERLAY SOURCE
################################################
re_id_overlay_source = 're_id_overlay.cpp'

shared_library('re_id_overlay',
  re_id_overlay_source,
  cpp_args : hailo_lib_args,
  include_directories: hailo_general_inc,
  dependencies : plugin_deps + [opencv_dep],
  gnu_symbol_visibility : 'default',
  install: true,
  install_dir: apps_install_dir + '/re_id',
)

################################################
# RE-ID Fisheye Dewarp
################################################
re_id_dewarp_source = 're_id_dewarp.cpp'

shared_library('re_id_dewarp',
  re_id_dewarp_source,
  cpp_args : hailo_lib_args,
  include_directories: hailo_general_inc,
  dependencies : plugin_deps + [opencv_dep],
  gnu_symbol_visibility : 'default',
  install: true,
  install_dir: apps_install_dir + '/re_id',
)
