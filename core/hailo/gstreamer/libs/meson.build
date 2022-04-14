################################################
# DETECTION DRAWING
################################################
# sources used to compile this plug-in
detection_draw_sources = [
  'postprocesses/detection_draw.cpp',
]

det_draw_lib = shared_library('detection_draw',
  detection_draw_sources,
  cpp_args : hailo_lib_args,
  include_directories: project_inc + xtensor_inc,
  dependencies : plugin_deps + [opencv_dep] + hailo_deps,
  gnu_symbol_visibility : 'default',
)

################################################
# DETECTION SOURCES
################################################
detection_post_sources = [
  'postprocesses/yolo_postprocess.cpp',
  'postprocesses/common/detection.cpp',
  'postprocesses/yolo_output.cpp',
]

det_post_lib = shared_library('yolo_post',
  detection_post_sources,
  cpp_args : hailo_lib_args,
  include_directories: project_inc + xtensor_inc,
  dependencies : plugin_deps + hailo_deps ,
  gnu_symbol_visibility : 'default',
)

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
)

################################################
# SEGMENTATION SOURCES
################################################
segmentation_draw_sources = [
  'postprocesses/segmentation_draw.cpp',
]

seg_draw_lib = shared_library('segmentation_draw',
  segmentation_draw_sources,
  cpp_args : hailo_lib_args,
  include_directories: project_inc,
  dependencies : plugin_deps + hailo_deps,
  gnu_symbol_visibility : 'default',
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
  include_directories: project_inc + xtensor_inc,
  dependencies : plugin_deps + [opencv_dep] + hailo_deps,
  gnu_symbol_visibility : 'default',
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
  include_directories: project_inc + xtensor_inc,
  dependencies : plugin_deps + hailo_deps,
  gnu_symbol_visibility : 'default',
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
)

################################################
# INSTANCE SEGMENTATION SOURCES
################################################
yolact_post_sources = [
  'postprocesses/yolact_postprocess.cpp',
]

seg_draw_lib = shared_library('yolact_post',
  yolact_post_sources,
  cpp_args : hailo_lib_args + xtensor_args,
  include_directories: project_inc + xtensor_inc,
  dependencies : plugin_deps + hailo_deps,
  gnu_symbol_visibility : 'default',
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
  dependencies : post_deps + hailo_deps,
  gnu_symbol_visibility : 'default',
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

)

################################################
# DETECTION_COUNTER SOURCES
################################################
detection_counter_post_sources = [
  'postprocesses/detection_counter.cpp',
  'postprocesses/detection_counter.hpp',
]

detection_counter_post_lib = shared_library('detection_counter_draw',
  detection_counter_post_sources,
  cpp_args : hailo_lib_args,
  include_directories: project_inc,
  dependencies : plugin_deps + [opencv_dep] + hailo_deps,
  gnu_symbol_visibility : 'default',
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
  dependencies : post_deps + hailo_deps,
  gnu_symbol_visibility : 'default',
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
)

################################################
# LPR OVERLAY SOURCES
################################################
lpr_overlay_sources = [
  'postprocesses/lpr_overlay/lpr_overlay.cpp',
]

lpr_overlay_lib = shared_library('lpr_overlay',
  lpr_overlay_sources,
  cpp_args : hailo_lib_args,
  include_directories: project_inc,
  dependencies : plugin_deps + [opencv_dep] + hailo_deps,
  gnu_symbol_visibility : 'default',
)

subdir('apps')
subdir('croppers')
