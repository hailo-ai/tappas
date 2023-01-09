#!/bin/bash

function print_tappas_version() {
    echo $(meson introspect --projectinfo "$TAPPAS_WORKSPACE/core/hailo/build.release" | jq -r '.version')
}

print_tappas_version $@
