# Dev container for the P1AM Wokwi Lab.
#
# Provides everything the two builds need, so nothing has to be installed on the
# host:
#   - wasi-sdk  -> `make chip`   (compiles the custom chip to WASM)
#   - PlatformIO -> `pio run -e pico` (RP2040 firmware) and `pio test -e native`
#   - g++/make   -> `make host-test`
#
# wasi-sdk is self-contained (clang + wasi-sysroot + wasm builtins), which is
# why WASI_ROOT points into it and the chip Makefile's default `clang` resolves
# to wasi-sdk's clang via PATH.
#
# If the wasi-sdk download 404s, bump WASI_SDK_VER to a current release
# (see https://github.com/WebAssembly/wasi-sdk/releases) and check the asset
# name. The zero-setup fallback is to let CI build the chip and download the
# `chip` artifact (see docs/DESIGN.md and .github/workflows/ci.yml).
FROM mcr.microsoft.com/devcontainers/base:debian-12

ARG TARGETARCH
ARG WASI_SDK_VER=24.0

RUN apt-get update && apt-get install -y --no-install-recommends \
        build-essential make git curl ca-certificates xz-utils \
        python3 python3-pip python3-venv \
    && rm -rf /var/lib/apt/lists/*

# Corporate TLS-intercepting proxy support. If your network MITMs TLS (curl
# error 60 "unable to get local issuer certificate" during build), populate
# .devcontainer/certs/ with your host's root CAs and rebuild. On macOS just run
#   .devcontainer/import-host-certs.sh
# (exports the System-keychain roots, incl. e.g. the Zscaler root). With no
# certs present this is a harmless no-op.
COPY certs/ /usr/local/share/ca-certificates/
RUN update-ca-certificates

# wasi-sdk (clang + wasi-sysroot + wasm builtins) for the custom-chip build.
RUN set -eux; \
    case "${TARGETARCH}" in \
        amd64) A=x86_64 ;; \
        arm64) A=arm64  ;; \
        *)     A=x86_64 ;; \
    esac; \
    MAJOR="${WASI_SDK_VER%%.*}"; \
    curl -fsSL -o /tmp/wasi.tgz \
        "https://github.com/WebAssembly/wasi-sdk/releases/download/wasi-sdk-${MAJOR}/wasi-sdk-${WASI_SDK_VER}-${A}-linux.tar.gz"; \
    mkdir -p /opt/wasi-sdk; \
    tar -xzf /tmp/wasi.tgz -C /opt/wasi-sdk --strip-components=1; \
    rm /tmp/wasi.tgz

ENV PATH=/opt/wasi-sdk/bin:${PATH}
ENV WASI_ROOT=/opt/wasi-sdk/share/wasi-sysroot

# PlatformIO for firmware + native unit-test builds.
RUN pip3 install --break-system-packages platformio
