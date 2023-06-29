install_subdir('post_processes_data', install_dir: post_proc_install_dir)

################################################
# DETECTION SOURCES - MobilenetSSD
################################################
mobilenet_ssd_post_sources = [
    'detection/mobilenet_ssd.cpp'
]

shared_library('mobilenet_ssd_post',
    mobilenet_ssd_post_sources,
    cpp_args : hailo_lib_args,
    include_directories: [hailo_general_inc, include_directories('./')],
    dependencies : post_deps,
    gnu_symbol_visibility : 'default',
    install: true,
    install_dir: post_proc_install_dir,
)

################################################
# DETECTION SOURCES - YOLOV5M
################################################
yolo_hailortpp_sources = [
    'detection/yolo_hailortpp.cpp'
]

shared_library('yolo_hailortpp_post',
    yolo_hailortpp_sources,
    cpp_args : hailo_lib_args,
    include_directories: [hailo_general_inc, include_directories('./')],
    dependencies : post_deps,
    gnu_symbol_visibility : 'default',
    install: true,
    install_dir: post_proc_install_dir,
)

################################################
# FACE DETECTION SOURCES
################################################
face_detection_post_sources = [
    'detection/face_detection.cpp',
]

shared_library('face_detection_post',
    face_detection_post_sources,
    cpp_args : hailo_lib_args,
    include_directories: [hailo_general_inc, include_directories('./'), rapidjson_inc] + xtensor_inc,
    dependencies : post_deps,
    gnu_symbol_visibility : 'default',
    install: true,
    install_dir: post_proc_install_dir,
)

################################################
# SCRFD DETECTION SOURCES
################################################
scrfd_post_sources = [
    'detection/scrfd.cpp',
]

shared_library('scrfd_post',
    scrfd_post_sources,
    cpp_args : hailo_lib_args,
    include_directories: [hailo_general_inc, include_directories('./'), rapidjson_inc] + xtensor_inc,
    dependencies : post_deps,
    gnu_symbol_visibility : 'default',
    install: true,
    install_dir: post_proc_install_dir,
)

################################################
# NANO DETECTION SOURCES
################################################
nanodet_post_sources = [
    'detection/nanodet.cpp',
]

shared_library('nanodet_post',
    nanodet_post_sources,
    cpp_args : hailo_lib_args,
    include_directories: [hailo_general_inc, include_directories('./')] + xtensor_inc,
    dependencies : post_deps,
    gnu_symbol_visibility : 'default',
    install: true,
    install_dir: post_proc_install_dir,
)

################################################
# SEMANTIC SEGMENTATION SOURCES
################################################
semantic_segmentation_sources = [
    'semantic_segmentation/semantic_segmentation.cpp'
]

shared_library('semantic_segmentation',
    semantic_segmentation_sources,
    cpp_args : hailo_lib_args,
    include_directories: [hailo_general_inc, include_directories('./')] + xtensor_inc,
    dependencies : post_deps,
    gnu_symbol_visibility : 'default',
    install: true,
    install_dir: post_proc_install_dir,
)

################################################
# DEPTH ESTIMATION SOURCES
################################################
depth_estimation_sources = [
    'depth_estimation/depth_estimation.cpp',
]

shared_library('depth_estimation',
    depth_estimation_sources,
    cpp_args : hailo_lib_args,
    include_directories: [hailo_general_inc, include_directories('./')] + xtensor_inc,
    dependencies : post_deps,
    gnu_symbol_visibility : 'default',
    install: true,
    install_dir: post_proc_install_dir,
)


################################################
# CENTERPOSE SOURCES
################################################
centerpose_post_sources = [
    'pose_estimation/centerpose.cpp',
]

shared_library('centerpose_post',
    centerpose_post_sources,
    cpp_args : hailo_lib_args,
    include_directories: [hailo_general_inc, include_directories('./')] + xtensor_inc,
    dependencies : post_deps,
    gnu_symbol_visibility : 'default',
    install: true,
    install_dir: post_proc_install_dir,
)

################################################
# MSPN SOURCES
################################################
mspn_post_sources = [
    'pose_estimation/mspn.cpp',
]

shared_library('mspn_post',
    mspn_post_sources,
    cpp_args : hailo_lib_args,
    include_directories: [hailo_general_inc, include_directories('./')] + xtensor_inc + rapidjson_inc,
    dependencies : post_deps + [opencv_dep],
    gnu_symbol_visibility : 'default',
    install: true,
    install_dir: post_proc_install_dir,
)

################################################
# FACIAL LANDMARKING SOURCES
################################################

facial_landmarks_post_sources = [
    'facial_landmarking/tddfa_mobilenet.cpp',
]

