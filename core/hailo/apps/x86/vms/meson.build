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