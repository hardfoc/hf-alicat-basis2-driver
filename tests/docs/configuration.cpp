/**
 * @file configuration.cpp
 * @brief The Configuration page's snippets, compiled by the host build (never run).
 */
#include "my_uart.hpp"

using Driver = alicat_basis2::Driver<MyUart>;
namespace al = alicat_basis2;

bool configure_meter(Driver& meter) {
  // --8<-- [start:meter]
  using al::DriverError;
  DriverError e = meter.SetGas(al::Gas::Nitrogen).error;                     // register 2100
  if (e == DriverError::None) e = meter.SetReferenceTemperatureC(20.0f).error;  // 52: 0..30 C
  if (e == DriverError::None) e = meter.SetFlowAveragingMs(50).error;         // 55: 0..2500 ms
  if (e == DriverError::None) {
    e = meter.SetTotalizerLimitMode(al::TotalizerLimitMode::ResetWithOvr).error;  // 54
  }
  return e == DriverError::None;
  // --8<-- [end:meter]
}

bool configure_controller(Driver& controller) {
  // --8<-- [start:controller]
  using al::DriverError;
  DriverError e = controller.SetSetpointSource(al::SetpointSource::DigitalUnsaved).error;  // 516
  if (e == DriverError::None) e = controller.SetCommWatchdogMs(500).error;    // 514: 0..5000 ms
  if (e == DriverError::None) e = controller.SetAutotareEnabled(true).error;  // 515
  if (e == DriverError::None) {
    // 524-525: % of full scale per ms x 10^7. 0.01 %/ms (full scale in 10 s) = 100000.
    e = controller.SetMaxSetpointRamp(100000).error;
  }
  return e == DriverError::None;
  // --8<-- [end:controller]
}
