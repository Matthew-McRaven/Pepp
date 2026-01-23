ARG BASE_IMAGE=ubuntu:24.04
# Will be the latest LTS by the time we release the book, which I assume to be ~ 2025.
FROM ${BASE_IMAGE} AS base-gcc
# Prevent apt-get from pausing for input in an environment in which it can never
ENV DEBIAN_FRONTEND=noninteractive
# Building boost + final build image needs all of the same tools.
# Combination of explicit Qt dependencies, implicit dependencies (as reported by aqtinstall),
# and my development dependencies.
RUN apt update \
    &&  apt install -y --no-install-recommends \
        bash build-essential ccache cloc curl curl fontconfig git git-lfs gnupg jq libgl1-mesa-dev libpulse-dev \
        libxcb-glx0 libxcb-icccm4 libxcb-image0 libxcb-keysyms1 libxcb-randr0 libxcb-render-util0 libxcb-render0 \
        libxcb-shape0 libxcb-shm0 libxcb-sync1 libxcb-util1 libxcb-xfixes0 libxcb-xinerama0 libxcb-xkb-dev \
        libxcb1 libxkbcommon-dev lsb-release python3-dev python3-pip software-properties-common tar\
        unzip wget zip zstd \
    && rm -rf /var/lib/apt/lists/*
# Install latest cmake that does not support modules, which is 3.27. Module detection breaks CI when the compiler doesn't support modules.
# Remove cmake docs, since that's 40MB we really don't need.
RUN wget -q -O cmake.sh "https://github.com/Kitware/CMake/releases/download/v3.27.7/cmake-3.27.7-linux-$(arch).sh" \
    && chmod +x cmake.sh && mkdir /cmake && ./cmake.sh --skip-license --exclude-subdir --prefix=/cmake && rm cmake.sh \
    && cp -r /cmake/* /usr && rm -rf /cmake && rm -rf /usr/doc/cmake
# Install ninja, which supports modules (for future development) and generally should be faster than make.
RUN wget -q -O ninja.zip "https://github.com/ninja-build/ninja/releases/download/v1.11.1/ninja-linux.zip" \
    && unzip ninja.zip -d /bin && rm ninja.zip

# Need to pick this locale otherwise Qt complains and overrides the locale anyway.
ENV LANG="C.UTF-8"
# Must occur after the "FROM" directive, allows choice of Qt version.
ARG QT_MINOR="10"
ARG QT_PATCH="1"
# major/minor
ARG QT_RELEASE="6.${QT_MINOR}"
# major/minor/patch
ARG QT_VERSION="6.${QT_MINOR}.${QT_PATCH}"
# Depends on major/minor
ARG EMSDK_VERSION="4.0.7"
# Don't cache pip internal files, reduce image size.
ENV PIP_NO_CACHE_DIR=1
SHELL ["/bin/bash", "-c"]
LABEL org.opencontainers.image.authors="Matthew McRaven <matthew.mcraven@gmail.com>"
LABEL org.opencontainers.image.source="https://github.com/Matthew-McRaven/Pepp"

FROM base-gcc AS build-gcc-riscv
ENV RISCV=/opt/riscv
ENV PATH=$PATH:$RISCV/bin
ARG TOOLCHAIN_VERSION=2025.12.18
RUN apt update \
    && apt-get install -y --no-install-recommends autoconf automake autotools-dev \
        curl python3 python3-pip python3-tomli libmpc-dev libmpfr-dev \
        libgmp-dev gawk build-essential bison flex texinfo gperf libtool patchutils bc zlib1g-dev \
        libexpat-dev ninja-build git cmake libglib2.0-dev libslirp-dev  \
    && rm -rf /var/lib/apt/lists/*
RUN git clone --recursive https://github.com/riscv/riscv-gnu-toolchain -b ${TOOLCHAIN_VERSION}
RUN cd riscv-gnu-toolchain && ./configure --prefix=$RISCV --enable-multilib
RUN cd riscv-gnu-toolchain && make -j8

FROM base-gcc AS output-gcc-riscv
ENV RISCV=/opt/riscv
ENV PATH=$PATH:$RISCV/bin
COPY --from=build-gcc-riscv /opt/riscv /opt/riscv

FROM base-gcc AS base-wasm
# Must install documentation-building tools in WASM image.
# Otherwise it will be very difficult to build GH pages deployments.
RUN apt update \
    && apt install -y --no-install-recommends graphviz imagemagick python3.12-venv \
    && rm -rf /var/lib/apt/lists/*
RUN git clone https://github.com/emscripten-core/emsdk.git /emsdk
# Must combine, otherwise a layer is the same in multi-arch builds, breaking everything
RUN cd /emsdk && git pull && ./emsdk install ${EMSDK_VERSION} && ./emsdk activate ${EMSDK_VERSION} && echo uname -a /emsdk.arch
# Install shadertools and 3d modules otherwise building from sources will fail.
RUN python3 -m venv /venv && source /venv/bin/activate \
  && pip install aqtinstall &&  aqt install-qt linux desktop ${QT_VERSION} -m qtshadertools qtquick3d qt3d --outputdir /qt \
  && rm aqtinstall.log && rm -rf /venv

# Use separate step to build Qt to avoid inflating the size of the build image further.
FROM base-wasm AS build-wasm
# Install Qt requirements: https://doc.qt.io/qt-6/linux-requirements.html
RUN apt update \
    && apt install -y --no-install-recommends \
       libfreetype-dev libx11-dev libx11-xcb-dev libxcb-cursor-dev libxcb-glx0-dev libxcb-icccm4-dev \
       libxcb-image0-dev libxcb-keysyms1-dev libxcb-randr0-dev libxcb-render-util0-dev libxcb-shape0-dev \
       libxcb-shm0-dev libxcb-sync-dev libxcb-util-dev libxcb-xfixes0-dev libxcb-xinerama0-dev libxcb-xkb-dev \
       libxcb1-dev libxext-dev libxfixes-dev libxi-dev libxkbcommon-dev libxkbcommon-x11-dev libxrender-dev\
    && rm -rf /var/lib/apt/lists/*
# Get sources for the current version of Qt.
RUN wget -q "https://download.qt.io/official_releases/qt/${QT_RELEASE}/${QT_VERSION}/single/qt-everywhere-src-${QT_VERSION}.tar.xz" \
    && mkdir -p /build-qt \
    && tar xf qt-everywhere-src-${QT_VERSION}.tar.xz --directory /build-qt \
    && mv /build-qt/qt-everywhere-src-${QT_VERSION} /build-qt/${QT_VERSION} \
    && rm qt-everywhere-src-${QT_VERSION}.tar.xz

# Extra args with which to build Qt
ARG QT_WASM_XARGS=""
# Must activate EMSDK or it will not build. Skip some features that would require installing additional libraries.
# Enable excptions, because that was the whole point of building Qt from the sources
RUN source /emsdk/emsdk_env.sh \
    && cd /build-qt/${QT_VERSION} \
    && ./configure -qt-host-path /qt/${QT_VERSION}/gcc_64 -platform wasm-emscripten \
       -no-feature-thread -feature-wasm-exceptions ${QT_WASM_XARGS} \
       -skip qtremoteobjects -skip qtwebengine -skip qtscxml -skip qthttpserver \
       -skip qtwebview -skip qtwebchannel -skip qtsensors -skip qtwebsockets -skip qtlocation \
       -skip qtpositioning -skip qtactiveqt -skip qt5compat -skip qtlottie \
       -prefix /qt/${QT_VERSION}/wasm_singlethread
# Separate build from configure so that I get better logs
RUN cd /build-qt/${QT_VERSION} \
    && cmake --build . --parallel \
    && cmake --install .

# Only copy over WASM artifacts, since we already have GCC from the base image.
FROM base-wasm AS output-wasm
COPY --from=build-wasm /qt/${QT_VERSION}/wasm_singlethread /qt/${QT_VERSION}/wasm_singlethread
# Define standard Qt environment variables so that executing cmake directly will work.
ENV LD_LIBRARY_PATH=/qt/${QT_VERSION}/wasm_singlethread/lib:${LD_LIBRARY_PATH}
ENV Qt6_DIR=/qt/${QT_VERSION}/wasm_singlethread/lib/cmake/Qt6
ENV QT_PLUGIN_PATH=/qt/${QT_VERSION}/wasm_singlethread/plugins
ENV QML2_IMPORT_PATH=/qt/${QT_VERSION}/wasm_singlethread/qml
ENV PATH=/qt/${QT_VERSION}/wasm_singlethread/bin/:$PATH
LABEL description="A wasm build container using Qt $QT_VERSION."

FROM base-gcc AS output-gcc
RUN apt update \
    && apt install -y --no-install-recommends python3.12-venv \
    && rm -rf /var/lib/apt/lists/*
# Install Qt with aqtinstall (see https://github.com/miurahr/aqtinstall), selecting only the modules we need.
RUN python3 -m venv /venv && source /venv/bin/activate \
  && pip install aqtinstall && aqt install-qt linux desktop ${QT_VERSION} --outputdir /qt \
  && rm aqtinstall.log && rm -rf /venv
# Define standard Qt environment variables so that executing cmake directly will work.
ENV LD_LIBRARY_PATH=/qt/${QT_VERSION}/gcc_64/lib:${LD_LIBRARY_PATH}
ENV Qt6_DIR=/qt/${QT_VERSION}/gcc_64
ENV QT_PLUGIN_PATH=/qt/${QT_VERSION}/gcc_64/plugins
ENV QML2_IMPORT_PATH=/qt/${QT_VERSION}/gcc_64/qml
ENV PATH=/qt/${QT_VERSION}/gcc_64/bin/:$PATH
# Install mold, so that we can reduce link times
ARG MOLD_VERSION="2.40.4"
RUN --mount=type=tmpfs,target=/mold,size=2G \
    cd mold \
    && wget -q -O mold.tar.gz "https://github.com/rui314/mold/releases/download/v$MOLD_VERSION/mold-$MOLD_VERSION-$(arch)-linux.tar.gz" \
    && tar xf mold.tar.gz && ls && cd mold-$MOLD_VERSION-$(arch)-linux \
    && find -type d -exec mkdir -vp ""/{} \; -or -exec mv -nv {} ""/{} \;
    # No need to clean up, all temp data should be in tmpfs
LABEL description="A GCC build container using Qt $QT_VERSION, and Mold $MOLD_VERSION."

FROM output-gcc AS output-clang
ARG LLVM_VERSION="20"
RUN apt update && apt install  --no-install-recommends -y \
    ca-certificates clang-$LLVM_VER clang-tools-$LLVM_VERSION clang-format-$LLVM_VERSION \
    libfuzzer-${LLVM_VERSION}-dev lld-$LLVM_VERSION llvm-$LLVM_VERSION \
    libclang-rt-$LLVM_VERSION-dev \
    && ln -s /usr/bin/clang++-$LLVM_VERSION /usr/bin/clang++ \
    && ln -s /usr/bin/clang-$LLVM_VERSION /usr/bin/clang \
    && rm -rf /var/lib/apt/lists/*
LABEL description="A clang $LLVM_VERSION build container using Qt $QT_VERSION."
