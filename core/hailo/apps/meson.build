apps_install_dir = get_option('apps_install_dir')
target_platform = get_option('target_platform')
install_lpr = get_option('install_lpr')
if apps_install_dir == ''
  # By default install for x86 workspace
  apps_install_dir = tappas_workspace + '/apps/h8/gstreamer/libs/apps/'
endif

thread_deps = [dependency('threads')]

################################################
# GST Image Handling
################################################
image_dep = dependency('tappas_image_utils', method : 'pkg-config')

################################################
# Tracker Dep
################################################
tracker_dep = dependency('hailo_tracker', method : 'pkg-config')

################################################
# General Dep
################################################
tappas_general_dep = dependency('hailo_tappas_general', method : 'pkg-config')

################################################
# CV Singleton
################################################
cv_singleton_src = '../general/hailo_cv_singleton.cpp'

cv_singleton_lib = shared_library('hailo_cv_singleton',
  cv_singleton_src,
  cpp_args : hailo_lib_args,
  include_directories: [hailo_general_inc],
  dependencies : [opencv_dep, image_dep, plugin_deps],
  gnu_symbol_visibility : 'default',
  version: meson.project_version(),
  install: true,
  install_dir: get_option('libdir'),
)

cv_singleton_dep = declare_dependency(
  include_directories: [hailo_general_inc],
  link_with : cv_singleton_lib)

# App Subdirectories
if target_platform == 'x86' or target_platform == 'rpi' or target_platform == 'rockchip'
    subdir('x86')
elif target_platform == 'imx8'
    subdir('imx')
elif target_platform == 'hailo15'
    subdir('hailo15')
endif
