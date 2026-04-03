FROM mcr.microsoft.com/devcontainers/base:debian-13

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    ninja-build \
    clangd \
    clang-format \
    clang-tidy \
    ccache \
    git \
    curl \
    pkg-config \
    python3 \
    python3-pip \
    python3-venv \
    ca-certificates \
    mosquitto-clients \
    doxygen \
 && rm -rf /var/lib/apt/lists/*

RUN python3 -m pip install --no-cache-dir --break-system-packages conan

RUN mkdir -p /home/vscode/.conan2 /home/vscode/.cache/ccache \
 && chown -R vscode:vscode /home/vscode/.conan2 /home/vscode/.cache

USER vscode
