> Offline reference snapshot for citation. Source: https://docs.wokwi.com/wokwi-ci/getting-started
> Retrieved: 2026-07-22. See the sibling .html for the full original page.

# Wokwi CI — Getting Started

Wokwi CI runs your embedded firmware on cloud-based simulation servers for
automated testing on CI systems (GitHub Actions, GitLab CI, etc.). Firmware is
**not stored** — it is deleted from the cloud server after the simulation
finishes. The methodology is called **Wokwi in the Loop (WITL)**.

## Required files

- **`wokwi.toml`** — project configuration.
- **`diagram.json`** — circuit diagram.

## API token

- Set the **`WOKWI_CLI_TOKEN`** environment variable to your API token.
- Create the token on the **Wokwi CI Dashboard** (https://wokwi.com/dashboard/ci).

## `wokwi-cli` command flags (documented)

| Flag | Purpose |
|------|---------|
| `--timeout` | Limit simulation time (milliseconds) |
| `--scenario` | Load an automation-scenario YAML file |
| `--serial-log-file` | Capture serial output to a file |
| `--expect-text` | Pass only if this text appears in the serial output |
| `--fail-text` | Fail immediately if this text appears in the serial output |

## Monthly simulation-time limits (by plan)

- Free: 50 minutes
- Hobby / Hobby+: 200 minutes
- Pro: 2000 minutes

## Note

The CLI also has experimental **Model Context Protocol (MCP)** support so AI
agents can drive Wokwi simulations.
