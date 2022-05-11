# See here for image contents: https://github.com/microsoft/vscode-dev-containers/tree/v0.154.2/containers/cpp/.devcontainer/base.Dockerfile

# [Choice] Debian / Ubuntu version: debian-10, debian-9, ubuntu-20.04, ubuntu-18.04
ARG VARIANT="ubuntu-22.04"


FROM mcr.microsoft.com/vscode/devcontainers/cpp:0-${VARIANT}

# Must occur after the "FROM" directive
ARG BOOST_VERSION="1.76.0"
SHELL ["/bin/bash", "-c"]

# Required to move an ARG to an environment variable.
RUN echo $BOOST_VERSION
# Install additional packages.
RUN apt-get update && export DEBIAN_FRONTEND=noninteractive \
     && apt-get -y install --no-install-recommends g++-10 make zip cmake git libsqlite3-dev zlib1g-dev \
     libreadline-dev libncursesw5-dev libssl-dev tk-dev libgdbm-dev libc6-dev libbz2-dev libffi-dev \
     vim ripgrep doxygen clang-tidy ssh-client lcov libsecret-1-0 \
     libx11-dev libxkbfile-dev lsb-release wget software-properties-common

WORKDIR /

# Install LLVM 14
RUN wget https://apt.llvm.org/llvm.sh && chmod +x llvm.sh &&  ./llvm.sh 14 && rm llvm.sh

# Download boost code.
RUN git clone --jobs 10 --recursive --branch boost-${BOOST_VERSION}  https://github.com/boostorg/boost && git clone https://github.com/emscripten-core/emsdk.git /emsdk

# Install EMSDK, yarn
# Must combine, otherwise a layer is the same in multi-arch builds, breaking everything
RUN cd /emsdk && git pull && ./emsdk install 3.1.8  && ./emsdk activate 3.1.8 && source /emsdk/emsdk_env.sh && npm i -g corepack && yarn set version stable

# Boost will install to /usr/local/lib
RUN cd boost && ./bootstrap.sh \
    && ./b2 cxxflags="-fPIC -std=c++17" address-model=64 link=static runtime-link=static -j 20 threading=multi --build-dir=./build-native --stagedir=./stage-native \
    && ./b2 cxxflags="-fPIC -std=c++17" address-model=64 link=static runtime-link=static -j 20 threading=multi install --build-dir=./build-native --stagedir=./stage-native

RUN source /emsdk/emsdk_env.sh \
    && cd boost \
    && ./b2 --clean-all -n \
    && emconfigure ./b2 cxxflags="-fPIC -std=c++17" link=static runtime-link=static -j 20 --build-dir=./build-js --stagedir=./stage-js \
    && emconfigure ./b2 cxxflags="-fPIC -std=c++17" link=static runtime-link=static -j 20 install --build-dir=./build-js --stagedir=./stage-js
