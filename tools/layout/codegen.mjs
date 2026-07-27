// codegen.mjs — turn a layout.json into the controller's typed contract.
//
// A layout and a controller are a BOUND PAIR: the layout is the source of truth,
// and it GENERATES the exact Inputs/Outputs structs + compile-time geometry the
// controller is written against. A controller that references a module its layout
// doesn't have fails to compile (that's the point — LAYOUTS.md L-5/OQ-1).
//
// Pure functions (parseLayout / emitLayoutHeader / emitContractHeader /
// computeOffsets / meta) are exported so the same generator can run in-browser
// later (the in-app editor); a small CLI at the bottom wires it for `make`.
//
// This is NOT firmware and makes no claims about real hardware, so no citation is
// required (CLAUDE.md citation policy); it only shapes our own contract.

import { parseModuleDb } from "./modules.mjs";

// ---- C type sizes/alignments for the wasm32 target (LP32; matches clang) ----
const T = {
  u32:  { size: 4, align: 4, c: "uint32_t" },
  i32:  { size: 4, align: 4, c: "int32_t" },
  f32:  { size: 4, align: 4, c: "float" },
  u8:   { size: 1, align: 1, c: "uint8_t" },
  bool: { size: 1, align: 1, c: "bool" },
  stat: { size: 4, align: 4, c: "Status" },     // enum Status : int32_t
};

const INGREDIENT_ENUM = { graham: "Graham", chocolate: "Chocolate", marshmallow: "Marshmallow" };

// ---- hardware binding (HAL.md §H-6/§H-7) --------------------------------------
// A layout MAY bind each logical signal to a real (slot, channel). If it does, we
// can generate the P1AM implementation AND reject a mis-wired machine at build
// time. `io` is optional: without it a layout is sim-only (H-6.4), so every
// existing layout keeps working.
//
// Signal direction per HAL interface, used by VAL-H3/VAL-H4:
const OUT_SIGNALS = new Set(["gate", "actuator", "heater", "press"]);
const IN_SIGNALS  = new Set(["sense", "confirm", "entry", "exit"]);
const ANALOG_SIGNALS = new Set(["temp"]);

function checkRef(ref, what, sigName, slots, catalog, used, errors) {
  if (!ref || typeof ref !== "object") { errors.push(`${what}: signal "${sigName}" binding must be an object {slot, channel}`); return; }
  const slot = +ref.slot, channel = +ref.channel;
  const spec = slots.get(slot);
  if (!spec) { errors.push(`${what}: signal "${sigName}" is bound to slot ${ref.slot}, which is not in base[]`); return; }   // VAL-H1
  const props = catalog[spec.part];
  if (!props) { errors.push(`${what}: slot ${slot} declares unknown part "${spec.part}"`); return; }                        // VAL-H1
  if (!(channel >= 1 && channel <= props.channels))                                                                        // VAL-H2
    errors.push(`${what}: signal "${sigName}" uses channel ${ref.channel} but ${spec.part} in slot ${slot} has channels 1..${props.channels}`);
  if (OUT_SIGNALS.has(sigName) && !(props.doBytes > 0))                                                                    // VAL-H3
    errors.push(`${what}: output signal "${sigName}" is bound to ${spec.part} (slot ${slot}), which has no discrete outputs`);
  if (IN_SIGNALS.has(sigName) && !(props.diBytes > 0))                                                                     // VAL-H3
    errors.push(`${what}: input signal "${sigName}" is bound to ${spec.part} (slot ${slot}), which has no discrete inputs`);
  if (ANALOG_SIGNALS.has(sigName) && !(props.aiBytes > 0))                                                                 // VAL-H4
    errors.push(`${what}: analog signal "${sigName}" is bound to ${spec.part} (slot ${slot}), which has no analog inputs`);
  // VAL-H5 — collision. Keyed by DIRECTION as well as slot/channel: a combo module
  // (e.g. P1-16CDR = 8 DC in + 8 relay out) has separate input and output images,
  // so input ch1 and output ch1 are different physical points, not a conflict.
  const dir = OUT_SIGNALS.has(sigName) ? "out" : ANALOG_SIGNALS.has(sigName) ? "ai" : "in";
  const key = `${slot}:${dir}:${channel}`;
  if (used.has(key)) errors.push(`${what}: slot ${slot} ${dir} channel ${channel} is already used by ${used.get(key)} — two signals cannot share a channel`);
  else used.set(key, `${what}.${sigName}`);
  return { slot, channel };
}

