
################################################
# POST PROCESSES PYTHON API SOURCES
################################################
python_bind_install_directory = python_package_install_dir/'tappas_postprocesses'

face_detection_post_sources = [
    '../detection/face_detection.cpp',
    'face_detection_python_api.cpp',
]

python_installation.extension_module('face_detection',
    face_detection_post_sources,
    include_directories: hailo_general_inc+rapidjson_inc+xtensor_inc + [include_directories('../')],
    dependencies : [pybind_dep],
    link_language : 'cpp',
    override_options: ['cpp_rtti=true'],
    gnu_symbol_visibility : 'default',
    install: true,
    install_dir: python_bind_install_directory,
)

yolo_post_sources = [
  '../detection/yolo_postprocess.cpp',
  '../detection/yolo_output.cpp',
  'yolo_python_api.cpp',
]

python_installation.extension_module('yolo',
    yolo_post_sources,
    include_directories: hailo_general_inc+rapidjson_inc+ [include_directories('../')],
    dependencies : [pybind_dep],
    link_language : 'cpp',
    override_options: ['cpp_rtti=true'],
    gnu_symbol_visibility : 'default',
    install: true,
    install_dir: python_bind_install_directory,
)

mspn_post_sources = [
  '../pose_estimation/mspn.cpp',
  'mspn_python_api.cpp',
]

python_installation.extension_module('mspn',
    mspn_post_sources,
    include_directories: hailo_general_inc+rapidjson_inc + xtensor_inc +[include_directories('../')],
    dependencies : [pybind_dep,opencv_dep],
    link_language : 'cpp',
    override_options: ['cpp_rtti=true'],
    gnu_symbol_visibility : 'default',
    install: true,
    install_dir: python_bind_install_directory,
)

yolov5seg_post_sources = [
  '../instance_segmentation/yolov5seg.cpp',
  'yolov5seg_python_api.cpp',
]

python_installation.extension_module('yolov5seg',
    yolov5seg_post_sources,
    include_directories: hailo_general_inc+rapidjson_inc + xtensor_inc +[include_directories('../')],
    dependencies : [pybind_dep,opencv_dep],
    link_language : 'cpp',
    override_options: ['cpp_rtti=true'],
    gnu_symbol_visibility : 'default',
    install: true,
    install_dir: python_bind_install_directory,
)