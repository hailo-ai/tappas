set -e

script_dir=$(dirname $(realpath "$0"))
source $script_dir/../../../../../scripts/misc/checks_before_run.sh

video_sink_element=$([ "$XV_SUPPORTED" = "true" ] && echo "xvimagesink" || echo "ximagesink")
PIPELINE="gst-launch-1.0 videotestsrc ! ${video_sink_element}"
eval ${PIPELINE}
