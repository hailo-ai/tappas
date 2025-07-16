################################################
#
#  Tests should have separate dependencies /
#  coverage as fits their needs. Any new 
#  unit tests that apply to a new lib should be
#  isolated to their own executable.
#
################################################

# Catch2 Include Directories
catch2_base_inc = include_directories(get_option('libcatch2'), is_system: true)
catch2_inc = [catch2_base_inc]

################################################
# KALMAN FILTER TEST SOURCES
################################################
kalman_test_sources = [
    'tracker_tests/kalman_filter_tests.cpp',
]

executable('kalman_filter_unit_tests',
    kalman_test_sources,
    include_directories: [hailo_general_inc, catch2_inc, xtensor_inc] + [include_directories('../tracking/jde_tracker')],
    dependencies : [],
    gnu_symbol_visibility : 'default',
)


################################################
# STRACK TEST SOURCES
################################################
strack_test_sources = [
    'tracker_tests/strack_tests.cpp',
]

executable('strack_unit_tests',
    strack_test_sources,
    include_directories: [hailo_general_inc, catch2_inc, xtensor_inc] + [include_directories('../tracking/jde_tracker')],
    dependencies : [opencv_dep],
    gnu_symbol_visibility : 'default',
)

################################################
# JDE TRACKER TEST SOURCES
################################################
jde_tracker_test_sources = [
    'tracker_tests/jde_tracker_tests.cpp',
]

executable('jde_tracker_unit_tests',
    jde_tracker_test_sources,
    include_directories: [hailo_general_inc, catch2_inc, xtensor_inc] + [include_directories('../tracking/jde_tracker')],
    dependencies : plugin_deps + [opencv_dep, meta_dep],
    gnu_symbol_visibility : 'default',
)

################################################
# LPR CROPPERS TEST SOURCES
################################################
lpr_cropper_test_sources = [
    '../libs/croppers/lpr/lpr_croppers.cpp',
    'cropper_tests/lpr_cropper_tests.cpp',
]

executable('lpr_cropper_unit_tests',
    lpr_cropper_test_sources,
    include_directories: [hailo_general_inc, catch2_inc, hailo_mat_inc] + [include_directories('../libs/croppers/lpr/')],
    dependencies : plugin_deps + [opencv_dep],
    gnu_symbol_visibility : 'default',
)

################################################
# GALLERY TEST SOURCES
################################################
gallery_test_sources = [
    'gallery_tests/gallery_tests.cpp',
]

executable('gallery_unit_tests',
    gallery_test_sources,
    cpp_args : hailo_lib_args,
    include_directories: [hailo_general_inc, catch2_inc, xtensor_inc, rapidjson_inc] + [include_directories('../plugins'), include_directories('../libs')],
    dependencies : plugin_deps + [opencv_dep, tracker_dep],
    gnu_symbol_visibility : 'default',
)

subdir('postprocess_tests')
subdir('export_tests')
subdir('import_tests')
subdir('element_tests')