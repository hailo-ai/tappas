croppers_install_dir =  post_proc_install_dir + '/cropping_algorithms/'

################################################
# 3ddfa algorithm
################################################
# sources used to compile this plug-in
face_crop_sources = [
    'face_detection_and_landmarks/3ddfa.cpp',
]

shared_library('3ddfa',
    face_crop_sources,
    cpp_args : hailo_lib_args,
    include_directories: [hailo_general_inc, hailo_mat_inc],
    dependencies : post_deps + [opencv_dep],
    gnu_symbol_visibility : 'default',
    install: true,
    install_dir: croppers_install_dir,
)

################################################
# License Plate Recognition algorithms
################################################
# sources used to compile this plug-in
lpr_croppers_sources = [
    'lpr/lpr_croppers.cpp',
]

lpr_croppers_lib = shared_library('lpr_croppers',
    lpr_croppers_sources,
    cpp_args : hailo_lib_args,
    include_directories: [hailo_general_inc, hailo_mat_inc],
    dependencies : post_deps + [opencv_dep],
    gnu_symbol_visibility : 'default',
    install: true,
    install_dir: croppers_install_dir,
)

################################################
# RE-ID algorithm
################################################
re_id_sources = [
    're_id/re_id.cpp',
]

shared_library('re_id',
    re_id_sources,
    cpp_args : hailo_lib_args,
    include_directories: [hailo_general_inc, hailo_mat_inc],
    dependencies : post_deps + [opencv_dep],
    gnu_symbol_visibility : 'default',
    install: true,
    install_dir: croppers_install_dir,
)

################################################
# MSPN algorithm
################################################
mspn_sources = [
    'mspn/mspn.cpp',
]

shared_library('mspn',
    mspn_sources,
    cpp_args : hailo_lib_args,
    include_directories: [hailo_general_inc, hailo_mat_inc],
    dependencies : post_deps + [opencv_dep],
    gnu_symbol_visibility : 'default',
    install: true,
    install_dir: croppers_install_dir,
)

################################################
# Video Monitoring System algorithms
################################################
vms_sources = [
    'vms/vms_croppers.cpp',
]

shared_library('vms_croppers',
    vms_sources,
    cpp_args : hailo_lib_args,
    include_directories: [hailo_general_inc, hailo_mat_inc],
    dependencies : post_deps + [opencv_dep],
    gnu_symbol_visibility : 'default',
    install: true,
    install_dir: croppers_install_dir,
)

################################################
# Whole Buffer algorithm
################################################
whole_buffer_sources = [
    'whole_buffer/whole_buffer.cpp',
]

shared_library('whole_buffer',
    whole_buffer_sources,
    cpp_args : hailo_lib_args,
    include_directories: [hailo_general_inc, hailo_mat_inc],
    dependencies : post_deps + [opencv_dep],
    gnu_symbol_visibility : 'default',
    install: true,
    install_dir: croppers_install_dir,
)

################################################
# Detection Croppers algorithm
################################################
detections_sources = [
    'detections/detection_croppers.cpp',
]

shared_library('detection_croppers',
    detections_sources,
    cpp_args : hailo_lib_args,
    include_directories: [hailo_general_inc, hailo_mat_inc],
    dependencies : post_deps + [opencv_dep],
    gnu_symbol_visibility : 'default',
    install: true,
    install_dir: croppers_install_dir,
)