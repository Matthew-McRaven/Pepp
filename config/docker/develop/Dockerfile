# See here for image contents: https://github.com/microsoft/vscode-dev-containers/tree/v0.154.2/containers/cpp/.devcontainer/base.Dockerfile

# [Choice] Debian / Ubuntu version: debian-10, debian-9, ubuntu-20.04, ubuntu-18.04
ARG VARIANT="ubuntu-22.04"


FROM mcr.microsoft.com/vscode/devcontainers/cpp:0-${VARIANT} as base

SHELL ["/bin/bash", "-c"]

# Install additional packages.
RUN apt-get update && export DEBIAN_FRONTEND=noninteractive \
     && apt-get -y install --no-install-recommends make zip cmake git \
     vim ripgrep doxygen clang-tidy ssh-client lcov libsecret-1-0 libsecret-1-dev\
     libx11-dev libxkbfile-dev lsb-release wget software-properties-common git-lfs \
     cloc

WORKDIR /

FROM base as boost-build
# Must occur after the "FROM" directive
ARG BOOST_VERSION="1.76.0"
# Download boost code.
RUN git clone --jobs 10 --recursive --branch boost-${BOOST_VERSION}  https://github.com/boostorg/boost && git clone https://github.com/emscripten-core/emsdk.git /emsdk

# Install EMSDK
# Must combine, otherwise a layer is the same in multi-arch builds, breaking everything
RUN cd /emsdk && git pull && ./emsdk install 3.1.8  && ./emsdk activate 3.1.8 && echo uname -a /arch.delete

# Boost will install to /usr/local/lib
RUN cd boost && ./bootstrap.sh --prefix=/bmagic \
    && ./b2 cxxflags="-fPIC -std=c++17" address-model=64 link=static runtime-link=static -j 20 threading=multi install --build-dir=./build-native --stagedir=./stage-native --prefix=/bmagic

RUN source /emsdk/emsdk_env.sh \
    && cd boost \
    && ./b2 --clean-all -n \
    && emconfigure ./b2 cxxflags="-fPIC -std=c++17" link=static runtime-link=static -j 20 install --build-dir=./build-js --stagedir=./stage-js --prefix=/bmagic



FROM base as final
COPY --from=boost-build /bmagic /usr/local
COPY --from=boost-build /emsdk /emsdk
# Really remove any traces of existing yarn.
RUN apt remove -y yarn || true
# Install yarn into emsdk node environment
RUN echo uname -a /arch && source /emsdk/emsdk_env.sh && npm i -g corepack && corepack prepare yarn@3.2.1 --activate && corepack enable
