"""The three layers of the driver: hardware-agnostic core, Modbus-RTU framing and the serial interface, platform port.

Reads: From top to bottom. Your application holds one Driver<Uart> object per instrument on the bus; its public
calls (listed from inc/alicat_basis2.hpp) turn into holding-register reads and writes. The Modbus-RTU layer
(inc/alicat_basis2_modbus.hpp) builds function 03, 06 and 16 frames and checks the CRC-16 of every reply. The
platform port is the only code you write: a class derived from UartInterface (inc/alicat_basis2_uart_interface.hpp)
that writes bytes, reads bytes with a timeout and flushes the receive buffer. Below it are the UART, the RS-485 or
RS-232 transceiver and the instruments.
"""

import re
from pathlib import Path

from hardfoc_mkdocs_theme.figkit import Fig

ROOT = Path(__file__).resolve().parents[3]
DRIVER = "inc/alicat_basis2.hpp"
MODBUS = "inc/alicat_basis2_modbus.hpp"
UART = "inc/alicat_basis2_uart_interface.hpp"


def data():
    drv = (ROOT / DRIVER).read_text()
    calls = re.findall(r"^    DriverResult<[\w:]+> (\w+)\(", drv, re.M)
    mb = (ROOT / MODBUS).read_text()
    framing = re.findall(r"^inline \S+ (Build\w+|Parse\w+)\(", mb, re.M)
    uart = (ROOT / UART).read_text()
    hooks = [h for h in ("write", "read", "flush_rx", "delay_ms_impl", "log_impl") if re.search(rf"\b{h}\(", uart)]
    return {
        "calls": calls,
        "framing": framing,
        "port_hooks": hooks,
        "sources": [f"{DRIVER}:Driver members", f"{MODBUS}:frame builders", f"{UART}:UartInterface"],
    }


def build(d):
    f = Fig(
        "driver-layers",
        "Three layers, one of them yours",
        alt="Layer diagram: the application calls the hardware-agnostic BASIS-2 driver, which builds Modbus-RTU "
        "frames with a CRC-16, which go through the serial interface your platform port implements, which drives "
        "the UART, the RS-485 or RS-232 transceiver and the BASIS-2 instruments on the bus.",
        width=1200,
        kicker="Architecture",
        subtitle="The driver and the framing are the same on every MCU. The port is a small class with three methods.",
        profile="overview",
        legend=[("brand", "this library"), ("teal", "you write"), ("slate", "hardware")],
    )
    W = f.body_w
    app = f.card(0, 0, W, "Your application", ["one Driver per instrument: read flow, command setpoints, tare"],
                 tone="slate")
    top = app.b + 40
    chips_h = f.chips(16, f.region_inner_top(top), W - 32, d["calls"], tone="brand")
    reg = f.region(0, top, W, f.region_inner_top(top) - top + chips_h + 18, "Hardware-agnostic core", tone="brand",
                   sub="Driver<UartT>, inc/alicat_basis2.hpp")
    mb = f.card(0, reg.b + 40, W, "Modbus-RTU framing",
                ["functions 03, 06 and 16, CRC-16 on every frame, replies checked against the request:",
                 "  ".join(d["framing"])],
                tone="brand", accent=True)
    port = f.card(0, mb.b + 40, W, "Platform port (your class, derived from UartInterface)",
                  ["  ".join(h + "()" for h in d["port_hooks"])], tone="teal", accent=True, mono=True)
    hw = f.card(0, port.b + 40, W, "UART, transceiver, BASIS-2 instruments",
                ["RS-485 multidrop (addresses 1 to 247) or RS-232, 8N1, 4800 to 115200 baud"], tone="slate")
    for a, b in ((app, reg), (reg, mb), (mb, port), (port, hw)):
        f.connect(a, b, "bottom", "top", tone="rail")
    return f
