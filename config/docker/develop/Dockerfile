# Will be the latest LTS by the time we release the book, which I assum to be ~ 2025.
FROM ubuntu:22.04 as base
# Prevent apt-get from pausing for input in an environment in which it can never
ENV DEBIAN_FRONTEND=noninteractive
# Building boost + final build image needs all of the same tools.
# Combination of explicit Qt dependencies, implicit dependencies (as reported by aqtinstall),
# and my development dependencies.
RUN apt update \
    &&  apt install -y --no-install-recommends curl bash wget python3-pip zstd jq python3-dev \
        cloc curl build-essential libgl1-mesa-dev libpulse-dev libxcb-glx0 libxcb-icccm4 \
        libxcb-image0 libxcb-keysyms1 libxcb-randr0 libxcb-render-util0 libxcb-render0 libxcb-shape0 \
        libxcb-shm0 libxcb-sync1 libxcb-util1 libxcb-xfixes0 libxcb-xinerama0 libxcb1 fontconfig \
        libxkbcommon-dev libxcb-xkb-dev zip unzip tar git git-lfs lsb-release software-properties-common gnupg  \
    && rm -rf /var/lib/apt/lists/*

SHELL ["/bin/bash", "-c"]



# Build Boost in separate stage to limit image bloat
FROM base as boost-build
# Must occur after the "FROM" directive, must be a valid tag from GitHub.
ARG BOOST_VERSION="1.83.0"
RUN git clone --jobs 10 --recursive --branch boost-${BOOST_VERSION}  https://github.com/boostorg/boost
# Boost will (eventually) install to /usr/local/lib
WORKDIR boost
# Copy to a empty directory to make copying only the desired files easier.
RUN ./bootstrap.sh --prefix=/bmagic
RUN ./b2 cxxflags="-fPIC -std=c++17" address-model=64 link=static runtime-link=static -j $(nproc) threading=multi install --build-dir=./build-native --stagedir=./stage-native --prefix=/bmagic



# Final output image, should be minimum possible size.
# Install true app build dependencies: Qt, cmake, and ninja.
FROM base as output
# Must occur after the "FROM" directive, allows choice of Qt version.
ARG QT_VERSION="6.6.0"
# Don't cache pip internal files, reduce image size.
ENV PIP_NO_CACHE_DIR=1
# Install Qt with aqtinstall (see https://github.com/miurahr/aqtinstall), selecting only the modules we need.
RUN pip install aqtinstall && aqt install-qt linux desktop ${QT_VERSION} --outputdir /qt && pip uninstall aqtinstall -y && rm aqtinstall.log
# Install latest cmake that does not support modules, which is 3.27. Module detection breaks CI when the compiler doesn't support modules.
RUN wget -O cmake.sh "https://github.com/Kitware/CMake/releases/download/v3.27.7/cmake-3.27.7-linux-$(arch).sh" \
    && chmod +x cmake.sh && mkdir /cmake && ./cmake.sh --skip-license --exclude-subdir --prefix=/cmake && rm cmake.sh \
    && cp -r /cmake/* /usr && rm -rf /cmake
# Define standard Qt environment variables so that executing cmake directly will work.
ENV LD_LIBRARY_PATH=/qt/${QT_VERSION}/gcc_64/lib:${LD_LIBRARY_PATH}
ENV Qt6_DIR=/qt/${QT_VERSION}/gcc_64
ENV QT_PLUGIN_PATH=/qt/${QT_VERSION}/gcc_64/plugins
ENV QML2_IMPORT_PATH=/qt/${QT_VERSION}/gcc_64/qml
ENV PATH=/qt/${QT_VERSION}/gcc_64/bin/:$PATH
# Install ninja, which supports modules (for future development) and generally should be faster than make.
RUN wget -O ninja.zip "https://github.com/ninja-build/ninja/releases/download/v1.11.1/ninja-linux.zip" \
    && unzip ninja.zip -d /bin && rm ninja.zip
# Don't install LLVM, because it takes up too much image space (~7GB)
# If you do go this route in the future, you will need to make this arch-aware since we do aarch64 and x64 builds.
## Install LLVM, which allows for C++ modules support
# RUN wget https://apt.llvm.org/llvm.sh && chmod +x llvm.sh && ./llvm.sh 16 && rm llvm.sh
# RUN --mount=type=tmpfs,target=/clang,size=10G cd clang && wget -O llvm.tar.xz "https://github.com/llvm/llvm-project/releases/download/llvmorg-17.0.2/clang+llvm-17.0.2-x86_64-linux-gnu-ubuntu-22.04.tar.xz" \
#    && mkdir -p /clang && tar -xf llvm.tar.xz -C /clang && cd clang+llvm-17.0.2-x86_64-linux-gnu-ubuntu-22.04 \
#    && find -type d -exec mkdir -vp ""/{} \; -or -exec mv -nv {} ""/{} \;

# Need to pick this locale otherwise Qt complains and overrides the locale anyway.
ENV LANG="C.UTF-8"

# Copy over compiled boost binaries and set environment variables.
COPY --from=boost-build /bmagic /usr/local
ENV Boost_ROOT=/usr/local

LABEL org.opencontainers.image.authors="matthew.mcraven@gmail.com"
LABEL description="A build container using Qt $QT_VERSION and Boost $BOOST_VERSION."

# Set up ccache to cache all compilations, skipping install in CI.
RUN apt update && apt install -y ccache && rm -rf /var/lib/apt/lists/*
