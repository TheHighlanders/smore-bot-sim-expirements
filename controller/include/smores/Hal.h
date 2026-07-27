// Hal.h — THE hardware abstraction layer, at the SUBSYSTEM level (HAL.md §H-2).
//
// One interface per subsystem type. This is the only seam between the controller
// and the machine: the controller talks to these interfaces and nothing else, so
// the same Controller.cpp runs in the browser and on a real P1AM base.
//
// Module communication (SPI, slots, channels, opcodes) lives BEHIND these
// interfaces, inside the implementations:
//   - Sim*   (SimMachine.h)  — backed by the generated Inputs/Outputs structs
//   - P1am*  (future)        — owns its slot/channel binding, issues P1.* calls
//
// Rule (H-1.3): the controller must never name a slot, channel, module, opcode,
// or the Inputs/Outputs structs. Those are implementation details of the HAL.
#pragma once
#include <cstdint>

namespace smores {

// Board time + the operator run/E-stop line.
struct IClock {
    virtual uint32_t nowMs()   = 0;
    virtual bool     running() = 0;
    virtual ~IClock() {}
};

// The conveyor belt (one shared belt for the whole line).
struct IConveyor {
    virtual void setSpeed(float mm_s) = 0;      // 0 = stopped
    virtual ~IConveyor() {}
};

// A dispenser station: presence sensor, hold gate, 1-2 actuators, and (when
// fitted) a drop-confirm sensor. `confirmedDrops()` returns 0 where no confirm
// sensor exists — the layout says whether to trust it (H-2.2).
struct IDispenser {
    virtual bool    trayPresent()                   = 0;
    virtual uint8_t confirmedDrops()                = 0;
    virtual void    setGate(bool open)              = 0;
    virtual void    runActuator(int servo, bool on) = 0;   // servo in [0, servos())
    virtual int     servos() const                  = 0;
    virtual ~IDispenser() {}
};

// The heating tunnel: entry/exit beams, temperature, a hold gate, and a heater.
struct ITunnel {
    virtual bool  atEntry()          = 0;
    virtual bool  atExit()           = 0;
    virtual float temperatureC()     = 0;
    virtual void  setGate(bool open) = 0;       // closed = hold a tray to toast
    virtual void  setHeater(bool on) = 0;
    virtual ~ITunnel() {}
};

// An optional press downstream of a dispenser (LAYOUTS.md §L-3). Specified now so
// the interface set is stable; no implementation ships yet.
struct ISmusher {
    virtual bool trayPresent()      = 0;
    virtual bool pressConfirmed()   = 0;
    virtual void setGate(bool open) = 0;
    virtual void press(bool on)     = 0;
    virtual ~ISmusher() {}
};

} // namespace smores
