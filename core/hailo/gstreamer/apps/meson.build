apps_install_dir = get_option('apps_install_dir')
target_platform = get_option('target_platform')
if apps_install_dir == ''
  # By default install for x86 workspace
  apps_install_dir = tappas_workspace + '/apps/gstreamer/' + target_platform + '/'
  cxxopts_inc_dir = '../' + cxxopts_inc_dir
endif

thread_deps = [dependency('threads')]

# cxxopts Include Directories
cxxopts_inc = [include_directories(cxxopts_inc_dir, is_system: true)]

# App Subdirectories
if target_platform == 'x86'
    subdir('x86')
elif target_platform == 'arm'
    subdir('arm')
endif
