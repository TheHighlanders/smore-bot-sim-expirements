> Offline reference snapshot for citation. Source: https://facts-engineering.github.io/config.html
> Retrieved: 2026-07-22. See the sibling .html for the full original page.

# P1AM Module Configuration Tool — Key Technical Facts

## What the page is

The page hosts the **P1AM Module Configuration Tool** — a web-based generator for a
module's configuration array. Page text:

- "Select any module on the list below to access the generator for its configuration array."
- "All default properties are displayed in bold."

The generated array is passed to the `P1.configureModule(cfgData[], slot)` API
(see `api_reference.md`) so a module powers up in the desired mode.

## Which modules need configuration

The module list on this page is **rendered dynamically by JavaScript**
(`modules.js` / `config.js` / `main.js`); the static HTML captured in the sibling
`config.html` contains only empty container elements (`#moduleList`,
`#configTool`), so the full configurable-module list could not be extracted by a
static fetch. See `_FETCH_NOTES.md`.

However, the guiding principle is confirmed by the individual discrete-module
reference pages captured in this same directory: **discrete I/O modules do NOT
require configuration.** Each of the following module pages explicitly states the
module "does not provide any status data and does not require configuration":

- P1-08TRS (relay output) — see `P1-08TRS.md`
- P1-16TR (relay output) — see `P1-16TR.md`
- P1-08TD1 (sinking DC output) — see `P1-08TD1.md`
- P1-08TD2 (sourcing DC output) — see `P1-08TD2.md`

Configuration is instead used by non-discrete modules (e.g. analog input/output
and temperature/RTD/thermocouple modules), where properties such as range,
input type, and filtering must be selected before use. Those specific modules
are the ones populated in the tool's dynamic list.
