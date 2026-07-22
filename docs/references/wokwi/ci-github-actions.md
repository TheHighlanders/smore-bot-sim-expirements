> Offline reference snapshot for citation. Source: https://docs.wokwi.com/wokwi-ci/github-actions
> Retrieved: 2026-07-22. See the sibling .html for the full original page.

# Wokwi CI — GitHub Actions

Runs Wokwi CI on every commit via the official action.

## Action name & version

**`wokwi/wokwi-ci-action@v1`**

## Example workflow step (verbatim from the page)

```yaml
- name: Test with Wokwi
  uses: wokwi/wokwi-ci-action@v1
  with:
    token: ${{ secrets.WOKWI_CLI_TOKEN }}
    path: /                     # directory with wokwi.toml, relative to repo's root
    expect_text: 'Hello, world!' # optional
```

## Inputs documented on THIS page

| Input | Purpose |
|-------|---------|
| `token` | The Wokwi CLI token — use `${{ secrets.WOKWI_CLI_TOKEN }}` |
| `path` | Directory containing `wokwi.toml`, relative to the repo root |
| `expect_text` | (optional) Text that must appear in the serial output |

> Caveat for citation: the GitHub Actions doc page only enumerates the three
> inputs above and then says, verbatim, "For a complete list of options, check
> out the action's README." The additional inputs sometimes assumed
> (`scenario`, `timeout`, `elf`, `fail_text`, `serial_log_file`,
> `diagram_file`) are **not listed on this page** — they live in the action's
> README on GitHub, not in these docs. Do not cite this page for them.

## CLI token secret

Configure a repository secret named **`WOKWI_CLI_TOKEN`**. Create the API token
on the **Wokwi CI Dashboard**.

## Example projects referenced

ESP32 WiFi + FreeRTOS, STM32 Nucleo64 C031C6, Raspberry Pi Pico SDK (with an
LED-blink scenario), PlatformIO Pushbutton Counter (scenario pushes button +
checks serial), ESP32-C6 LP I2C, Embedded Wizard Breakout (screenshot of
simulated LCD).
