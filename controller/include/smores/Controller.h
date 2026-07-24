// Controller.h — the one stateful piece (the part we compile to WASM + visualize).
// Reads inputs via subsystems, tracks trays by dead-reckoning, runs the per-tray
// state machine, and writes outputs via subsystems. Two variants (open/closed).
//
// This is a faithful SKELETON ported from the verified JS mockup; the real
// control logic may differ — the structure and the §4 contract are what matter.
#pragma once
#include "Subsystems.h"
#include "Contract.h"
#include <vector>

namespace smores {

enum Mode { OpenLoop, ClosedLoop };
enum Status { Moving, Held, Toasting, Done, Lost };

// The controller's BELIEF about one tray — it never reads the world's truth.
struct Track {
    int      id           = 0;
    float    est_pos_mm   = 0;
    int      stage        = 0;      // # stations completed (3 = assembled, 4 = toasted)
    Status   status       = Moving;
    int      hold         = -1;     // station index being held at, or -1
    uint32_t phase_until  = 0;      // ms: dispense/toast phase end
    int      placed[layout::N_DISP] = {};  // believed units per dispenser
    int      retries      = 0;
};

// Geometry (station positions, tunnel, belt, speed) now comes from the generated
// layout (layout::*, from the bound layout.json). Config keeps only the control
// timings — decisions the controller makes, not physical facts of the machine.
struct Config {
    uint32_t dispense_ms     = 650;     // dispenser output run time before release
    uint32_t toast_ms        = 3800;    // tunnel dwell
};

// Optional log sink: (kind, message). kind in {"evt","warn","crit"}.
using LogSink = void (*)(const char* kind, const char* msg);

class Controller {
public:
    Controller(Hal& hal, Mode mode, Config cfg = Config(), LogSink log = nullptr)
        : hal_(hal), mode_(mode), cfg_(cfg), log_(log),
          conveyor_(hal), tunnel_(hal) {}

    void update();                                   // one control tick
    const std::vector<Track>& tracks() const { return tracks_; }
    Mode mode() const { return mode_; }

private:
    Track* nearest(float pos);
    void   say(const char* kind, const char* fmt, ...);

    Hal&           hal_;
    Mode           mode_;
    Config         cfg_;
    LogSink        log_;
    Conveyor       conveyor_;
    HeatingTunnel  tunnel_;
    std::vector<Track> tracks_;
    int      next_id_   = 1;
    bool     last_sense_[layout::N_DISP] = {};
    uint32_t last_now_  = 0;
    bool     have_now_  = false;
};

} // namespace smores
