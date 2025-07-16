################################################
#
#  Tests should have separate dependencies /
#  coverage as fits their needs. Any new 
#  unit tests that apply to a new lib should be
#  isolated to their own executable.
#
################################################

################################################
# DECODE JSON TEST SOURCES
################################################
decode_json_test_sources = [
  'decode_json_tests.cpp',
]

decode_json_unit_tests_exe = executable('decode_json_unit_tests',
  decode_json_test_sources,
  include_directories: [hailo_general_inc, catch2_inc, rapidjson_inc] + [include_directories('../../plugins/import/'),
                                                                         include_directories('../.')],
  dependencies : post_deps,
  gnu_symbol_visibility : 'default',
)