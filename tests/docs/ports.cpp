/**
 * @file ports.cpp
 * @brief Instantiates the driver over the documented port, so every member compiles in full.
 */
#include "my_uart.hpp"

template class alicat_basis2::Driver<MyUart>;

// The optional hook is found: delay_ms() forwards to MyUart::delay_ms_impl().
void use_delay(MyUart& uart) { uart.delay_ms(5); }
