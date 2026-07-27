// HAL unit tests: each subsystem method is a single intent that must land on the
// right contract field (HAL.md §H-2.1). These are the sim-side equivalent of
// "writeDiscrete(slot, channel) set the expected module output bit" — when the
// P1am* implementations land, the same assertions run against a mock P1AM base.
// Build+run: see controller/Makefile (host-test). No simulator needed.
#include "smores/SimMachine.h"
#include <cassert>
#include <cstdio>
using namespace smores;

int main() {
    Inputs in; Outputs out;
    SimMachine sim(&in, &out);
    Machine& m = sim.machine();

    // ---- wiring: every subsystem the layout declares is present ----
    assert(m.clock && m.belt);
    for (int k = 0; k < layout::N_DISP; k++) assert(m.disp[k]);
#if SMORES_HAS_TUNNEL
    assert(m.tunnel);
#else
    assert(!m.tunnel);
#endif
    assert(Machine::nDisp() == layout::N_DISP);

    // ---- clock ----
    in.now_ms = 1234; in.run = false;
    assert(m.clock->nowMs() == 1234);
    assert(m.clock->running() == false);
    in.run = true; assert(m.clock->running() == true);

    // ---- conveyor: intent -> belt_speed ----
    m.belt->setSpeed(110.f); assert(out.belt_speed == 110.f);
    m.belt->setSpeed(0.f);   assert(out.belt_speed == 0.f);

    // ---- dispensers: each one touches ONLY its own channel ----
    for (int k = 0; k < layout::N_DISP; k++) {
        in.sense[k] = true;  assert(m.disp[k]->trayPresent() == true);
        in.sense[k] = false; assert(m.disp[k]->trayPresent() == false);

        in.dispense_confirm[k] = 2; assert(m.disp[k]->confirmedDrops() == 2);
        in.dispense_confirm[k] = 0; assert(m.disp[k]->confirmedDrops() == 0);

        m.disp[k]->setGate(false); assert(out.gate_open[k] == false);
        m.disp[k]->setGate(true);  assert(out.gate_open[k] == true);

        m.disp[k]->runActuator(0, true);  assert(out.dispense[k] == true);
        m.disp[k]->runActuator(0, false); assert(out.dispense[k] == false);

        assert(m.disp[k]->servos() == layout::DISP[k].servos);

        // no cross-talk: closing gate k leaves every other gate untouched
        m.disp[k]->setGate(false);
        for (int j = 0; j < layout::N_DISP; j++) if (j != k) assert(out.gate_open[j] == true);
        m.disp[k]->setGate(true);
    }

    // an out-of-range servo for this layout is ignored, not undefined behaviour
    m.disp[0]->runActuator(7, true);
    assert(out.dispense[0] == false);

#if SMORES_HAS_TUNNEL
    // ---- tunnel ----
    in.tunnel_entry = true;  assert(m.tunnel->atEntry() == true);
    in.tunnel_entry = false; assert(m.tunnel->atEntry() == false);
    in.tunnel_exit  = true;  assert(m.tunnel->atExit()  == true);
    in.tunnel_exit  = false; assert(m.tunnel->atExit()  == false);
    in.tunnel_temp_c = 205.f; assert(m.tunnel->temperatureC() == 205.f);

    m.tunnel->setGate(false);   assert(out.tunnel_gate_open == false);
    m.tunnel->setGate(true);    assert(out.tunnel_gate_open == true);
    m.tunnel->setHeater(true);  assert(out.heater == true);
    m.tunnel->setHeater(false); assert(out.heater == false);
#endif

    printf("test_subsystems: PASS (%d dispensers, tunnel=%d)\n", layout::N_DISP, SMORES_HAS_TUNNEL);
    return 0;
}