// Validate a layout's binding against the module catalog. Returns
// { bound, base, errors }: `bound` is false when the layout has no `io` at all.
export function validateBinding(L, catalog) {
  const errors = [];
  const rawBase = Array.isArray(L.raw.base) ? L.raw.base : [];
  const anyIo = L.modules.some(m => m.io);
  if (!rawBase.length && !anyIo) return { bound: false, base: [], errors };            // sim-only: fine
  if (anyIo && !rawBase.length) errors.push("modules declare io{} but the layout has no base[] module inventory");
  if (rawBase.length && !anyIo) errors.push("layout declares base[] but no module binds any io{} signal");

  const slots = new Map();
  for (const b of rawBase) {
    const slot = +b.slot;
    if (!(slot >= 1 && slot <= 15)) { errors.push(`base[]: slot ${b.slot} out of range (1..15)`); continue; }               // VAL-H1
    if (slots.has(slot)) { errors.push(`base[]: duplicate slot ${slot}`); continue; }                                       // VAL-H1
    const props = catalog[b.part];
    if (!props) { errors.push(`base[]: slot ${slot} declares unknown part "${b.part}" (not in module_db.h)`); continue; }   // VAL-H1
    const cfg = b.config == null ? null : b.config;
    if (props.configBytes > 0) {                                                                                           // VAL-H7
      if (!Array.isArray(cfg)) errors.push(`base[]: ${b.part} in slot ${slot} needs a ${props.configBytes}-byte config array (configureModule); none supplied`);
      else if (cfg.length !== props.configBytes) errors.push(`base[]: ${b.part} in slot ${slot} config must be ${props.configBytes} bytes, got ${cfg.length}`);
      else if (cfg.some(v => !Number.isInteger(v) || v < 0 || v > 255)) errors.push(`base[]: ${b.part} slot ${slot} config bytes must be integers 0..255`);
    } else if (cfg != null) {
      errors.push(`base[]: ${b.part} in slot ${slot} takes no configuration, but config was supplied`);
    }
    slots.set(slot, { slot, part: b.part, config: Array.isArray(cfg) ? cfg : null, props });
  }

  const used = new Map();
  const dispIo = [];
  for (const d of L.dispensers) {
    const src = L.modules.find(m => m.id === d.id);
    const io = src && src.io;
    const what = `dispenser "${d.id}"`;
    if (!io) { errors.push(`${what}: no io{} binding (every module must be bound once the layout declares base[])`); dispIo.push(null); continue; }
    const b = {};
    b.sense = checkRef(io.sense, what, "sense", slots, catalog, used, errors);
    b.gate  = checkRef(io.gate,  what, "gate",  slots, catalog, used, errors);
    const acts = Array.isArray(io.actuator) ? io.actuator : (io.actuator ? [io.actuator] : []);
    if (acts.length !== d.servos)                                                                                          // VAL-H6
      errors.push(`${what}: declares servos=${d.servos} but binds ${acts.length} actuator channel(s)`);
    b.act = acts.map((a, i) => checkRef(a, what, "actuator", slots, catalog, used, errors));
    if (d.confirm) {
      if (!io.confirm) errors.push(`${what}: confirm=true but no "confirm" signal is bound`);                              // VAL-H6
      else b.confirm = checkRef(io.confirm, what, "confirm", slots, catalog, used, errors);
    }
    dispIo.push(b);
  }

  let tunnelIo = null;
  if (L.hasTunnel) {
    const src = L.modules.find(m => m.id === L.tunnel.id);
    const io = src && src.io;
    const what = `tunnel "${L.tunnel.id}"`;
    if (!io) errors.push(`${what}: no io{} binding`);
    else tunnelIo = {
      entry:  checkRef(io.entry,  what, "entry",  slots, catalog, used, errors),
      exit:   checkRef(io.exit,   what, "exit",   slots, catalog, used, errors),
      temp:   checkRef(io.temp,   what, "temp",   slots, catalog, used, errors),
      gate:   checkRef(io.gate,   what, "gate",   slots, catalog, used, errors),
      heater: checkRef(io.heater, what, "heater", slots, catalog, used, errors),
    };
  }

  return { bound: errors.length === 0, base: [...slots.values()], dispIo, tunnelIo, errors };
}

