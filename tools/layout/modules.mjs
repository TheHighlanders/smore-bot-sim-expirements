// modules.mjs — read the P1000 module catalog out of shared/module_db.h.
//
// module_db.h stays the SINGLE SOURCE OF TRUTH for module properties (its IDs and
// byte counts are quoted from the real library's Module_List.h and carry the
// citations). Parsing it here — rather than re-typing a JS copy — is what lets the
// layout validator check a wiring binding against real module capabilities without
// a second, driftable table.
//
// parseModuleDb() is pure so the same code runs in Node (codegen) and in the
// browser (the in-app layout editor).

// Matches a row of the P1_MODULE_DB table:
//   { 0x1404008FUL,  0, 1,  0,  0,  8, "P1-08TRS"  }, /* :66 ... */
const ROW = /\{\s*(0x[0-9A-Fa-f]+)UL\s*,\s*(\d+)\s*,\s*(\d+)\s*,\s*(\d+)\s*,\s*(\d+)\s*,\s*(\d+)\s*,\s*"([^"]+)"\s*\}/g;

export function parseModuleDb(text) {
  const catalog = {};
  let m;
  ROW.lastIndex = 0;
  while ((m = ROW.exec(text)) !== null) {
    const [, id, di, dout, ai, cfg, channels, name] = m;
    catalog[name] = {
      name,
      id,
      diBytes: +di,          // discrete input bytes  (>0 => can host inputs)
      doBytes: +dout,        // discrete output bytes (>0 => can host outputs)
      aiBytes: +ai,          // analog input bytes    (>0 => can host analog/temp)
      configBytes: +cfg,     // >0 => module must be configured before use
      channels: +channels,   // I/O channels, for range-checking a binding
    };
  }
  if (!Object.keys(catalog).length) throw new Error("no modules parsed from module_db.h — has the table format changed?");
  return catalog;
}

// Convenience predicates used by the validator, so the rules read declaratively.
export const canOutput = p => p.doBytes > 0;
export const canInput  = p => p.diBytes > 0;
export const canAnalog = p => p.aiBytes > 0;
