
post_proc_dep = declare_dependency(
  include_directories: [include_directories('./postprocesses/')])
hailo_post_proc_dep = [post_proc_dep]

install_subdir('post_processes_data', install_dir: post_proc_install_dir)
install_data('postprocesses/post_processes_data/w_exp_base.npy', install_dir: post_proc_install_dir+'/post_processes_data/')
install_data('postprocesses/post_processes_data/w_shp_base.npy', install_dir: post_proc_install_dir+'/post_processes_data/')

################################################
# DETECTION SOURCES - MobilenetSSD
################################################
mobilenet_ssd_post_sources = [
  'postprocesses/mobilenet_ssd_postprocess.cpp'
]

mobilenet_ssd_post_lib = shared_library('mobilenet_ssd_post',
  mobilenet_ssd_post_sources,
  cpp_args : hailo_lib_args,
  include_directories: [hailo_general_inc],
  dependencies : post_deps,
  gnu_symbol_visibility : 'default',
  install: true,
  install_dir: post_proc_install_dir,
)

################################################
# FACE DETECTION SOURCES
################################################
face_detection_post_sources = [
  'postprocesses/face_detection.cpp',
]

centerpose_post_lib = shared_library('face_detection_post',
  face_detection_post_sources,
  cpp_args : hailo_lib_args,
  include_directories: [hailo_general_inc] + xtensor_inc,
  dependencies : post_deps,
  gnu_symbol_visibility : 'default',
  install: true,
  install_dir: post_proc_install_dir,
)

################################################
# SEMANTIC SEGMENTATION SOURCES
################################################
semantic_segmentation_sources = [
  'postprocesses/semantic_segmentation.cpp'
  ]

seg_draw_lib = shared_library('semantic_segmentation',
  semantic_segmentation_sources,
  cpp_args : hailo_lib_args,
  include_directories: [hailo_general_inc] + xtensor_inc,
  dependencies : post_deps,
  gnu_symbol_visibility : 'default',
  install: true,
  install_dir: post_proc_install_dir,
)

################################################
# DEPTH ESTIMATION SOURCES
################################################
depth_estimation_sources = [
  'postprocesses/depth_estimation_postprocess.cpp',
]

depth_estimation_lib = shared_library('depth_estimation',
  depth_estimation_sources,
  cpp_args : hailo_lib_args,
  include_directories: [hailo_general_inc] + xtensor_inc,
  dependencies : post_deps,
  gnu_symbol_visibility : 'default',
  install: true,
  install_dir: post_proc_install_dir,
)


################################################
# CENTERPOSE SOURCES
################################################
centerpose_post_sources = [
  'postprocesses/centerpose_postprocess.cpp',
]

centerpose_post_lib = shared_library('centerpose_post',
  centerpose_post_sources,
  cpp_args : hailo_lib_args,
  include_directories: [hailo_general_inc] + xtensor_inc,
  dependencies : post_deps,
  gnu_symbol_visibility : 'default',
  install: true,
  install_dir: post_proc_install_dir,
)

################################################
# FACE_LANDMARKS SOURCES
################################################

facial_landmarks_post_sources = [
    'postprocesses/facial_landmarks_postprocess.cpp',
  ]

facial_landmarks_post_lib = shared_library('facial_landmarks_post',
    facial_landmarks_post_sources,
    cpp_args : hailo_lib_args,
    include_directories: [hailo_general_inc] + xtensor_inc,
    dependencies : post_deps,
    gnu_symbol_visibility : 'default',
    install: true,
    install_dir: post_proc_install_dir,
)

################################################
# YOLACT SOURCES
################################################
yolact_post_sources = [
  'postprocesses/yolact_postprocess.cpp',
]

seg_draw_lib = shared_library('yolact_post',
  yolact_post_sources,
  cpp_args : hailo_lib_args + xtensor_args,
    include_directories: [hailo_general_inc] + xtensor_inc,
    dependencies : post_deps,
  gnu_symbol_visibility : 'default',
  install: true,
  install_dir: post_proc_install_dir,
)

################################################
# CLASSIFICATION SOURCES
################################################
classification_sources = [
  'postprocesses/classification.cpp',
]

classification_post_lib = shared_library('classification',
  classification_sources,
  cpp_args : hailo_lib_args,
  include_directories: [hailo_general_inc] + xtensor_inc,
  dependencies : post_deps,
  gnu_symbol_visibility : 'default',
  install: true,
  install_dir: post_proc_install_dir,
)

################################################
# DEBUG SOURCES
################################################
debug_sources = [
  'postprocesses/debug/debug.cpp',
]

debug_lib = shared_library('debug',
  debug_sources,
  cpp_args : hailo_lib_args,
  include_directories: project_inc,
  dependencies : post_deps,
  gnu_symbol_visibility : 'default',
  install: true,
  install_dir: post_proc_install_dir,
)

################################################
# OCR SOURCES
################################################
ocr_sources = [
  'postprocesses/ocr/ocr_postprocess.cpp',
]

ocr_post_lib = shared_library('ocr_post',
  ocr_sources,
  cpp_args : hailo_lib_args,
  include_directories: [hailo_general_inc, include_directories('./postprocesses/')] + xtensor_inc,
  dependencies : post_deps,
  gnu_symbol_visibility : 'default',
  install: true,
  install_dir: post_proc_install_dir,
)

################################################
# DETECTION NEW API SOURCES
################################################
detection_new_api_post_sources = [
  'postprocesses/yolo/yolo_postprocess.cpp',
  'postprocesses/yolo/yolo_output.cpp',
]

new_det_post_lib = shared_library('new_yolo_post',
  detection_new_api_post_sources,
  cpp_args : hailo_lib_args,
  include_directories: [hailo_general_inc, include_directories('./postprocesses/')] + xtensor_inc,
  dependencies : post_deps,
  gnu_symbol_visibility : 'default',
  install: true,
  install_dir: post_proc_install_dir,
)

subdir('croppers')
