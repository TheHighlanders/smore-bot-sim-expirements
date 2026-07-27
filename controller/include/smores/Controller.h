// Controller.h — the one stateful piece (the part we compile to WASM + visualize).
// Reads inputs via subsystems, tracks trays by dead-reckoning, runs the per-tray
// state machine, and writes outputs via subsystems. Two variants (open/closed).
//
// This is a faithful SKELETON ported from the verified JS mockup; the real
// control logic may differ — the structure and the §4 contract are what matter.
#pragma once
#include "Machine.h"
#include "generated/State.h"     // generated: enum Status + struct Track (HAL.md §H-8.1)
#include <vector>

namespace smores {

enum Mode { OpenLoop, ClosedLoop };

// `Status` and `Track` are GENERATED (generated/State.h) so their memory layout is
// described by the same descriptor the visualizer decodes with — no hand-written
// per-field accessors, and no drift as the layout's dispenser count changes.

// Geometry (positions, tunnel, belt, speed) comes from the generated layout
// (layout::*). Config is controller POLICY only. Mechanical dwells (how long a dispenser must run, how
// long the tunnel toasts) are properties of the MACHINE and now come from the
// layout — layout::DISP[k].dispense_ms / layout::TUNNEL_TOAST_MS (HAL.md OQ-2).
struct Config {
    int max_retries = 3;                // closed-loop: attempts before flagging
};

// Optional log sink: (kind, message). kind in {"evt","warn","crit"}.
using LogSink = void (*)(const char* kind, const char* msg);

class Controller {
public:
    Controller(Machine& machine, Mode mode, Config cfg = Config(), LogSink log = nullptr)
        : m_(machine), mode_(mode), cfg_(cfg), log_(log) {}

    void update();                                   // one control tick
    const std::vector<Track>& tracks() const { return tracks_; }
    Mode mode() const { return mode_; }

private:
    Track* nearest(float pos);
    void   say(const char* kind, const char* fmt, ...);

    Machine&       m_;            // the HAL: subsystem interfaces (HAL.md §H-3)
    Mode           mode_;
    Config         cfg_;
    LogSink        log_;
    std::vector<Track> tracks_;
    int      next_id_   = 1;
    bool     last_sense_[layout::N_DISP] = {};
    uint32_t last_now_  = 0;
    bool     have_now_  = false;
};

} // namespace smores
