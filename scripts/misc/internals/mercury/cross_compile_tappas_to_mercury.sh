# parse --help argument
if [ "$1" == "-h" ] || [ "$1" == "--help" ]; then
  echo "Usage: $0 <build type> <toolchain tar path>"
  echo "  build type: release/debug"
  echo "  toolchain tar path"
  echo "  "
  echo "  Optional arguments:"
  echo "  --build-lib <lib name>: build only the specified library ([all, apps, libs, plugins, tracers] default: all))"
  echo "  --remove-cache: remove the cache before building"
  echo "  Example: $0 /local/workspace/tappas libgsthailotools"
  exit 1
fi


build_type=$1
toolchain_tar_path=$2

build_lib="all"

# check if --build-lib argument is passed in one of the arguments. set the build_lib to the value passed in
if [[ "$@" =~ "--build-lib" ]]; then
    build_lib=$(echo "$@" | grep -oP "(?<=--build-lib ).*")
fi
command="./tools/cross_compiler/cross_compile_gsthailotools.py armv8a hailo15 $build_type $toolchain_tar_path --build-lib $build_lib"

if [[ "$@" =~ "--remove-cache" ]]; then
    command="$command --remove-cache"
fi

echo "Executing command: $command"
output=$($command)
echo "$output"
# Validate output
if [[ $output =~ "build stopped" ]]; then
  echo "Error: Build failed"
  exit 1
fi

toolchain_rootfs_dir="tools/cross_compiler/unpacked-toolchain/sysroots/armv8a-poky-linux"
toolchain_rootfs_dir=$(realpath $toolchain_rootfs_dir)
echo "toolchain_rootfs_dir: $toolchain_rootfs_dir"
build_dir="tools/cross_compiler/armv8a-gsthailotools-build-$build_type"
pushd $build_dir
# Run meson introspect to get installed files and their destinations
output=$(meson introspect --installed | tr ',' '\n')

remote_ip="root@10.0.0.1"

while read -r line; do
    file=$(echo "$line" | awk -F'"' '{print $2}')
    dest=$(echo "$line" | awk -F'"' '{print $4}')

    # check if file exists
    if [ -f "$file" ]; then
        # check if dest contains $toolchain_rootfs_dir
        if [[ $dest =~ $toolchain_rootfs_dir ]]; then
            dest="${dest//$toolchain_rootfs_dir/}"
            echo "scp $file $remote_ip:$dest"
            if [[ $file =~ "libgsthailotools.so" ]]; then
                scp $file $remote_ip:$dest
                scp $file $remote_ip:/usr/lib/gstreamer-1.0/libgsthailotools.so
            elif [[ $file =~ "libgsthailometa.so" ]]; then
                scp $file $remote_ip:$dest
            fi
        fi
    fi
done <<< "$output"


