################################################
# LICENSE PLATE RECOGNITION
################################################
lpr_sources = [
'license_plate_recognition_arm.cpp',
]

executable('license_plate_recognition',
    lpr_sources,
    install_dir: apps_install_dir + '/license_plate_recognition/resources',
    install: true,
    cpp_args : hailo_lib_args,
    include_directories: [metadata_inc] + hailo_general_inc + cxxopts_inc,
    dependencies : plugin_deps + thread_deps + [meta_dep] + [opencv_dep],
    gnu_symbol_visibility : 'default',
)