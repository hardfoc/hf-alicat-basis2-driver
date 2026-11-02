# Changelog

What changed in each release of the driver. Entries marked **BREAKING** need source changes in code that used the
affected API.

## Unreleased

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

### Changed

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
