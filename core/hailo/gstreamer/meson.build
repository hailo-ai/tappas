# Project Declaration

project('gst-hailo-tools', 'c', 'cpp',
        version : '3.18.0',
        default_options : [ 'warning_level=1',
                            'buildtype=debugoptimized',
                            'c_std=c11', 'cpp_std=c++17']
       )

# Project Configuration
core_conf = configuration_data()
core_conf.set('PACKAGE', '"@0@"'.format(meson.project_name()))
core_conf.set('VERSION', '"@0@"'.format(meson.project_version()))

configure_file(output : 'config.h', configuration : core_conf)

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
configinc = include_directories('.')
plugins_inc = include_directories(get_option('libgst_plugins_path'))
metadata_inc = include_directories('./metadata/')
hailo_general_inc = [include_directories(get_option('libgeneral_path'))]
project_inc = [configinc, plugins_inc, metadata_inc] + hailo_general_inc

cxxopts_inc_dir = get_option('libcxxopts')

# Xtensor Include Directories
xtensor_base_inc = include_directories(get_option('libxtensor'), is_system: true)
xtensor_inc = [xtensor_base_inc]
if get_option('include_blas')
  xtensor_blas_inc = include_directories(get_option('libblas'), is_system: true)
  xtensor_inc += [xtensor_blas_inc]
endif

# Extra Compiler Arguments
common_args = ['-DHAVE_CONFIG_H']
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
opencv_dep = dependency('opencv4', version : '>= 4.0')

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
    python_include_path = python_installation.get_path('include')
    pybind_inc = [include_directories(get_option('pybind11'), is_system: true),
                  include_directories(python_include_path, is_system: true)]
    # Pybind Dependecy
    pybind_dep = declare_dependency(
        compile_args : [
            # see https://github.com/pybind/pybind11/issues/1604
            '-fsized-deallocation',
            '-fvisibility=hidden'
        ],
        include_directories : pybind_inc,
        dependencies : [
            python_installation.dependency()
        ],
    )
    python_dep = python_installation.dependency()
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
hailo_lib_args = get_option('libargs') + common_args

tappas_workspace =  meson.current_source_dir() + '/../../../'

post_proc_install_dir = get_option('post_processes_install_dir')
if post_proc_install_dir == ''
  # By default install for x86 workspace
  post_proc_install_dir = tappas_workspace + '/apps/gstreamer/x86/libs'
endif

target = get_option('target')
# Project Subdirectories

if target == 'all'
  subdir('metadata')
  subdir('plugins')
  subdir('libs')
  subdir('apps')

elif target == 'apps'
  subdir('metadata')
  subdir(target)
elif target == 'plugins'
  subdir('metadata')
  subdir(target)
elif target == 'libs'
  subdir(target)
endif