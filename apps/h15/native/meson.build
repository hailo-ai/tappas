# Project Declaration

project('gst-hailo-tools', 'c', 'cpp',
        version : '1.0.0',
        default_options : [ 'buildtype=release',
                            'c_std=c11', 'cpp_std=c++17']
       )

# Compiler Arguments
compiler = meson.get_compiler('cpp')
compiler_version = compiler.version().split('.')[0].to_int()
if compiler_version >= 9
  message('GCC >= 9.0.0 detected, applying extra arguments.')
  add_global_arguments('-Wpessimizing-move', language : 'cpp')
  add_global_arguments('-Wredundant-move', language : 'cpp')
  add_global_arguments('-fconcepts', language : 'cpp')
endif

# Pkgconfig
pkgc = import('pkgconfig')

# Gstreamer Dependencies
gst_req = '>= 1.0.0'
gst_dep = dependency('gstreamer-1.0', version : gst_req,
  fallback : ['gstreamer', 'gst_dep'])
gst_base_dep = dependency('gstreamer-base-1.0', version : gst_req,
  fallback : ['gstreamer', 'gst_base_dep'])
gst_app_dep = dependency('gstreamer-app-1.0', version : gst_req,
  fallback : ['gstreamer', 'gst_app_dep'])
gstvideo_dep = dependency('gstreamer-video-1.0', version : gst_req,
  fallback : ['gst-plugins-base', 'video_dep'])

# Plugin Dependencies
gst_deps = [gst_dep, gst_base_dep, gstvideo_dep, gst_app_dep]

# Opencv Dependencies
opencv_dep = dependency('opencv4', version : '>= 4.0', method : 'pkg-config')

# Library Arguments
hailo_lib_args = get_option('libargs')

# Apps install dir
apps_install_dir = get_option('apps_install_dir')

################################################
# HAILORT DEP
################################################

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
# MEDIA LIBRARY DEPS
################################################
media_library_common_dep = dependency('hailo_media_library_common', method : 'pkg-config')
media_library_frontend_dep = dependency('hailo_media_library_frontend', method : 'pkg-config')
media_library_encoder_dep = dependency('hailo_media_library_encoder', method : 'pkg-config')
media_library_api_dep = dependency('hailo_media_library_api', method : 'pkg-config')
encoder_dep = dependency('hailo_encoder', method : 'pkg-config')
gstmedialibrary_utils_dep = dependency('gstmedialibutils', method : 'pkg-config')
gyro_lib_dep = dependency('hailo_media_library_gyro', method : 'pkg-config')

dependencies_apps = gst_deps + media_library_encoder_dep + media_library_common_dep + media_library_frontend_dep + encoder_dep + media_library_api_dep + gstmedialibrary_utils_dep + gyro_lib_dep

target = get_option('target')

if target == 'all'
  subdir('reference_camera_api')
  subdir('ai_example_app')
elif target == 'api'
  subdir('reference_camera_api')
elif target == 'apps'
  subdir('ai_example_app')
endif
