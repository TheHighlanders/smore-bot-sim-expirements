// Layout-driven integration test: instantiate the REAL compiled controller.wasm
// and drive it across the WASM<->JS boundary against a simulated world built
// from the layout's generated meta (offsets + geometry) — the same path the
// browser visualizer uses. Works for ANY layout; the Makefile runs it per LAYOUT.
//
// Run:  make -C controller integration            (classic3)
//       make -C controller integration LAYOUT=sandwich
import fs from "node:fs";
import { fileURLToPath } from "node:url";
import path from "node:path";

const here = path.dirname(fileURLToPath(import.meta.url));
const root = path.join(here, "..");
const layoutName = process.env.LAYOUT || "classic3";
const metaPath = process.env.LAYOUT_META || path.join(root, "generated", `${layoutName}.meta.json`);
const wasmPath = process.env.WASM || path.join(root, "build", "controller.wasm");

const M = JSON.parse(fs.readFileSync(metaPath, "utf8"));
const wasmBytes = fs.readFileSync(wasmPath);
const N = M.dispensers.length;

// field-name -> byte offset, from the generated struct layout
const inOff = Object.fromEntries(M.offsets.inputs.fields.map(f => [f.name, f.offset]));
const outOff = Object.fromEntries(M.offsets.outputs.fields.map(f => [f.name, f.offset]));

const noop = () => 0;
let logCount = 0;
const watches = {};                        // last value per WATCH(name, value)
const cstr = (p) => { const u = new Uint8Array(ex.memory.buffer); let q = p; while (u[q]) q++; return new TextDecoder().decode(u.subarray(p, q)); };
const imports = {
  wasi_snapshot_preview1: { fd_close: noop, fd_seek: noop, fd_write: noop },
  env: {
    host_log: () => { logCount++; },
    host_watch: (np, v) => { watches[cstr(np)] = v; },
  },
};
const OPEN = 0, CLOSED = 1;

const wasm = await WebAssembly.instantiate(new WebAssembly.Module(wasmBytes), imports);
const ex = wasm.exports;
ex._initialize();
const inPtr = ex.inputs_ptr(), outPtr = ex.outputs_ptr();
const dv = () => new DataView(ex.memory.buffer);

function writeInputs(w) {
  const d = dv();
  d.setUint32(inPtr + inOff.now_ms, w.now, true);
  for (let k = 0; k < N; k++) d.setUint8(inPtr + inOff.sense + k, w.sense[k] ? 1 : 0);
  for (let k = 0; k < N; k++) d.setUint8(inPtr + inOff.dispense_confirm + k, w.confirm[k]);
  if ("tunnel_entry" in inOff) {
    d.setUint8(inPtr + inOff.tunnel_entry, w.tin ? 1 : 0);
    d.setUint8(inPtr + inOff.tunnel_exit, w.tout ? 1 : 0);
    d.setFloat32(inPtr + inOff.tunnel_temp_c, w.temp, true);
  }
  d.setUint8(inPtr + inOff.run, 1);
}
function readOutputs() {
  const d = dv();
  return {
    belt: d.getFloat32(outPtr + outOff.belt_speed, true),
    gate: Array.from({ length: N }, (_, k) => !!d.getUint8(outPtr + outOff.gate_open + k)),
    dispense: Array.from({ length: N }, (_, k) => !!d.getUint8(outPtr + outOff.dispense + k)),
    tunnel_gate_open: "tunnel_gate_open" in outOff ? !!d.getUint8(outPtr + outOff.tunnel_gate_open) : true,
    heater: "heater" in outOff ? !!d.getUint8(outPtr + outOff.heater) : false,
  };
}
// Decode the controller's Track array generically, from the GENERATED descriptor
// (no hand-written accessors — HAL.md §H-8.1). Re-read the base pointer each call:
// the backing store can move.
const TOFF = Object.fromEntries(M.offsets.track.fields.map(f => [f.name, f.offset]));
function readTracks() {
  const n = ex.track_count(); if (!n) return [];
  const base = ex.tracks_ptr(), stride = ex.track_stride(), d = dv(), out = [];
  for (let i = 0; i < n; i++) {
    const p = base + i * stride;
    out.push({
      id: d.getInt32(p + TOFF.id, true),
      est_pos_mm: d.getFloat32(p + TOFF.est_pos_mm, true),
      stage: d.getInt32(p + TOFF.stage, true),
      status: d.getInt32(p + TOFF.status, true),
      retries: d.getInt32(p + TOFF.retries, true),
      placed: Array.from({ length: N }, (_, k) => d.getInt32(p + TOFF.placed + 4 * k, true)),
    });
  }
  return out;
}

