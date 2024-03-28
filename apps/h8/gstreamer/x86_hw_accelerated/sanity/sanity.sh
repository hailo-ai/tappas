set -e

script_dir=$(dirname $(realpath "$0"))
source $script_dir/../../../../../scripts/misc/checks_before_run.sh --check-vaapi
readonly VIDEO_PATH="$TAPPAS_WORKSPACE/apps/h8/gstreamer/x86_hw_accelerated/sanity/resources/street.mp4"
video_sink_element=$([ "$XV_SUPPORTED" = "true" ] && echo "xvimagesink" || echo "ximagesink")
PIPELINE="gst-launch-1.0 filesrc num-buffers=200 location=$VIDEO_PATH ! qtdemux ! vaapidecodebin ! video/x-raw, format=NV12, width=640, height=640 ! videoconvert ! ${video_sink_element} sync=true"
eval ${PIPELINE}
