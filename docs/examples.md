# Examples

Complete ESP32-S3 applications that exercise the driver on a BASIS-2 over RS-485, from a minimal read loop to every
feature.

They run on an ESP32-S3 with a half-duplex RS-485 transceiver on UART1: TX on GPIO17, RX on GPIO18 and DE/RE on
GPIO8, at 38400 baud and Modbus address 1. The pins are constants at the top of each source file. CI builds every
app below on each pull request.

## The apps

<!-- generated: examples -->
| App | What it shows | Source | CI |
| --- | --- | --- | --- |
| `alicat_basis2_minimal_example` :material-star:{ title="featured" } | Minimal single-instrument BASIS-2 read loop on ESP32-S3 UART1 RS-485 | [`alicat_basis2_minimal_example.cpp`](../examples/esp32/main/alicat_basis2_minimal_example.cpp) | yes |
| `alicat_basis2_full_example` :material-star:{ title="featured" } | Comprehensive BASIS-2 example: discovery sweep, identity, configuration walkthrough, periodic instantaneous + measurement-window reads | [`alicat_basis2_full_example.cpp`](../examples/esp32/main/alicat_basis2_full_example.cpp) | yes |

Build and flash one (from `examples/esp32`, ESP-IDF environment loaded):

```bash
./scripts/build_app.sh alicat_basis2_minimal_example Release
./scripts/flash_app.sh flash_monitor alicat_basis2_minimal_example Release
```

Source of this table: [`examples/esp32/app_config.yml`](../examples/esp32/app_config.yml).
<!-- /generated -->

## Set up once

```bash
git submodule update --init examples/esp32/scripts   # the HardFOC ESP-IDF build scripts
cd examples/esp32
. $IDF_PATH/export.sh                                 # ESP-IDF v5.5
./scripts/build_app.sh list                           # every app and build type
```

The scripts read [`app_config.yml`](../examples/esp32/app_config.yml) (the same file this page's table comes
from) and build each app as its own firmware image.

## What to read in an example

| You want to | Read |
| --- | --- |
| Set up an ESP-IDF UART in RS-485 mode and write the port | `alicat_basis2_minimal_example.cpp`, `EspUartAdapter` and `app_main()` |
| Read identity and live data in a loop | `alicat_basis2_minimal_example.cpp` |
| Sweep the bus for instruments | `alicat_basis2_full_example.cpp`, the discovery step |
| Configure gas, averaging, watchdog and the totalizer | `alicat_basis2_full_example.cpp`, the configuration step |
| Command a setpoint and handle a meter's refusal | `alicat_basis2_full_example.cpp`, the setpoint step |
| Use the measurement window | `alicat_basis2_full_example.cpp`, the last step |

The [example folder's README](../examples/esp32/README.md) has the wiring and the `idf.py` commands.
