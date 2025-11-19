#!/usr/bin/env bash

set -euo pipefail


if [[ $# -ne 1 ]]; then
    echo "Usage: ./build.sh <release|debug>"
    exit 1
fi

build_type=$(echo "$1" | tr '[:upper:]' '[:lower:]')

if [[ "$build_type" == "release" ]]; then
    build_type="Release"
elif [[ "$build_type" == "debug" ]]; then
    build_type="Debug"
else
    echo "Usage: ./build.sh <release|debug>"
    exit 1
fi


(
    cd "$(dirname "$0")/.."

    # Prioritize Ninja over Makefile
    if command -v ninja >/dev/null 2>&1; then
        generator="Ninja"
    elif command -v make >/dev/null 2>&1; then
        generator='Unix Makefiles'
    else
        echo "No 'ninja' or 'make' defined in PATH"
        exit 1
    fi

    build_dir=build/"$build_type"

    if [[ ! -d "$build_dir" ]]; then
        mkdir -p "$build_dir"
        cmake -G "$generator" -B "$build_dir" -DCMAKE_BUILD_TYPE="$build_type"
    fi

    echo "Building in $build_type mode..."
    cmake --build "$build_dir"
    mkdir -p bin/"$build_type"
    cp -rf "$build_dir"/bin/* bin/"$build_type"
    cp -f "$build_dir"/compile_commands.json "$build_dir"/..
    echo "CMake has completed building the project..."
    echo "Output can be found in bin/$build_type"
)
