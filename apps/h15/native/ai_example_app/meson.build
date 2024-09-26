################################################
# MEDIA LIBRARY DEPS
################################################
media_library_common_dep = dependency('hailo_media_library_common', method : 'pkg-config')
media_library_frontend_dep = dependency('hailo_media_library_frontend', method : 'pkg-config')
media_library_encoder_dep = dependency('hailo_media_library_encoder', method : 'pkg-config')
media_library_api_dep = dependency('hailo_media_library_api', method : 'pkg-config')
encoder_dep = dependency('hailo_encoder', method : 'pkg-config')
gstmedialibrary_utils_dep = dependency('gstmedialibutils', method : 'pkg-config')

dependencies_apps = gst_deps + media_library_encoder_dep + media_library_common_dep + media_library_frontend_dep + encoder_dep + media_library_api_dep + gstmedialibrary_utils_dep

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
  include_directories:  cxxopts_inc,
  dependencies : dependencies_apps + [libhailort_dep, opencv_dep, tracker_dep, tappas_general_dep, image_dep],
  gnu_symbol_visibility : 'default',
  install: true,
  install_dir: apps_install_dir + '/ai_example_app',
)

# install json config files
install_subdir('configs', install_dir : apps_install_dir + '/ai_example_app/resources')