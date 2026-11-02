---
hide:
  - navigation
  - toc
---

<div class="hf-hero" markdown>
<span class="hf-kicker">HardFOC driver</span>

# Alicat BASIS-2 Driver

<p>A C++17 driver for Alicat BASIS-2 mass flow meters and controllers. It reads flow, temperature and the totalizer,
commands setpoints, tares and reconfigures the instruments over Modbus-RTU, on any MCU with a UART and an RS-485 or
RS-232 transceiver.</p>

[Quickstart](quickstart.md){ .md-button .md-button--primary }
[API guide](api-guide.md){ .md-button }
[Source on GitHub](https://github.com/hardfoc/hf-alicat-basis2-driver){ .md-button }
</div>

The driver has no platform code. You give it a small class that writes bytes, reads bytes with a timeout and
flushes the receive buffer; it does the rest: Modbus-RTU framing, the CRC-16, reply checks, register scaling and
error reporting. It is header-only and allocates nothing.

<div class="hf-chip" markdown>
<div class="hf-chip__head">
<span class="hf-chip__kicker">Thermal mass flow meter and controller</span>
<span class="hf-chip__name">BASIS-2</span>
<span class="hf-chip__vendor">Alicat Scientific</span>
</div>

Instruments
:   B-Series meters and BC-Series controllers, 100 SCCM to 20 SLPM full scale, 9 calibrated gases

Interface
:   RS-485 or RS-232 (by model), 8 data bits, no parity, 1 stop bit; 38400 baud default, 4800 to 115200

Protocols
:   Modbus-RTU (functions 03, 06, 16), which this driver uses; Alicat ASCII commands

Addressing
:   Modbus address 1 to 247; ASCII unit ID `A` to `Z`

Supply
:   12 V to 24 V DC; 210 mA (controller), 12 mA (meter)

Connector and limits
:   6-pin JST-GH; 145 PSIG maximum common-mode pressure

[Operation manual (Rev 4, May 2025)](datasheet/DOC-MANUAL-BASIS2.pdf) ·
[Alicat manuals](https://www.alicat.com/manuals/)
</div>

## What you get

- **One object per instrument.** `Driver<YourUart>` targets one Modbus address; several drivers share one port
  for a multidrop RS-485 bus. See [API guide](api-guide.md#talk-to-several-instruments).
- **Live data in one request.** `ReadInstantaneous()` reads registers 2100 to 2109 (gas, status, temperature,
  flow, total, setpoint, valve drive, batch) and decodes them. See [API guide](api-guide.md#read-live-data).
- **Control and configuration.** Setpoint, setpoint source, watchdog, ramp, tare, totalizer, gas, averaging,
  reference temperature. See [Configuration](configuration.md).
- **Bus bring-up.** An address sweep, address and baud-rate changes, factory restore. See
  [Modbus-RTU protocol](modbus-protocol.md).
- **Zero-overhead portability.** The port is a CRTP class: no virtual calls, no heap, no exceptions, no RTOS
  dependency. See [Porting](porting.md).
- **Tested without hardware.** Host unit tests run the driver against a model of a BASIS-2 Modbus slave. See
  [Testing and CI](testing.md).

## Where to go next

<div class="grid cards" markdown>

-   :material-rocket-launch-outline: **[Quickstart](quickstart.md)**

    ---

    Wire the bus, write the port, read flow in about 15 lines.

-   :material-chip: **[Hardware](hardware.md)**

    ---

    Connector pinout, bus settings, timing and power.

-   :material-layers-outline: **[Architecture](architecture.md)**

    ---

    The three layers and what each one owns.

-   :material-code-braces: **[API guide](api-guide.md)**

    ---

    Read, command, configure and handle errors, task by task.

-   :material-swap-horizontal: **[Porting](porting.md)**

    ---

    Implement the serial interface for your MCU.

-   :material-book-open-variant: **[API reference](api-reference.md)**

    ---

    Every class and member, generated from the headers.

</div>
