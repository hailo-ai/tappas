#!/bin/bash
set -e
linux_release=$(grep '^VERSION_ID' /etc/os-release | cut -d '=' -f 2 | tr -d '"')
version_to_ignore=""

linux_releases_list=( 20.04 21.04 22.04 24.04)

if [[ ! "${linux_releases_list[*]}" =~ "${linux_release=}" ]]; then
    echo "$linux_release is not supported"
    exit 1
fi

# Replace spaces with pipe
versions_to_ignore=$(echo "${linux_releases_list[@]/$linux_release}" | tr -s ' ' '|')

# If the first char is pipe, remove it 
if [[ "${versions_to_ignore::1}" == "|" ]]; then
    versions_to_ignore="${versions_to_ignore:1}"
fi

# If the last char is pipe, remove it 
if [[ "${versions_to_ignore: -1}" == "|" ]]; then
    versions_to_ignore="${versions_to_ignore:: -1}"
fi

sudo apt-get -y --no-install-recommends install \
     $(cat $1 | grep -v '^\s*$\|^\s*\#' | grep -Ev $versions_to_ignore | awk -F":" '{ if ($2) { print $2 } else { print $1 } }')
