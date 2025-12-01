#!/usr/bin/env bash

set -euo pipefail

program_name=sona_lexical_analyzer

(
    cd "$(dirname "$0")/.."
    ./scripts/build.sh "debug"
    gdb ./bin/Debug/"$program_name"
)
