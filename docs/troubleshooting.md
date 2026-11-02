# Troubleshooting

Symptoms you may see while bringing up a BASIS-2, their causes and the fix for each, cheapest check first.

Start with the `DriverError` of the failing call and the LED: they tell transport problems (no reply, bad CRC)
apart from instrument conditions (over range, valve hold).

## No reply

| Symptom | Cause | Fix |
| --- | --- | --- |
| `Timeout` on every call, LED solid green | Wrong baud rate | The factory rate is 38400 8N1; if someone changed it, sweep 4800 to 115200 |
| `Timeout` on every call, LED never flashes green | Nothing reaches the instrument | A and B swapped, DE/RE never asserted, TX and RX swapped, RS-232 model on an RS-485 bus |
| `Timeout` at one address only | Wrong address | `DiscoverPresentAddresses()` with only that instrument on the bus |
| `Timeout` after `SetBaudRate()` | The host UART is still at the old rate | Switch the host rate, then `ReadIdentity()` ([API guide](api-guide.md#change-the-baud-rate)) |
| `Timeout` after `FactoryRestore()` | Bus settings back to the factory values | Power-cycle, then rediscover |

## Corrupted replies

| Symptom | Cause | Fix |
| --- | --- | --- |
| `BadCrc` or `InvalidFrame` now and then | Noise, no termination on a long bus, no common ground | 120 Ω at both ends, ground to pin 3, twisted pair |
| `BadCrc` every time at one address | Two instruments at the same address | Disconnect all but one, `SetModbusAddress()`, repeat |
| `InvalidFrame` that comes and goes | The port's `read()` returns before all bytes arrived | Wait for `max` bytes or the timeout ([Porting](porting.md#rules-for-the-transfer)) |
| `ResponseMismatch` | A late reply to an earlier request | Raise the timeout; `flush_rx()` must drop stale bytes |

## The instrument refuses

| Symptom | Cause | Fix |
| --- | --- | --- |
| `ModbusException` on a controller setting, `IllegalDataAddress` | A meter has no control registers | Expected on meters; see [API guide](api-guide.md#handle-errors) |
| `ModbusException` on a read, `IllegalDataAddress` | The register is not in this firmware (many were added in V3.0.0) | Read the firmware version (register 25) |
| Setpoint writes have no effect | Setpoint source is `Analog` | `SetSetpointSource(SetpointSource::DigitalUnsaved)` |
| Setpoint jumps to 0, valve closes | Communication watchdog expired | Poll faster than the watchdog, or `SetCommWatchdogMs(0)` |
| The setpoint read back differs from the command | A ramp is active, or the instrument rounded it | `SetMaxSetpointRamp(0)` to disable ramping |

## Readings look wrong

| Symptom | Cause | Fix |
| --- | --- | --- |
| Flow 10, 100 or 1000 times off | Wrong decimals passed to `ReadInstantaneous()` | Compare with the ASCII data frame or calibration sheet ([API guide](api-guide.md#read-live-data)) |
| Negative or offset flow at zero | Poor tare | Tare on the process gas with no flow; controllers: enable autotare |
| Readings inaccurate after a gas change | Old gas still in the line, or not tared on the new gas | Flow the new gas long enough, then tare |
| Readings will not settle | The instrument is fast and sees real fluctuation | `SetFlowAveragingMs()`; tune P and I (registers 519, 520) |
| Controller does not reach its setpoint | Supply pressure too low, or a blocked port | Raise inlet pressure; check for thread tape in the ports |
| LED blinks red fast, `mass_flow_overrange` set | Flow above the range | Reduce flow; check the full scale with `ReadIdentity()` |
| LED blinks red slowly, `valve_thermal_management` set | A setpoint with no flow for a while | Check that gas is supplied; the valve resumes when flow changes |

## Getting help

Open an issue at <https://github.com/hardfoc/hf-alicat-basis2-driver/issues> with the driver version
(`GetDriverVersion()`), the output of `ReadIdentity()`, your platform and a byte trace of the failing request and
reply.
