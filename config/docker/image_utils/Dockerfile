FROM ubuntu:22.04
# Prevent apt-get from pausing for input in an environment in which it can never
ENV DEBIAN_FRONTEND=noninteractive
RUN apt update \
    && apt install -y imagemagick icnsutils librsvg2-bin \
    && rm -rf /var/lib/apt/lists/*
LABEL org.opencontainers.image.authors="Matthew McRaven <matthew.mcraven@gmail.com>"
LABEL org.opencontainers.image.source="https://github.com/Matthew-McRaven/Pepp"
