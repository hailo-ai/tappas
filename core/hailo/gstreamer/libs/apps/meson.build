if get_option('include_network_switch_app')
    cmd = run_command('sh', '-c', 'echo $TAPPAS_WORKSPACE')
    tappas_workspace = cmd.stdout().strip()
    thread_deps = [dependency('threads')]

    # Opencv Include Directories
    opencv_base_inc = include_directories('../../../../open_source/opencv/include/opencv4', is_system: true)
    opencv_inc = [opencv_base_inc]

    ################################################
    # NETWORK SWITCH APP
    ################################################

    src = 'detection_and_depth_estimation_networks_switch.cpp'
    executable('detection_and_depth_estimation_networks_switch',
      src,
      install_dir: tappas_workspace+'/apps/gstreamer/x86/network_switch/',
      install: true,
      cpp_args : hailo_lib_args,
      include_directories: project_inc + cxxopts_inc,
      dependencies : plugin_deps + hailo_deps,
      gnu_symbol_visibility : 'default',
    )

    ################################################
    # LICENSE PLATE RECOGNITION
    ################################################
    lpr_sources = [
    'license_plate_recognition.cpp',
    ]

    executable('license_plate_recognition',
        lpr_sources,
        install_dir: tappas_workspace+'/apps/gstreamer/x86/license_plate_recognition/resources',
        install: true,
        cpp_args : hailo_lib_args,
        include_directories: project_inc + cxxopts_inc + opencv_inc,
        dependencies : plugin_deps + hailo_deps + thread_deps + [meta_dep] + [opencv_dep],
        gnu_symbol_visibility : 'default',
    )
endif