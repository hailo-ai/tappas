################################################
# LPR OVERLAY SOURCE
################################################
lpr_overlay_source = 're_id_overlay.cpp'

lpr_overlay_lib = shared_library('re_id_overlay',
  lpr_overlay_source,
  cpp_args : hailo_lib_args,
  include_directories: hailo_general_inc,
  dependencies : plugin_deps + [opencv_dep, meta_dep],
  gnu_symbol_visibility : 'default',
  install: true,
  install_dir: apps_install_dir + '/re_id',
)
