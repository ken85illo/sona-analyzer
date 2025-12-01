#!/usr/bin/env bash

set -euo pipefail

if [[ $# -ne 1 ]]; then
    echo "Usage: ./rebuild.sh <release|debug>"
    exit 1
fi

build_type=$(echo "$1" | tr '[:upper:]' '[:lower:]')

if [[ "$build_type" == "release" ]]; then
    build_type="Release"
elif [[ "$build_type" == "debug" ]]; then
    build_type="Debug"
else
    echo "Usage: ./rebuild.sh <release|debug>"
    exit 1
fi

(
    cd "$(dirname "$0")/.."
    rm -rf build/"$build_type"
    scripts/build.sh "$build_type"

)
