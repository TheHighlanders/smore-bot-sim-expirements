# Fetch notes

Retrieved 2026-07-22. All 8 pages fetched successfully with HTTP 200 — no 404s.

## config.html — dynamically rendered (limited markdown extraction)

- URL: https://facts-engineering.github.io/config.html
- `curl` returned a valid 200 page but it is small (~1.6 KB). This is expected:
  the page is a **JavaScript-driven configuration tool**. Its body ships only
  empty container elements (`#moduleList`, `#configTool`), and the actual module
  list + configuration generator are rendered client-side by `modules.js`,
  `main.js`, and `config.js` (none of which run under `curl`/WebFetch).
- Consequence: WebFetch could not extract a concrete "which modules need
  configuration" list from this page — that content only exists after JS runs.
- Mitigation: `config.md` records what the static page states, plus the
  confirmed principle (discrete modules do not require configuration) cross-cited
  to the four discrete-module reference files in this directory, each of which
  states verbatim that the module "does not provide any status data and does not
  require configuration."

## Module pages (P1-08TRS, P1-16TR, P1-08TD1, P1-08TD2, P1AM-100, P1AM-GPIO)

- Each is ~5–14 KB. Verified NOT 404 pages (real `<title>… | P1AM Documentation</title>`).
- Verified the key technical facts are embedded **inline** in the saved HTML body
  (e.g. P1-08TRS.html contains "6–24", "does not provide", "does not require"),
  so these `.html` snapshots are faithful and self-contained.
- These pages also load `modules.js` for the sidebar nav, but the spec content
  itself is present in the static HTML.

## api_reference.html

- ~60 KB, full content present inline. No issues.
