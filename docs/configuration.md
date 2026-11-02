# Configuration

The settings a BASIS-2 takes over Modbus, their ranges and defaults, and the order to apply them in.

Every setting here is a register write that the instrument keeps until you change it or restore the factory
settings. Every snippet is compiled from [`tests/docs/configuration.cpp`](../tests/docs/configuration.cpp).

## Order

| Step | Call | Why this position |
| --- | --- | --- |
| 1 | `SetModbusAddress()`, `SetBaudRate()` | Bus settings first, one instrument at a time ([API guide](api-guide.md#find-and-readdress-instruments)) |
| 2 | `ReadIdentity()` | Flow units and full scale decide how to read every value |
| 3 | `SetGas()` | Calibration for the process gas |
| 4 | `SetReferenceTemperatureC()`, `SetFlowAveragingMs()`, `SetTotalizerLimitMode()` | How flow and total are reported |
| 5 | `SetSetpointSource()`, `SetCommWatchdogMs()`, `SetAutotareEnabled()`, `SetMaxSetpointRamp()` | Controllers: control behaviour |
| 6 | `Tare()` | Last, on the process gas with no flow |

## Meters and controllers

```cpp
--8<-- "tests/docs/configuration.cpp:meter"
```

| Setting | Call | Register | Range | Factory default |
| --- | --- | --- | --- | --- |
| Gas | `SetGas(Gas)` | 2100 | `Air`, `Argon`, `CarbonDioxide`, `Nitrogen`, `Oxygen`, `NitrousOxide`, `Hydrogen`, `Helium`, `Methane` | Per order |
| Reference temperature | `SetReferenceTemperatureC(float)` | 52 | 0 °C to 30 °C, in 0.01 °C | Not stated |
| Flow averaging | `SetFlowAveragingMs(uint16_t)` | 55 | 0 ms to 2500 ms time constant | Not stated |
| Totalizer limit | `SetTotalizerLimitMode(TotalizerLimitMode)` | 54 | See below | Not stated |
| ASCII unit ID | `SetAsciiUnitId(char)` | 46 | `'A'` to `'Z'` | `'A'` |

The driver checks the ranges of the averaging time, the reference temperature, the address and the unit ID before
it sends anything, and returns `InvalidParameter` or `InvalidAddress`. Writing a gas the instrument does not have
leaves the gas unchanged (manual, register 2100).

| `TotalizerLimitMode` | At the maximum total |
| --- | --- |
| `SaturateNoError` (0) | Hold the maximum, no `OVR` |
| `ResetNoError` (1) | Restart from zero, no `OVR` |
| `SaturateWithOvr` (2) | Hold the maximum, set `OVR` |
| `ResetWithOvr` (3) | Restart from zero, set `OVR` |

## Controllers only

```cpp
--8<-- "tests/docs/configuration.cpp:controller"
```

| Setting | Call | Register | Range |
| --- | --- | --- | --- |
| Setpoint source | `SetSetpointSource(SetpointSource)` | 516 | `Analog` (0), `DigitalSaved` (1), `DigitalUnsaved` (2) |
| Communication watchdog | `SetCommWatchdogMs(uint16_t)` | 514 | 0 ms (off) to 5000 ms |
| Autotare | `SetAutotareEnabled(bool)` | 515 | Tares after the setpoint has been 0 for at least 2 s |
| Maximum setpoint ramp | `SetMaxSetpointRamp(uint32_t)` | 524-525 | % of full scale per ms × 10<sup>7</sup>; 0 = no ramp |
| Batch volume | `SetTotalizerBatch(uint32_t)` | 521-522 | Total-volume units, scaled by the factory-set decimals |

A meter has none of these registers and rejects the writes ([API guide](api-guide.md#handle-errors)).

## Driver settings

These live in the `Driver` object, not in the instrument.

| Setting | Call | Default |
| --- | --- | --- |
| Target address | constructor, `SetAddress()` | As constructed |
| Response timeout | constructor, `SetTimeoutMs()` | 200 ms (`kDefaultTimeoutMs`) |
| Bus baud rate, for the inter-frame gap | `SetBusBaudRate()`; `SetBaudRate()` sets it too | 38400 (`kDefaultBusBaudBps`) |
| Flow decimals | argument of `ReadInstantaneous()` | 3 |

## Back to factory settings

`FactoryRestore()` writes `0x5214` to register 80 and copies the factory settings over the user settings. The
manual recommends a power cycle afterwards. It may change the address and the baud rate back, so expect to
rediscover the instrument.
