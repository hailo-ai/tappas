
meta_sources = [
    'metadata/gst_buffer_info_meta.cpp',
    'metadata/gst_hailo_meta.cpp',
]
meta_lib = shared_library('gsthailometa',
  meta_sources,
  c_args : hailo_lib_args,
  cpp_args : hailo_lib_args,
  include_directories: [hailo_general_inc],
  dependencies : plugin_deps,
  gnu_symbol_visibility : 'default',
)

meta_dep = declare_dependency(
  include_directories: [include_directories('./metadata')],
  link_with : meta_lib)

plugin_sources = [
  'gsthailotools.cpp',
  'gsthailofilter.cpp',
  'gsthailofilter2.cpp',
  'gsthailomuxer.cpp',
  'common/image.cpp',
  'overlay/overlay.cpp',
  'overlay/gsthailooverlay.cpp',
  'cropping/gsthailobasecropper.cpp',
  'cropping/gsthailocropper.cpp',
  'cropping/gsthailoaggregator.cpp',
  'cropping/gst_hailo_cropping_meta.cpp',
  'tiling/gsthailotilecropper.cpp',
  'tiling/gsthailotileaggregator.cpp',
  'tracking/gsthailotracker.cpp',
]

# equivalent to - dl_dep = dependency('dl')
dl_dep = meson.get_compiler('c').find_library('dl', required : false)


################################################
# Hailo Gstreamer Shared Library
################################################
libhailotools = shared_library('gsthailotools',
  plugin_sources,
  c_args : hailo_lib_args,
  cpp_args : hailo_lib_args,
  include_directories: project_inc + [xtensor_inc, include_directories('./metadata')],
  dependencies : plugin_deps + [meta_dep, dl_dep, opencv_dep],
  gnu_symbol_visibility : 'default',
)

gst_hailo_dep = declare_dependency(
  include_directories: [include_directories('./metadata'), include_directories('.')],
  link_with : libhailotools)
hailo_deps = [gst_hailo_dep]

if get_option('include_python')
    ################################################
    # Hailo Tracker Python Extension Module
    ################################################
    tracker_pybind_sources = [
        'tracking/jde_tracker/python_bindings/tracker_pybind_main.cpp'
    ]
    
    libtracker = python_installation.extension_module('pyhailotracker',
    tracker_pybind_sources,
    c_args : hailo_lib_args,
    cpp_args : hailo_lib_args,
    include_directories: [hailo_general_inc, xtensor_inc, pybind_inc, include_directories('./tracking/jde_tracker')],
    dependencies : [dl_dep, opencv_dep, pybind_dep, python_dep],
    link_language : 'cpp',
    gnu_symbol_visibility : 'default',
    override_options: [
        'cpp_rtti=true',
    ]
    )
    subdir('python')
endif

pkg_mod = import('pkgconfig')
pkg_mod.generate(
  name : 'gst-hailo-tools',
  filebase : 'gst-hailo-tools',
  description : 'Hailo gstreamer tools',
  subdirs : 'plugins',
  libraries : libhailotools,
  version : '"@0@"'.format(meson.project_version()),
)
