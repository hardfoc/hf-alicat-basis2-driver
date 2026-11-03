# Changelog

What changed in each release of the driver. Entries marked **BREAKING** need source changes in code that used the
affected API.

## Unreleased

### Breaking

- The port's `write()` returns `bool` (`true` = every byte accepted, `false` = serial error) instead of `void`;
  a port whose `write()` returns anything else is rejected at compile time. A failed write fails the call with
  `DriverError::SerialError` at once: the driver no longer waits out the read timeout for a reply to a request
  that was never sent and reports `Timeout`, and a broadcast (`address 0`) that was not sent is no longer
  reported as a success. **BREAKING** for ports: change `void write(...)` to `bool write(...)` and return the
  UART's result.

### Added

- Host unit tests (`-DHF_ALICAT_BASIS2_BUILD_TESTS=ON`) against a mock BASIS-2 Modbus slave, and a CI workflow.
- `ModbusExceptionCode`, `ToString(ModbusExceptionCode)` and `Driver::GetLastException()`: the code of the last
  exception reply. `modbus::ParseException()` and `modbus::IsExceptionReply()`.

### Fixed

- **BREAKING (behaviour):** an exception reply to a write (functions 06 and 16) returned `InvalidFrame` after the
  full response timeout. The driver now reads the 3-byte reply header first, recognises an exception reply by bit 7
  of the function code, reads its CRC and returns `ModbusException` at once; reads (function 03) also no longer
  wait for the timeout on an exception. Code that matched `InvalidFrame` for a rejected write must match
  `ModbusException`. (Modbus Application Protocol V1.1b3, section 7; manual Rev 4, "Communicating Via
  Modbus-RTU", p. 18.)

- `ReadMeasurement()` decoded the previous window's average flow from registers 4213-4214 (the previous average
  temperature and the high half of the flow). The manual gives it as a signed 4-byte integer at 4214, so it is now
  read from 4214-4215 (high register first) and the request covers 4201 to 4215. `num_samples_collected` now
  reports register 4206 (samples averaged) as its name says, not 4201 (samples requested, now
  `num_samples_requested`), and `avg_flow` is filled in using the new `flow_decimals` argument (default 3).
  **BREAKING (behaviour)** for code that relied on the old `num_samples_collected` value. (Manual Rev 4,
  "Measurement Data Registers", pp. 22-23.)

- The optional port hooks did not do what their documentation said. `UartInterface::log()` was a no-op that no
  port could hook into and the driver never logged; it now forwards (as a variadic template) to an optional
  `log_impl(tag, fmt, ...)` in the port, and the driver logs every failed transaction through it (tag
  `alicat_basis2`; the address sweep stays quiet). `delay_ms()` missed a ref-qualified `delay_ms_impl()`; it is
  found now. The header said a port without `delay_ms_impl()` gets a busy-wait: that was wrong (portable code has
  no time base to busy-wait on) and the documentation now says it is a no-op, as the code does.

- The Modbus-RTU 3.5-character inter-frame gap was not enforced: the driver wrote each request as soon as the
  previous call returned. It now waits t3.5 through the port's `delay_ms()` before every request: 3.5 characters
  of 11 bits up to 19200 baud, a fixed 1.75 ms above, rounded up to whole milliseconds (2 ms at the factory
  38400 baud). New: `Driver::SetBusBaudRate()` / `GetBusBaudRate()` / `GetInterFrameGapUs()` (default 38400,
  `kDefaultBusBaudBps`; `SetBaudRate()` updates it), `modbus::InterFrameGapUs()` / `InterFrameGapMs()`. Every
  request now takes up to that long more; a port without `delay_ms_impl()` gets no wait, as before. The ESP32
  examples' `delay_ms_impl()` rounded short sleeps down to zero ticks and now sleeps at least the time asked
  for. (Modbus over Serial Line V1.02, sections 2.5.1 and 2.5.1.1; manual Rev 4, p. 18 for the baud rates.)

- `cmake --install` installed nothing, and the package config included its targets file as `OPTIONAL`, so
  `find_package(hf_alicat_basis2)` succeeded without defining a target. The install now puts every header of
  `inc/` and `alicat_basis2_version.h` in `include/hf_alicat_basis2/` and exports the target as `hf::alicat_basis2`
  (the documented name; without `EXPORT_NAME` it would have been `hf::hf_alicat_basis2`). New option
  `HF_ALICAT_BASIS2_INSTALL` (on for a top-level build). A CTest installs to a temporary prefix and builds a
  `find_package()` consumer.

### Changed

- `kInterFrameDelayMs` is deprecated: the driver never used it.
- `reg::MeasurementPrevAvgFlowLow` is deprecated: 4214 is the high register. Use `reg::MeasurementPrevAvgFlow`.
- Documentation moved to the HardFOC MkDocs platform: new page set (quickstart, hardware, architecture, API guide,
  configuration, porting, testing), code snippets compiled by the host build from `tests/docs/`, Doxygen under
  `/api/`, versioned with mike. The Jekyll site files and the doxygen-awesome submodule are gone.

## 1.0.0

### Added

- `Driver<UartT>` over a CRTP `UartInterface`: Modbus-RTU functions 03, 06 and 16 with CRC-16.
- Live data (`ReadInstantaneous()`), identity, setpoint and setpoint source, communication watchdog, setpoint ramp,
  tare and autotare, gas selection, totalizer and batch, flow averaging, reference temperature, measurement
  window, address, unit ID and baud-rate changes, factory restore and an address sweep.
- `GetDriverVersion()` and the generated `alicat_basis2_version.h`.
- ESP-IDF component wrapper and two ESP32-S3 examples.
