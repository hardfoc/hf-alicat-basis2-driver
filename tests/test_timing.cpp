/**
 * @file test_timing.cpp
 * @brief The Modbus-RTU inter-frame gap (Modbus over Serial Line V1.02, section 2.5.1.1).
 */
#include "test_harness.hpp"

#include "mock_basis2.hpp"

namespace al = alicat_basis2;
namespace mb = alicat_basis2::modbus;

TEST_CASE("t3.5 is 3.5 11-bit characters up to 19200 baud and a fixed 1750 us above") {
  CHECK_EQ(mb::InterFrameGapUs(4800), 8021U);  // 38.5e6 / 4800 = 8020.8
  CHECK_EQ(mb::InterFrameGapUs(9600), 4011U);  // 4010.4
  CHECK_EQ(mb::InterFrameGapUs(19200), 2006U); // 2005.2
  CHECK_EQ(mb::InterFrameGapUs(19201), 1750U);
  CHECK_EQ(mb::InterFrameGapUs(38400), 1750U);
  CHECK_EQ(mb::InterFrameGapUs(115200), 1750U);
  CHECK_EQ(mb::InterFrameGapUs(0), 8021U); // unknown: the longest gap
  CHECK_EQ(mb::InterFrameGapMs(38400), 2U);
  CHECK_EQ(mb::InterFrameGapMs(19200), 3U);
  CHECK_EQ(mb::InterFrameGapMs(9600), 5U);
  CHECK_EQ(mb::InterFrameGapMs(4800), 9U);
}

TEST_CASE("every request waits t3.5 after the previous frame, from the bus baud rate") {
  MockBasis2 m(1);
  m.regs[25] = 0x0312;
  m.regs[39] = 0;
  al::Driver<MockBasis2> d(m, 1);
  CHECK_EQ(d.GetBusBaudRate(), 38400U); // factory default
  CHECK_EQ(d.GetInterFrameGapUs(), 1750U);

  uint16_t fw = 0;
  REQUIRE(d.ReadHoldingRegisters(al::reg::FirmwareVersion, &fw, 1) == al::DriverError::None);
  REQUIRE(d.Tare().ok());
  REQUIRE(m.idle_before_write.size() == 2U);
  CHECK(m.idle_before_write[0] >= 2U); // 1.75 ms, rounded up to whole ms
  CHECK(m.idle_before_write[1] >= 2U);

  d.SetBusBaudRate(9600);
  REQUIRE(d.Tare().ok());
  CHECK(m.idle_before_write.back() >= 5U); // 4.01 ms

  al::Driver<MockBasis2> broadcast(m, al::kBroadcastAddress);
  broadcast.SetBusBaudRate(19200);
  REQUIRE(broadcast.Tare().ok()); // no reply: the gap still separates the frames
  REQUIRE(broadcast.Tare().ok());
  CHECK(m.idle_before_write.back() >= 3U); // 2.01 ms
}

TEST_CASE("SetBaudRate switches the driver's gap to the new rate") {
  MockBasis2 m(1);
  m.regs[21] = 3;
  m.regs[39] = 0;
  al::Driver<MockBasis2> d(m, 1);
  REQUIRE(d.SetBaudRate(al::BaudRate::Bps_4800).ok());
  CHECK_EQ(d.GetBusBaudRate(), 4800U);
  REQUIRE(d.Tare().ok());
  CHECK(m.idle_before_write.back() >= 9U); // 8.02 ms
}
