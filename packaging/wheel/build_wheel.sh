#!/usr/bin/env bash
set -euo pipefail

# Builds the tappas core wheel from existing compiled artifacts.
# Usage: ./build_wheel.sh --tappas-dir </path/to/tappas> [--python-version <version>]

script_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"

function print_usage() {
  echo "Usage: ${BASH_SOURCE[0]} --tappas-dir </path/to/tappas> [--python-version <version>]" >&2
  exit 1
}

python_version="3"
repo_root=""

# Parse arguments
while [[ $# -gt 0 ]]; do
  case $1 in
    --tappas-dir)
      repo_root="$2"
      shift 2
      ;;
    --python-version)
      python_version="$2"
      shift 2
      ;;
    *)
      echo "Unknown parameter: $1" >&2
      print_usage
      ;;
  esac
done

if [[ -z "${repo_root}" ]]; then
  echo "Error: --tappas-dir is required" >&2
  print_usage
fi

repo_root="$(cd -- "${repo_root}" && pwd)"

# Extract version from meson.build
tappas_version=$(grep -a1 project "${repo_root}/core/hailo/meson.build" | grep version | cut -d':' -f2 | tr -d "', ")
export TAPPAS_VERSION="${tappas_version}"

wheel_content_dir="${script_dir}/"
plugins_glob="${repo_root}/core/hailo/build.release/plugins/hailo.cpython-"*"-linux-gnu.so"
gsthailo_src="${repo_root}/core/hailo/python/gsthailo"

# Clean previous wheel artifacts
if command -v git >/dev/null 2>&1 && git -C "${repo_root}" rev-parse --show-toplevel >/dev/null 2>&1; then
  git -C "${repo_root}" clean -ffdx packaging/wheel || true
fi

if ! compgen -G "${plugins_glob}" > /dev/null; then
  echo "No tappas pybind shared objects found under ${repo_root}/core/hailo/build.release/plugins" >&2
  exit 1
fi

if [[ ! -d "${gsthailo_src}" ]]; then
  echo "Missing gsthailo package at ${gsthailo_src}" >&2
  exit 1
fi

# Clean previous wheel artifacts
rm -rf "${wheel_content_dir}/hailo.cpython-"*"-linux-gnu.so"
rm -rf "${wheel_content_dir}/gsthailo"

# Copy new wheel artifacts
cp ${plugins_glob} "${wheel_content_dir}/"
cp -a "${gsthailo_src}" "${wheel_content_dir}/"

# Build wheel inside a local virtualenv
pushd "${script_dir}" >/dev/null

python${python_version} -m venv build_venv || true
. build_venv/bin/activate
pip install -r requirements.txt
python3 -m build --wheel
deactivate 2>/dev/null || true
popd >/dev/null

# Copy built wheels into repo build/ directory
mkdir -p "${repo_root}/build"
cp "${script_dir}/dist/"*.whl "${repo_root}/build/"

# Remove the temporary virtual environment
rm -rf "${script_dir}/build_venv"

echo "Wheel build completed successfully!"
echo "  - Built wheels copied to: ${repo_root}/build"
