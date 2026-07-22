# Fetch notes

Retrieved 2026-07-22 for the p1am-wokwi-lab offline reference set.

## curl status
All 16 `curl` downloads succeeded and produced full-size static Docusaurus HTML
(range ~24 KB to ~72 KB). **No tiny/empty files** — every `.html` contains its
page's real content, so each `.md` has a valid sibling to cite.

## Verification / accuracy notes

- **supported-hardware.md** — The MCU table was cross-checked against the raw
  HTML (not just the WebFetch summary). The odd-looking names `ESP32-C61` and
  `ESP32-S31` are genuinely on the page. Confirmed: RP2040 / Pi Pico supported,
  ESP32 supported, and **no SAMD21 / SAMD51** anywhere in the table. Supported
  architectures listed: ARM, AVR, RISC-V, Xtensa.

- **ci-github-actions.md** — The docs page itself only documents three action
  inputs (`token`, `path`, `expect_text`) and then defers to the action's README
  ("For a complete list of options, check out the action's README"). The other
  inputs the task listed (`scenario`, `timeout`, `elf`, `fail_text`,
  `serial_log_file`, `diagram_file`) are NOT on this page and must not be cited
  to it — this caveat is recorded inline in the `.md`.

- **ci-scenarios.md** — Step list cross-checked against the raw HTML. Documented
  steps: `delay`, `expect-pin`, `set-control`, `wait-serial`, `write-serial`,
  `take-screenshot`, and `touch` / `touch-press` / `touch-move` /
  `touch-release`. The page flags the scenario API as alpha.

- The `.md` files are condensed technical-fact extracts (via WebFetch + spot
  verification against the HTML). For any full-fidelity quote, cite the sibling
  `.html`, which is the complete original page.
