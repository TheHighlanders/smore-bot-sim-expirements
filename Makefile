# Convenience targets. The canonical flows are PlatformIO (pio test / pio run)
# and the chip's own Makefile; these wrappers just make local checks one-liners.

CXX      ?= g++
CXXFLAGS ?= -std=c++17 -Wall -Wextra
INCLUDES  = -Ishared -Ilib/P1AM_Sim

WASI_SDK_DIR := .toolchains/wasi-sdk

.PHONY: host-test chip chip-local test controller-test app serve clean

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

## controller-test: host unit tests for the s'mores controller + subsystems
controller-test:
	$(MAKE) -C controller host-test

# Layouts shipped in the Pages app (each is a bound controller.wasm + meta pair).
APP_LAYOUTS ?= classic3 sandwich

## app: assemble the GitHub Pages app — build EACH layout's controller to WASM
## and stage it (docs/app/wasm/<layout>.wasm) alongside its generated meta
## (docs/app/layouts/<layout>.layout.json). The visualizer loads whichever the
## user picks. Build artifacts (gitignored); run after a clone or let CI do it.
## Needs the repo-local wasi-sdk (see `make chip-local` / tools/get-wasi-sdk.sh).
app:
	@mkdir -p docs/app/wasm docs/app/layouts
	@for L in $(APP_LAYOUTS); do \
	  echo "  building layout $$L ..."; \
	  $(MAKE) -s -C controller wasm LAYOUT=$$L >/dev/null; \
	  cp controller/build/controller.wasm docs/app/wasm/$$L.wasm; \
	  cp controller/generated/$$L.meta.json docs/app/layouts/$$L.layout.json; \
	done
	@$(MAKE) -s -C controller codegen LAYOUT=classic3 >/dev/null   # leave default checked out
	@echo "app assembled -> docs/app/  (layouts: $(APP_LAYOUTS)) — run 'make serve' to view"

## serve: assemble + serve docs/app over http (fetch() of the .wasm needs http,
## not file://). Open http://localhost:8000/ .
serve: app
	cd docs/app && python3 -m http.server 8000

clean:
	rm -rf build docs/app/wasm docs/app/layouts
	rm -f docs/app/controller.wasm
	$(MAKE) -C wokwi/chips/p1-base-controller clean
