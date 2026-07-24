// Subsystems.h — one class per station type. Each method is a single, stateless
// intent that maps to a HAL call: no line logic lives here, so every method is
// trivially unit-testable ("setGate(closed) writes the expected module output").
// The Controller owns all the logic and drives these.
#pragma once
#include "Hal.h"

namespace smores {

// The conveyor belt (one shared belt for the whole line).
class Conveyor {
public:
    explicit Conveyor(Hal& h) : h_(h) {}
    void run(float mmps) { h_.setBeltSpeed(mmps); }
    void stop()          { h_.setBeltSpeed(0.0f); }
private:
    Hal& h_;
};

// A dispenser station (graham / chocolate / marshmallow): light sensor, solenoid
// gate, dispenser actuator, and (for closed-loop) a drop-confirm reading.
class Station {
public:
    Station(Hal& h, int index) : h_(h), idx_(index) {}
    int  index() const           { return idx_; }
    bool trayPresent()           { return h_.trayPresent(idx_); }
    uint8_t confirmedDrops()     { return h_.dispenseConfirm(idx_); }
    void hold(bool closed)       { h_.setGate(idx_, !closed); }   // closed gate = hold tray
    void runDispenser(bool on)   { h_.setDispense(idx_, on); }
private:
    Hal& h_;
    int  idx_;
};

// The heating tunnel: entry/exit sensors, temperature, a hold gate, and a heater.
class HeatingTunnel {
public:
    explicit HeatingTunnel(Hal& h) : h_(h) {}
    bool  atEntry()        { return h_.tunnelEntry(); }
    bool  atExit()         { return h_.tunnelExit(); }
    float temperatureC()   { return h_.tunnelTempC(); }
    void  hold(bool closed){ h_.setTunnelGate(!closed); }         // closed gate = dwell to toast
    void  heater(bool on)  { h_.setHeater(on); }
private:
    Hal& h_;
};

} // namespace smores
