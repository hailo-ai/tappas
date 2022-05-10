#!/usr/bin/env bash
# npm install -g markdown-link-check

script_dir=$(dirname $(realpath "$0"))
tappas_workspace_path=$(realpath $script_dir/../..)


 for file in $(ag -g ".md" $tappas_workspace_path)
 do
 	markdown-link-check -p -a 200,403 $file || exit 1;
 done
