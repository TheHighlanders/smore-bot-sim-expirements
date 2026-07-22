# Convenience targets. The canonical flows are PlatformIO (pio test / pio run)
# and the chip's own Makefile; these wrappers just make local checks one-liners.

CXX      ?= g++
CXXFLAGS ?= -std=c++17 -Wall -Wextra
INCLUDES  = -Ishared -Ilib/P1AM_Sim

.PHONY: host-test chip test clean

## host-test: build+run the dependency-free host smoke test (needs only g++)
host-test:
	@mkdir -p build
	$(CXX) $(CXXFLAGS) $(INCLUDES) lib/P1AM_Sim/P1AM_Sim.cpp tools/host_smoke.cpp -o build/host_smoke
	./build/host_smoke

## chip: build the base-controller WASM chip (needs clang + wasi-libc)
chip:
	$(MAKE) -C wokwi/chips/p1-base-controller

## test: the full Unity suite (needs PlatformIO)
test:
	pio test -e native

clean:
	rm -rf build
	$(MAKE) -C wokwi/chips/p1-base-controller clean
