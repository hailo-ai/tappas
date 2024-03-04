################################################
# HAILO 15 CPP APPLICATIONS
################################################

frontend_update_src = ['frontend_update.cpp', '../encoder_applications/apps_common.cpp']

executable('frontend_update',
  frontend_update_src,
  cpp_args : hailo_lib_args,
  include_directories: hailo_general_inc + cxxopts_inc + [include_directories('../encoder_applications/')],
  dependencies : plugin_deps,
  gnu_symbol_visibility : 'default',
  install: true,
  install_dir: apps_install_dir + '/vision_preproc',
)