// Machine.h — the composition root the controller is handed (HAL.md §H-3).
//
// The controller iterates SUBSYSTEMS, never signal indices. Counts and presence
// come from the layout (generated Layout.h), so this stays in lockstep with the
// layout JSON: disp[i] corresponds to layout::DISP[i].
//
// Two factories produce one: SimMachine (browser/host) and, later, P1amMachine
// (real base). Only the factory differs between targets — Controller.cpp does not.
#pragma once
#include "Hal.h"
#include "generated/Layout.h"

namespace smores {

struct Machine {
    IClock*     clock   = nullptr;
    IConveyor*  belt    = nullptr;
    IDispenser* disp[layout::N_DISP] = {};   // belt order, parallel to layout::DISP
    ITunnel*    tunnel  = nullptr;           // nullptr if this layout has none
    ISmusher*   smusher = nullptr;           // nullptr if absent

    static constexpr int nDisp() { return layout::N_DISP; }
};

} // namespace smores
