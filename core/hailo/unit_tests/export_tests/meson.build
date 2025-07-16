################################################
#
#  Tests should have separate dependencies /
#  coverage as fits their needs. Any new 
#  unit tests that apply to a new lib should be
#  isolated to their own executable.
#
################################################

################################################
# ENCODE JSON TEST SOURCES
################################################
encode_json_test_sources = [
  'encode_json_tests.cpp',
]

encode_json_unit_tests_exe = executable('encode_json_unit_tests',
  encode_json_test_sources,
  include_directories: [hailo_general_inc, catch2_inc, rapidjson_inc] + [include_directories('../../plugins/export/')],
  dependencies : post_deps,
  gnu_symbol_visibility : 'default',
)