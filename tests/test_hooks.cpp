/**
 * @file test_hooks.cpp
 * @brief The optional port hooks of UartInterface: delay_ms_impl() and log_impl().
 */
#include "test_harness.hpp"

#include "mock_basis2.hpp"

#include <cstdarg>
#include <cstdio>
#include <string>

namespace al = alicat_basis2;

namespace {

/// A port with no optional hook: delay_ms() and log() compile and do nothing.
struct BarePort : al::UartInterface<BarePort> {
  bool write(const uint8_t*, std::size_t) noexcept { return true; }
  std::size_t read(uint8_t*, std::size_t, uint32_t) noexcept {
    return 0;
  }
  void flush_rx() noexcept {}
};

/// Hooks declared with an lvalue ref-qualifier.
struct RefQualifiedPort : al::UartInterface<RefQualifiedPort> {
  uint32_t slept = 0;
  std::string logged;
  void delay_ms_impl(uint32_t ms) & noexcept {
    slept += ms;
  }
  void log_impl(const char* tag, const char* fmt, ...) & noexcept {
    char buf[64];
    va_list ap;
    va_start(ap, fmt);
    std::vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    logged = std::string(tag) + " " + buf;
  }
};

/// Hooks kept private, with the base class as a friend.
class PrivatePort : public al::UartInterface<PrivatePort> {
  friend class al::UartInterface<PrivatePort>;

public:
  uint32_t slept = 0;
  int logs = 0;

private:
  void delay_ms_impl(uint32_t ms) noexcept {
    slept += ms;
  }
  void log_impl(const char*, const char*, ...) noexcept {
    ++logs;
  }
};

} // namespace

TEST_CASE("delay_ms() forwards to delay_ms_impl() however it is declared, else does nothing") {
  RefQualifiedPort r;
  r.delay_ms(3);
  CHECK_EQ(r.slept, 3U);
  PrivatePort p;
  p.delay_ms(4);
  CHECK_EQ(p.slept, 4U);
  BarePort b;
  b.delay_ms(5); // no hook: a no-op, not a busy-wait
}

TEST_CASE("log() forwards tag, format and arguments to log_impl(), else does nothing") {
  RefQualifiedPort r;
  al::UartInterface<RefQualifiedPort>& base = r;
  base.log("tag", "x=%d s=%s", 42, "ok");
  CHECK(r.logged == "tag x=42 s=ok");
  PrivatePort p;
  p.log("tag", "no arguments");
  CHECK_EQ(p.logs, 1);
  BarePort b;
  b.log("tag", "%d", 1);
}

TEST_CASE("the driver logs a failed transaction through log() and stays quiet on success") {
  MockBasis2 m(1);
  m.regs[25] = 0x0312;
  al::Driver<MockBasis2> d(m, 1);
  uint16_t fw = 0;
  REQUIRE(d.ReadHoldingRegisters(al::reg::FirmwareVersion, &fw, 1) == al::DriverError::None);
  CHECK(m.logs.empty());

  CHECK(d.SetCommWatchdogMs(100).error == al::DriverError::ModbusException);
  REQUIRE(m.logs.size() == 1U);
  CHECK(m.logs[0] == "alicat_basis2: addr 1 fn 0x06 reg 514: ModbusException IllegalDataAddress");

  d.SetAddress(9); // nobody there
  CHECK(d.ReadHoldingRegisters(al::reg::FirmwareVersion, &fw, 1) == al::DriverError::Timeout);
  REQUIRE(m.logs.size() == 2U);
  CHECK(m.logs[1] == "alicat_basis2: addr 9 fn 0x03 reg 25: Timeout");
}

TEST_CASE("the address sweep does not log the timeouts it expects") {
  MockBasis2 m(42);
  m.regs[25] = 0x0312;
  al::Driver<MockBasis2> d(m, 1);
  uint8_t bitmap[32]{};
  REQUIRE(d.DiscoverPresentAddresses(bitmap, sizeof(bitmap), 20, 40, 45).ok());
  CHECK(m.logs.empty());
  d.SetAddress(9);
  uint16_t fw = 0;
  CHECK(d.ReadHoldingRegisters(al::reg::FirmwareVersion, &fw, 1) == al::DriverError::Timeout);
  CHECK_EQ(m.logs.size(), 1U); // logging is back on after the sweep
}
