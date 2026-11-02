# API guide

How to do the common jobs with the driver: identify, read, command, tare, set up a bus and handle errors.

Every snippet on this page is compiled by the host CI from
[`tests/docs/api_tasks.cpp`](../tests/docs/api_tasks.cpp), with `Driver` standing for
`alicat_basis2::Driver<MyUart>` and `al` for `alicat_basis2`. The [API reference](api-reference.md) lists every
member.

## The shape of the API

One `Driver<UartT>` object per instrument. Its calls fall into a few groups:

| Group | Calls | Instruments |
| --- | --- | --- |
| Live data | `ReadInstantaneous()` | All |
| Identity | `ReadIdentity()`; the free function `GetDriverVersion()` for the driver | All |
| Setpoint | `SetSetpoint()`, `SetSetpointSource()`, `SetCommWatchdogMs()`, `SetMaxSetpointRamp()` | Controllers |
| Tare | `Tare()`, `SetAutotareEnabled()` (controllers) | All |
| Gas and flow processing | `SetGas()`, `SetFlowAveragingMs()`, `SetReferenceTemperatureC()` | All |
| Totalizer | `ResetTotalizer()`, `SetTotalizerLimitMode()`, `SetTotalizerBatch()` (controllers) | All |
| Measurement window | `ConfigureMeasurementTrigger()` (controllers), `StartMeasurementSamples()`, `ReadMeasurement()` | All |
| Bus configuration | `SetModbusAddress()`, `SetAsciiUnitId()`, `SetBaudRate()`, `FactoryRestore()`, `DiscoverPresentAddresses()` | All |
| Raw access | `ReadHoldingRegisters()`, `WriteSingleRegister()`, `WriteMultipleRegisters()` | All |
| Driver settings | `SetAddress()`, `GetAddress()`, `SetTimeoutMs()`, `GetTimeoutMs()` | |

Every call is `noexcept` and returns `DriverResult<T>` (`.ok()`, `.value`, `.error`) or, for the raw read, a bare
`DriverError`. Check it: a failed call leaves `value` default-constructed.

## Talk to several instruments

```cpp
--8<-- "tests/docs/api_tasks.cpp:bus"
```

