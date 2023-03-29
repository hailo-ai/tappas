set -e

script_dir=$(dirname $(realpath "$0"))
source $script_dir/../../../../../scripts/misc/checks_before_run.sh

PIPELINE="gst-launch-1.0 videotestsrc ! ximagesink"
eval ${PIPELINE}