// ---- validate + normalize a raw layout object (LAYOUTS.md L-8) ----
export function parseLayout(raw) {
  if (!raw || typeof raw !== "object") throw new Error("layout must be an object");
  const name = raw.name;
  if (!/^[a-zA-Z][a-zA-Z0-9_]*$/.test(name || "")) throw new Error(`layout.name must be a C identifier, got ${JSON.stringify(name)}`);
  const belt = raw.belt || {};
  const beltLen = +belt.length_mm, speed = +belt.nominal_speed_mm_s;
  if (!(beltLen > 0)) throw new Error("belt.length_mm must be > 0");
  if (!(speed > 0)) throw new Error("belt.nominal_speed_mm_s must be > 0");
  if (!Array.isArray(raw.modules) || raw.modules.length === 0) throw new Error("layout.modules must be a non-empty array");

  const ids = new Set();
  let lastPos = -Infinity, tunnelIndex = -1;
  const modules = raw.modules.map((m, i) => {
    if (!/^[a-zA-Z][a-zA-Z0-9_]*$/.test(m.id || "")) throw new Error(`module[${i}].id must be a C identifier, got ${JSON.stringify(m.id)}`);
    if (ids.has(m.id)) throw new Error(`duplicate module id "${m.id}"`);
    ids.add(m.id);
    const pos = +m.pos_mm;
    if (!(pos >= 0)) throw new Error(`module "${m.id}" pos_mm must be >= 0`);
    if (pos <= lastPos) throw new Error(`modules must be in strictly increasing belt order: "${m.id}" at ${pos} <= previous ${lastPos}`);
    lastPos = pos;
    if (!["dispenser", "tunnel", "smusher", "sensor"].includes(m.type)) throw new Error(`module "${m.id}" has unknown type "${m.type}"`);
    if (m.type === "tunnel") {
      if (tunnelIndex >= 0) throw new Error("only one tunnel is supported this iteration");
      tunnelIndex = i;
      if (!(+m.exit_mm > pos)) throw new Error(`tunnel "${m.id}" exit_mm must be > pos_mm`);
    }
    if (m.type === "dispenser") {
      if (!INGREDIENT_ENUM[m.ingredient]) throw new Error(`dispenser "${m.id}" needs a known ingredient (graham|chocolate|marshmallow), got ${JSON.stringify(m.ingredient)}`);
      const servos = m.servos == null ? 1 : +m.servos;
      if (servos !== 1 && servos !== 2) throw new Error(`dispenser "${m.id}" servos must be 1 or 2`);
    }
    return { ...m, pos_mm: pos, _index: i };
  });
  if (lastPos > beltLen) throw new Error(`last module at ${lastPos}mm is past belt end ${beltLen}mm`);

  // Assembly STAGES in belt order: each dispenser / tunnel / smusher is one step
  // a tray must complete in sequence. A sensor-only module consumes no stage.
  // `stage` is what makes the controller topology-agnostic: it holds dispenser d
  // when tray.stage == d.stage, regardless of whether d is before or after the
  // tunnel (the post-tunnel cap just gets a higher stage than the tunnel).
  const stageOf = {};
  let step = 0;
  for (const m of modules) if (["dispenser", "tunnel", "smusher"].includes(m.type)) stageOf[m.id] = step++;
  const nStages = step;

  // Timings live on the MACHINE, not the controller (HAL.md OQ-2): two different
  // machines should be able to differ, and a controller shouldn't hard-code the
  // mechanical dwell of a dispenser it was never told about.
  const dispensers = modules.filter(m => m.type === "dispenser").map(m => ({
    id: m.id, pos_mm: m.pos_mm, ingredient: m.ingredient,
    servos: m.servos == null ? 1 : +m.servos,
    confirm: m.confirm !== false,
    role: m.role || "base",
    after_tunnel: tunnelIndex >= 0 && m._index > tunnelIndex,
    stage: stageOf[m.id],
    dispense_ms: m.dispense_ms == null ? 650 : +m.dispense_ms,
  }));
  const tunnel = tunnelIndex >= 0 ? {
    id: modules[tunnelIndex].id, pos_mm: modules[tunnelIndex].pos_mm,
    exit_mm: +modules[tunnelIndex].exit_mm, stage: stageOf[modules[tunnelIndex].id],
    toast_ms: modules[tunnelIndex].toast_ms == null ? 3800 : +modules[tunnelIndex].toast_ms,
  } : null;
  const smusherM = modules.find(m => m.type === "smusher");
  const smusher = smusherM ? { id: smusherM.id, pos_mm: smusherM.pos_mm, dwell_ms: smusherM.dwell_ms == null ? 400 : +smusherM.dwell_ms, confirm: smusherM.confirm !== false, stage: stageOf[smusherM.id] } : null;

  const maxServos = dispensers.reduce((a, d) => Math.max(a, d.servos), 1);
  const ingredients = [...new Set(dispensers.map(d => d.ingredient))]; // dedup, first-seen order

  return {
    raw,                                   // kept so the binding validator can read base[]
    name, title: raw.title || name, description: raw.description || "",
    beltLen, speed, modules, dispensers, tunnel, smusher, maxServos, ingredients, nStages,
    hasTunnel: !!tunnel, hasSmusher: !!smusher,
    schema: raw.schema == null ? 1 : +raw.schema,   // OQ-5: missing => 1 (sim-only)
  };
}

