################################################
# NETWORK SWITCH APP
################################################
detection_and_depth_estimation_src = 'detection_and_depth_estimation_networks_switch.cpp'

executable('detection_and_depth_estimation_networks_switch',
  detection_and_depth_estimation_src,
  install_dir: apps_install_dir + '/network_switch/resources',
  install: true,
  cpp_args : hailo_lib_args,
  include_directories: hailo_general_inc + cxxopts_inc,
  dependencies : plugin_deps,
  gnu_symbol_visibility : 'default',
)

subdir('lpr')
