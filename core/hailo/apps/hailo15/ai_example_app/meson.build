# Hailort Arguments
sysroot_arg = []
if meson.is_cross_build()
    cmd_array = meson.get_compiler('cpp').cmd_array()
    foreach arg : cmd_array
        if arg.startswith('--sysroot=')
            sysroot = arg.split('=')[1]
            break
        endif
    endforeach
    message('Cross building using sysroot: ' + sysroot)
    if sysroot != ''
        sysroot_arg += '--sysroot=' + sysroot
    endif
endif

libhailort_dep = [meson.get_compiler('cpp').find_library('hailort', required: false)]

################################################
# HAILO 15 AI EXAMPLE APP
################################################

ai_example_app_src = ['main.cpp']

executable('ai_example_app',
  ai_example_app_src,
  cpp_args : hailo_lib_args,
  include_directories: hailo_general_inc + cxxopts_inc + [include_directories('../'), include_directories('../../../plugins/')],
  dependencies : dependencies_apps + [libhailort_dep, opencv_dep, tracker_dep],
  gnu_symbol_visibility : 'default',
  install: true,
  install_dir: apps_install_dir + '/ai_example_app',
)

# install json config files
install_subdir('configs', install_dir : apps_install_dir + '/ai_example_app/resources')