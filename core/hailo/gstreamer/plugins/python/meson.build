fs_dep = meson.get_compiler('c').find_library('stdc++fs', required : true)

python_sources = [
  'gsthailopython.cpp',
  'hailopython_infra.cpp',
]

hailo_python_link_args = []
hailo_python_deps = [fs_dep]
if python_version == 'python3.6'
  hailo_python_deps += python_dep
elif python_version == 'python3.8'
  hailo_python_link_args += ['-L/usr/lib/python3.8/config-3.8-x86_64-linux-gnu', '-L/usr/lib', '-lpython3.8', '-lcrypt', '-lpthread', '-ldl',  '-lutil', '-lm', '-lm']
else
  error('Python Version not supportedi:', python_version)
endif

################################################
# Hailo Gstreamer Shared Library
################################################
libhailopython = shared_library('gsthailopython',
  python_sources,
  c_args : hailo_lib_args,
  cpp_args : hailo_lib_args,
  link_args : hailo_python_link_args,
  include_directories: project_inc + [python_include_path],
  dependencies : plugin_deps + hailo_python_deps + [meta_dep, dl_dep] + gx_deps,
  gnu_symbol_visibility : 'default',
)

################################################
# Hailo Python Module
################################################

hailo_python_sources = [
'hailo_python_api.cpp'
]


hailo_python_lib = shared_library('hailo',
hailo_python_sources,
c_args : hailo_lib_args,
cpp_args : hailo_lib_args,
link_args : hailo_python_link_args,
include_directories: hailo_general_inc + [pybind_inc, python_include_path],
dependencies : hailo_python_deps,
gnu_symbol_visibility : 'default',
)
