################################################
# HAILO 15 CPP APPLICATIONS
################################################

output_path = '/encoder_pipelines_new_api/'

include_encoder_app =  hailo_general_inc + cxxopts_inc + [include_directories('../')]
encoder_appsink_tee_src = ['encoder_appsink_tee.cpp', '../apps_common.cpp']

executable('encoder_appsink_tee',
  encoder_appsink_tee_src,
  cpp_args : hailo_lib_args,
  include_directories: include_encoder_app,
  dependencies : dependencies_apps,
  gnu_symbol_visibility : 'default',
  install: true,
  install_dir: apps_install_dir + output_path,
)

encoder_change_bitrate_src = ['encoder_change_bitrate.cpp', '../apps_common.cpp']

executable('encoder_change_bitrate',
  encoder_change_bitrate_src,
  cpp_args : hailo_lib_args,
  include_directories: include_encoder_app,
  dependencies : dependencies_apps,
  gnu_symbol_visibility : 'default',
  install: true,
  install_dir: apps_install_dir + output_path,
)

encoder_change_framerate_src = ['encoder_change_framerate.cpp', '../apps_common.cpp']

executable('encoder_change_framerate',
  encoder_change_framerate_src,
  cpp_args : hailo_lib_args,
  include_directories: include_encoder_app,
  dependencies : dependencies_apps,
  gnu_symbol_visibility : 'default',
  install: true,
  install_dir: apps_install_dir + output_path,
)

encoder_change_gop_src = ['encoder_change_gop.cpp', '../apps_common.cpp']

executable('encoder_change_gop',
  encoder_change_gop_src,
  cpp_args : hailo_lib_args,
  include_directories: include_encoder_app,
  dependencies : dependencies_apps,
  gnu_symbol_visibility : 'default',
  install: true,
  install_dir: apps_install_dir + output_path,
)

encoder_change_gop_size_src = ['encoder_change_gop_size.cpp', '../apps_common.cpp']

executable('encoder_change_gop_size',
  encoder_change_gop_size_src,
  cpp_args : hailo_lib_args,
  include_directories: include_encoder_app,
  dependencies : dependencies_apps,
  gnu_symbol_visibility : 'default',
  install: true,
  install_dir: apps_install_dir + output_path,
)

encoder_change_qp_src = ['encoder_change_qp.cpp', '../apps_common.cpp']

executable('encoder_change_qp',
  encoder_change_qp_src,
  cpp_args : hailo_lib_args,
  include_directories: include_encoder_app,
  dependencies : dependencies_apps,
  gnu_symbol_visibility : 'default',
  install: true,
  install_dir: apps_install_dir + output_path,
)

encoder_change_roi_src = ['encoder_change_roi.cpp', '../apps_common.cpp']

executable('encoder_change_roi',
  encoder_change_roi_src,
  cpp_args : hailo_lib_args,
  include_directories: include_encoder_app,
  dependencies : dependencies_apps,
  gnu_symbol_visibility : 'default',
  install: true,
  install_dir: apps_install_dir + output_path,
)

encoder_force_keyframe_src = ['encoder_force_keyframe.cpp', '../apps_common.cpp']

executable('encoder_force_keyframe',
  encoder_force_keyframe_src,
  cpp_args : hailo_lib_args,
  include_directories: include_encoder_app,
  dependencies : dependencies_apps,
  gnu_symbol_visibility : 'default',
  install: true,
  install_dir: apps_install_dir + output_path,
)