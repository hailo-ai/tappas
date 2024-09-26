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

# cxxopts Include Directories
cxxopts_inc_dir = get_option('libcxxopts')
cxxopts_inc = [include_directories(cxxopts_inc_dir, is_system: true)]

# Library Arguments
hailo_lib_args = get_option('libargs')

# Apps install dir
apps_install_dir = get_option('apps_install_dir')

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

subdir('ai_example_app')
