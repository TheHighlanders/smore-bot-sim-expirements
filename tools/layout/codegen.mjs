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

// ---- C type sizes/alignments for the wasm32 target (LP32; matches clang) ----
const T = {
  u32:  { size: 4, align: 4, c: "uint32_t" },
  f32:  { size: 4, align: 4, c: "float" },
  u8:   { size: 1, align: 1, c: "uint8_t" },
  bool: { size: 1, align: 1, c: "bool" },
};

const INGREDIENT_ENUM = { graham: "Graham", chocolate: "Chocolate", marshmallow: "Marshmallow" };

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

  const dispensers = modules.filter(m => m.type === "dispenser").map(m => ({
    id: m.id, pos_mm: m.pos_mm, ingredient: m.ingredient,
    servos: m.servos == null ? 1 : +m.servos,
    confirm: m.confirm !== false,
    role: m.role || "base",
    after_tunnel: tunnelIndex >= 0 && m._index > tunnelIndex,
    stage: stageOf[m.id],
  }));
  const tunnel = tunnelIndex >= 0 ? { id: modules[tunnelIndex].id, pos_mm: modules[tunnelIndex].pos_mm, exit_mm: +modules[tunnelIndex].exit_mm, stage: stageOf[modules[tunnelIndex].id] } : null;
  const smusherM = modules.find(m => m.type === "smusher");
  const smusher = smusherM ? { id: smusherM.id, pos_mm: smusherM.pos_mm, dwell_ms: smusherM.dwell_ms == null ? 400 : +smusherM.dwell_ms, confirm: smusherM.confirm !== false, stage: stageOf[smusherM.id] } : null;

  const maxServos = dispensers.reduce((a, d) => Math.max(a, d.servos), 1);
  const ingredients = [...new Set(dispensers.map(d => d.ingredient))]; // dedup, first-seen order

  return {
    name, title: raw.title || name, description: raw.description || "",
    beltLen, speed, modules, dispensers, tunnel, smusher, maxServos, ingredients, nStages,
    hasTunnel: !!tunnel, hasSmusher: !!smusher,
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

export function computeOffsets(L) {
  return { inputs: layoutStruct(inputFields(L)), outputs: layoutStruct(outputFields(L)) };
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
    `    { "${d.id}", ${d.pos_mm.toFixed(1)}f, Ingredient::${INGREDIENT_ENUM[d.ingredient]}, ${d.servos}, ${d.confirm}, ${d.after_tunnel}, ${d.role === "cap"}, ${d.stage} },`
  ).join("\n");
  let tunnel = "";
  if (L.hasTunnel) tunnel =
    `constexpr float TUNNEL_ENTRY_MM = ${L.tunnel.pos_mm.toFixed(1)}f;\n` +
    `constexpr float TUNNEL_EXIT_MM  = ${L.tunnel.exit_mm.toFixed(1)}f;\n` +
    `constexpr int   TUNNEL_STAGE    = ${L.tunnel.stage};\n`;
  let smusher = "";
  if (L.hasSmusher) smusher =
    `constexpr float    SMUSHER_POS_MM   = ${L.smusher.pos_mm.toFixed(1)}f;\n` +
    `constexpr uint32_t SMUSHER_DWELL_MS = ${L.smusher.dwell_ms};\n` +
    `constexpr int      SMUSHER_STAGE    = ${L.smusher.stage};\n`;
  return `${banner(L, "Layout.h")}#pragma once
#include <cstdint>

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
// (e.g. the graham cap); \`is_cap\` = role hint; \`stage\` = its step in belt order.
struct DispSpec {
    const char* id;
    float       pos_mm;
    Ingredient  ingredient;
    uint8_t     servos;
    bool        has_confirm;
    bool        after_tunnel;
    bool        is_cap;
    int         stage;
};

constexpr DispSpec DISP[N_DISP] = {
${disp}
};

${tunnel}${smusher}} // namespace layout
} // namespace smores
`;
}

// ---- JS-side metadata (module descriptors + struct offsets) for the visualizer ----
export function meta(L) {
  const o = computeOffsets(L);
  return {
    name: L.name, title: L.title, description: L.description,
    belt: { length_mm: L.beltLen, nominal_speed_mm_s: L.speed },
    nStages: L.nStages, tunnelStage: L.tunnel ? L.tunnel.stage : -1,
    dispensers: L.dispensers,
    tunnel: L.tunnel, smusher: L.smusher, maxServos: L.maxServos,
    modules: L.modules.map(m => ({ id: m.id, type: m.type, pos_mm: m.pos_mm, ingredient: m.ingredient || null, servos: m.servos || null, exit_mm: m.exit_mm || null, role: m.role || null })),
    offsets: o,
  };
}

// ---- CLI: node codegen.mjs --layout <p> --include-dir <d> --meta <p> ----
const isMain = import.meta.url === `file://${process.argv[1]}`;
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
  if (includeDir) {
    mkdirSync(includeDir, { recursive: true });
    writeFileSync(path.join(includeDir, "Layout.h"), emitLayoutHeader(L));
    writeFileSync(path.join(includeDir, "Contract.h"), emitContractHeader(L));
  }
  if (metaPath) {
    mkdirSync(path.dirname(metaPath), { recursive: true });
    writeFileSync(metaPath, JSON.stringify(meta(L), null, 2) + "\n");
  }
  const off = computeOffsets(L);
  console.error(`codegen: ${L.name} — ${L.dispensers.length} dispenser(s)${L.hasTunnel ? " + tunnel" : ""}${L.hasSmusher ? " + smusher" : ""}; Inputs=${off.inputs.size}B Outputs=${off.outputs.size}B`);
}
