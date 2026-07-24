// Subsystem unit tests: each method is a stateless intent -> one HAL call.
// Build+run: see controller/Makefile (host-test). No simulator needed.
#include "smores/Subsystems.h"
#include <cassert>
#include <cstdio>
using namespace smores;

struct MockHal : Hal {
    // scripted inputs
    bool tray[3]={false,false,false}; bool tin=false,tout=false; float temp=20; uint8_t conf[3]={0,0,0}; bool run=true; uint32_t now=0;
    // recorded last outputs
    float belt=-1; int gateS=-1; bool gateOpen=false; int dispS=-1; bool dispOn=false; int tgate=-1; bool heater=false;
    uint32_t nowMs() override { return now; }
    bool running() override { return run; }
    bool trayPresent(int s) override { return tray[s]; }
    bool tunnelEntry() override { return tin; }
    bool tunnelExit() override { return tout; }
    float tunnelTempC() override { return temp; }
    uint8_t dispenseConfirm(int s) override { return conf[s]; }
    void setBeltSpeed(float v) override { belt=v; }
    void setGate(int s,bool o) override { gateS=s; gateOpen=o; }
    void setDispense(int s,bool o) override { dispS=s; dispOn=o; }
    void setTunnelGate(bool o) override { tgate=o?1:0; }
    void setHeater(bool o) override { heater=o; }
};

int main() {
    MockHal h;

    Conveyor cv(h);
    cv.run(110.f);  assert(h.belt==110.f);
    cv.stop();      assert(h.belt==0.f);

    Station st(h, 1);
    assert(st.index()==1);
    h.tray[1]=true;  assert(st.trayPresent());
    h.tray[1]=false; assert(!st.trayPresent());
    st.hold(true);   assert(h.gateS==1 && h.gateOpen==false);   // hold => gate NOT open
    st.hold(false);  assert(h.gateS==1 && h.gateOpen==true);
    st.runDispenser(true);  assert(h.dispS==1 && h.dispOn==true);
    st.runDispenser(false); assert(h.dispS==1 && h.dispOn==false);
    h.conf[1]=2; assert(st.confirmedDrops()==2);

    HeatingTunnel tun(h);
    tun.heater(true);  assert(h.heater==true);
    tun.hold(true);    assert(h.tgate==0);                      // hold => tunnel gate NOT open
    tun.hold(false);   assert(h.tgate==1);
    h.temp=180.f; assert(tun.temperatureC()==180.f);
    h.tin=true; assert(tun.atEntry()); h.tout=true; assert(tun.atExit());

    printf("test_subsystems: PASS\n");
    return 0;
}
