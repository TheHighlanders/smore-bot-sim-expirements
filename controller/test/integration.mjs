// Complete integration test: instantiate the REAL compiled controller.wasm and
// drive it across the WASM<->JS boundary against a simulated world — the same
// path the browser visualizer uses. Asserts the full s'mores cycle for both
// controller variants, incl. the flaky-graham divergence.
//
// Run:  make -C controller integration   (or: node controller/test/integration.mjs)
import fs from "node:fs";
import { fileURLToPath } from "node:url";
import path from "node:path";

const here = path.dirname(fileURLToPath(import.meta.url));
const wasmBytes = fs.readFileSync(path.join(here, "..", "build", "controller.wasm"));

// The only imports controller.wasm needs are three WASI stdio fns that are never
// actually reached (vsnprintf writes to a buffer). Stub them.
const noop = () => 0;
const imports = { wasi_snapshot_preview1: { fd_close: noop, fd_seek: noop, fd_write: noop } };

// ---- Contract struct layout (wasm32, standard alignment) ----
// Inputs (size 20): now_ms u32@0 | sense[3] b@4..6 | tunnel_entry@7 | tunnel_exit@8
//                   | tunnel_temp_c f32@12 | dispense_confirm[3] u8@16..18 | run@19
// Outputs(size 12): belt_speed f32@0 | gate_open[3]@4..6 | dispense[3]@7..9
//                   | tunnel_gate_open@10 | heater@11
const OPEN = 0, CLOSED = 1;

const wasm = await WebAssembly.instantiate(new WebAssembly.Module(wasmBytes), imports);
const ex = wasm.exports;
ex._initialize();                             // reactor: run global constructors
const inPtr = ex.inputs_ptr(), outPtr = ex.outputs_ptr();
const mem = () => new DataView(ex.memory.buffer);   // fresh view (memory may grow)

function writeInputs(w) {
  const dv = mem();
  dv.setUint32(inPtr + 0, w.now, true);
  for (let k = 0; k < 3; k++) dv.setUint8(inPtr + 4 + k, w.sense[k] ? 1 : 0);
  dv.setUint8(inPtr + 7, w.tin ? 1 : 0);
  dv.setUint8(inPtr + 8, w.tout ? 1 : 0);
  dv.setFloat32(inPtr + 12, w.temp, true);
  for (let k = 0; k < 3; k++) dv.setUint8(inPtr + 16 + k, w.confirm[k]);
  dv.setUint8(inPtr + 19, 1);                 // run = true
}
function readOutputs() {
  const dv = mem();
  return {
    belt: dv.getFloat32(outPtr + 0, true),
    gate: [0,1,2].map(k => !!dv.getUint8(outPtr + 4 + k)),
    dispense: [0,1,2].map(k => !!dv.getUint8(outPtr + 7 + k)),
    tunnel_gate_open: !!dv.getUint8(outPtr + 10),
    heater: !!dv.getUint8(outPtr + 11),
  };
}
const trackField = (i, f) => ex.track_field(i, f);   // 2=stage, 3=status(Done=3)

// ---- JS world (same physics as controller/test test_controller.cpp) ----
function makeWorld(grahamFlaky) {
  return {
    now: 0, pos: -32, counts: [0,0,0], energy: 0, grahamFlaky,
    att: [0,0,0], dispOn: [0,0,0], lastAtt: [0,0,0],
    sense: [false,false,false], tin: false, tout: false, temp: 20, confirm: [0,0,0],
    out: null,
    SP: [300,600,900], TIN: 1050, TEX: 1185, SLIP: 0.965, DROP: 450,
    senseUpdate() {
      for (let k=0;k<3;k++) this.sense[k] = Math.abs(this.pos-this.SP[k]) < 22;
      this.tin = Math.abs(this.pos-this.TIN) < 22;
      this.tout = Math.abs(this.pos-this.TEX) < 22;
      this.temp = this.out && this.out.heater ? 205 : 20;
    },
    step(dt) {
      const o = this.out;
      let blocked = false;
      for (let k=0;k<3;k++) if (!o.gate[k] && this.pos < this.SP[k] && this.pos > this.SP[k]-70) blocked = true;
      if (!o.tunnel_gate_open && this.pos > this.TIN-30 && this.pos < this.TEX) blocked = true;
      if (!blocked) this.pos += o.belt * this.SLIP * dt;
      for (let k=0;k<3;k++) {
        if (o.dispense[k]) {
          this.dispOn[k] += dt*1000;
          if (this.dispOn[k]-this.lastAtt[k] >= this.DROP) {
            this.lastAtt[k] = this.dispOn[k];
            if (Math.abs(this.pos-this.SP[k]) < 70) {
              this.att[k]++; let add = 1;
              if (k===0 && this.grahamFlaky) add = (this.att[k]===1) ? 0 : 1;
              this.counts[k] += add; this.confirm[k] = add;
            } else this.confirm[k] = 0;
          }
        } else { this.dispOn[k] = 0; this.lastAtt[k] = 0; }
      }
      if (o.heater && this.pos > this.TIN-40 && this.pos < this.TEX+40 && this.counts[2] > 0)
        this.energy += (this.temp-20)*dt;
    },
  };
}

function run(mode, flaky) {
  ex.init(mode);
  const w = makeWorld(flaky);
  w.out = readOutputs();
  const dt = 0.02;
  for (let i=0;i<1500;i++) {
    w.now += 20;
    w.senseUpdate();
    writeInputs(w);
    ex.tick();
    w.out = readOutputs();
    w.step(dt);
  }
  const n = ex.track_count();
  const status = n > 0 ? trackField(0, 3) : -1;     // 3 = Done
  const believedG = n > 0 ? trackField(0, 4) : -1;
  return { counts: w.counts, status, believedG, tracks: n };
}

// ---- assertions ----
let fails = 0;
function check(name, cond, detail) {
  console.log(`${cond ? "  ok  " : "FAIL  "}${name}${detail ? "  — "+detail : ""}`);
  if (!cond) fails++;
}
console.log("== controller.wasm integration ==");

let r = run(OPEN, false);
check("open-loop, no fault: all ingredients placed", JSON.stringify(r.counts)==="[1,1,1]", `counts=${r.counts}`);
check("open-loop, no fault: tray completed (status Done)", r.status===3, `status=${r.status}`);

r = run(OPEN, true);
check("open-loop + flaky graham: ships graham-less", r.counts[0]===0, `counts=${r.counts}`);
check("open-loop + flaky graham: controller still believes it placed graham", r.believedG===1, `believed=${r.believedG}`);
check("open-loop + flaky graham: still completes (oblivious)", r.status===3, `status=${r.status}`);

r = run(CLOSED, true);
check("closed-loop + flaky graham: recovers (graham present)", r.counts[0]===1, `counts=${r.counts}`);
check("closed-loop + flaky graham: completes", r.status===3, `status=${r.status}`);

console.log(fails ? `\nintegration: FAIL (${fails})` : "\nintegration: PASS");
process.exit(fails ? 1 : 0);
