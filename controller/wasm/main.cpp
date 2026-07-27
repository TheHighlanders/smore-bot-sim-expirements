// WASM entry: exposes the controller to the JS visualizer. The page fills the
// shared Inputs struct, calls tick(), then reads the Outputs struct + the
// exposed track state. This is the seam that replaces the JS stand-in controller
// in docs/simulator/mockup.html with the real C++ logic (unchanged UI).
#include "smores/Controller.h"
#include "smores/SimMachine.h"
using namespace smores;

// Imported from the host (JS): the controller's decision log crosses the
// boundary here. kind/msg are pointers into wasm memory (null-terminated UTF-8).
extern "C" __attribute__((import_module("env"), import_name("host_log")))
void host_log(const char* kind, const char* msg);
static void wasm_log(const char* kind, const char* msg) { host_log(kind, msg); }

static Inputs     g_in;
static Outputs    g_out;
static SimMachine g_machine(&g_in, &g_out);     // the HAL: Sim* subsystems over the contract
static Controller* g_ctrl = nullptr;

extern "C" {

// Pointers into linear memory the JS side reads/writes each tick.
__attribute__((export_name("inputs_ptr")))  Inputs*  inputs_ptr()  { return &g_in; }
__attribute__((export_name("outputs_ptr"))) Outputs* outputs_ptr() { return &g_out; }

// mode: 0 = open-loop, 1 = closed-loop.
__attribute__((export_name("init"))) void init(int mode) {
    delete g_ctrl;
    g_ctrl = new Controller(g_machine.machine(), mode ? ClosedLoop : OpenLoop, Config(), wasm_log);
}
__attribute__((export_name("tick"))) void tick() { if (g_ctrl) g_ctrl->update(); }

// Exposed controller state for the visualizer/debugger. Instead of a hand-written
// per-field accessor, we hand out the ADDRESS of the live Track array: the JS side
// decodes it with the generated descriptor (offsets.track), so every field is
// visible and nothing drifts when the layout changes. HAL.md §H-8.1.
__attribute__((export_name("track_count"))) int track_count() {
    return g_ctrl ? (int)g_ctrl->tracks().size() : 0;
}
// Base address of tracks[0]. Re-read every tick: the backing store may move.
__attribute__((export_name("tracks_ptr"))) const Track* tracks_ptr() {
    return (g_ctrl && !g_ctrl->tracks().empty()) ? g_ctrl->tracks().data() : nullptr;
}
// Element stride, so JS never assumes a packing rule.
__attribute__((export_name("track_stride"))) int track_stride() { return (int)sizeof(Track); }

} // extern "C"
