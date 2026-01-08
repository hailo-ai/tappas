
################################################
# hailo_opencv_utils Dynamic Library
# When static_opencv=true: uses static OpenCV (core + imgproc + calib3d)
# When static_opencv=false: uses dynamic OpenCV (default)
################################################

hailo_opencv_utils_sources = [
    'hailomat.cpp',
    'overlay.cpp',
    'opencv_utils.cpp',
]
hailo_opencv_utils_lib = shared_library('hailo_opencv_utils',
    hailo_opencv_utils_sources,
    cpp_args : hailo_lib_args + common_args + opencv_helper_extra_cpp_args,
    c_args : hailo_lib_args + common_args + opencv_helper_extra_cpp_args,
    link_args : opencv_helper_extra_link_args,
    include_directories: [hailo_general_inc, xtensor_inc],
    dependencies : [opencv_dep, meta_dep],
    gnu_symbol_visibility : 'default',
    version: meson.project_version(),
    install: true,
    install_dir: install_dir,
)

hailo_opencv_utils_dep = declare_dependency(
    link_with: hailo_opencv_utils_lib,
    include_directories: hailo_general_inc,
)