// ---- flat field list for a struct, in the canonical order that keeps classic3
//      byte-identical to the original hand-written contract. `def` is the C++
//      default init (scalar literal, or "0"/"true" for array fill). ----
function inputFields(L) {
  const N = L.dispensers.length;
  const f = [{ name: "now_ms", t: "u32", def: "0" }, { name: "sense", t: "bool", n: N, def: "0" }];
  if (L.hasTunnel) f.push({ name: "tunnel_entry", t: "bool", def: "false" }, { name: "tunnel_exit", t: "bool", def: "false" }, { name: "tunnel_temp_c", t: "f32", def: "20.0f" });
  f.push({ name: "dispense_confirm", t: "u8", n: N, def: "0" });
  if (L.hasSmusher) f.push({ name: "smusher_sense", t: "bool", def: "false" }, { name: "smusher_confirm", t: "u8", def: "0" });
  f.push({ name: "run", t: "bool", def: "true" });
  return f;
}
function outputFields(L) {
  const N = L.dispensers.length;
  const f = [{ name: "belt_speed", t: "f32", def: "0.0f" }, { name: "gate_open", t: "bool", n: N, def: "true" }, { name: "dispense", t: "bool", n: N, def: "0" }];
  if (L.maxServos >= 2) f.push({ name: "dispense_b", t: "bool", n: N, def: "0" });
  if (L.hasTunnel) f.push({ name: "tunnel_gate_open", t: "bool", def: "true" }, { name: "heater", t: "bool", def: "false" });
  if (L.hasSmusher) f.push({ name: "smusher_gate_open", t: "bool", def: "true" }, { name: "smusher_press", t: "bool", def: "false" });
  return f;
}

// ---- struct layout (offsets + size) mirroring C default alignment ----
function layoutStruct(fields) {
  let off = 0, maxAlign = 1;
  const out = [];
  for (const fld of fields) {
    const ty = T[fld.t]; const count = fld.n || 1;
    off = Math.ceil(off / ty.align) * ty.align;
    out.push({ name: fld.name, type: fld.t, count: fld.n || null, offset: off, elemSize: ty.size });
    off += ty.size * count;
    maxAlign = Math.max(maxAlign, ty.align);
  }
  const size = Math.ceil(off / maxAlign) * maxAlign;
  return { fields: out, size };
}

