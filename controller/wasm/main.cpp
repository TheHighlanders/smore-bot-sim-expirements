// WASM entry: exposes the controller to the JS visualizer. The page fills the
// shared Inputs struct, calls tick(), then reads the Outputs struct + the
// exposed track state. This is the seam that replaces the JS stand-in controller
// in docs/simulator/mockup.html with the real C++ logic (unchanged UI).
#include "smores/Controller.h"
#include "smores/StructHal.h"
using namespace smores;

static Inputs   g_in;
static Outputs  g_out;
static StructHal g_hal(&g_in, &g_out);
static Controller* g_ctrl = nullptr;

extern "C" {

// Pointers into linear memory the JS side reads/writes each tick.
__attribute__((export_name("inputs_ptr")))  Inputs*  inputs_ptr()  { return &g_in; }
__attribute__((export_name("outputs_ptr"))) Outputs* outputs_ptr() { return &g_out; }

// mode: 0 = open-loop, 1 = closed-loop.
__attribute__((export_name("init"))) void init(int mode) {
    delete g_ctrl;
    g_ctrl = new Controller(g_hal, mode ? ClosedLoop : OpenLoop);
}
__attribute__((export_name("tick"))) void tick() { if (g_ctrl) g_ctrl->update(); }

// Minimal exposed controller state for the visualizer overlay.
__attribute__((export_name("track_count"))) int track_count() {
    return g_ctrl ? (int)g_ctrl->tracks().size() : 0;
}
// field: 0=id, 1=est_pos_mm, 2=stage, 3=status, 4..6=placed[0..2]
__attribute__((export_name("track_field"))) float track_field(int i, int field) {
    if (!g_ctrl || i < 0 || i >= (int)g_ctrl->tracks().size()) return 0.f;
    const Track& t = g_ctrl->tracks()[i];
    switch (field) {
        case 0: return (float)t.id;
        case 1: return t.est_pos_mm;
        case 2: return (float)t.stage;
        case 3: return (float)t.status;
        case 4: case 5: case 6: return (float)t.placed[field-4];
        default: return 0.f;
    }
}

} // extern "C"