// ---- generic JS world derived from the layout meta ----
function makeWorld(flakyDisp /* dispenser index that misfires its 1st attempt, or -1 */) {
  const SP = M.dispensers.map(d => d.pos_mm);
  const TIN = M.tunnel ? M.tunnel.pos_mm : Infinity;
  const TEX = M.tunnel ? M.tunnel.exit_mm : Infinity;
  const marshIdx = M.dispensers.findIndex(d => d.ingredient === "marshmallow");
  return {
    now: 0, pos: -32, counts: Array(N).fill(0), energy: 0, flakyDisp,
    att: Array(N).fill(0), dispOn: Array(N).fill(0), lastAtt: Array(N).fill(0),
    sense: Array(N).fill(false), tin: false, tout: false, temp: 20, confirm: Array(N).fill(0),
    out: null, SP, TIN, TEX, SLIP: 0.965, DROP: 450,
    senseUpdate() {
      for (let k = 0; k < N; k++) this.sense[k] = Math.abs(this.pos - this.SP[k]) < 22;
      this.tin = Math.abs(this.pos - this.TIN) < 22;
      this.tout = Math.abs(this.pos - this.TEX) < 22;
      this.temp = this.out && this.out.heater ? 205 : 20;
    },
    step(dt) {
      const o = this.out;
      let blocked = false;
      for (let k = 0; k < N; k++) if (!o.gate[k] && this.pos < this.SP[k] && this.pos > this.SP[k] - 70) blocked = true;
      if (!o.tunnel_gate_open && this.pos > this.TIN - 30 && this.pos < this.TEX) blocked = true;
      if (!blocked) this.pos += o.belt * this.SLIP * dt;
      for (let k = 0; k < N; k++) {
        if (o.dispense[k]) {
          this.dispOn[k] += dt * 1000;
          if (this.dispOn[k] - this.lastAtt[k] >= this.DROP) {
            this.lastAtt[k] = this.dispOn[k];
            if (Math.abs(this.pos - this.SP[k]) < 70) {
              this.att[k]++; let add = 1;
              if (k === flakyDisp) add = (this.att[k] === 1) ? 0 : 1;   // misfire the 1st attempt
              this.counts[k] += add; this.confirm[k] = add;
            } else this.confirm[k] = 0;
          }
        } else { this.dispOn[k] = 0; this.lastAtt[k] = 0; }
      }
      if (o.heater && this.pos > this.TIN - 40 && this.pos < this.TEX + 40 && marshIdx >= 0 && this.counts[marshIdx] > 0)
        this.energy += (this.temp - 20) * dt;
    },
  };
}

function run(mode, flakyDisp) {
  ex.init(mode);
  const w = makeWorld(flakyDisp);
  w.out = readOutputs();
  const dt = 0.02;
  for (let i = 0; i < 2000; i++) {
    w.now += 20;
    w.senseUpdate();
    writeInputs(w);
    ex.tick();
    w.out = readOutputs();
    w.step(dt);
  }
  const tracks = readTracks();
  const t0 = tracks[0];
  return {
    counts: w.counts,
    status: t0 ? t0.status : -1,                 // 3 = Done
    believed: t0 ? t0.placed : Array(N).fill(-1),
    tracks: tracks.length,
  };
}

// ---- assertions ----
let fails = 0;
function check(name, cond, detail) {
  console.log(`${cond ? "  ok  " : "FAIL  "}${name}${detail ? "  — " + detail : ""}`);
  if (!cond) fails++;
}
console.log(`== controller.wasm integration (layout "${M.name}", ${N} dispensers) ==`);

let r = run(OPEN, -1);
check("open-loop, no fault: every ingredient placed once", r.counts.every(c => c === 1), `counts=${r.counts}`);
check("open-loop, no fault: tray completed (Done)", r.status === 3, `status=${r.status}`);

r = run(OPEN, 0);
check("open-loop + flaky graham: ships graham-less", r.counts[0] === 0, `counts=${r.counts}`);
check("open-loop + flaky graham: believes it placed EVERYTHING (belief != reality)", r.believed.every(b => b === 1), `believed=${r.believed}`);
check("open-loop + flaky graham: still completes (oblivious)", r.status === 3, `status=${r.status}`);

r = run(CLOSED, 0);
check("closed-loop + flaky graham: recovers (graham present)", r.counts[0] === 1, `counts=${r.counts}`);
check("closed-loop + flaky graham: completes", r.status === 3, `status=${r.status}`);

// WATCH telemetry (HAL.md §H-8.2): drift_mm exists only as a local at the sensor
// snap, so it proves we can observe a value the generated state block can't reach.
check("WATCH published dt_ms + tracked", "dt_ms" in watches && "tracked" in watches, `keys=${Object.keys(watches)}`);
check("WATCH published drift_mm (dead-reckoning error at a sensor snap)",
      "drift_mm" in watches && Number.isFinite(watches.drift_mm), `drift_mm=${watches.drift_mm}`);

// Layout-specific: a post-tunnel cap must also be placed (sandwich et al.)
const cap = M.dispensers.findIndex(d => d.role === "cap");
if (cap >= 0) {
  r = run(OPEN, -1);
  check(`open-loop: post-tunnel cap "${M.dispensers[cap].id}" placed`, r.counts[cap] === 1, `counts=${r.counts}`);
}

console.log(fails ? `\nintegration (${M.name}): FAIL (${fails})` : `\nintegration (${M.name}): PASS`);
process.exit(fails ? 1 : 0);
