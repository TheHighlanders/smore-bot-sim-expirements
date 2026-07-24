#include "smores/Controller.h"
#include <cstdio>
#include <cstdarg>
#include <cmath>

namespace smores {

Track* Controller::nearest(float pos) {
    Track* best = nullptr; float bd = 1e9f;
    for (auto& tr : tracks_) {
        if (tr.status == Done || tr.status == Lost) continue;
        float d = std::fabs(tr.est_pos_mm - pos);
        if (d < bd && d < 120.f) { bd = d; best = &tr; }
    }
    return best;
}

void Controller::say(const char* kind, const char* fmt, ...) {
    if (!log_) return;
    char buf[160];
    va_list ap; va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    log_(kind, buf);
}

void Controller::update() {
    const uint32_t now = hal_.nowMs();
    float dt = have_now_ ? (now - last_now_) / 1000.0f : 0.0f;
    if (dt < 0.f || dt > 0.5f) dt = 0.f;           // guard against clock jumps
    last_now_ = now; have_now_ = true;
    const bool run = hal_.running();

    // ---- read inputs via subsystems ----
    bool    sense[N_STATIONS]; uint8_t confirm[N_STATIONS];
    for (int k = 0; k < N_STATIONS; k++) { sense[k] = stations_[k].trayPresent(); confirm[k] = stations_[k].confirmedDrops(); }
    const bool tEntry = tunnel_.atEntry(), tExit = tunnel_.atExit();

    // ---- outputs (defaults) ----
    float belt = run ? cfg_.nominal_speed : 0.f;
    bool  gate[N_STATIONS] = {true,true,true};
    bool  disp[N_STATIONS] = {false,false,false};

    // 1) dead-reckon moving tracks
    for (auto& tr : tracks_) if (tr.status == Moving) tr.est_pos_mm += belt * dt;

    // 2) sensor rising edges -> register / correct + start a hold
    for (int k = 0; k < N_STATIONS; k++) {
        if (sense[k] && !last_sense_[k]) {
            Track* tr = nearest(cfg_.station_pos_mm[k]);
            if (!tr && k == 0) {
                Track nt; nt.id = next_id_++; nt.est_pos_mm = cfg_.station_pos_mm[0];
                tracks_.push_back(nt); tr = &tracks_.back();
                say("evt", "tray #%d detected at GRAHAM", tr->id);
            }
            if (tr) {
                float corr = cfg_.station_pos_mm[k] - tr->est_pos_mm;
                tr->est_pos_mm = cfg_.station_pos_mm[k];       // snap: correct drift
                if (tr->stage == k) {
                    tr->status = Held; tr->hold = k; tr->retries = 0; tr->phase_until = now + cfg_.dispense_ms;
                    say("evt", "saw #%d at station %d (drift %+.0fmm) - closing gate, running dispenser", tr->id, k, corr);
                }
            }
        }
    }
    for (int k = 0; k < N_STATIONS; k++) last_sense_[k] = sense[k];

    // 3) service held trays once the dispenser has run long enough
    for (auto& tr : tracks_) {
        if (tr.status == Held && now >= tr.phase_until) {
            int k = tr.hold;
            if (mode_ == ClosedLoop) {
                uint8_t c = confirm[k];
                if (c == 0 && tr.retries < 3) {                // nothing dropped -> keep running (retry)
                    tr.retries++; tr.phase_until = now + cfg_.dispense_ms;
                    say("warn", "no drop confirmed for #%d at station %d - extending dispenser run (retry %d)", tr.id, k, tr.retries);
                    continue;
                }
                tr.placed[k] = c;                              // belief = confirmed count
                if (c == 0)      say("crit", "station %d UNDER-FILLED after %d retries - flagging #%d", k, tr.retries, tr.id);
                else if (c >= 2) say("warn", "station %d OVER-FILL: sensor counted %d - flagging #%d", k, c, tr.id);
                else             say("evt",  "station %d confirmed on #%d - opening gate", k, tr.id);
            } else {
                tr.placed[k] = 1;                              // believes it placed (ran output long enough)
                say("evt", "ran station %d dispenser %ums -> assuming placed, opening gate for #%d", k, cfg_.dispense_ms, tr.id);
            }
            tr.stage = k + 1; tr.status = Moving; tr.hold = -1;
        }
    }
    // sustained dispenser output during a hold; gate closed only where held
    for (auto& tr : tracks_) if (tr.status == Held && now < tr.phase_until) disp[tr.hold] = true;
    for (int k = 0; k < N_STATIONS; k++) {
        bool held = false; for (auto& tr : tracks_) if (tr.status == Held && tr.hold == k) held = true;
        gate[k] = !held;
    }

    // 4) tunnel: the tunnel GATE holds a tray to toast (belt keeps running, so
    //    other trays keep working). Release when the dwell has elapsed.
    if (tEntry) {
        Track* tr = nearest(cfg_.tunnel_entry_mm);
        if (tr && tr->stage == 3 && tr->status == Moving) {
            tr->status = Toasting; tr->phase_until = now + cfg_.toast_ms;
            say("evt", "#%d reached tunnel - heater ON, closing tunnel gate to toast", tr->id);
        }
    }
    for (auto& tr : tracks_)
        if (tr.status == Toasting && now >= tr.phase_until) {
            tr.stage = 4; tr.status = Moving;
            say("evt", "tunnel dwell %ums elapsed - opening tunnel gate, releasing #%d", cfg_.toast_ms, tr.id);
        }
    bool toasting = false;
    for (auto& tr : tracks_) if (tr.status == Toasting) toasting = true;
    const bool heater = toasting;
    const bool tunnel_gate_open = !toasting;         // closed (hold) while toasting
    if (tExit) {
        Track* tr = nearest(cfg_.tunnel_exit_mm);
        if (tr && tr->stage >= 4 && tr->status != Done) { tr->status = Done; say("evt", "s'more #%d complete", tr->id); }
    }

    // 5) lost-tray watchdog
    for (auto& tr : tracks_)
        if (tr.status != Done && tr.status != Lost && tr.est_pos_mm > cfg_.belt_len_mm + 90.f) {
            tr.status = Lost; say("crit", "tray #%d LOST (no exit edge)", tr.id);
        }

    // ---- write outputs via subsystems ----
    conveyor_.run(belt);
    for (int k = 0; k < N_STATIONS; k++) { stations_[k].hold(!gate[k]); stations_[k].runDispenser(disp[k]); }
    tunnel_.hold(!tunnel_gate_open);
    tunnel_.heater(heater);
}

} // namespace smores
