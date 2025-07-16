gstcheck_dep = dependency('gstreamer-check-1.0', version : gst_req,
  fallback : ['gstreamer', 'gst_check_dep'])

if gstcheck_dep.found()
  subdir('check')
endif
