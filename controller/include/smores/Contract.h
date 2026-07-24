// Contract.h — the controller <-> world I/O contract (REQUIREMENTS.md §4).
//
// This is now GENERATED from the active layout (controller/layouts/<name>.json)
// by tools/layout/codegen.mjs — the layout and the controller are a bound pair,
// and the layout is the source of truth for the struct shape (LAYOUTS.md §L-5).
// This shim forwards to the generated header so existing `#include "Contract.h"`
// sites keep working; the real Inputs/Outputs + layout constants live in
// generated/Contract.h and generated/Layout.h. Regenerate: `make -C controller codegen`.
#pragma once
#include "generated/Contract.h"
