#!/bin/bash

set -e

readonly VERSION=3.28.0
readonly YOCTO_VALIDATION_BASE_DIR="/local/users/bu_env/hailo-yocto-validation"
readonly WORK_DIR_BASE="/local/users/bu_env/hailo-yocto-validation/build/tmp/work/armv8a-poky-linux"
readonly TAPPAS_RECIPES="libgsthailotools tappas-apps hailo-post-processes"
readonly YML_KAS_FILE="kas/CI/hailo15-evb-2-camera-vpu-tappas.yml"

function copy_git_repo_diff()
{
  pushd $src_dir

  rm -f diff.txt
  echo -e "Creating git diff\n"
  git diff > diff.txt

  popd

  # if diff is empty, exit
  if [ ! -s $src_dir/diff.txt ]; then
    echo "There are no changes to apply"
  else
    echo -e "Moving git diff changes to $target_git_dir\n"

    pushd $target_git_dir
    git reset --hard
    git apply $src_dir/diff.txt

    popd
  fi

  pushd $src_dir

  echo -e "Moving untrackd files to $target_git_dir\n"

  # Apply the git diff to the target directory
  untracked_files=$(git status -uall --porcelain | grep "^??" | awk '{print $2}')

  # Copy the non-tracked files to the target directory
  for file in $untracked_files; do
    echo "Copying $file to $target_git_dir/$file"
    if [ ! -d "$target_git_dir/$(dirname $file)" ]; then
      mkdir -p "$target_git_dir/$(dirname $file)"
    fi
    
    cp -r "$file" "$target_git_dir/$file"
  done

  popd
}

function clean_recipe_build()
{
  pushd $YOCTO_VALIDATION_BASE_DIR

  echo "Cleaning $recipe_name using KAS"

  # kas build $YML_KAS_FILE --target $recipe_name -c clean

  kas build $YML_KAS_FILE --target $recipe_name -c cleansstate

  kas build $YML_KAS_FILE --target $recipe_name -c do_unpack

  popd
}

function copy_to_rootfs()
{
  echo -e "Copying rottfs to /usr on mercury\n"

  # check if fast_mode
  if [ "$fast_mode" = true ]; then
    scp $BASE_RECIPE_WORK_DIR/build/plugins/libgsthailotools.so root@10.0.0.1:/usr/lib/gstreamer-1.0/
  else
    scp -r $build_rootfs_dir/usr/* root@10.0.0.1:/usr/
  fi
}

# parse --help argument
if [ "$1" == "-h" ] || [ "$1" == "--help" ]; then
  echo "Usage: $0 <source_dir> <recipe_name>"
  echo "  source_dir: The directory containing the source code to be compiled"
  echo "  recipe_name: The name of the recipe to be compiled"
  echo "  "
  echo "  Example: $0 /local/workspace/tappas libgsthailotools"
  exit 1
fi
skip_clean=false
# check if --skip-clean argument is passed in one of the arguments
if [[ "$@" =~ "--skip-clean" ]]; then
  skip_clean=true
fi

force_task=""
# check if --force <task> argument is passed in one of the arguments and set the task
if [[ "$@" =~ "--force-task" ]]; then
  force_task=$(echo "$@" | grep -oP "(?<=--force-task ).*")
fi
echo "force_task: $force_task"


fast_mode=false
# check if --fast is passed in one of the arguments
if [[ "$@" =~ "--fast" ]]; then
  echo "Fast mode enabled"
  fast_mode=true
  skip_clean=true
  force_task="do_compile"
fi

src_dir=$1
recipe_name=$2

BASE_RECIPE_WORK_DIR="$WORK_DIR_BASE/$recipe_name/$VERSION-r0"
target_git_dir="$BASE_RECIPE_WORK_DIR/git"
build_rootfs_dir="$BASE_RECIPE_WORK_DIR/sysroot-destdir"

# validate recipe name is in TAPPAS_RECIPES
if [[ ! $TAPPAS_RECIPES =~ (^|[[:space:]])$recipe_name($|[[:space:]]) ]]; then
  echo "Error: $recipe_name is not a valid recipe name"
  exit 1
fi

if [ ! -d $src_dir ]; then
  echo "Error: $src_dir is not a valid directory"
  exit 1
fi

if [ "$skip_clean" = false ]; then
  clean_recipe_build
fi


# check that build_dir exists
if [ ! -d $target_git_dir ]; then
  echo "Error: $target_git_dir is not a valid directory"
  exit 1
fi

echo -e "Compiling source directory : $src_dir\n"
echo -e "for recipe : $recipe_name\n"
echo -e "to target directory : $target_git_dir\n"

copy_git_repo_diff

pushd $YOCTO_VALIDATION_BASE_DIR

echo -e "Building $recipe_name using KAS\n"

# check if force_task is not empty and skip_clean is true
if [ ! -z "$force_task" ] && [ "$skip_clean" = true ]; then
  kas build $YML_KAS_FILE --target $recipe_name -c $force_task --force
else
  kas build $YML_KAS_FILE --target $recipe_name
fi


popd

# check that $build_rootfs_dir exists
if [ ! -d $build_rootfs_dir ]; then
  echo "Error: $build_rootfs_dir is not a valid directory"
  exit 1
fi

copy_to_rootfs
