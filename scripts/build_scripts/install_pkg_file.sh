#!/bin/bash
set -e
linux_release=$(grep '^VERSION_ID' /etc/os-release | cut -d '=' -f 2 | tr -d '"')
version_to_ignore=""

if [ $linux_release == "18.04" ]; then
    version_to_ignore="20.04|21.04"
elif [ $linux_release == "20.04" ]; then
    version_to_ignore="18.04|21.04"
elif [ $linux_release == "21.04" ]; then
    version_to_ignore="18.04|20.04"
else
    echo "Version $linux_release not supported"
    exit 1
fi

sudo apt-get -y --no-install-recommends install \
    $(cat $1 | grep -v '^\s*$\|^\s*\#' | grep -Ev $version_to_ignore | awk -F":" '{ if ($2) { print $2 } else { print $1 } }')
