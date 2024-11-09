source /emsdk/emsdk_env.sh
chmod +x /qt/6.6.0/wasm_singlethread/bin/qt-cmake

qt-cmake \
  -S /pepp -B /build -D USE_SYSTEM_BOOST=on -D VCPKG_TARGET_TRIPLET=wasm32-emscripten \
  -D Boost_DEBUG=ON -D Boost_INCLUDE_DIR=/usr/local/include \
  -D QT_HOST_PATH=/qt/6.6.0/gcc_64

cmake --build /build -j32

