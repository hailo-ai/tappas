# Project Declaration

project('gst-hailo-tools', 'c', 'cpp',
        version : '5.3.0',
        default_options : [ 'buildtype=release',
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
common_args = ['-DHAVE_CONFIG_H']
if get_option('target_platform') == 'imx8'
    common_args += ['-DIMX8_TARGET']
endif

# Xtensor Include Directories
xtensor_base_inc = include_directories(get_option('libxtensor'), is_system: true)
xtensor_inc = [xtensor_base_inc]

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

# Library Arguments
hailo_lib_args = get_option('libargs')
target_platform = get_option('target_platform')
arch = host_machine.cpu_family()
arch_dir = arch + '-linux-gnu/'


if target_platform in ['x86', 'x86_64']
  arch = 'x86_64'
elif target_platform in ['rpi', 'rpi5', 'rockchip', 'aarch64']
  arch = 'aarch64'
elif target_platform == 'imx8'
  arch_dir = ''
else
  error('Unknown target platform: ' + target_platform)  
endif

post_proc_install_dir = get_option('post_processes_install_dir')
if post_proc_install_dir == ''
  # By default install for x86 workspace
  post_proc_install_dir = '/usr/lib/' + arch_dir + 'hailo/tappas/post_processes'
endif

install_dir = '/usr/lib/' + arch_dir

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

# OpenCV dependency configuration
tbb_dep = dependency('tbb', required: false)
gl_dep = dependency('gl', required: false)
# Opencv Dependencies

if get_option('static_opencv')
    # Minimal static OpenCV dependency (only core + imgproc + calib3d) for hailo_opencv_utils
    # This avoids pulling in features2d, flann, imgcodecs which add unnecessary size
    # We link the static .a files directly to ensure static linking
    opencv_minimal_dep = declare_dependency(
        compile_args: ['-I/usr/local/include/opencv4'],
        link_args: [
            '-Wl,--gc-sections',
            '-Wl,--as-needed',
            # Static OpenCV libraries (order matters - imgproc depends on core)
            '/usr/local/lib/libopencv_calib3d.a',
            '/usr/local/lib/libopencv_imgproc.a',
            '/usr/local/lib/libopencv_core.a',
            # 3rdparty static libraries
            '/usr/local/lib/opencv4/3rdparty/libzlib.a',
            # System libraries
            '-ldl', '-lm', '-lpthread', '-lrt'
        ]
    )
    opencv_dep = opencv_minimal_dep
    opencv_helper_extra_cpp_args = ['-ffunction-sections', '-fdata-sections']
    opencv_helper_extra_link_args = ['-Wl,--gc-sections', '-Wl,--as-needed']
else
    # Dynamic OpenCV dependency (default)
    opencv_dep = dependency('opencv4', version : '>= 4.0', method : 'pkg-config')
    opencv_helper_extra_cpp_args = []
    opencv_helper_extra_link_args = []
endif

subdir('metadata')
subdir('general')

if target == 'all'
  subdir('tracking')
  subdir('plugins')
  subdir('libs')
  subdir('apps')
  subdir('tracers')

elif target == 'apps'
  subdir('tracking')
  subdir(target)
elif target == 'plugins'
  subdir('tracking')
  subdir(target)
elif target == 'libs'
  subdir('tracking')
  subdir(target)
elif target == 'tracers'
  subdir(target)
endif
