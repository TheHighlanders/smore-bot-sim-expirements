// Contract.h — the controller <-> world I/O contract (REQUIREMENTS.md §4).
// This flat struct pair is the ONE seam: the visualizer (JS/WASM) or the P1AM
// HAL fills Inputs and reads Outputs each control tick. Kept POD so it marshals
// trivially across the WASM boundary.
#pragma once
#include <cstdint>

namespace smores {

constexpr int N_STATIONS = 3;                 // 0=graham, 1=chocolate, 2=marshmallow

struct Inputs {
    uint32_t now_ms       = 0;                // monotonic controller/sim time
    bool     sense[N_STATIONS] = {false,false,false}; // station light sensors (tray present)
    bool     tunnel_entry = false;
    bool     tunnel_exit  = false;
    float    tunnel_temp_c = 20.0f;
    uint8_t  dispense_confirm[N_STATIONS] = {0,0,0};  // units the drop-confirm sensor counted (closed-loop)
    bool     run          = true;             // operator run / E-stop clear
};

struct Outputs {
    float belt_speed = 0.0f;                  // mm/s (0 = stopped)
    bool  gate_open[N_STATIONS] = {true,true,true};   // dispenser-station solenoid gates
    bool  dispense[N_STATIONS]  = {false,false,false}; // dispenser actuator output (sustained)
    bool  tunnel_gate_open = true;            // tunnel hold gate (holds a tray to toast)
    bool  heater = false;
};

} // namespace smores
