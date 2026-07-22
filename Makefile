# Convenience targets. The canonical flows are PlatformIO (pio test / pio run)
# and the chip's own Makefile; these wrappers just make local checks one-liners.

CXX      ?= g++
CXXFLAGS ?= -std=c++17 -Wall -Wextra
INCLUDES  = -Ishared -Ilib/P1AM_Sim

WASI_SDK_DIR := .toolchains/wasi-sdk

.PHONY: host-test chip chip-local test clean

## host-test: build+run the dependency-free host smoke test (needs only g++)
host-test:
	@mkdir -p build
	$(CXX) $(CXXFLAGS) $(INCLUDES) lib/P1AM_Sim/P1AM_Sim.cpp tools/host_smoke.cpp -o build/host_smoke
	./build/host_smoke

## chip: build the base-controller WASM chip. Auto-uses the repo-local wasi-sdk
## (.toolchains/wasi-sdk) if present; otherwise expects clang + WASI_ROOT.
chip:
ifneq ($(wildcard $(WASI_SDK_DIR)/bin/clang),)
	$(MAKE) -C wokwi/chips/p1-base-controller \
		CLANG=$(CURDIR)/$(WASI_SDK_DIR)/bin/clang \
		WASI_ROOT=$(CURDIR)/$(WASI_SDK_DIR)/share/wasi-sysroot
else
	$(MAKE) -C wokwi/chips/p1-base-controller
endif

## chip-local: download a repo-local wasi-sdk once (online), then build the chip
## offline. The host trusts your corporate proxy CA, so the download works.
chip-local:
	./tools/get-wasi-sdk.sh
	$(MAKE) chip

## test: the full Unity suite (needs PlatformIO)
test:
	pio test -e native

clean:
	rm -rf build
	$(MAKE) -C wokwi/chips/p1-base-controller clean
