source /emsdk/emsdk_env.sh
cmake -B build -D CMAKE_TOOLCHAIN_FILE=/qt/6.7.2/wasm_singlethread/lib/cmake/Qt6/qt.toolchain.cmake  -D QT_HOST_PATH=/qt/6.7.2/gcc_64 .
cd build && cmake --build . -j16