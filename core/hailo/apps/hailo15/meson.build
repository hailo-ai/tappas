################################################
# MEDIA LIBRARY DEPS
################################################
media_library_common_dep = dependency('hailo_media_library_common', method : 'pkg-config')
media_library_frontend_dep = dependency('hailo_media_library_frontend', method : 'pkg-config')
media_library_encoder_dep = dependency('hailo_media_library_encoder', method : 'pkg-config')
encoder_dep = dependency('hailo_encoder', method : 'pkg-config')
gstmedialibrary_utils_dep = dependency('gstmedialibutils', method : 'pkg-config')

dependencies_apps = plugin_deps + media_library_encoder_dep + media_library_common_dep + media_library_frontend_dep + encoder_dep + gstmedialibrary_utils_dep

subdir('lpr')
subdir('encoder_pipelines')
subdir('encoder_pipelines_new_api')
subdir('vision_preproc_applications')