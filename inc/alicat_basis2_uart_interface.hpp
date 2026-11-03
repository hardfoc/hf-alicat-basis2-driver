/**
 * @file alicat_basis2_uart_interface.hpp
 * @brief CRTP serial transport for the Alicat BASIS-2 driver.
 *
 * @details The driver itself only deals in byte buffers. Each platform
 *          (ESP-IDF, mock test harness, embedded MCU project, …) provides
 *          a concrete adapter inheriting from this CRTP base, implementing
 *          three small methods:
 *
 *            bool write(const uint8_t* data, std::size_t length); // true = all sent
 *            std::size_t read(uint8_t* out, std::size_t max, uint32_t timeout_ms);
 *            void flush_rx();
 *
 *          and, optionally, two hooks the base class finds at compile time:
 *
 *            void delay_ms_impl(uint32_t ms);                      // sleep >= ms
 *            void log_impl(const char* tag, const char* fmt, ...); // printf-style
 *
 *          Without `delay_ms_impl`, `delay_ms()` does nothing: portable code
 *          has no time base to busy-wait on. The driver calls `delay_ms()`
 *          before every request to leave the Modbus-RTU 3.5-character idle
 *          gap (Modbus over Serial Line V1.02, section 2.5.1.1), so a port
 *          without the hook must leave the gap itself (in `write()` or in the
 *          UART). Without `log_impl`, `log()` does nothing; the driver logs
 *          every failed transaction through it.
 *
 *          Concrete adapters should map RS-485 DE/RE handling (when needed)
 *          onto the underlying UART driver's RS-485 mode before passing the
 *          adapter to the BASIS-2 driver.
 *
 * @copyright Copyright (c) 2026 HardFOC. All rights reserved.
 */
#pragma once

#include <cstdint>
#include <cstddef>
#include <type_traits>
#include <utility>

namespace alicat_basis2 {

/**
 * @brief CRTP base for the BASIS-2 serial transport.
 *
 * @tparam Derived The concrete adapter (e.g. `AlicatUartEspAdapter`).
 *
 * The driver dispatches every byte through `static_cast<Derived*>(this)`,
 * so there is *zero* virtual-call overhead. The compiler can fully inline
 * the adapter calls into the driver's read/write loops.
 */
template <typename Derived>
class UartInterface {
public:
    /**
     * @brief Send `length` bytes synchronously. Must block until all bytes
     *        are accepted by the underlying serial driver.
     * @return `true` when every byte was accepted; `false` on a serial error
     *         (the driver then fails the request with `DriverError::SerialError`
     *         and does not wait for a reply). A port's `write()` must return
     *         `bool`; one returning anything else is rejected at compile time.
     */
    bool write(const uint8_t* data, std::size_t length) noexcept {
        static_assert(
            std::is_same_v<decltype(std::declval<Derived&>().write(data, length)), bool>,
            "alicat_basis2::UartInterface: the port's write(const uint8_t*, std::size_t) must "
            "return bool (true = every byte accepted, false = serial error)");
        return static_cast<Derived*>(this)->write(data, length);
    }

    /**
     * @brief Read up to `max` bytes into `out` with a per-call timeout.
     * @return Number of bytes actually written into `out`.
     */
    std::size_t read(uint8_t* out, std::size_t max, uint32_t timeout_ms) noexcept {
        return static_cast<Derived*>(this)->read(out, max, timeout_ms);
    }

    /// Drop any unread bytes from the RX path.
    void flush_rx() noexcept {
        static_cast<Derived*>(this)->flush_rx();
    }

    /**
     * @brief Sleep at least `ms` milliseconds.
     *
     * Calls `Derived::delay_ms_impl(ms)` when the adapter has one (public, or
     * private with `friend class UartInterface<Derived>`; any cv/ref
     * qualifier). Otherwise a no-op: there is no portable time base to
     * busy-wait on.
     */
    void delay_ms(uint32_t ms) noexcept {
        if constexpr (HasDelay<Derived>::value) {
            static_cast<Derived*>(this)->delay_ms_impl(ms);
        } else {
            (void)ms;
        }
    }

    /**
     * @brief Optional logging hook, printf-style.
     *
     * Forwards `tag`, `fmt` and the arguments unchanged to
     * `Derived::log_impl(tag, fmt, args...)` when the adapter has one;
     * otherwise a no-op. The driver logs every failed transaction here with
     * the tag `"alicat_basis2"`; `fmt` is always a string literal.
     */
    template <typename... Args>
    void log(const char* tag, const char* fmt, Args... args) noexcept {
      if constexpr (HasLog<Derived, Args...>::value) {
        static_cast<Derived*>(this)->log_impl(tag, fmt, args...);
      } else {
        (void)tag;
        (void)fmt;
        ((void)args, ...);
      }
    }

private:
    template <typename, typename = void>
    struct HasDelay : std::false_type {};
    template <typename T>
    struct HasDelay<T, std::void_t<decltype(std::declval<T&>().delay_ms_impl(uint32_t{0}))>>
        : std::true_type {};

    template <typename T, typename Void, typename... Args>
    struct HasLogImpl : std::false_type {};
    template <typename T, typename... Args>
    struct HasLogImpl<T,
                      std::void_t<decltype(std::declval<T&>().log_impl(
                          static_cast<const char*>(nullptr), static_cast<const char*>(nullptr),
                          std::declval<Args>()...))>,
                      Args...> : std::true_type {};
    template <typename T, typename... Args>
    using HasLog = HasLogImpl<T, void, Args...>;
};

}  // namespace alicat_basis2
