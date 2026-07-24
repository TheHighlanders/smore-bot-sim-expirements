// Controller logic tests. A compact fake world (belt + one tray + dispensers +
// tunnel) drives the controller through the StructHal; we assert the resulting
// actual contents and decision log. Mirrors the browser-verified behaviour.
#include "smores/Controller.h"
#include "smores/StructHal.h"
#include <cassert>
#include <cstdio>
#include <cmath>
#include <string>
using namespace smores;

static std::string g_log;
static void sink(const char* kind, const char* msg){ g_log += "["; g_log += kind; g_log += "] "; g_log += msg; g_log += "\n"; }

struct FakeWorld {
    Inputs in; Outputs out;
    float pos = -32.f; int counts[3] = {0,0,0}; float energy = 0.f;
    bool grahamFlaky = false; int att[3] = {0,0,0}; float dispOn[3] = {0,0,0}, lastAtt[3] = {0,0,0};
    const float SP[3] = {300,600,900}, TIN = 1050, TEX = 1185, SLIP = 0.965f, DROP = 450;

    void sense(uint32_t now){
        for (int k=0;k<3;k++) in.sense[k] = std::fabs(pos-SP[k]) < 22;
        in.tunnel_entry = std::fabs(pos-TIN) < 22;
        in.tunnel_exit  = std::fabs(pos-TEX) < 22;
        in.now_ms = now; in.run = true; in.tunnel_temp_c = out.heater ? 205.f : 20.f;
    }
    void step(float dt){
        bool blocked = false;
        for (int k=0;k<3;k++) if (!out.gate_open[k] && pos < SP[k] && pos > SP[k]-70) blocked = true;
        if (!out.tunnel_gate_open && pos > TIN-30 && pos < TEX) blocked = true;   // tunnel hold
        if (!blocked) pos += out.belt_speed * SLIP * dt;
        for (int k=0;k<3;k++){
            if (out.dispense[k]){
                dispOn[k] += dt*1000;
                if (dispOn[k]-lastAtt[k] >= DROP){          // output ran long enough -> a drop attempt
                    lastAtt[k] = dispOn[k];
                    if (std::fabs(pos-SP[k]) < 70){
                        att[k]++; int add = 1;
                        if (k==0 && grahamFlaky) add = (att[k]==1) ? 0 : 1;   // first attempt misfires
                        counts[k] += add; in.dispense_confirm[k] = (uint8_t)add;
                    } else in.dispense_confirm[k] = 0;
                }
            } else { dispOn[k] = 0; lastAtt[k] = 0; }
        }
        if (out.heater && pos > TIN-40 && pos < TEX+40 && counts[2] > 0) energy += (in.tunnel_temp_c-20.f)*dt;
    }
};

static void run(Mode mode, bool flaky, int& g, int& c, int& m, std::string& log){
    g_log.clear();
    FakeWorld w; w.grahamFlaky = flaky;
    StructHal hal(&w.in, &w.out);
    Controller ctrl(hal, mode, Config(), sink);
    uint32_t t = 0; const float dt = 0.02f;
    for (int i=0;i<1500;i++){ t += 20; w.sense(t); ctrl.update(); w.step(dt); }
    g = w.counts[0]; c = w.counts[1]; m = w.counts[2]; log = g_log;
}

int main(){
    int g,c,m; std::string log;

    // 1) open-loop, no fault -> fully assembled + completes
    run(OpenLoop, false, g, c, m, log);
    assert(g==1 && c==1 && m==1);
    assert(log.find("assuming placed") != std::string::npos);
    assert(log.find("s'more #1 complete") != std::string::npos);

    // 2) open-loop + flaky graham -> ships with no graham, controller oblivious
    run(OpenLoop, true, g, c, m, log);
    assert(g==0 && c==1 && m==1);
    assert(log.find("assuming placed") != std::string::npos);
    assert(log.find("s'more #1 complete") != std::string::npos);

    // 3) closed-loop + flaky graham -> retries, recovers, ships complete
    run(ClosedLoop, true, g, c, m, log);
    assert(g==1 && c==1 && m==1);
    assert(log.find("extending dispenser run") != std::string::npos);
    assert(log.find("s'more #1 complete") != std::string::npos);

    printf("test_controller: PASS\n");
    return 0;
}
