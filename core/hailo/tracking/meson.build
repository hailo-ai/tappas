tracker_sources = ['hailo_tracker.cpp']

################################################
# Hailo Tracker Shared Library
################################################
tracker_lib = shared_library('hailo_tracker',
    tracker_sources,
    c_args : hailo_lib_args,
    cpp_args : hailo_lib_args,
    include_directories: hailo_general_inc + [xtensor_inc],
    dependencies : [opencv_dep],
    gnu_symbol_visibility : 'default',
    version: meson.project_version(),
    install: true,
    install_dir: get_option('libdir'),
)

tracker_dep = declare_dependency(
  include_directories: [include_directories('.')],
  link_with : tracker_lib)

if not get_option('include_python')
    subdir_done()
endif

################################################
# Hailo Tracker Python Extension Module
################################################
tracker_pybind_sources = [
    'jde_tracker/python_bindings/tracker_pybind_main.cpp'
]

python_installation.extension_module('pyhailotracker',
    tracker_pybind_sources,
    include_directories: [hailo_general_inc, xtensor_inc, include_directories('./jde_tracker')],
    dependencies : [opencv_dep, pybind_dep],
    link_language : 'cpp',
    override_options: [
        'cpp_rtti=true',
    ],
    gnu_symbol_visibility : 'default',
    install: true,
    install_dir: python_package_install_dir,
)