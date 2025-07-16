################################################
#
#  Tests should have separate dependencies /
#  coverage as fits their needs. Any new 
#  unit tests that apply to a new lib should be
#  isolated to their own executable.
#
################################################

################################################
# DEBUG TEST SOURCES
################################################
debug_test_sources = [
  '../../libs/tools/debug.cpp',
  'debug_tests.cpp',
]

debug_unit_tests_exe = executable('debug_unit_tests',
  debug_test_sources,
  include_directories: [hailo_general_inc, catch2_inc] + xtensor_inc + [include_directories('../../libs/tools/')],
  dependencies : post_deps,
  gnu_symbol_visibility : 'default',
)