// The controller's exposed BELIEF about one tray. Generating this struct (rather
// than hand-mirroring it in JS) is what lets the UI decode any field with no
// per-field accessor — it replaces the old hand-written track_field() switch,
// which silently capped at 3 dispensers. See HAL.md §H-8.1.
const STATUS_NAMES = ["moving", "held", "toasting", "done", "lost"];
function trackFields(L) {
  return [
    { name: "id",          t: "i32", def: "0" },
    { name: "est_pos_mm",  t: "f32", def: "0.f" },
    { name: "stage",       t: "i32", def: "0" },
    { name: "status",      t: "stat", def: "Moving" },
    { name: "hold",        t: "i32", def: "-1" },
    { name: "phase_until", t: "u32", def: "0" },
    { name: "placed",      t: "i32", n: L.dispensers.length, def: "0" },
    { name: "retries",     t: "i32", def: "0" },
  ];
}

export function computeOffsets(L) {
  return {
    inputs:  layoutStruct(inputFields(L)),
    outputs: layoutStruct(outputFields(L)),
    track:   layoutStruct(trackFields(L)),
    statusNames: STATUS_NAMES,
  };
}

export function emitStateHeader(L) {
  const flds = trackFields(L);
  const size = layoutStruct(flds).size;
  return `${banner(L, "State.h")}#pragma once
#include <cstdint>
#include "Layout.h"

namespace smores {

// Underlying type is fixed so the struct layout is stable across targets and the
// visualizer can decode it from the generated offsets.
enum Status : int32_t { ${STATUS_NAMES.map((n, i) => `${n[0].toUpperCase() + n.slice(1)} = ${i}`).join(", ")} };

// One tracked tray, as the controller BELIEVES it to be. Never the world's truth.
struct Track {
${flds.map(cFieldDecl).join("\n")}
};

// If this check ever fails, the app's idea of how a Track is arranged in memory no
// longer matches what the compiler produced, and it would show meaningless numbers.
static_assert(sizeof(Track) == ${size}, "Track layout must match the generated descriptor");

} // namespace smores
`;
}

// ---- emit C++ ----
const banner = (L, file) =>
  `// ${file} — GENERATED from controller/layouts/${L.name}.json by tools/layout/codegen.mjs.\n` +
  `// DO NOT EDIT BY HAND. Regenerate with \`make -C controller codegen\`.\n` +
  `// Layout "${L.name}": ${L.title}\n`;

function cFieldDecl(fld) {
  const ty = T[fld.t];
  let init;
  if (fld.n) init = (fld.def === "true") ? ` = {${Array(fld.n).fill("true").join(", ")}}` : " = {}";
  else init = ` = ${fld.def}`;
  const decl = fld.n ? `${fld.name}[${fld.n}]` : fld.name;
  return `    ${ty.c.padEnd(8)} ${decl}${init};`;
}

export function emitContractHeader(L) {
  const inF = inputFields(L), outF = outputFields(L);
  return `${banner(L, "Contract.h")}#pragma once
#include <cstdint>
#include "Layout.h"

namespace smores {

// Inputs (world -> controller) — one tick. Field set is fixed by the layout.
struct Inputs {
${inF.map(cFieldDecl).join("\n")}
};

// Outputs (controller -> world) — one tick.
struct Outputs {
${outF.map(cFieldDecl).join("\n")}
};

} // namespace smores
`;
}

