cmake --build build -j32 && \
gdb -batch -q -ex "run" -ex "bt" --args ./build/bin/Renegade