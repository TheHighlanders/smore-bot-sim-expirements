// The portability proof (HAL.md §H-10 / CI-H1).
//
// This builds the REAL-HARDWARE machine — P1amMachine, the layer that speaks
// slots and channels — over the P1AM façade backed by an in-process base model,
// and then runs the SAME Controller.cpp the browser runs against it. If the
// controller ever grows a sim-only dependency, this stops compiling.
//
// Two things are asserted:
//   1) each subsystem intent lands on the exact module output the LAYOUT says it
//      should (so a wiring mistake is caught here, not on a bench), and
//   2) the unchanged controller drives this hardware-shaped machine to a complete
//      s'more.
//
// Requires a layout with a hardware binding: build with LAYOUT=sandwich_wired.
#include "smores/generated/Binding.h"

#if !SMORES_HAS_BINDING
#include <cstdio>
int main() { printf("test_p1am_machine: SKIP (layout is sim-only; use LAYOUT=sandwich_wired)\n"); return 0; }
#else

#include "smores/P1amMachine.h"
#include "smores/Controller.h"
#include "MockTransport.h"
#include <cassert>
#include <cstdio>
#include <cmath>
using namespace smores;

namespace smores { uint32_t g_host_millis = 0; }   // the board clock P1amBus reads

// Build the module lineup the layout declares, in slot order, for the base model.
static const char* lineup[binding::N_SLOTS];
static void buildLineup() {
    for (int i = 0; i < binding::N_SLOTS; i++) lineup[i] = binding::SLOTS[i].part;
}

// Is a given (slot, channel) currently energised in the base's output image?
static bool outBit(MockTransport& bus, binding::ChannelRef r) {
    const uint8_t byteIndex = (uint8_t)((r.channel - 1) / 8);
    const uint8_t bit       = (uint8_t)((r.channel - 1) % 8);
    return (bus.rawOutByte(r.slot, byteIndex) >> bit) & 1u;      // LSB = channel 1
}

