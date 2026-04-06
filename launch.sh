cmake --build build -j32 && \
gdb -batch -q -ex "run" -ex "bt" --args ./build/bin/Renegade 2>&1 | tee log.txt
