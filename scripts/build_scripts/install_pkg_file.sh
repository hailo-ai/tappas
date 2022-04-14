#!/bin/bash
set -e
linux_release=$(grep '^VERSION_ID' /etc/os-release | cut -d '=' -f 2 | tr -d '"')
version_to_ignore=$(if [ $linux_release == "20.04" ]; then echo "18.04"; else echo "20.04"; fi)

sudo apt-get -y --no-install-recommends install \
$(cat $1 | grep -v '^\s*$\|^\s*\#' | grep -v $version_to_ignore |  awk -F":" '{ if ($2) { print $2 } else { print $1 } }')
