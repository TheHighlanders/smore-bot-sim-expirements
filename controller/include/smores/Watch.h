// Watch.h — WATCH(name, value): publish a named value from inside the control
// loop (HAL.md §H-8.2).
//
// The generated State block (generated/State.h) already exposes the controller's
// *declared* state with no code at all. WATCH covers what that can't reach:
// locals, intermediates, and "why did it decide that" quantities — e.g. the
// dead-reckoning drift measured at a sensor snap, which exists only for one line.
//
// This is not sim-only scaffolding: it is the same discipline as exposing PLC tags
// to an HMI. On the sim it crosses to the debugger's watch window; on hardware it
// compiles to NOTHING by default (OQ-4), so it cannot perturb control-loop timing.
// Opt in on hardware with -DSMORES_WATCH_ENABLE=1 and provide host_watch().
#pragma once

#ifndef SMORES_WATCH_ENABLE
#  if defined(__wasm__)
#    define SMORES_WATCH_ENABLE 1      // the visualizer always wants telemetry
#  else
#    define SMORES_WATCH_ENABLE 0      // host tests / real board: off unless asked
#  endif
#endif

#if SMORES_WATCH_ENABLE

extern "C" __attribute__((import_module("env"), import_name("host_watch")))
void host_watch(const char* name, float value);

namespace smores { namespace watch_detail {
inline void publish(const char* name, float value) { host_watch(name, value); }
}} // namespace smores::watch_detail

// `value` is taken by value and converted to float — cheap, and keeps the host
// boundary a single stable signature regardless of the expression's type.
#define WATCH(name, value) ::smores::watch_detail::publish((name), static_cast<float>(value))

#else

// Evaluate nothing: a disabled WATCH must not change behaviour OR cost time.
#define WATCH(name, value) ((void)0)

#endif
