################################################
# Infra API Libs
################################################
reference_camera_infra_src = 'reference_camera_infra.cpp'

reference_camera_infra_lib = shared_library('hailo_reference_camera',
  reference_camera_infra_src,
  cpp_args : hailo_lib_args,
  dependencies : dependencies_apps + [libhailort_dep, opencv_dep, tracker_dep, tappas_general_dep, image_dep],
  gnu_symbol_visibility : 'default',
  version: meson.project_version(),
  install: true,
  install_dir: get_option('libdir'),
)

reference_camera_dep = declare_dependency(
  include_directories: [include_directories('pipeline_infra')],
  link_with : reference_camera_infra_lib)

pkgc.generate(
    name : 'reference_camera_api',
    libraries : reference_camera_infra_lib,
    subdirs : 'hailo/tappas/reference_camera',
    version : meson.project_version(),
    description : 'Hailo Tappas Reference Camera API',
    requires: ['opencv4', 'hailo_tracker'],
)

reference_camera_headers = ['pipeline_infra/aggregator_stage.hpp',
                            'pipeline_infra/ai_stage.hpp',
                            'pipeline_infra/buffer.hpp',
                            'pipeline_infra/dsp_stages.hpp',
                            'pipeline_infra/overlay_native.hpp',
                            'pipeline_infra/overlay_stage.hpp',
                            'pipeline_infra/pipeline.hpp',
                            'pipeline_infra/postprocess_stage.hpp',
                            'pipeline_infra/queue.hpp',
                            'pipeline_infra/stage.hpp',
                            'pipeline_infra/tracker_stage.hpp',
                            'pipeline_infra/udp_stage.hpp']
install_headers(reference_camera_headers, subdir: 'hailo/tappas/reference_camera')