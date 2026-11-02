# Quickstart

Read flow from a BASIS-2 on your MCU: wire the bus and power, write a small port class, call two driver functions.

!!! warning "Power goes to pin 4 only"
    The manual warns that power on any pin other than pin 4 can permanently damage the instrument. Wire every
    signal before you switch on the 12 V to 24 V supply.

## 1. Wire it

The usual setup is RS-485 through a half-duplex transceiver; the instrument's model decides whether pins 5 and 6
are RS-485 or RS-232.

| From | To | Notes |
| --- | --- | --- |
| MCU UART TX | Transceiver DI | 8N1, 38400 baud out of the factory |
| MCU UART RX | Transceiver RO | |
| MCU GPIO or RTS | Transceiver DE and RE, tied | High while the MCU transmits |
| Transceiver A | BASIS-2 pin 6, A (−) | 6-pin JST-GH on top |
| Transceiver B | BASIS-2 pin 5, B (+) | |
| MCU GND | Transceiver GND, BASIS-2 pin 3 | One common ground |
| 12 V to 24 V DC | BASIS-2 pin 4 | Last, after everything else |

The [Hardware](hardware.md) page has the RS-232 wiring, the bus settings and the power figures.

## 2. Add the driver

Add the repository as a submodule and link the `hf::alicat_basis2` CMake target (or put `inc/` on the include
path and build with C++17). [Installation](installation.md) covers CMake and ESP-IDF.

```bash
git submodule add https://github.com/hardfoc/hf-alicat-basis2-driver.git external/hf-alicat-basis2-driver
```

## 3. Write the port

The port is the only platform code. It writes bytes, reads bytes with a timeout and flushes the receive buffer.
Replace the `platform_*` calls with your SDK's.

```cpp title="my_uart.hpp"
--8<-- "tests/docs/my_uart.hpp:port"
```

`read()` must wait until `max` bytes have arrived or the timeout has passed, then return what it has.
[Porting](porting.md) explains each method and the RS-485 direction pin.

## 4. Read flow

```cpp title="main.cpp"
--8<-- "tests/docs/quickstart.cpp:quickstart"
```

What each step does:

1. `Driver<MyUart>(uart, 1)` targets Modbus address 1 with the default 200 ms response timeout. Nothing is sent
   yet.
2. `ReadIdentity()` reads the firmware version, serial number, address, unit ID, full scale and flow units. A
   reply proves the wiring, the baud rate and the address.
3. `ReadInstantaneous(3)` reads registers 2100 to 2109 in one request. The `3` is the number of decimals the
   instrument's flow registers carry; see [API guide](api-guide.md#read-live-data).

This example is built by the host CI, so it compiles against the current headers.

## If it does not work

| Symptom | First thing to check |
| --- | --- |
| `Timeout` on every call | Baud rate (38400 default), A and B swapped, DE/RE not driven, wrong address |
| `BadCrc` | A missing common ground, termination on a long bus, two instruments at one address |
| `flow` is 10 or 100 times off | The decimals passed to `ReadInstantaneous()` ([API guide](api-guide.md#read-live-data)) |
| The LED blinks red fast | Mass flow, temperature or totalizer over range ([Troubleshooting](troubleshooting.md)) |

## Next

- [API guide](api-guide.md): setpoints, taring, the totalizer, errors.
- [Examples](examples.md): complete ESP32-S3 applications.
- [Modbus-RTU protocol](modbus-protocol.md): frames, registers and what each call sends.
