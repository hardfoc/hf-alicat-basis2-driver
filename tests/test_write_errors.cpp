/**
 * @file test_write_errors.cpp
 * @brief A request the UART did not send is a SerialError, not a Timeout, and the driver does not
 * wait for a reply to it. A broadcast that was not sent is not reported as done.
 */
#include "test_harness.hpp"

#include "mock_basis2.hpp"

#include <type_traits>

namespace al = alicat_basis2;

// The port contract: write() returns whether every byte was accepted.
static_assert(std::is_same_v<decltype(std::declval<al::UartInterface<MockBasis2>&>().write(
                                 static_cast<const uint8_t*>(nullptr), std::size_t{0})),
                             bool>,
              "UartInterface::write() must return bool");

TEST_CASE("A read whose request was not sent fails with SerialError without waiting") {
  MockBasis2 m(1);
  m.regs[2100] = 7;
  m.write_fails = true;
  al::Driver<MockBasis2> d(m, 1);
  uint16_t out = 0;
  CHECK(d.ReadHoldingRegisters(2100, &out, 1) == al::DriverError::SerialError);
  CHECK_EQ(m.failed_writes, 1);
  CHECK_EQ(m.timeouts, 0);  // no read of a reply that cannot come
  CHECK_EQ(m.waited_ms, 0U);
}

TEST_CASE("A write whose request was not sent fails with SerialError") {
  MockBasis2 m(1);
  m.regs[21] = 0;
  m.write_fails = true;
  al::Driver<MockBasis2> d(m, 1);
  CHECK(d.WriteSingleRegister(21, 5).error == al::DriverError::SerialError);
  const uint16_t regs[2] = {1, 2};
  CHECK(d.WriteMultipleRegisters(21, regs, 2).error == al::DriverError::SerialError);
  CHECK_EQ(m.timeouts, 0);
  CHECK_EQ(m.regs[21], 0);
}

TEST_CASE("A broadcast that was not sent is not reported as done") {
  MockBasis2 m(1);
  m.write_fails = true;
  al::Driver<MockBasis2> d(m, al::kBroadcastAddress);
  CHECK(d.WriteSingleRegister(21, 5).error == al::DriverError::SerialError);
  const uint16_t regs[1] = {1};
  CHECK(d.WriteMultipleRegisters(21, regs, 1).error == al::DriverError::SerialError);
}

TEST_CASE("High-level calls pass the SerialError through") {
  MockBasis2 m(1);
  m.write_fails = true;
  al::Driver<MockBasis2> d(m, 1);
  CHECK(d.ReadIdentity().error == al::DriverError::SerialError);
}

TEST_CASE("The bus works again once write() succeeds") {
  MockBasis2 m(1);
  m.regs[2100] = 7;
  m.write_fails = true;
  al::Driver<MockBasis2> d(m, 1);
  uint16_t out = 0;
  CHECK(d.ReadHoldingRegisters(2100, &out, 1) == al::DriverError::SerialError);
  m.write_fails = false;
  CHECK(d.ReadHoldingRegisters(2100, &out, 1) == al::DriverError::None);
  CHECK_EQ(out, 7);
}
