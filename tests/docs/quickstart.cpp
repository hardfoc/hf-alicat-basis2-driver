/**
 * @file quickstart.cpp
 * @brief The Quickstart page's example, compiled by the host build (never run).
 */
#include "my_uart.hpp"

#include <cstdio>

// --8<-- [start:quickstart]
#include "alicat_basis2.hpp"

int read_flow_once(MyUart& uart) {
  alicat_basis2::Driver<MyUart> meter(uart, 1);         // Modbus address 1, 200 ms timeout

  const auto id = meter.ReadIdentity();                 // 1. who is on the bus
  if (!id.ok()) {
    std::printf("no reply: %s\n", alicat_basis2::ToString(id.error).data());
    return -1;                                          // wiring, baud rate or address
  }

  const auto live = meter.ReadInstantaneous(3);         // 2. one request: registers 2100..2109
  if (!live.ok()) {
    return -1;
  }
  std::printf("SN %s  %s  flow %.3f %s  %.2f C\n", id.value.serial_number,
              alicat_basis2::GasShortName(live.value.gas).data(), static_cast<double>(live.value.flow),
              alicat_basis2::ToString(id.value.flow_units).data(),
              static_cast<double>(live.value.temperature_c));
  return 0;
}
// --8<-- [end:quickstart]
