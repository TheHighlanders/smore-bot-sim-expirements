# S'mores controller — host tests (g++) and WASM build (wasi-sdk).
#   make host-test   # subsystem + controller logic tests on the host
#   make wasm        # compile the controller to WASM for the visualizer

CXX      ?= g++
CXXFLAGS ?= -std=c++17 -Wall -Wextra -Iinclude
SRC       = src/Controller.cpp

# Reuse the repo-local wasi-sdk fetched by `make chip-local` at the repo root.
REPO_ROOT := $(abspath ..)
WASI      ?= $(REPO_ROOT)/.toolchains/wasi-sdk

.PHONY: host-test test-subsystems test-controller wasm clean
host-test: test-subsystems test-controller

test-subsystems:
	@mkdir -p build
	$(CXX) $(CXXFLAGS) test/test_subsystems.cpp -o build/test_subsystems
	./build/test_subsystems

test-controller:
	@mkdir -p build
	$(CXX) $(CXXFLAGS) $(SRC) test/test_controller.cpp -o build/test_controller
	./build/test_controller

## wasm: build controller.wasm (reactor module) — exports init/tick + the shared
## Inputs/Outputs pointers for the JS visualizer to drive. Needs wasi-sdk.
wasm:
	@mkdir -p build
	$(WASI)/bin/clang++ --target=wasm32-wasip1 -std=c++17 -O2 -Iinclude \
	  -fno-exceptions -fno-rtti -mexec-model=reactor \
	  -Wl,--export-dynamic $(SRC) wasm/main.cpp -o build/controller.wasm

clean:
	rm -rf build
