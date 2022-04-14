################################################
# 3ddfa algorithm
################################################
# sources used to compile this plug-in
face_crop_sources = [
  '3ddfa.cpp',
]

det_draw_lib = shared_library('3ddfa',
  face_crop_sources,
  cpp_args : hailo_lib_args,
  include_directories: [hailo_general_inc],
  dependencies : post_deps + [opencv_dep] + hailo_deps,
  gnu_symbol_visibility : 'default',
)

################################################
# License Plate Recognition algorithms
################################################
# sources used to compile this plug-in
lpr_croppers_sources = [
  'lpr_croppers.cpp',
]

lpr_croppers_lib = shared_library('lpr_croppers',
  lpr_croppers_sources,
  cpp_args : hailo_lib_args,
  include_directories: [hailo_general_inc],
  dependencies : post_deps + [opencv_dep] + hailo_deps,
  gnu_symbol_visibility : 'default',
)