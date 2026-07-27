// SimMachine.h — the SIM implementation of the HAL (HAL.md §H-5.1).
//
// Backed by the generated Inputs/Outputs contract structs: the visualizer (over
// the WASM boundary) or a host test fills Inputs and reads Outputs, and the
// controller runs on top unchanged. This is the file that replaced the old
// per-signal StructHal — the same work, redistributed into one class per
// subsystem so each owns its own signals.
//
// The real-hardware counterpart (P1amMachine) will mirror this file, with each
// class holding a slot/channel binding from the layout instead of struct fields.
#pragma once
#include "Machine.h"
#include "generated/Contract.h"

namespace smores {

class SimClock : public IClock {
public:
    void bind(const Inputs* in) { in_ = in; }
    uint32_t nowMs()   override { return in_->now_ms; }
    bool     running() override { return in_->run; }
private:
    const Inputs* in_ = nullptr;
};

class SimConveyor : public IConveyor {
public:
    void bind(Outputs* out) { out_ = out; }
    void setSpeed(float mm_s) override { out_->belt_speed = mm_s; }
private:
    Outputs* out_ = nullptr;
};

class SimDispenser : public IDispenser {
public:
    void bind(const Inputs* in, Outputs* out, int idx) { in_ = in; out_ = out; idx_ = idx; }
    bool    trayPresent()    override { return in_->sense[idx_]; }
    uint8_t confirmedDrops() override { return in_->dispense_confirm[idx_]; }
    void    setGate(bool open) override { out_->gate_open[idx_] = open; }
    int     servos() const     override { return layout::DISP[idx_].servos; }
    void    runActuator(int servo, bool on) override {
        if (servo == 0) { out_->dispense[idx_] = on; return; }
#if SMORES_MAX_SERVOS >= 2
        if (servo == 1) { out_->dispense_b[idx_] = on; return; }
#endif
        (void)servo;   // out-of-range servo for this layout: ignored, not UB
    }
private:
    const Inputs* in_  = nullptr;
    Outputs*      out_ = nullptr;
    int           idx_ = 0;
};

#if SMORES_HAS_TUNNEL
class SimTunnel : public ITunnel {
public:
    void bind(const Inputs* in, Outputs* out) { in_ = in; out_ = out; }
    bool  atEntry()      override { return in_->tunnel_entry; }
    bool  atExit()       override { return in_->tunnel_exit; }
    float temperatureC() override { return in_->tunnel_temp_c; }
    void  setGate(bool open) override { out_->tunnel_gate_open = open; }
    void  setHeater(bool on) override { out_->heater = on; }
private:
    const Inputs* in_  = nullptr;
    Outputs*      out_ = nullptr;
};
#endif

// Owns the subsystem instances and hands out a wired Machine.
class SimMachine {
public:
    SimMachine(const Inputs* in, Outputs* out) {
        clock_.bind(in); belt_.bind(out);
        for (int k = 0; k < layout::N_DISP; k++) { disp_[k].bind(in, out, k); m_.disp[k] = &disp_[k]; }
        m_.clock = &clock_; m_.belt = &belt_;
#if SMORES_HAS_TUNNEL
        tunnel_.bind(in, out); m_.tunnel = &tunnel_;
#endif
    }
    Machine& machine() { return m_; }

private:
    SimClock     clock_;
    SimConveyor  belt_;
    SimDispenser disp_[layout::N_DISP];
#if SMORES_HAS_TUNNEL
    SimTunnel    tunnel_;
#endif
    Machine      m_;
};

} // namespace smores
