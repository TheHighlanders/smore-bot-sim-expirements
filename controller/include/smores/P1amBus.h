// P1amBus.h — selects which P1AM implementation P1amMachine talks to, and supplies
// a board clock. This is the ONLY target-dependent file in the controller tree.
//
// Three targets, one API surface:
//   - real ProductivityOpen : the FACTS `P1AM` library (class P1AM, global `P1`)
//   - Wokwi / host tests    : `lib/P1AM_Sim` — a drop-in fake with the same calls,
//                             over SPI to the custom chip or an in-process model
//   - browser (WASM)        : not used; the browser uses SimMachine instead
#pragma once
#include <cstdint>

#if defined(SMORES_REAL_P1AM)
  #include <P1AM.h>
  namespace smores { using P1Bus = P1AM; }
#else
  #include "P1AM_Sim.h"
  namespace smores { using P1Bus = P1AM_Sim; }
#endif

namespace smores {

// Monotonic board time. On Arduino this is millis(); on a host test we use a
// steady clock so the controller's timing logic behaves identically.
inline uint32_t p1am_millis() {
#if defined(ARDUINO)
    return (uint32_t)millis();
#else
    extern uint32_t g_host_millis;      // advanced by the host test harness
    return g_host_millis;
#endif
}

} // namespace smores
