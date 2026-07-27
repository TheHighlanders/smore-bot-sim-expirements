// P1amMachine.h — the REAL-HARDWARE implementation of the HAL (HAL.md §H-5.2).
//
// This is the layer where module communication lives. The controller above it is
// unchanged; everything P1AM-specific — slots, channels, discrete bitmaps, module
// configuration — appears here and nowhere else.
//
// It is deliberately HAND-WRITTEN (only the wiring DATA in generated/Binding.h is
// generated), because reading and editing this file is how a student learns the
// P1AM API at the layer where it belongs.
//
// The API used below is the real FACTS Engineering P1AM surface:
//   uint8_t init()                                     [ref: docs/references/p1am-library/P1AM.h:45]
//   uint32_t readDiscrete(slot, channel = 0)            [ref: .../P1AM.h:50]
//   void     writeDiscrete(data, slot, channel = 0)     [ref: .../P1AM.h:51]
//   float    readTemperature(slot, channel)             [ref: .../P1AM.h:53]
//   bool     configureModule(const char cfg[], slot)    [ref: .../P1AM.h:72-73]
//   bool     isBaseActive()                             [ref: .../P1AM.h:80]
// Slots and channels are 1-based; channel 0 addresses a whole module as a bitmap
// with LSB = channel 1 [ref: docs/references/facts-docs/api_reference.md:172-180].
#pragma once
#include "Machine.h"
#include "generated/Binding.h"
#include "P1amBus.h"          // selects the P1AM implementation for this target

namespace smores {

#if SMORES_HAS_BINDING

class P1amClock : public IClock {
public:
    // Board time. `running` is the operator/E-stop input; until an E-stop channel is
    // wired in the layout we report true, matching the sim's behaviour.
    uint32_t nowMs()   override { return p1am_millis(); }
    bool     running() override { return true; }
};

class P1amConveyor : public IConveyor {
public:
    P1amConveyor(P1Bus& p1, binding::ChannelRef run) : p1_(p1), run_(run) {}
    // The belt drive here is a simple run/stop contactor: any non-zero commanded
    // speed energises it. A VFD/PWM drive would use writePWM instead.
    void setSpeed(float mm_s) override { p1_.writeDiscrete(mm_s > 0.f ? 1 : 0, run_.slot, run_.channel); }
private:
    P1Bus& p1_; binding::ChannelRef run_;
};

class P1amDispenser : public IDispenser {
public:
    void bind(P1Bus* p1, const binding::DispIo* io, int servos) { p1_ = p1; io_ = io; servos_ = servos; }
    bool    trayPresent()    override { return p1_->readDiscrete(io_->sense.slot, io_->sense.channel) != 0; }
    uint8_t confirmedDrops() override {
        if (!io_->has_confirm) return 0;                  // no sensor fitted (H-2.2)
        return p1_->readDiscrete(io_->confirm.slot, io_->confirm.channel) != 0 ? 1 : 0;
    }
    void setGate(bool open) override { p1_->writeDiscrete(open ? 1 : 0, io_->gate.slot, io_->gate.channel); }
    int  servos() const     override { return servos_; }
    void runActuator(int servo, bool on) override {
        if (servo < 0 || servo >= servos_) return;        // ignore, never UB
        const binding::ChannelRef& a = io_->act[servo];
        p1_->writeDiscrete(on ? 1 : 0, a.slot, a.channel);
    }
private:
    P1Bus* p1_ = nullptr; const binding::DispIo* io_ = nullptr; int servos_ = 1;
};

#if SMORES_HAS_TUNNEL
class P1amTunnel : public ITunnel {
public:
    P1amTunnel(P1Bus& p1) : p1_(p1) {}
    bool  atEntry()      override { return p1_.readDiscrete(binding::TUNNEL_ENTRY_IO.slot, binding::TUNNEL_ENTRY_IO.channel) != 0; }
    bool  atExit()       override { return p1_.readDiscrete(binding::TUNNEL_EXIT_IO.slot,  binding::TUNNEL_EXIT_IO.channel)  != 0; }
    float temperatureC() override { return p1_.readTemperature(binding::TUNNEL_TEMP_IO.slot, binding::TUNNEL_TEMP_IO.channel); }
    void  setGate(bool open) override { p1_.writeDiscrete(open ? 1 : 0, binding::TUNNEL_GATE_IO.slot, binding::TUNNEL_GATE_IO.channel); }
    void  setHeater(bool on) override { p1_.writeDiscrete(on ? 1 : 0, binding::HEATER_IO.slot, binding::HEATER_IO.channel); }
private:
    P1Bus& p1_;
};
#endif

// Owns the subsystems and hands out a wired Machine — the hardware counterpart of
// SimMachine. Construct it with the P1 instance (the global `P1` on a real sketch;
// a MockTransport-backed instance in host tests).
class P1amMachine {
public:
    explicit P1amMachine(P1Bus& p1, binding::ChannelRef beltRun = {1, 15})
        : p1_(p1), belt_(p1, beltRun)
#if SMORES_HAS_TUNNEL
        , tunnel_(p1)
#endif
    {
        for (int k = 0; k < layout::N_DISP; k++) {
            disp_[k].bind(&p1_, &binding::DISP_IO[k], layout::DISP[k].servos);
            m_.disp[k] = &disp_[k];
        }
        m_.clock = &clock_; m_.belt = &belt_;
#if SMORES_HAS_TUNNEL
        m_.tunnel = &tunnel_;
#endif
    }

    // Enumerate the base and configure any module that needs it, BEFORE the first
    // read (H-5.4). This is the same startup shape a real sketch uses,
    // `while (!P1.init()) {}`
    // [ref: docs/references/p1am-library.md#discrete-api -> P1AM.h:45].
    bool begin() {
        if (!p1_.init()) return false;
        return configureModules();
    }

    Machine& machine() { return m_; }

private:
    // Every module the layout says needs configuration, in slot order. The payload
    // is opaque bytes from the layout (see generated/Binding.h).
    bool configureModules() {
#if SMORES_N_CONFIGS > 0
        for (int i = 0; i < binding::N_CONFIGS; i++)
            if (!p1_.configureModule(binding::CONFIGS[i].data, binding::CONFIGS[i].slot)) return false;
#endif
        return true;
    }

    P1Bus&       p1_;
    P1amClock    clock_;
    P1amConveyor belt_;
    P1amDispenser disp_[layout::N_DISP];
#if SMORES_HAS_TUNNEL
    P1amTunnel   tunnel_;
#endif
    Machine      m_;
};

} // namespace smores
#else   // !SMORES_HAS_BINDING
namespace smores {
// This layout is sim-only: it declares no base[]/io{}. Add a hardware binding to
// build a P1amMachine (HAL.md §H-6.4).
} // namespace smores
#endif
