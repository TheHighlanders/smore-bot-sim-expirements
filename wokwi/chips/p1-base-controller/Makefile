# Build the P1 Base Controller custom chip to WASM.
#
# Local build needs clang + the wasi-libc sysroot (see docs.wokwi.com/chips-api).
# In CI we instead use wokwi/wokwi-chip-clang-action, which provides the
# toolchain -- no local install required (see .github/workflows/ci.yml).
#
# The shared behavioural model lives in ../../../shared (repo-root /shared),
# so both this chip and the host unit tests compile the SAME logic.

SOURCES   = src/main.c
TARGET    = dist/chip.wasm
SHARED    = ../../../shared
# Override CLANG/WASI_ROOT for a local build, e.g. with a wasi-sdk download
# (works on the host even behind a TLS-intercepting corporate proxy):
#   make chip CLANG=/opt/wasi-sdk/bin/clang \
#             WASI_ROOT=/opt/wasi-sdk/share/wasi-sysroot
# In the dev container both are already on PATH/env.
CLANG     ?= clang
WASI_ROOT ?= /opt/wasi-libc

# Flags mirror the official wokwi/wokwi-chip-clang-action (used by CI) so the
# locally-built chip.wasm is identical to the CI-built one. -I$(SHARED) lets
# main.c say #include "base_model.h"; CI instead copies the shared headers next
# to main.c (see .github/workflows/ci.yml), so both build paths resolve it.
CLANG_FLAGS = --target=wasm32-unknown-wasi --sysroot $(WASI_ROOT) \
              -nostartfiles -Wl,--export-table -Wl,--no-entry -Werror \
              -I$(SHARED) -Isrc

.PHONY: all clean
all: $(TARGET) dist/chip.json

clean:
	rm -rf dist

dist:
	mkdir -p dist

$(TARGET): dist $(SOURCES) src/wokwi-api.h $(SHARED)/base_model.h $(SHARED)/module_db.h
	$(CLANG) $(CLANG_FLAGS) -o $(TARGET) $(SOURCES)

dist/chip.json: dist chip.json
	cp chip.json dist/chip.json
