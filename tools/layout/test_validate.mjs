// Binding-validation tests (HAL.md CI-H4).
//
// The value of a hardware binding is that a MIS-WIRED machine fails the build
// instead of failing on a bench. That guarantee is only real if the validator
// actually rejects each mistake, so each case below deliberately breaks one rule
// and asserts the specific complaint.
//
// Run: node tools/layout/test_validate.mjs
import fs from "node:fs";
import path from "node:path";
import { fileURLToPath } from "node:url";
import { parseLayout, validateBinding } from "./codegen.mjs";
import { parseModuleDb } from "./modules.mjs";

const here = path.dirname(fileURLToPath(import.meta.url));
const root = path.join(here, "..", "..");
const catalog = parseModuleDb(fs.readFileSync(path.join(root, "shared", "module_db.h"), "utf8"));
const GOOD = JSON.parse(fs.readFileSync(path.join(root, "controller", "layouts", "sandwich_wired.json"), "utf8"));

const clone = () => JSON.parse(JSON.stringify(GOOD));
const modOf = (L, id) => L.modules.find(m => m.id === id);

let fails = 0;
function check(name, cond, detail) {
  console.log(`${cond ? "  ok  " : "FAIL  "}${name}${detail ? "  — " + detail : ""}`);
  if (!cond) fails++;
}
// Run the validator and return its error list (parse errors surface as one entry).
function errorsFor(raw) {
  try { return validateBinding(parseLayout(raw), catalog).errors; }
  catch (e) { return [e.message]; }
}
function rejects(name, raw, needle) {
  const errs = errorsFor(raw);
  const hit = errs.some(e => e.toLowerCase().includes(needle.toLowerCase()));
  check(name, hit, hit ? `"${needle}"` : `got: ${errs.length ? errs.join(" | ") : "(no errors — NOT rejected!)"}`);
}

console.log("== layout binding validation ==");

// The reference layout must be clean, or every negative case below is meaningless.
check("the shipped sandwich_wired layout validates cleanly", errorsFor(GOOD).length === 0, errorsFor(GOOD).join(" | "));

// A layout with no base[]/io{} at all is legitimately sim-only (H-6.4).
{
  const L = clone(); delete L.base; for (const m of L.modules) delete m.io;
  const r = validateBinding(parseLayout(L), catalog);
  check("a layout with no wiring is accepted as sim-only", r.errors.length === 0 && r.bound === false, `bound=${r.bound}`);
}

// VAL-H1 — inventory
rejects("VAL-H1 unknown part", (() => { const L = clone(); L.base[0].part = "P1-NOTAREALPART"; return L; })(), "unknown part");
rejects("VAL-H1 duplicate slot", (() => { const L = clone(); L.base[1].slot = L.base[0].slot; return L; })(), "duplicate slot");
rejects("VAL-H1 slot out of range", (() => { const L = clone(); L.base[0].slot = 99; return L; })(), "out of range");
rejects("VAL-H1 signal bound to a slot not in base[]",
  (() => { const L = clone(); modOf(L, "g1").io.gate.slot = 9; return L; })(), "not in base[]");

// VAL-H2 — channel range (P1-15TD1 has 15 channels)
rejects("VAL-H2 channel above the part's channel count",
  (() => { const L = clone(); modOf(L, "g1").io.gate.channel = 16; return L; })(), "has channels 1..15");
rejects("VAL-H2 channel zero is not a point",
  (() => { const L = clone(); modOf(L, "g1").io.gate.channel = 0; return L; })(), "has channels 1..");

// VAL-H3 — direction. P1-08ND3 is input-only; P1-15TD1 is output-only.
rejects("VAL-H3 output bound to an input-only module",
  (() => { const L = clone(); modOf(L, "g1").io.gate = { slot: 2, channel: 8 }; return L; })(), "has no discrete outputs");
rejects("VAL-H3 input bound to an output-only module",
  (() => { const L = clone(); modOf(L, "g1").io.sense = { slot: 1, channel: 12 }; return L; })(), "has no discrete inputs");

// VAL-H4 — analog. A thermocouple read must land on an analog module.
rejects("VAL-H4 temperature read from a discrete module",
  (() => { const L = clone(); modOf(L, "tun").io.temp = { slot: 2, channel: 1 }; return L; })(), "has no analog inputs");

// VAL-H5 — collision (same slot, same direction, same channel)
rejects("VAL-H5 two outputs on one channel",
  (() => { const L = clone(); modOf(L, "c1").io.gate = { ...modOf(L, "g1").io.gate }; return L; })(), "already used by");
// ...but a combo module's input ch1 and output ch1 are DIFFERENT points.
{
  const L = clone();
  modOf(L, "g2").io.sense = { slot: 3, channel: 1 };     // P1-16CDR input ch1
  modOf(L, "tun").io.heater = { slot: 3, channel: 1 };   // P1-16CDR output ch1
  check("VAL-H5 does NOT flag input ch1 vs output ch1 on a combo module",
        errorsFor(L).length === 0, errorsFor(L).join(" | "));
}

// VAL-H6 — completeness
rejects("VAL-H6 servos=2 but only one actuator bound",
  (() => { const L = clone(); modOf(L, "g1").servos = 2; return L; })(), "binds 1 actuator");
rejects("VAL-H6 confirm=true with no confirm binding",
  (() => { const L = clone(); delete modOf(L, "g1").io.confirm; return L; })(), 'no "confirm" signal is bound');
rejects("VAL-H6 a module with no io{} at all once base[] is declared",
  (() => { const L = clone(); delete modOf(L, "m1").io; return L; })(), "no io{} binding");

// VAL-H7 — module configuration (P1-04THM needs 20 bytes)
rejects("VAL-H7 missing config for a module that needs one",
  (() => { const L = clone(); delete L.base[3].config; return L; })(), "needs a 20-byte config");
rejects("VAL-H7 config of the wrong length",
  (() => { const L = clone(); L.base[3].config = [1, 2, 3]; return L; })(), "must be 20 bytes");
rejects("VAL-H7 config supplied for a module that takes none",
  (() => { const L = clone(); L.base[0].config = [1]; return L; })(), "takes no configuration");
rejects("VAL-H7 config byte out of range",
  (() => { const L = clone(); L.base[3].config[0] = 999; return L; })(), "integers 0..255");

console.log(fails ? `\nvalidate: FAIL (${fails})` : "\nvalidate: PASS");
process.exit(fails ? 1 : 0);
