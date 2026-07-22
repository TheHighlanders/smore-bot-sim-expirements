/*
 * host_smoke.cpp  --  dependency-free smoke test (plain g++, no PlatformIO).
 *
 * A fast sanity check of the shared model + shim that needs nothing but a C++
 * compiler. The thorough suite is the Unity tests under test/ (pio test -e
 * native); this exists so `make host-test` works on any machine and in CI
 * without installing PlatformIO or fetching Unity.
 */
#include "P1AM_Sim.h"
#include "MockTransport.h"
#include <cassert>
#include <cstring>
#include <cstdio>

int main() {
    const char *lineup[] = { "P1-08TRS" };
    MockTransport bus(lineup, 1);
    P1AM_Sim P1(bus);

    assert(P1.init() == 1);
    assert(strcmp(P1.slotName(1), "P1-08TRS") == 0);

    P1.writeDiscrete(HIGH, 1, 3);
    assert(P1.readDiscrete(1, 3) == 1);
    assert(P1.readDiscrete(1, 0) == 0x04);

    P1.writeDiscrete(0xA5, 1);
    assert(P1.readDiscrete(1, 0) == 0xA5);

    P1.writeDiscrete(LOW, 1, 1);
    assert(P1.readDiscrete(1, 0) == 0xA4);

    assert(P1.getFwVersion() == 0x00010000UL);
    assert(P1.readDiscrete(2, 0) == 0);   // bad slot safe

    printf("host_smoke: PASS\n");
    return 0;
}
