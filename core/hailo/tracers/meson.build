################################################
# BUFFER TRACE SOURCES 
################################################
gst_tracer_sources = [
	'gstdot.c',
	'gstcpuusagecompute.cpp',
	'gstthreadmonitorcompute.cpp',
	'gstproctimecompute.cpp',
	'gstctf.cpp',
	'gstparser.c',
	'gstplugin.cpp',
	'gstsharktracer.cpp',
	'gstgraphic.cpp',
	'gstcpuusage.cpp',
	'gstthreadmonitor.cpp',
	'gstproctime.cpp',
	'gstinterlatency.cpp',
	'gstscheduletime.cpp',
	'gstframerate.cpp',
	'gstqueuelevel.cpp',
	'gstnumerator.cpp',
	'gstdetections.cpp',
	'gstbitrate.cpp',
	'gstbuffer.cpp',
	'gstperiodictracer.cpp',
]

glib_dep = dependency('glib-2.0')
gio_dep = dependency('gio-2.0')

shared_library('gsthailotracers',
    gst_tracer_sources,
    cpp_args : hailo_lib_args+['-DGST_USE_UNSTABLE_API'],
    include_directories: [hailo_general_inc, include_directories('./')],
    dependencies : plugin_deps+[glib_dep, gio_dep, meta_dep],
    gnu_symbol_visibility : 'default',
    version: meson.project_version(),
    install: true,
    c_args : ['-DGST_USE_UNSTABLE_API'],
    install_dir: get_option('libdir') + '/gstreamer-1.0/',
)
