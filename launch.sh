#!/usr/bin/env bash
set -euo pipefail

script_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"

cmake --build "$script_dir/build" -j20 && \
gdb -batch -q -ex "run" -ex "bt" --args "$script_dir/build/bin/Renegade" --game-data-directory "$script_dir/Renegade" 2>&1 | tee "$script_dir/log.txt"
