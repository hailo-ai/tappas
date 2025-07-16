TARGET_STREAMS=16
TARGET_FPS=20
NUM_OF_PERSONS_IN_FRAME=4
NUM_OF_FACES_IN_FRAME=4

DETECTORS_TARGET_FPS=$((($TARGET_STREAMS / 2) * $TARGET_FPS))
FACE_ATTR_TARGET_FPS=$((($TARGET_STREAMS / 4) * $TARGET_FPS * $NUM_OF_FACES_IN_FRAME))
PERSON_ATTR_TARGET_FPS=$((($TARGET_STREAMS / 2) * $TARGET_FPS * $NUM_OF_PERSONS_IN_FRAME))

RUN_TTR=20

EXTRA_PTS="/dev/pts/10"

function nets_without_seperation() {
    nets_not_seperated=(
        "--device-count 3"

        # Person nets
        "set-net yolov5s_personface_nv12.hef --batch-size 8 --framerate $DETECTORS_TARGET_FPS"
        "set-net person_attr_resnet_v1_18_nv12.hef --framerate $PERSON_ATTR_TARGET_FPS"

        # Face nets
        "set-net scrfd_10g_nv12.hef --batch-size 8 --framerate $DETECTORS_TARGET_FPS"
        "set-net arcface_mobilefacenet_nv12.hef --framerate $FACE_ATTR_TARGET_FPS"
        "set-net face_attr_resnet_v1_18_rgba.hef --framerate $FACE_ATTR_TARGET_FPS"
    )

    hailortcli run2 -t $RUN_TTR ${nets_not_seperated[*]}
}

function nets_with_seperation() {
    nets_seperated_device_id_1=(
        "--device-count 2"

        "set-net yolov5s_personface_nv12.hef --batch-size 8 --framerate $DETECTORS_TARGET_FPS"
        "set-net scrfd_10g_nv12.hef --batch-size 8 --framerate $DETECTORS_TARGET_FPS"
    )

    nets_seperated_device_id_2=(
        "--device-count 1"

        "set-net person_attr_resnet_v1_18_nv12.hef --framerate $PERSON_ATTR_TARGET_FPS"
        "set-net arcface_mobilefacenet_nv12.hef --framerate $FACE_ATTR_TARGET_FPS"
        "set-net face_attr_resnet_v1_18_rgba.hef --framerate $FACE_ATTR_TARGET_FPS"
    )

    if [[ ! -f $EXTRA_PTS ]]; then
        echo "$EXTRA_PTS is not found.."
        exit 1
    fi

    hailortcli run2 -t $RUN_TTR ${nets_seperated_device_id_1[*]} >$EXTRA_PTS &
    hailortcli run2 -t $RUN_TTR ${nets_seperated_device_id_2[*]}
}

nets_without_seperation
nets_with_seperation
