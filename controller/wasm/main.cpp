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

// Minimal exposed controller state for the visualizer overlay.
__attribute__((export_name("track_count"))) int track_count() {
    return g_ctrl ? (int)g_ctrl->tracks().size() : 0;
}
// field: 0=id, 1=est_pos_mm, 2=stage, 3=status, 4..(4+N_DISP-1)=placed[0..N_DISP-1]
__attribute__((export_name("track_field"))) float track_field(int i, int field) {
    if (!g_ctrl || i < 0 || i >= (int)g_ctrl->tracks().size()) return 0.f;
    const Track& t = g_ctrl->tracks()[i];
    switch (field) {
        case 0: return (float)t.id;
        case 1: return t.est_pos_mm;
        case 2: return (float)t.stage;
        case 3: return (float)t.status;
        default:
            if (field >= 4 && field < 4 + layout::N_DISP) return (float)t.placed[field - 4];
            return 0.f;
    }
}

} // extern "C"
