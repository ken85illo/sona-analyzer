#!/usr/bin/env bash

set -euo pipefail

program_name=sona_lexical_analyzer

if [[ $# -ne 1 ]]; then
    echo "Usage: ./run.sh <release|debug>"
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
    ./scripts/build.sh "$build_type"
    ./bin/"$build_type"/"$program_name"
)