export function emitLayoutHeader(L) {
  const ing = L.ingredients.map((k, i) => `${INGREDIENT_ENUM[k]}${i < L.ingredients.length - 1 ? "," : ""}`).join(" ");
  const disp = L.dispensers.map(d =>
    `    { "${d.id}", ${d.pos_mm.toFixed(1)}f, Ingredient::${INGREDIENT_ENUM[d.ingredient]}, ${d.servos}, ${d.confirm}, ${d.after_tunnel}, ${d.role === "cap"}, ${d.stage}, ${d.dispense_ms} },`
  ).join("\n");
  let tunnel = "";
  if (L.hasTunnel) tunnel =
    `constexpr float    TUNNEL_ENTRY_MM = ${L.tunnel.pos_mm.toFixed(1)}f;\n` +
    `constexpr float    TUNNEL_EXIT_MM  = ${L.tunnel.exit_mm.toFixed(1)}f;\n` +
    `constexpr int      TUNNEL_STAGE    = ${L.tunnel.stage};\n` +
    `constexpr uint32_t TUNNEL_TOAST_MS = ${L.tunnel.toast_ms};   // machine dwell (OQ-2)\n`;
  let smusher = "";
  if (L.hasSmusher) smusher =
    `constexpr float    SMUSHER_POS_MM   = ${L.smusher.pos_mm.toFixed(1)}f;\n` +
    `constexpr uint32_t SMUSHER_DWELL_MS = ${L.smusher.dwell_ms};\n` +
    `constexpr int      SMUSHER_STAGE    = ${L.smusher.stage};\n`;
  return `${banner(L, "Layout.h")}#pragma once
#include <cstdint>

// Preprocessor mirrors of the layout facts below. Needed because a HAL
// implementation must conditionally COMPILE against contract fields that only
// exist for some layouts (a tunnel's sensors, a 2nd servo plane) — \`if constexpr\`
// can't guard a member that isn't declared. See HAL.md §H-5.
#define SMORES_N_DISP      ${L.dispensers.length}
#define SMORES_MAX_SERVOS  ${L.maxServos}
#define SMORES_HAS_TUNNEL  ${L.hasTunnel ? 1 : 0}
#define SMORES_HAS_SMUSHER ${L.hasSmusher ? 1 : 0}

namespace smores {
namespace layout {

constexpr const char* NAME = "${L.name}";

// ---- geometry & counts (compile-time; the controller is bound to these) ----
constexpr int   N_DISP      = ${L.dispensers.length};
constexpr int   N_STAGES    = ${L.nStages};   // assembly steps a tray completes in order
constexpr int   MAX_SERVOS  = ${L.maxServos};
constexpr bool  HAS_TUNNEL  = ${L.hasTunnel};
constexpr bool  HAS_SMUSHER = ${L.hasSmusher};
constexpr float BELT_LEN_MM = ${L.beltLen.toFixed(1)}f;
constexpr float NOMINAL_SPEED = ${L.speed.toFixed(1)}f;

enum class Ingredient : uint8_t { ${ing} };

// One dispenser's fixed spec. \`after_tunnel\` = fires downstream of the tunnel
// (e.g. the graham cap); \`is_cap\` = role hint; \`stage\` = its step in belt order;
// \`dispense_ms\` = how long its actuator must run (a MACHINE property, not a
// controller choice — HAL.md OQ-2).
struct DispSpec {
    const char* id;
    float       pos_mm;
    Ingredient  ingredient;
    uint8_t     servos;
    bool        has_confirm;
    bool        after_tunnel;
    bool        is_cap;
    int         stage;
    uint32_t    dispense_ms;
};

constexpr DispSpec DISP[N_DISP] = {
${disp}
};

${tunnel}${smusher}} // namespace layout
} // namespace smores
`;
}

