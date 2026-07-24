// Hal.h — hardware abstraction the subsystems talk to.
// Two implementations: StructHal (sim/WASM, backed by Contract structs) and a
// future P1AM HAL (real SPI). Subsystems call these; the Controller never does.
#pragma once
#include <cstdint>

namespace smores {

struct Hal {
    virtual ~Hal() {}
    // ---- inputs (world -> controller) ----
    virtual uint32_t nowMs()                    = 0;
    virtual bool     running()                  = 0;
    virtual bool     trayPresent(int station)   = 0;   // station light sensor
    virtual bool     tunnelEntry()              = 0;
    virtual bool     tunnelExit()               = 0;
    virtual float    tunnelTempC()              = 0;
    virtual uint8_t  dispenseConfirm(int station) = 0; // units the drop sensor counted
    // ---- outputs (controller -> world) ----
    virtual void setBeltSpeed(float mmps)       = 0;
    virtual void setGate(int station, bool open)= 0;   // solenoid gate
    virtual void setDispense(int station, bool on) = 0;// dispenser actuator output
    virtual void setTunnelGate(bool open)       = 0;
    virtual void setHeater(bool on)             = 0;
};

} // namespace smores
