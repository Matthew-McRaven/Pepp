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
ARG BOOST_VERSION="1.83.0"
# Download boost code.
RUN git clone --jobs 10 --recursive --branch boost-${BOOST_VERSION}  https://github.com/boostorg/boost

# Boost will install to /usr/local/lib
RUN cd boost && ./bootstrap.sh --prefix=/bmagic \
    && ./b2 cxxflags="-fPIC -std=c++17" address-model=64 link=static runtime-link=static -j 20 threading=multi install --build-dir=./build-native --stagedir=./stage-native --prefix=/bmagic

FROM ubuntu:20.04 as boostOnly
COPY --from=boost-build /bmagic /usr/local
RUN apt update && DEBIAN_FRONTEND=noninteractive apt install -y sudo curl wget python3-pip zstd build-essential libgl1-mesa-dev \
    libgstreamer-gl1.0-0 libpulse-dev libxcb-glx0 libxcb-icccm4 \
    libxcb-image0 libxcb-keysyms1 libxcb-randr0 libxcb-render-util0 \
    libxcb-render0 libxcb-shape0 libxcb-shm0 libxcb-sync1 libxcb-util1 \
    libxcb-xfixes0 libxcb-xinerama0 libxcb1 libxkbcommon-dev \
    libxkbcommon-x11-0 libxcb-xkb-dev \
    make zip git ripgrep doxygen clang-tidy lcov \
    libsecret-1-0 libsecret-1-dev libx11-dev libxkbfile-dev \
    lsb-release wget software-properties-common git-lfs cloc curl && rm -rf /var/lib/apt/lists/*
SHELL ["/bin/bash", "-c"]

