#!/bin/bash
set -e
sudo apt-get update
sudo apt-get install -y linux-headers-$(uname -r)

script_dir=$(dirname $(realpath "$0"))
hailort_dir="$script_dir/../../../hailort/"
driver_name=$(ls $hailort_dir  | grep hailort-pcie)
driver_path=$(realpath "$hailort_dir/$driver_name")
yes no | sudo dpkg -i $driver_path
