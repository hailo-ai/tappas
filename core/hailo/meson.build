# Project Declaration

project('gst-hailo-tools', 'c', 'cpp',
        version : '3.25.0',
        default_options : [ 'warning_level=1',
                            'buildtype=debugoptimized',
                            'c_std=c11', 'cpp_std=c++17']
       )

# Compiler Arguments
compiler = meson.get_compiler('cpp')
compiler_version = compiler.version().split('.')[0].to_int()
if compiler_version >= 9
  message('GCC >= 9.0.0 detected, applying extra arguments.')
  add_global_arguments('-Wpessimizing-move', language : 'cpp')
  add_global_arguments('-Wredundant-move', language : 'cpp')
  add_global_arguments('-fconcepts', language : 'cpp')
endif

# Include Directories
hailo_general_inc = [include_directories('./general')]
hailo_mat_inc = [include_directories('./plugins/common/')]

# Xtensor Include Directories
xtensor_base_inc = include_directories(get_option('libxtensor'), is_system: true)
xtensor_inc = [xtensor_base_inc]
if get_option('include_blas')
  xtensor_blas_inc = include_directories(get_option('libblas'), is_system: true)
  xtensor_inc += [xtensor_blas_inc]
endif

# Extra Compiler Arguments
xtensor_args = []  # '-mavx2', '-ffast-math' --> TODO: Causes arm issues
debug_args = ['-g', '-O0']

# Gstreamer Dependencies
gst_req = '>= 1.0.0'
gst_dep = dependency('gstreamer-1.0', version : gst_req,
  fallback : ['gstreamer', 'gst_dep'])
gst_base_dep = dependency('gstreamer-base-1.0', version : gst_req,
  fallback : ['gstreamer', 'gst_base_dep'])
gst_app_dep = dependency('gstreamer-app-1.0', version : gst_req,
  fallback : ['gstreamer', 'gst_app_dep'])
gstvideo_dep = dependency('gstreamer-video-1.0', version : gst_req,
  fallback : ['gst-plugins-base', 'video_dep'])

# Opencv Dependencies
opencv_dep = dependency('opencv4', version : '>= 4.0', method : 'pkg-config')

if get_option('include_python')
    # GObject dependencies
    glib_dep = dependency('glib-2.0')
    gmod_dep = dependency('gmodule-2.0')
    gobj_dep = dependency('gobject-2.0')
    pygobj_dep = dependency('pygobject-3.0')
    gx_deps = [glib_dep, gmod_dep, gobj_dep, pygobj_dep]

    # Pybind Include Directories
    pymod = import('python')
    python_version = 'python' + get_option('python_version')
    python_installation = pymod.find_installation(python_version)
    python_dep = python_installation.dependency(embed : true)
    python_package_install_dir = run_command(python_version, '-m', 'pip', 'show', 'pip').stdout().split('Location:')[1].split()[0]
    pybind_inc = [include_directories(get_option('pybind11'), is_system: true)]
    # Pybind Dependecy
    pybind_dep = declare_dependency(
        compile_args : [
            # see https://github.com/pybind/pybind11/issues/1604
            '-fsized-deallocation',
            '-fvisibility=hidden'
        ],
        include_directories : pybind_inc,
        dependencies : [python_dep],
    )
endif

# Plugin Dependencies
plugin_deps = [gst_dep, gst_base_dep, gstvideo_dep, gst_app_dep]
post_deps = []

if get_option('include_blas')
  is_ub20 = run_command('lsb_release', '-r').stdout().contains('20.04')
  if is_ub20
    libblas_dep = dependency('blas', version :'== 3.9.0')
    liblapack_dep = dependency('lapack', version :'== 3.9.0')
  else
    libblas_dep = dependency('blas', version :'== 3.7.1')
    liblapack_dep = dependency('lapack', version :'== 3.7.1')
  endif
  post_deps += [libblas_dep, liblapack_dep]
  plugin_deps += [libblas_dep, liblapack_dep]
endif

# Library Arguments
hailo_lib_args = get_option('libargs')

tappas_workspace =  meson.current_source_dir() + '/../../'

post_proc_install_dir = get_option('post_processes_install_dir')
if post_proc_install_dir == ''
  # By default install for x86 workspace
  post_proc_install_dir = tappas_workspace + '/apps/h8/gstreamer/libs/post_processes/'
endif

target = get_option('target')
# Project Subdirectories

if target in ['all','apps','libs']
    cxxopts_inc_dir = get_option('libcxxopts')
    # cxxopts Include Directories
    cxxopts_inc = [include_directories(cxxopts_inc_dir, is_system: true)]
endif

if target in ['all','libs','unit_tests','plugins']
    rapidjson_inc_dir = get_option('librapidjson')
    # rapidjson Include Directories
    rapidjson_inc = [include_directories(rapidjson_inc_dir, is_system: true)]

endif

libs_postprocesses_dep = declare_dependency(
  include_directories: [include_directories('./libs/postprocesses')])

if target == 'all'
  subdir('metadata')
  subdir('tracking')
  subdir('plugins')
  subdir('libs')
  subdir('apps')
  subdir('tracers')

elif target == 'apps'
  subdir('metadata')
  subdir('tracking')
  subdir(target)
elif target == 'plugins'
  subdir('metadata')
  subdir('tracking')
  subdir(target)
elif target == 'libs'
  subdir('tracking')
  subdir(target)
elif target == 'tracers'
  subdir('metadata')
  subdir(target)
endif