shared_library('facial_landmarks_post',
    facial_landmarks_post_sources,
    cpp_args : hailo_lib_args,
    include_directories: [hailo_general_inc, include_directories('./')] + xtensor_inc,
    dependencies : post_deps,
    gnu_symbol_visibility : 'default',
    install: true,
    install_dir: post_proc_install_dir,
)

################################################
# YOLACT SOURCES
################################################
yolact_post_sources = [
    'instance_segmentation/yolact.cpp',
]

shared_library('yolact_post',
    yolact_post_sources,
    cpp_args : hailo_lib_args,
    include_directories: [hailo_general_inc, include_directories('./')] + xtensor_inc,
    dependencies : post_deps,
    gnu_symbol_visibility : 'default',
    install: true,
    install_dir: post_proc_install_dir,
)

################################################
# YOLOV5SEG SOURCES
################################################
yolov5seg_post_sources = [
    'instance_segmentation/yolov5seg.cpp',
]

shared_library('yolov5seg_post',
    yolov5seg_post_sources,
    cpp_args : hailo_lib_args + ['-pthread'],
    include_directories: [hailo_general_inc, include_directories('./')] + xtensor_inc + rapidjson_inc,
    dependencies : post_deps + [dependency('threads')],
    gnu_symbol_visibility : 'default',
    install: true,
    install_dir: post_proc_install_dir,
)

################################################
# CLASSIFICATION SOURCES
################################################
classification_sources = [
    'classification/classification.cpp',
]

shared_library('classification',
    classification_sources,
    cpp_args : hailo_lib_args,
    include_directories: [hailo_general_inc, include_directories('./')] + xtensor_inc,
    dependencies : post_deps,
    gnu_symbol_visibility : 'default',
    install: true,
    install_dir: post_proc_install_dir,
)

################################################
# OCR SOURCES
################################################
ocr_sources = [
    'ocr/ocr_postprocess.cpp',
]

shared_library('ocr_post',
    ocr_sources,
    cpp_args : hailo_lib_args,
    include_directories: [hailo_general_inc, include_directories('./')] + xtensor_inc,
    dependencies : post_deps,
    gnu_symbol_visibility : 'default',
    install: true,
    install_dir: post_proc_install_dir,
)

################################################
# DETECTION YOLO SOURCES
################################################
detection_new_api_post_sources = [
  'detection/yolo_postprocess.cpp',
  'detection/yolo_output.cpp',
]

shared_library('yolo_post',
    detection_new_api_post_sources,
    cpp_args : hailo_lib_args,
    include_directories: [hailo_general_inc, include_directories('./')] + xtensor_inc + rapidjson_inc,
    dependencies : post_deps,
    gnu_symbol_visibility : 'default',
    install: true,
    install_dir: post_proc_install_dir,
)

################################################
# RE-ID SOURCES
################################################
re_id_sources = [
    'recognition/repvgg.cpp',
]

shared_library('re_id',
    re_id_sources,
    cpp_args : hailo_lib_args,
    include_directories: hailo_general_inc + xtensor_inc + [include_directories('./')],
    dependencies : post_deps,
    gnu_symbol_visibility : 'default',
    install: true,
    install_dir: post_proc_install_dir,
)

################################################
# FACE ATTRIBUTES SOURCES
################################################
face_attributes_sources = [
    'classification/face_attributes.cpp',
]

shared_library('face_attributes_post',
    face_attributes_sources,
    cpp_args : hailo_lib_args,
    include_directories: [hailo_general_inc, include_directories('./')] + xtensor_inc,
    dependencies : post_deps + [tracker_dep],
    gnu_symbol_visibility : 'default',
    install: true,
    install_dir: post_proc_install_dir,
)

################################################
# PERSON ATTRIBUTES SOURCES
################################################
person_attributes_sources = [
    'classification/person_attributes.cpp',
]

shared_library('person_attributes_post',
    person_attributes_sources,
    cpp_args : hailo_lib_args,
    include_directories: [hailo_general_inc, include_directories('./')] + xtensor_inc,
    dependencies : post_deps + [tracker_dep],
    gnu_symbol_visibility : 'default',
    install: true,
    install_dir: post_proc_install_dir,
)


################################################
# FACE RECOGNITION SOURCES
################################################
face_recognition_sources = [
    'recognition/arcface.cpp',
]

shared_library('face_recognition_post',
    face_recognition_sources,
    cpp_args : hailo_lib_args,
    include_directories: [hailo_general_inc, include_directories('./')] + xtensor_inc,
    dependencies : post_deps + [tracker_dep],
    gnu_symbol_visibility : 'default',
    install: true,
    install_dir: post_proc_install_dir,
)



if get_option('include_python')
    
endif
