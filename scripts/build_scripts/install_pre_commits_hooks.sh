#!/bin/bash
# Internal script that installs all pre_commits_hooks requirements
set -e
pip3 install pre-commit-hooks 
sudo apt install clang clang-format clang-tidy uncrustify cppcheck iwyu