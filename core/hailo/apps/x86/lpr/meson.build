################################################
# LPR OVERLAY SOURCE
################################################
lpr_overlay_source = 'lpr_overlay.cpp'

lpr_overlay_lib = shared_library('lpr_overlay',
  lpr_overlay_source,
  cpp_args : hailo_lib_args,
  include_directories: hailo_general_inc,
  dependencies : plugin_deps + [opencv_dep, meta_dep, cv_singleton_dep, image_dep],
  gnu_symbol_visibility : 'default',
  install: true,
  install_dir: apps_install_dir + '/license_plate_recognition',
)

################################################
# LPR OCR SINK SOURCE
################################################
lpr_ocrsink_source = 'lpr_ocrsink.cpp'

lpr_ocrsink_lib = shared_library('lpr_ocrsink',
  lpr_ocrsink_source,
  cpp_args : hailo_lib_args,
  include_directories: hailo_general_inc,
  dependencies : plugin_deps + [opencv_dep, meta_dep, cv_singleton_dep, image_dep, tracker_dep],
  gnu_symbol_visibility : 'default',
  install: true,
  install_dir: apps_install_dir + '/license_plate_recognition',
)
