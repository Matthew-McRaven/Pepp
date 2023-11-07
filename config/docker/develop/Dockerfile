# Will be the latest LTS by the time we release the book, which I assum to be ~ 2025.
FROM ubuntu:22.04 as base-native
# Prevent apt-get from pausing for input in an environment in which it can never
ENV DEBIAN_FRONTEND=noninteractive
# Building boost + final build image needs all of the same tools.
# Combination of explicit Qt dependencies, implicit dependencies (as reported by aqtinstall),
# and my development dependencies.
RUN apt update \
    &&  apt install -y --no-install-recommends curl bash wget python3-pip zstd jq python3-dev \
        cloc curl build-essential libgl1-mesa-dev libpulse-dev libxcb-glx0 libxcb-icccm4 \
        libxcb-image0 libxcb-keysyms1 libxcb-randr0 libxcb-render-util0 libxcb-render0 libxcb-shape0 \
        libxcb-shm0 libxcb-sync1 libxcb-util1 libxcb-xfixes0 libxcb-xinerama0 libxcb1 fontconfig ccache \
        libxkbcommon-dev libxcb-xkb-dev zip unzip tar git git-lfs lsb-release software-properties-common gnupg  \
    && rm -rf /var/lib/apt/lists/*
# Install latest cmake that does not support modules, which is 3.27. Module detection breaks CI when the compiler doesn't support modules.
RUN wget -O cmake.sh "https://github.com/Kitware/CMake/releases/download/v3.27.7/cmake-3.27.7-linux-$(arch).sh" \
    && chmod +x cmake.sh && mkdir /cmake && ./cmake.sh --skip-license --exclude-subdir --prefix=/cmake && rm cmake.sh \
    && cp -r /cmake/* /usr && rm -rf /cmake
# Install ninja, which supports modules (for future development) and generally should be faster than make.
RUN wget -O ninja.zip "https://github.com/ninja-build/ninja/releases/download/v1.11.1/ninja-linux.zip" \
    && unzip ninja.zip -d /bin && rm ninja.zip
# Need to pick this locale otherwise Qt complains and overrides the locale anyway.
ENV LANG="C.UTF-8"
# Must occur after the "FROM" directive, must be a valid tag from GitHub.
ARG BOOST_VERSION="1.83.0"
# Must occur after the "FROM" directive, allows choice of Qt version.
ARG QT_VERSION="6.6.0"
# Don't cache pip internal files, reduce image size.
ENV PIP_NO_CACHE_DIR=1
SHELL ["/bin/bash", "-c"]
LABEL org.opencontainers.image.authors="matthew.mcraven@gmail.com"


FROM base-native as base-wasm
RUN git clone https://github.com/emscripten-core/emsdk.git /emsdk
# Must combine, otherwise a layer is the same in multi-arch builds, breaking everything
RUN cd /emsdk && git pull && ./emsdk install 3.1.37  && ./emsdk activate 3.1.37 && echo uname -a /emsdk.arch


# Build Boost in separate stage to limit image bloat
# Boost will (eventually) install to /usr/local/lib
FROM base-native as boost-native-build
RUN git clone --jobs 10 --recursive --branch boost-${BOOST_VERSION}  https://github.com/boostorg/boost
WORKDIR boost
# Copy to a empty directory to make copying only the desired files easier.
RUN ./bootstrap.sh --prefix=/bmagic
RUN ./b2 cxxflags="-fPIC -std=c++17" address-model=64 link=static runtime-link=static -j $(nproc) threading=multi install --build-dir=./build --stagedir=./stage --prefix=/bmagic


# Boost will (eventually) install to /usr/local/lib
# Build Boost in separate stage to limit image bloat
FROM base-wasm as boost-wasm-build
RUN git clone --jobs 10 --recursive --branch boost-${BOOST_VERSION}  https://github.com/boostorg/boost
WORKDIR boost
# Copy to a empty directory to make copying only the desired files easier.
RUN source /emsdk/emsdk_env.sh && ./bootstrap.sh --prefix=/bmagic
# Must not specify threading for build to pass
RUN source /emsdk/emsdk_env.sh && emconfigure ./b2 cxxflags="-fPIC -std=c++17" address-model=64 link=static runtime-link=static -j $(nproc) install --build-dir=./build --stagedir=./stage --prefix=/bmagic



FROM base-wasm as output-wasm
RUN pip install aqtinstall \
    && aqt install-qt linux desktop ${QT_VERSION} wasm_singlethread --outputdir /qt  --autodesktop \
    && pip uninstall aqtinstall -y && rm aqtinstall.log
# Define standard Qt environment variables so that executing cmake directly will work.
ENV LD_LIBRARY_PATH=/qt/${QT_VERSION}/wasm_singlethread/lib:${LD_LIBRARY_PATH}
ENV Qt6_DIR=/qt/${QT_VERSION}/wasm_singlethread/lib/cmake/Qt6
ENV QT_PLUGIN_PATH=/qt/${QT_VERSION}/wasm_singlethread/plugins
ENV QML2_IMPORT_PATH=/qt/${QT_VERSION}/wasm_singlethread/qml
ENV PATH=/qt/${QT_VERSION}/wasm_singlethread/bin/:$PATH

COPY --from=boost-wasm-build /bmagic /usr/local
ENV Boost_ROOT=/usr/local
LABEL description="A wasm build container using Qt $QT_VERSION, and Boost $BOOST_VERSION."


FROM base-native as output-native
# Install Qt with aqtinstall (see https://github.com/miurahr/aqtinstall), selecting only the modules we need.
RUN pip install aqtinstall && aqt install-qt linux desktop ${QT_VERSION} --outputdir /qt && pip uninstall aqtinstall -y && rm aqtinstall.log

# Define standard Qt environment variables so that executing cmake directly will work.
ENV LD_LIBRARY_PATH=/qt/${QT_VERSION}/gcc_64/lib:${LD_LIBRARY_PATH}
ENV Qt6_DIR=/qt/${QT_VERSION}/gcc_64
ENV QT_PLUGIN_PATH=/qt/${QT_VERSION}/gcc_64/plugins
ENV QML2_IMPORT_PATH=/qt/${QT_VERSION}/gcc_64/qml
ENV PATH=/qt/${QT_VERSION}/gcc_64/bin/:$PATH

# Copy over compiled boost binaries and set environment variables.
COPY --from=boost-native-build /bmagic /usr/local
ENV Boost_ROOT=/usr/local

# Install mold, so that we can increase link times
ARG MOLD_VERSION="2.3.1"
RUN --mount=type=tmpfs,target=/mold,size=2G \
    cd mold \
    && wget -O mold.tar.gz "https://github.com/rui314/mold/releases/download/v$MOLD_VERSION/mold-$MOLD_VERSION-$(arch)-linux.tar.gz" \
    && tar xf mold.tar.gz && ls && cd mold-$MOLD_VERSION-$(arch)-linux \
    && find -type d -exec mkdir -vp ""/{} \; -or -exec mv -nv {} ""/{} \;
    # No need to clean up, all temp data should be in tmpfs
LABEL description="A native build container using Qt $QT_VERSION, Boost $BOOST_VERSION, and Mold $MOLD_VERSION."
