################################################
# REFERENCE CAMERA DEP
################################################
ref_cam_dep = dependency('reference_camera_api', method : 'pkg-config')

################################################
# HAILO 15 AI EXAMPLE APP
################################################

ai_example_app_src = ['main.cpp']

executable('ai_example_app',
  ai_example_app_src,
  cpp_args : hailo_lib_args,
  dependencies : dependencies_apps + [libhailort_dep, ref_cam_dep, tappas_general_dep, image_dep],
  gnu_symbol_visibility : 'default',
  install: true,
  install_dir: apps_install_dir + '/ai_example_app',
)

# install json config files
install_subdir('configs', install_dir : apps_install_dir + '/ai_example_app/resources')