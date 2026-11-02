/**
 * @file ports.cpp
 * @brief Instantiates the driver over the documented port, so every member compiles in full.
 */
#include "my_uart.hpp"

template class alicat_basis2::Driver<MyUart>;

// The optional hooks are found: delay_ms() and log() forward to MyUart's delay_ms_impl() and
// log_impl().
void use_hooks(MyUart& uart) {
  uart.delay_ms(5);
  uart.log("app", "%d", 1);
}
