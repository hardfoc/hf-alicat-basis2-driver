/**
 * @file test_modbus.cpp
 * @brief Modbus-RTU framing: CRC-16, request builders and response parsers.
 */
#include "test_harness.hpp"

#include "alicat_basis2_modbus.hpp"

namespace mb = alicat_basis2::modbus;
using alicat_basis2::DriverError;

TEST_CASE("CRC-16 matches the Modbus specification example") {
  // Modbus over Serial Line V1.02: 11 03 00 6B 00 03 -> CRC 0x8776, sent low byte first.
  const uint8_t req[] = {0x11, 0x03, 0x00, 0x6B, 0x00, 0x03};
  CHECK_EQ(mb::Crc16(req, sizeof(req)), 0x8776);
  const uint8_t one[] = {0x01, 0x03, 0x00, 0x00, 0x00, 0x0A};
  CHECK_EQ(mb::Crc16(one, sizeof(one)), 0xCDC5);
}

TEST_CASE("read-holding request is address, 03, register, count, CRC low then high") {
  uint8_t f[8]{};
  REQUIRE(mb::BuildReadHolding(0x11, 0x006B, 3, f, sizeof(f)) == 8);
  const uint8_t want[] = {0x11, 0x03, 0x00, 0x6B, 0x00, 0x03, 0x76, 0x87};
  for (int i = 0; i < 8; ++i) {
    CHECK_EQ(f[i], want[i]);
  }
  CHECK(mb::CheckCrc(f, 8));
  CHECK_EQ(mb::BuildReadHolding(1, 0, 1, f, 7), 0U);  // buffer too small
}

TEST_CASE("write-multiple request carries the byte count and big-endian values") {
  const uint16_t regs[] = {0x0007, 0xA120};  // 500000 = 500 SCCM setpoint
  uint8_t f[13]{};
  REQUIRE(mb::BuildWriteMultiple(1, 2053, regs, 2, f, sizeof(f)) == 13);
  CHECK_EQ(f[1], 0x10);
  CHECK_EQ(f[2], 0x08);  // 2053 = 0x0805
  CHECK_EQ(f[3], 0x05);
  CHECK_EQ(f[5], 2);
  CHECK_EQ(f[6], 4);
  CHECK_EQ(f[7], 0x00);
  CHECK_EQ(f[8], 0x07);
  CHECK_EQ(f[9], 0xA1);
  CHECK_EQ(f[10], 0x20);
  CHECK(mb::CheckCrc(f, 13));
}

TEST_CASE("read-holding parser classifies exceptions, CRC errors and wrong slaves") {
  uint8_t ex[5] = {0x01, 0x83, 0x02, 0, 0};
  mb::AppendCrc(ex, 3);
  uint16_t out[1]{};
  CHECK(mb::ParseReadHolding(ex, 5, 1, 1, out) == DriverError::ModbusException);

  uint8_t ok[7] = {0x01, 0x03, 0x02, 0x12, 0x34, 0, 0};
  mb::AppendCrc(ok, 5);
  CHECK(mb::ParseReadHolding(ok, 7, 1, 1, out) == DriverError::None);
  CHECK_EQ(out[0], 0x1234);
  CHECK(mb::ParseReadHolding(ok, 7, 2, 1, out) == DriverError::ResponseMismatch);
  CHECK(mb::ParseReadHolding(ok, 7, 1, 2, out) == DriverError::InvalidFrame);
  ok[6] ^= 0x01;
  CHECK(mb::ParseReadHolding(ok, 7, 1, 1, out) == DriverError::BadCrc);
}
