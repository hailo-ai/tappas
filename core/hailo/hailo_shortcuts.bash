function gst_set_debug() {
    # set gstreamer debug
    export HAILO_PROFILE_LOCATION=/tmp/profile
    export GST_DEBUG="GST_TRACER:7"
    export GST_DEBUG_FILE=$TAPPAS_WORKSPACE/tappas_traces.log
    export GST_DEBUG_NO_COLOR=1

    if [[ "$1" == "-r" ]]; then
        echo "Exporting in reduced mode"
        export GST_TRACERS="cpuusage;proctime;interlatency;framerate;queuelevel;graphic"
    else
        export GST_TRACERS="cpuusage;proctime;interlatency;scheduletime;bitrate;framerate;queuelevel;threadmonitor;numerator;buffer;detections;graphic"
    fi

    echo 'Options for TRACERS:"'
    echo 'export GST_TRACERS="cpuusage;proctime;interlatency;scheduletime;bitrate;framerate;queuelevel;threadmonitor;numerator;buffer;detections;graphic"'
}

function gst_set_graphic() {
    # set trace to collect graphic data of gstreamer pipeline
    export HAILO_PROFILE_LOCATION=/tmp/profile
    export GST_DEBUG_DUMP_DOT_DIR=/tmp/
    export GST_DEBUG="GST_TRACER:7"
    export GST_TRACERS="graphic"
    echo 'export GST_TRACERS="graphic"'
}

function gst_unset_debug() {
    # unset gstreamer debug
    unset GST_TRACERS
    unset GST_DEBUG
    unset GST_DEBUG_DUMP_DOT_DIR
    unset HAILO_PROFILE_LOCATION
    unset GST_DEBUG_FILE
    unset GST_DEBUG_NO_COLOR
}

function gst_prepare_tracers_dir() {
    # plot the gst-shark dump files
    export HAILO_PROFILE_LOCATION=/tmp/profile

    split_traces_dir=$TAPPAS_WORKSPACE/tappas_traces_$(date +%d_%m_%Y_%H_%M_%S)
    mkdir $split_traces_dir

    if ls -rt $HAILO_PROFILE_LOCATION/graphic/pipeline* 1>/dev/null 2>&1; then
        cp $(ls -rt $HAILO_PROFILE_LOCATION/graphic/pipeline* | tail -n 1) "$split_traces_dir"
    fi

    $TAPPAS_WORKSPACE/tools/trace_analyzer/split_traces.sh $split_traces_dir
    echo "Splited tracers dir: $split_traces_dir"
}

function gst_plot_debug() {
    gst_prepare_tracers_dir

    if [[ -z "$split_traces_dir" ]]; then
        echo "Error occured, split_tracer_dir is empty"
        exit
    fi

    python3 $TAPPAS_WORKSPACE/tools/trace_analyzer/plot_all_to_html.py -p $split_traces_dir

    echo 'To plot the graphic pipeline graph, run:'
    echo "dot $HAILO_PROFILE_LOCATION/graphic/pipeline_<timestamp>.dot -T x11"
}

function xdot_latest() {
    latest_file="$(ls -Art "${HAILO_PROFILE_LOCATION}/graphic/" | tail -n 1)"
    absolute_path_latest_file="$(realpath $HAILO_PROFILE_LOCATION/graphic/$latest_file)"
}

function aliases_declerations() {
    alias tappas_compile="$TAPPAS_WORKSPACE/scripts/gstreamer/install_hailo_gstreamer.sh --skip-hailort"
    alias identify="hailortcli fw-control identify"
    alias scan="hailortcli scan"
}

function commons() {
    if [[ -x "$(command -v figlet)" ]]; then
        figlet -t WELCOME to TAPPAS container!
    fi
}

function check_hailort_service_is_running() {
    num_of_hailort_service_proc=$(ps -C hailort_service | wc -l)
}

function hailort_stop_service() {
    check_hailort_service_is_running
    if [[ $num_of_hailort_service_proc > 1 ]]; then
        pkill hailort_service
    else
        echo "HailoRT service is not running"
    fi
}

function handle_hailort_as_a_service() {
    check_hailort_service_is_running

    if [[ $hailort_enable_service == "yes" ]] && [[ $num_of_hailort_service_proc == 1 ]]; then
        /usr/local/bin/hailort_service
    elif [[ $hailort_enable_service == "no" ]] && [[ $num_of_hailort_service_proc > 1 ]]; then
        hailort_stop_service
    fi
}

function main() {
    handle_hailort_as_a_service
    aliases_declerations
    commons
}

main
