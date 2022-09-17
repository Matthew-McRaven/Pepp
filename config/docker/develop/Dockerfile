# See here for image contents: https://github.com/microsoft/vscode-dev-containers/tree/v0.154.2/containers/cpp/.devcontainer/base.Dockerfile

# [Choice] Debian / Ubuntu version: debian-10, debian-9, ubuntu-20.04, ubuntu-18.04
FROM mcr.microsoft.com/vscode/devcontainers/javascript-node:18-bullseye as base

SHELL ["/bin/bash", "-c"]

# Install additional packages.
RUN apt-get update && export DEBIAN_FRONTEND=noninteractive \
     && apt-get -y install --no-install-recommends make zip git \
     vim ripgrep doxygen clang-tidy ssh-client lcov libsecret-1-0 libsecret-1-dev\
     libx11-dev libxkbfile-dev lsb-release wget software-properties-common git-lfs \
     cloc

WORKDIR /

FROM base as boost-build
# Must occur after the "FROM" directive
ARG BOOST_VERSION="1.76.0"
# Download boost code.
RUN git clone --jobs 10 --recursive --branch boost-${BOOST_VERSION}  https://github.com/boostorg/boost

# Boost will install to /usr/local/lib
RUN cd boost && ./bootstrap.sh --prefix=/bmagic \
    && ./b2 cxxflags="-fPIC -std=c++17" address-model=64 link=static runtime-link=static -j 20 threading=multi install --build-dir=./build-native --stagedir=./stage-native --prefix=/bmagic

FROM base as final
COPY --from=boost-build /bmagic /usr/local


# Really remove any traces of existing yarn, then reinstall.
RUN apt remove -y yarn || true && corepack prepare yarn@3.2.1 --activate && corepack enable
RUN wget https://apt.llvm.org/llvm.sh && chmod +x llvm.sh && ./llvm.sh 14 && rm llvm.sh
RUN wget -O cmake.sh "https://github.com/Kitware/CMake/releases/download/v3.24.2/cmake-3.24.2-linux-$(arch).sh" && bash cmake.sh --skip-license --exclude-subdir


