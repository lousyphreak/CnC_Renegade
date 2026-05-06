#!/usr/bin/env bash
set -euo pipefail

script_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"

cmake --build "$script_dir/build-rel" -j20 && \
"$script_dir/build-rel/bin/Renegade" --game-data-directory "$script_dir/Renegade" 2>&1 | tee "$script_dir/log.txt"