// ---- emit the hardware binding (HAL.md §H-6) ----------------------------------
// Data only. The P1AM implementation that consumes it (P1amMachine.h) is
// hand-written and editable, so a student can read/adjust the layer where slots and
// channels actually appear.
export function emitBindingHeader(L, B) {
  const ref = r => r ? `{ ${r.slot}, ${r.channel} }` : `{ 0, 0 }`;
  const slots = B.base.map(s =>
    `    { ${s.slot}, "${s.part}", ${s.config ? s.config.length : 0} },`).join("\n");
  const cfgs = B.base.filter(s => s.config).map(s =>
    `constexpr char CONFIG_SLOT${s.slot}[${s.config.length}] = { ${s.config.map(b => "(char)0x" + b.toString(16).padStart(2, "0")).join(", ")} };`).join("\n");
  const disp = B.dispIo.map((b, i) => {
    const acts = Array.from({ length: L.maxServos }, (_, s) => ref(b && b.act[s]));
    return `    { ${ref(b && b.sense)}, ${ref(b && b.gate)}, ${ref(b && b.confirm)}, { ${acts.join(", ")} }, ${!!(b && b.confirm)} },  // ${L.dispensers[i].id}`;
  }).join("\n");
  let tun = "";
  if (L.hasTunnel && B.tunnelIo) {
    const t = B.tunnelIo;
    tun = `constexpr ChannelRef TUNNEL_ENTRY_IO = ${ref(t.entry)};\n` +
          `constexpr ChannelRef TUNNEL_EXIT_IO  = ${ref(t.exit)};\n` +
          `constexpr ChannelRef TUNNEL_TEMP_IO  = ${ref(t.temp)};\n` +
          `constexpr ChannelRef TUNNEL_GATE_IO  = ${ref(t.gate)};\n` +
          `constexpr ChannelRef HEATER_IO       = ${ref(t.heater)};\n`;
  }
  const withCfg = B.base.filter(s => s.config);
  const cfgTable = withCfg.length
    ? `constexpr int N_CONFIGS = ${withCfg.length};\nstruct ConfigEntry { uint8_t slot; const char* data; uint8_t len; };\nconstexpr ConfigEntry CONFIGS[N_CONFIGS] = {\n` +
      withCfg.map(s => `    { ${s.slot}, CONFIG_SLOT${s.slot}, ${s.config.length} },`).join("\n") + `\n};\n`
    : `constexpr int N_CONFIGS = 0;\n`;
  return `${banner(L, "Binding.h")}#pragma once
#include <cstdint>
#include "Layout.h"

// This layout carries a full hardware binding, so a P1amMachine can be built.
#define SMORES_HAS_BINDING 1
#define SMORES_N_CONFIGS   ${withCfg.length}

namespace smores {
namespace binding {

// One physical I/O point. Mirrors the real P1AM library's \`channelLabel\`, which
// exists so code can name signals instead of passing raw numbers:
//   channelLabel highLevelSensor_1 = {1, 2};  // slot 1, channel 2
// [ref: docs/references/facts-docs/api_reference.md:159-168 -> P1AM.h:35-37]
// Slots are 1-based (1..15) and channels are 1-based
// [ref: docs/references/facts-docs/api_reference.md:172-180].
struct ChannelRef { uint8_t slot; uint8_t channel; };

constexpr bool BOUND    = true;      // this layout carries a full hardware binding
constexpr int  N_SLOTS  = ${B.base.length};

// The physical module inventory. \`config_len\` > 0 means the module must be
// configured via P1.configureModule() before its data is valid.
struct SlotSpec { uint8_t slot; const char* part; uint8_t config_len; };
constexpr SlotSpec SLOTS[N_SLOTS] = {
${slots}
};

${cfgs ? cfgs + "\n" : ""}
// Modules needing P1.configureModule() before their data is valid. The payload is
// an opaque byte array in the real API (\`char cfgData[]\`) and its per-field meaning
// is NOT published in our offline references, so we carry + length-check bytes only
// [ref: docs/references/facts-docs/api_reference.md:97-100].
${cfgTable}
// Per-dispenser wiring, parallel to layout::DISP.
struct DispIo {
    ChannelRef sense;
    ChannelRef gate;
    ChannelRef confirm;                       // {0,0} when not fitted
    ChannelRef act[layout::MAX_SERVOS];
    bool       has_confirm;
};
constexpr DispIo DISP_IO[layout::N_DISP] = {
${disp}
};

${tun}} // namespace binding
} // namespace smores
`;
}

// Emitted instead of Binding.h when a layout is sim-only, so including it is still
// valid but a hardware build fails loudly and specifically (H-6.4).
export function emitUnboundBindingHeader(L) {
  return `${banner(L, "Binding.h")}#pragma once
#define SMORES_HAS_BINDING 0
#define SMORES_N_CONFIGS   0
namespace smores { namespace binding {
// Layout "${L.name}" declares no hardware binding (no base[]/io{}), so it is
// SIM-ONLY. Add a base[] inventory and per-signal io{} to build for real hardware.
constexpr bool BOUND = false;
}}
`;
}

