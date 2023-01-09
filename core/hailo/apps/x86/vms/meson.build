################################################
# FACE ATTRIBUTES SOURCES
################################################
face_attributes_sources = [
    'postprocesses/face_attributes.cpp',
]

shared_library('face_attributes_post',
    face_attributes_sources,
    cpp_args : hailo_lib_args,
    include_directories: [hailo_general_inc, include_directories('./')] + xtensor_inc,
    dependencies : plugin_deps + [tracker_dep, libs_postprocesses_dep],
    gnu_symbol_visibility : 'default',
    install: true,
    install_dir: apps_install_dir + '/vms',
)

################################################
# PERSON ATTRIBUTES SOURCES
################################################
person_attributes_sources = [
    'postprocesses/person_attributes.cpp',
]

shared_library('person_attributes_post',
    person_attributes_sources,
    cpp_args : hailo_lib_args,
    include_directories: [hailo_general_inc, include_directories('./')] + xtensor_inc,
    dependencies : plugin_deps + post_deps + [tracker_dep, libs_postprocesses_dep],
    gnu_symbol_visibility : 'default',
    install: true,
    install_dir: apps_install_dir + '/vms',
)

################################################
# VMS FACE ALIGN SOURCE
################################################
face_align_src = 'face_align.cpp'

shared_library('vms_face_align',
  face_align_src,
  cpp_args : hailo_lib_args,
  include_directories: hailo_general_inc,
  dependencies : plugin_deps + [opencv_dep, image_dep],
  gnu_symbol_visibility : 'default',
  install: true,
  install_dir: apps_install_dir + '/vms',
)

################################################
# FACE RECOGNITION SOURCES
################################################
face_recognition_sources = [
    'postprocesses/arcface.cpp',
]

shared_library('face_recognition_post',
    face_recognition_sources,
    cpp_args : hailo_lib_args,
    include_directories: [hailo_general_inc, include_directories('./')] + xtensor_inc,
    dependencies : plugin_deps + [tracker_dep, libs_postprocesses_dep],
    gnu_symbol_visibility : 'default',
    install: true,
    install_dir: apps_install_dir + '/vms',
)

