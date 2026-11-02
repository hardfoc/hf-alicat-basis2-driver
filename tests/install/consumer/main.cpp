/**
 * @file main.cpp
 * @brief Builds against the installed headers only: every public header, and the driver over a port.
 */
#include "alicat_basis2.hpp"
#include "alicat_basis2_modbus.hpp"
#include "alicat_basis2_registers.hpp"
#include "alicat_basis2_types.hpp"
#include "alicat_basis2_uart_interface.hpp"
#include "alicat_basis2_version.h"

#include <cstdio>

struct NullPort : alicat_basis2::UartInterface<NullPort> {
  void write(const uint8_t*, std::size_t) noexcept {}
  std::size_t read(uint8_t*, std::size_t, uint32_t) noexcept { return 0; }
  void flush_rx() noexcept {}
};

template class alicat_basis2::Driver<NullPort>;

int main() {
  std::printf("%s\n", alicat_basis2::GetDriverVersion());
  return 0;
}
