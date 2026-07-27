# S'mores controller — host tests (g++) and WASM build (wasi-sdk).
#   make host-test   # subsystem + controller logic tests on the host
#   make wasm        # compile the controller to WASM for the visualizer
#   make codegen     # regenerate the typed contract from layouts/$(LAYOUT).json

CXX      ?= g++
CXXFLAGS ?= -std=c++17 -Wall -Wextra -Iinclude
SRC       = src/Controller.cpp

# The controller is a BOUND PAIR with one layout: codegen emits the typed
# Inputs/Outputs + geometry it compiles against. Pick the layout with LAYOUT=.
LAYOUT   ?= classic3
GEN_DIR   = include/smores/generated
GEN       = $(GEN_DIR)/Contract.h $(GEN_DIR)/Layout.h

# Reuse the repo-local wasi-sdk fetched by `make chip-local` at the repo root.
REPO_ROOT := $(abspath ..)
WASI      ?= $(REPO_ROOT)/.toolchains/wasi-sdk

.PHONY: host-test test-subsystems test-controller test-p1am wasm integration codegen validate-layouts clean

# The P1AM portability test needs the façade + the shared base model.
P1AM_INC = -I$(REPO_ROOT)/lib/P1AM_Sim -I$(REPO_ROOT)/shared

## codegen: (re)generate the contract for LAYOUT. Always run so a LAYOUT change
## takes effect; node is already a dep (integration test).
codegen:
	@node $(REPO_ROOT)/tools/layout/codegen.mjs --layout layouts/$(LAYOUT).json \
	  --include-dir $(GEN_DIR) --meta generated/$(LAYOUT).meta.json

host-test: test-subsystems test-controller

## integration: build controller.wasm for LAYOUT, then drive it through the
## JS<->WASM boundary against a world built from that layout's meta (needs node).
integration: wasm
	LAYOUT=$(LAYOUT) node test/integration.mjs

test-subsystems: codegen
	@mkdir -p build
	$(CXX) $(CXXFLAGS) test/test_subsystems.cpp -o build/test_subsystems
	./build/test_subsystems

test-controller: codegen
	@mkdir -p build
	$(CXX) $(CXXFLAGS) $(SRC) test/test_controller.cpp -o build/test_controller
	./build/test_controller

## test-p1am: the PORTABILITY proof (HAL.md CI-H1) — build the real-hardware
## machine (P1amMachine, slots+channels) over the P1AM façade and an in-process
## base model, then run the SAME Controller.cpp against it. Needs a layout that
## carries a hardware binding, so it forces LAYOUT=sandwich_wired.
test-p1am:
	@$(MAKE) -s codegen LAYOUT=sandwich_wired
	@mkdir -p build
	$(CXX) $(CXXFLAGS) $(P1AM_INC) $(SRC) $(REPO_ROOT)/lib/P1AM_Sim/P1AM_Sim.cpp \
	  test/test_p1am_machine.cpp -o build/test_p1am_machine
	./build/test_p1am_machine

## validate-layouts: every layout must generate cleanly (wiring included).
validate-layouts:
	@for f in layouts/*.json; do \
	  n=$$(basename $$f .json); \
	  node $(REPO_ROOT)/tools/layout/codegen.mjs --layout $$f >/dev/null || exit 1; \
	done
	@echo "validate-layouts: all layouts OK"

## wasm: build controller.wasm (reactor module) — exports init/tick + the shared
## Inputs/Outputs pointers for the JS visualizer to drive. Needs wasi-sdk.
wasm: codegen
	@mkdir -p build
	$(WASI)/bin/clang++ --target=wasm32-wasip1 -std=c++17 -O2 -Iinclude \
	  -fno-exceptions -fno-rtti -mexec-model=reactor \
	  -Wl,--export-dynamic $(SRC) wasm/main.cpp -o build/controller.wasm

clean:
	rm -rf build $(GEN_DIR) generated