// ---- JS-side metadata (module descriptors + struct offsets) for the visualizer ----
export function meta(L, B) {
  const o = computeOffsets(L);
  return {
    name: L.name, title: L.title, description: L.description,
    belt: { length_mm: L.beltLen, nominal_speed_mm_s: L.speed },
    nStages: L.nStages, tunnelStage: L.tunnel ? L.tunnel.stage : -1,
    dispensers: L.dispensers,
    tunnel: L.tunnel, smusher: L.smusher, maxServos: L.maxServos,
    modules: L.modules.map(m => ({ id: m.id, type: m.type, pos_mm: m.pos_mm, ingredient: m.ingredient || null, servos: m.servos || null, exit_mm: m.exit_mm || null, role: m.role || null })),
    offsets: o,
    schema: L.schema,
    // hardware binding, for the UI's wiring view (null when sim-only)
    binding: (B && B.bound) ? { slots: B.base.map(s => ({ slot: s.slot, part: s.part, config_len: s.config ? s.config.length : 0 })), dispensers: B.dispIo, tunnel: B.tunnelIo } : null,
  };
}

// ---- CLI: node codegen.mjs --layout <p> --include-dir <d> --meta <p> ----
const isMain = typeof process !== "undefined" && process.argv && import.meta.url === `file://${process.argv[1]}`;
if (isMain) {
  const { readFileSync, writeFileSync, mkdirSync } = await import("node:fs");
  const path = await import("node:path");
  const argv = process.argv.slice(2);
  const get = (flag) => { const i = argv.indexOf(flag); return i >= 0 ? argv[i + 1] : null; };
  const layoutPath = get("--layout");
  if (!layoutPath) { console.error("usage: codegen.mjs --layout <file> [--include-dir <dir>] [--meta <file>]"); process.exit(2); }
  const includeDir = get("--include-dir");
  const metaPath = get("--meta");
  let L;
  try {
    L = parseLayout(JSON.parse(readFileSync(layoutPath, "utf8")));
  } catch (e) { console.error(`layout error in ${layoutPath}: ${e.message}`); process.exit(1); }

  // Validate the hardware binding against the real module catalog. A mis-wired
  // layout must fail the BUILD, not fail at runtime (HAL.md §H-7.1).
  const dbPath = get("--module-db") || path.join(path.dirname(new URL(import.meta.url).pathname), "..", "..", "shared", "module_db.h");
  let B = { bound: false, base: [], errors: [] };
  try {
    const catalog = parseModuleDb(readFileSync(dbPath, "utf8"));
    B = validateBinding(L, catalog);
  } catch (e) { console.error(`module catalog error (${dbPath}): ${e.message}`); process.exit(1); }
  if (B.errors.length) {
    console.error(`layout "${L.name}": ${B.errors.length} wiring error(s):`);
    for (const err of B.errors) console.error(`  - ${err}`);
    process.exit(1);
  }

  if (includeDir) {
    mkdirSync(includeDir, { recursive: true });
    writeFileSync(path.join(includeDir, "Layout.h"), emitLayoutHeader(L));
    writeFileSync(path.join(includeDir, "Contract.h"), emitContractHeader(L));
    writeFileSync(path.join(includeDir, "State.h"), emitStateHeader(L));
    writeFileSync(path.join(includeDir, "Binding.h"), B.bound ? emitBindingHeader(L, B) : emitUnboundBindingHeader(L));
  }
  if (metaPath) {
    mkdirSync(path.dirname(metaPath), { recursive: true });
    writeFileSync(metaPath, JSON.stringify(meta(L, B), null, 2) + "\n");
  }
  const off = computeOffsets(L);
  console.error(`codegen: ${L.name} — ${L.dispensers.length} dispenser(s)${L.hasTunnel ? " + tunnel" : ""}${L.hasSmusher ? " + smusher" : ""}; Inputs=${off.inputs.size}B Outputs=${off.outputs.size}B; wiring=${B.bound ? B.base.length + " slot(s)" : "sim-only"}`);
}
