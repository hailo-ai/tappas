meta_sources = [
    'gst_hailo_meta.cpp',
    'gst_hailo_cropping_meta.cpp',
    'gst_hailo_counter_meta.cpp',
    'gst_hailo_stream_meta.cpp',
]

meta_lib = shared_library('gsthailometa',
    meta_sources,
    cpp_args : hailo_lib_args,
    include_directories: [hailo_general_inc],
    dependencies : plugin_deps,
    gnu_symbol_visibility : 'default',
    version: meson.project_version(),
    install: true,
    install_dir: get_option('libdir'),
)

meta_dep = declare_dependency(
  include_directories: [include_directories('.')],
  link_with : meta_lib)

pkgc = import('pkgconfig')
pkgc.generate(name: 'gsthailometa',
              libraries: meta_lib,
              subdirs: '.',
              version: meson.project_version(),
              description: 'Meta plugin for GStreamer Hailo'
)

    
install_headers('gst_hailo_meta.hpp', install_dir: get_option('includedir') + '/gsthailometa')