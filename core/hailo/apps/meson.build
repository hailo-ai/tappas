apps_install_dir = get_option('apps_install_dir')
install_lpr = get_option('install_lpr')
if apps_install_dir == ''
  # By default install for x86 workspace
  apps_install_dir = '/usr/lib/' + arch_dir + 'hailo/tappas/apps'
endif

thread_deps = [dependency('threads')]

################################################
# GST Image Handling
################################################
image_src = '../plugins/common/image.cpp'

image_lib = shared_library('hailo_gst_image',
  image_src,
  cpp_args : hailo_lib_args,
  include_directories: [hailo_general_inc, include_directories('../plugins')],
  dependencies : plugin_deps + [opencv_dep],
  gnu_symbol_visibility : 'default',
  version: meson.project_version(),
  install: true,
  install_dir: install_dir,
)

image_dep = declare_dependency(
  include_directories: [include_directories('../plugins/common')],
  link_with : image_lib)

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
  install_dir: install_dir,
)

cv_singleton_dep = declare_dependency(
  include_directories: [hailo_general_inc],
  link_with : cv_singleton_lib)

# App Subdirectories
if target_platform == 'x86' or target_platform == 'rpi' or target_platform == 'rockchip'
    subdir('x86')
elif target_platform == 'imx8'
    subdir('imx')
endif
