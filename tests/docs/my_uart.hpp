/**
 * @file my_uart.hpp
 * @brief The UART port shown on the documentation's Quickstart and Porting pages, compiled by the host build.
 *
 * Every platform call goes through the `platform_*` functions declared below; a real port replaces
 * them with its SDK (ESP-IDF, STM32 HAL, Zephyr, Linux termios). The class is what the driver needs,
 * nothing more.
 */
#pragma once

#include "alicat_basis2.hpp"

#include <cstddef>
#include <cstdint>

// Platform hooks: your SDK's calls. Declared only, so the docs build checks the port compiles.
void platform_uart_write(const uint8_t* data, std::size_t len) noexcept;  // blocks until sent
// Returns when `max` bytes arrived or `timeout_ms` passed, with the number of bytes read.
std::size_t platform_uart_read(uint8_t* out, std::size_t max, uint32_t timeout_ms) noexcept;
void platform_uart_flush_rx() noexcept;
void platform_delay_ms(uint32_t ms) noexcept;

// --8<-- [start:port]
class MyUart : public alicat_basis2::UartInterface<MyUart> {
public:
  // Send a whole Modbus request; return once the last byte is on the wire.
  void write(const uint8_t* data, std::size_t length) noexcept {
    platform_uart_write(data, length);
  }

  // Wait for up to `max` bytes; return early only when all of them arrived.
  std::size_t read(uint8_t* out, std::size_t max, uint32_t timeout_ms) noexcept {
    return platform_uart_read(out, max, timeout_ms);
  }

  // Drop stale bytes (a late reply, line noise) before the next request.
  void flush_rx() noexcept { platform_uart_flush_rx(); }

  // Optional: a real sleep for UartInterface::delay_ms().
  void delay_ms_impl(uint32_t ms) noexcept { platform_delay_ms(ms); }
};
// --8<-- [end:port]