The constructor sends nothing. The port must outlive every driver that uses it. Drivers that share a port must
not run at the same time ([Porting](porting.md#threads)).

## Identify the instrument

```cpp
--8<-- "tests/docs/api_tasks.cpp:identity"
```

`ReadIdentity()` costs three requests. Read it once at start-up and keep the result: the flow units and full
scale tell you how to interpret every flow value.

## Read live data

```cpp
--8<-- "tests/docs/api_tasks.cpp:read"
```

One request reads registers 2100 to 2109. Temperature and valve drive are always value / 100. Flow, total,
setpoint and batch are divided by 10<sup>decimals</sup>, where `decimals` (default 3) is the factory-set number of
decimal places of the instrument. The manual names no register that reports it: check one reading against the
ASCII data frame or the calibration sheet and fix the value in your code. The raw register values are in the
`raw_*` fields for logging.

## Command a setpoint

```cpp
--8<-- "tests/docs/api_tasks.cpp:setpoint"
```

| Step | Why |
| --- | --- |
| `SetSetpointSource(DigitalUnsaved)` | With `Analog` the instrument ignores digital setpoints. `DigitalSaved` stores every setpoint in flash; the manual warns that changing it more often than every few minutes can wear the memory out |
| `SetCommWatchdogMs(ms)` | 0 to 5000 ms. The controller forces a zero setpoint and closes the valve when no serial traffic arrives in time. It works only with `DigitalUnsaved` |
| `SetSetpoint(value)` | Writes value × 1000 as a signed 32-bit integer to 2053-2054. The instrument rounds to its own resolution |

`ReadInstantaneous().value.setpoint` reads back the setpoint in use, which lags the command while a ramp
(`SetMaxSetpointRamp()`) is active.

## Tare and the totalizer

```cpp
--8<-- "tests/docs/api_tasks.cpp:tare"
```

Tare with the process gas in the line and no flow: the manual says BASIS instruments reach their accuracy only
when tared on pure process gas. A tare interrupts a running measurement window. The totalizer is lost when power
is lost. `SetTotalizerBatch()` takes the volume already scaled to the total-volume decimals.

## Measure over a window

```cpp
--8<-- "tests/docs/api_tasks.cpp:measurement"
```

The instrument samples about every 2.5 ms. Writing the sample count starts a new window and moves the running one
into the "previous" registers. `ReadMeasurement(decimals)` reads registers 4201 to 4215 in one request and scales
`avg_flow` by the same decimals as live flow; scale the raw minimum, maximum and previous average flow
(`prev_avg_flow_raw`, 32-bit at 4214-4215) the same way.

## Find and readdress instruments

```cpp
--8<-- "tests/docs/api_tasks.cpp:discover"
```

The sweep reads register 25 at each address with a 30 ms timeout (by default): a full sweep of 247 empty
addresses takes about 7.5 s. It restores the driver's address and timeout afterwards. It finds instruments at the
host's current baud rate only.

```cpp
--8<-- "tests/docs/api_tasks.cpp:address"
```

Give each instrument a unique address before you connect the next one: two instruments at one address answer on
top of each other.

## Change the baud rate

```cpp
--8<-- "tests/docs/api_tasks.cpp:baud"
```

!!! warning "The instrument switches before it answers"
    The manual warns that the host must change its own rate in time to receive the confirmation, and may not.
    Treat the result of `SetBaudRate()` as unreliable, switch the host UART, then confirm with a read at the new
    rate. If that fails, sweep the six rates to find the instrument again.

## Use any register directly

```cpp
--8<-- "tests/docs/api_tasks.cpp:raw"
```

The register constants are in `alicat_basis2::reg`; the [Modbus-RTU protocol](modbus-protocol.md) page lists them
with their scaling. `ReadHoldingRegisters()` reads up to 32 registers; `WriteMultipleRegisters()` writes up to 32.

## Handle errors

```cpp
--8<-- "tests/docs/api_tasks.cpp:errors"
```

| `DriverError` | Meaning | Where to look |
| --- | --- | --- |
| `Timeout` | No byte arrived within the timeout | Wiring, baud rate, address, DE/RE, the timeout |
| `InvalidFrame` | Too short, wrong byte count | Line noise, a `read()` that returns early |
| `BadCrc` | CRC-16 mismatch | Ground, termination, two instruments at one address |
| `ModbusException` | The instrument returned an exception reply; `GetLastException()` has the code | Register not in this model or firmware (`IllegalDataAddress`), a meter asked for a controller setting |
| `ResponseMismatch` | Reply from another address, or an echo that differs from the request | Address clash, a late reply from an earlier request |
| `InvalidParameter`, `InvalidAddress` | The driver refused the argument; nothing was sent | Ranges on the [Configuration](configuration.md) page |
| `BufferTooSmall` | A request longer than the driver's 32-register buffers | Split the access |

!!! note "Exception replies come back at once"
    An exception reply is 5 bytes: the function code with bit 7 set, the exception code and the CRC. The driver
    reads the first 3 bytes of every reply, sees bit 7 and reads only the CRC, so a rejected read or write (for
    example `SetSetpointSource()` on a meter) returns `ModbusException` as soon as the reply has arrived, not after
    the timeout. `GetLastException()` returns the code (`ModbusExceptionCode`, `ToString()` names it) until the
    next request.

The status bits in `InstantaneousData` (`mass_flow_overrange`, `temperature_overrange`, `totalizer_overrange`,
`valve_held`, `valve_thermal_management`) report instrument conditions, not transport errors.
[Troubleshooting](troubleshooting.md) maps symptoms to causes.
