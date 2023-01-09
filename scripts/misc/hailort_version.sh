#!/bin/bash

RED='\033[0;31m'
YELLOW='\033[1;33m'
NC='\033[0m'

function log_error() {
    echo -e "${RED}ERROR:${NC} ${1}"
}

function print_hailort_version() {
    if ! [ -x "$(command -v hailortcli)" ]; then
        log_error "hailortcli was not found"
        return 1
    fi

    echo $(hailortcli fw-control identify | grep -a "Firmware Version" | tail -n 1 | grep -aoP "(\d+.\d+.\d+)")
}

print_hailort_version
