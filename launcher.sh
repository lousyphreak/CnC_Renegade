#!/bin/sh

set -eu

script_dir="$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)"
build_dir="${script_dir}/build-emr"
cache_file="${build_dir}/CMakeCache.txt"
server_script="${script_dir}/cmake/renegade_emscripten_http_server.py"
port="${RENEGADE_EMSCRIPTEN_PORT:-8099}"
url="http://127.0.0.1:${port}/Renegade.html"

if ! command -v emcmake >/dev/null 2>&1; then
    echo "emcmake was not found in PATH." >&2
    exit 1
fi

if ! command -v python3 >/dev/null 2>&1; then
    echo "python3 was not found in PATH." >&2
    exit 1
fi

if [ -f "${cache_file}" ]; then
    if ! grep -q '^EMSCRIPTEN:INTERNAL=1$' "${cache_file}" || \
       ! grep -q '^CMAKE_BUILD_TYPE:STRING=RelWithDebInfo$' "${cache_file}"; then
        cmake -E rm -rf "${build_dir}"
    fi
fi

emcmake cmake -S "${script_dir}" -B "${build_dir}" \
    -DCMAKE_BUILD_TYPE=RelWithDebInfo \
    -DRENEGADE_EMSCRIPTEN_PACKAGE_GAME_DATA=ON \
    -DRENEGADE_EMSCRIPTEN_LAZY_FETCH_GAME_DATA=ON \
    -DRENEGADE_EMSCRIPTEN_DATA_ROOT="${script_dir}/Renegade" \
    -DRENEGADE_EMSCRIPTEN_ALLOW_MEMORY_GROWTH=ON

# cmake --build "${build_dir}" --target clean
cmake --build "${build_dir}" -j20

if [ "${RENEGADE_EMSCRIPTEN_NO_BROWSER:-0}" != "1" ]; then
    (
        sleep 1
        if command -v xdg-open >/dev/null 2>&1; then
            xdg-open "${url}" >/dev/null 2>&1 || true
        else
            python3 -m webbrowser "${url}" >/dev/null 2>&1 || true
        fi
    ) &
fi

echo "Launching Renegade at ${url}"
exec python3 "${server_script}" --directory "${build_dir}/bin" "${port}"