int main() {
    buildLineup();
    MockTransport bus(lineup, binding::N_SLOTS);
    P1AM_Sim p1(bus);
    P1amMachine hw(p1);
    assert(hw.begin());                                  // enumerates + configures
    Machine& m = hw.machine();

    // ---- startup configured every module the layout says needs it ----
    assert(p1.configuredCount() == SMORES_N_CONFIGS);
#if SMORES_N_CONFIGS > 0
    assert(p1.lastConfiguredSlot() == binding::CONFIGS[SMORES_N_CONFIGS - 1].slot);
#endif

    // ---- 1) each intent lands on the channel the LAYOUT specifies ----
    for (int k = 0; k < layout::N_DISP; k++) {
        const binding::DispIo& io = binding::DISP_IO[k];

        m.disp[k]->setGate(true);
        assert(outBit(bus, io.gate) == true);
        m.disp[k]->setGate(false);
        assert(outBit(bus, io.gate) == false);

        m.disp[k]->runActuator(0, true);
        assert(outBit(bus, io.act[0]) == true);
        m.disp[k]->runActuator(0, false);
        assert(outBit(bus, io.act[0]) == false);

        // an out-of-range servo must not scribble on some other channel
        m.disp[k]->runActuator(5, true);
        assert(outBit(bus, io.act[0]) == false);
    }
#if SMORES_HAS_TUNNEL
    m.tunnel->setHeater(true);  assert(outBit(bus, binding::HEATER_IO) == true);
    m.tunnel->setHeater(false); assert(outBit(bus, binding::HEATER_IO) == false);
    m.tunnel->setGate(true);    assert(outBit(bus, binding::TUNNEL_GATE_IO) == true);
    m.tunnel->setGate(false);   assert(outBit(bus, binding::TUNNEL_GATE_IO) == false);
#endif
    printf("  ok  every subsystem intent lands on the layout's slot/channel\n");

    // ---- 2) the UNCHANGED controller drives this hardware-shaped machine ----
    // A compact world: it reads the commanded outputs out of the base image and
    // drives the sensor inputs back in, exactly as a real line would.
    Controller ctrl(m, ClosedLoop);
    float pos = -32.f;
    int   counts[layout::N_DISP] = {};
    float dispOn[layout::N_DISP] = {}, lastAtt[layout::N_DISP] = {};
    const float SLIP = 0.965f, DROP = 450.f, dt = 0.02f;

    for (int step = 0; step < 2000; step++) {
        // --- drive inputs into the base model from the world ---
        for (int k = 0; k < layout::N_DISP; k++) {
            const bool near = std::fabs(pos - layout::DISP[k].pos_mm) < 22.f;
            bus.setDiscreteIn(binding::DISP_IO[k].sense.slot, binding::DISP_IO[k].sense.channel, near);
        }
#if SMORES_HAS_TUNNEL
        bus.setDiscreteIn(binding::TUNNEL_ENTRY_IO.slot, binding::TUNNEL_ENTRY_IO.channel, std::fabs(pos - layout::TUNNEL_ENTRY_MM) < 22.f);
        bus.setDiscreteIn(binding::TUNNEL_EXIT_IO.slot,  binding::TUNNEL_EXIT_IO.channel,  std::fabs(pos - layout::TUNNEL_EXIT_MM)  < 22.f);
        bus.setTemperature(binding::TUNNEL_TEMP_IO.slot, binding::TUNNEL_TEMP_IO.channel,
                           outBit(bus, binding::HEATER_IO) ? 205.f : 20.f);
#endif
        g_host_millis += 20;
        ctrl.update();

        // --- apply the commanded outputs to the world ---
        bool blocked = false;
        for (int k = 0; k < layout::N_DISP; k++)
            if (!outBit(bus, binding::DISP_IO[k].gate) && pos < layout::DISP[k].pos_mm && pos > layout::DISP[k].pos_mm - 70.f) blocked = true;
#if SMORES_HAS_TUNNEL
        if (!outBit(bus, binding::TUNNEL_GATE_IO) && pos > layout::TUNNEL_ENTRY_MM - 30.f && pos < layout::TUNNEL_EXIT_MM) blocked = true;
#endif
        if (!blocked) pos += layout::NOMINAL_SPEED * SLIP * dt;

        for (int k = 0; k < layout::N_DISP; k++) {
            if (outBit(bus, binding::DISP_IO[k].act[0])) {
                dispOn[k] += dt * 1000.f;
                if (dispOn[k] - lastAtt[k] >= DROP) {
                    lastAtt[k] = dispOn[k];
                    const bool atStation = std::fabs(pos - layout::DISP[k].pos_mm) < 70.f;
                    if (atStation) counts[k]++;
                    // the drop-confirm sensor reports what actually fell
                    if (binding::DISP_IO[k].has_confirm)
                        bus.setDiscreteIn(binding::DISP_IO[k].confirm.slot, binding::DISP_IO[k].confirm.channel, atStation);
                }
            } else { dispOn[k] = 0.f; lastAtt[k] = 0.f;
                if (binding::DISP_IO[k].has_confirm)
                    bus.setDiscreteIn(binding::DISP_IO[k].confirm.slot, binding::DISP_IO[k].confirm.channel, false);
            }
        }
    }

    for (int k = 0; k < layout::N_DISP; k++) {
        printf("      %-4s placed %d\n", layout::DISP[k].id, counts[k]);
        assert(counts[k] == 1);
    }
    assert(!ctrl.tracks().empty() && ctrl.tracks()[0].status == Done);
    printf("  ok  the unchanged Controller.cpp completed a s'more on the P1AM machine\n");

    printf("test_p1am_machine: PASS (%d slots, %d dispensers)\n", binding::N_SLOTS, layout::N_DISP);
    return 0;
}
#endif
