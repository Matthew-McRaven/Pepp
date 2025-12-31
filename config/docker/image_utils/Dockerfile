FROM ubuntu:22.04 AS build
# Prevent apt-get from pausing for input
ENV DEBIAN_FRONTEND=noninteractive
RUN apt update && apt install -y build-essential curl
RUN curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh -s -- -y
# Add .cargo/bin to PATH
ENV PATH="/root/.cargo/bin:${PATH}"
RUN cargo install resvg
RUN which resvg
FROM python:3.13 AS output
ENV DEBIAN_FRONTEND=noninteractive
RUN apt update \
    && apt install -y --no-install-recommends imagemagick icnsutils librsvg2-bin \
    && rm -rf /var/lib/apt/lists/*
RUN pip install --no-cache-dir icnsutil pillow
COPY --from=build /root/.cargo/bin/resvg /usr/local/bin/resvg
ENTRYPOINT ["/bin/bash"]
LABEL org.opencontainers.image.authors="Matthew McRaven <matthew.mcraven@gmail.com>"
LABEL org.opencontainers.image.source="https://github.com/Matthew-McRaven/Pepp"
