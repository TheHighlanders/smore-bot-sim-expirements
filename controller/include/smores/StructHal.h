// StructHal.h — a HAL backed by the flat Contract structs. The visualizer (via
// WASM) or a host test fills `Inputs` and reads `Outputs`; the controller +
// subsystems run unchanged on top. The real P1AM HAL would implement Hal over SPI.
#pragma once
#include "Hal.h"
#include "Contract.h"

namespace smores {

class StructHal : public Hal {
public:
    StructHal(const Inputs* in, Outputs* out) : in_(in), out_(out) {}
    uint32_t nowMs()                     override { return in_->now_ms; }
    bool     running()                   override { return in_->run; }
    bool     trayPresent(int s)          override { return in_->sense[s]; }
    bool     tunnelEntry()               override { return in_->tunnel_entry; }
    bool     tunnelExit()                override { return in_->tunnel_exit; }
    float    tunnelTempC()               override { return in_->tunnel_temp_c; }
    uint8_t  dispenseConfirm(int s)      override { return in_->dispense_confirm[s]; }
    void     setBeltSpeed(float v)       override { out_->belt_speed = v; }
    void     setGate(int s, bool open)   override { out_->gate_open[s] = open; }
    void     setDispense(int s, bool on) override { out_->dispense[s] = on; }
    void     setTunnelGate(bool open)    override { out_->tunnel_gate_open = open; }
    void     setHeater(bool on)          override { out_->heater = on; }
private:
    const Inputs* in_;
    Outputs*      out_;
};

} // namespace smores
