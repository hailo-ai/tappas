
################################################
# LICENSE PLATE RECOGNITION APP
################################################
lpr_source = 'license_plate_recognition.cpp'

executable('license_plate_recognition',
    lpr_source,
    install_dir: apps_install_dir + '/license_plate_recognition/resources',
    install: true,
    cpp_args : hailo_lib_args,
    include_directories: [metadata_inc] + hailo_general_inc + cxxopts_inc,
    dependencies : plugin_deps + thread_deps + [meta_dep] + [opencv_dep],
    gnu_symbol_visibility : 'default',
)

################################################
# LPR OVERLAY SOURCE
################################################
lpr_overlay_source = 'lpr_overlay.cpp'

lpr_overlay_lib = shared_library('lpr_overlay',
  lpr_overlay_source,
  cpp_args : hailo_lib_args,
  include_directories: [metadata_inc] + hailo_general_inc,
  dependencies : plugin_deps + [opencv_dep, meta_dep],
  gnu_symbol_visibility : 'default',
  install: true,
  install_dir: post_proc_install_dir,
)