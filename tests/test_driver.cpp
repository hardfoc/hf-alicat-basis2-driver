/**
 * @file test_driver.cpp
 * @brief Driver calls end to end against the mock BASIS-2 slave.
 */
#include "test_harness.hpp"

#include "mock_basis2.hpp"

namespace al = alicat_basis2;

namespace {

/// A controller at address 1 with every register the driver uses.
MockBasis2 makeController() {
  MockBasis2 m(1);
  for (uint16_t r : {21, 25, 39, 45, 46, 47, 48, 49, 52, 53, 54, 55, 80, 514, 515, 516, 521, 522, 524,
                     525, 2053, 2054}) {
    m.regs[r] = 0;
  }
  for (uint16_t r = 26; r <= 31; ++r) {
    m.regs[r] = 0;
  }
  for (uint16_t r = 2100; r <= 2109; ++r) {
    m.regs[r] = 0;
  }
  for (uint16_t r = 4200; r <= 4215; ++r) {
    m.regs[r] = 0;
  }
  return m;
}

} // namespace

TEST_CASE("ReadIdentity decodes firmware, serial number, address, unit id, full scale and units") {
  MockBasis2 m = makeController();
  m.regs[25] = 256 * 3 + 16 * 1 + 2;  // V3.1.2
  const char sn[] = "BC1234567890";
  for (int i = 0; i < 6; ++i) {
    m.regs[static_cast<uint16_t>(26 + i)] = static_cast<uint16_t>((sn[i * 2] << 8) | sn[i * 2 + 1]);
  }
  m.regs[45] = 1;
  m.regs[46] = 'A';
  m.regs[47] = 0x0000;
  m.regs[48] = 0x4E20;  // 20000 -> 20 (flow units)
  m.regs[49] = 2;       // SLPM

  al::Driver<MockBasis2> d(m, 1);
  const auto id = d.ReadIdentity();
  REQUIRE(id.ok());
  CHECK_EQ(id.value.fw_major, 3);
  CHECK_EQ(id.value.fw_minor, 1);
  CHECK_EQ(id.value.fw_patch, 2);
  CHECK(std::strcmp(id.value.serial_number, "BC1234567890") == 0);
  CHECK_EQ(id.value.ascii_unit_id, 'A');
  CHECK_EQ(id.value.full_scale_flow_raw, 20000U);
  CHECK(id.value.flow_units == al::FlowUnits::SLPM);
}

TEST_CASE("ReadInstantaneous reads 2100..2109 in one request and scales temperature and flow") {
  MockBasis2 m = makeController();
  m.regs[2100] = 3;            // N2
  m.regs[2101] = 0x0001 | 0x0008;  // MOV, HLD
  m.regs[2102] = 2457;         // 24.57 C
  m.regs[2103] = 12345;        // 12.345 with 3 decimals
  m.regs[2107] = 5513;         // 55.13 %
  al::Driver<MockBasis2> d(m, 1);
  const auto live = d.ReadInstantaneous(3);
  REQUIRE(live.ok());
  CHECK_EQ(m.requests.size(), 1U);
  CHECK(live.value.gas == al::Gas::Nitrogen);
  CHECK(live.value.mass_flow_overrange);
  CHECK(live.value.valve_held);
  CHECK(!live.value.temperature_overrange);
  CHECK(live.value.temperature_c > 24.56f && live.value.temperature_c < 24.58f);
  CHECK(live.value.flow > 12.344f && live.value.flow < 12.346f);
  CHECK(live.value.valve_drive_pct > 55.12f && live.value.valve_drive_pct < 55.14f);
}

TEST_CASE("SetSetpoint writes value x 1000 as a signed 32-bit integer to 2053-2054") {
  MockBasis2 m = makeController();
  al::Driver<MockBasis2> d(m, 1);
  REQUIRE(d.SetSetpoint(500.0f).ok());  // manual example: 500 SCCM -> 500000
  CHECK_EQ(m.regs[2053], 0x0007);
  CHECK_EQ(m.regs[2054], 0xA120);
  CHECK_EQ(m.requests.back()[1], 0x10);
}

TEST_CASE("Tare and ResetTotalizer write 0xAA55; FactoryRestore writes 0x5214") {
  MockBasis2 m = makeController();
  al::Driver<MockBasis2> d(m, 1);
  REQUIRE(d.Tare().ok());
  CHECK_EQ(m.regs[39], 0xAA55);
  REQUIRE(d.ResetTotalizer().ok());
  CHECK_EQ(m.regs[53], 0xAA55);
  REQUIRE(d.FactoryRestore().ok());
  CHECK_EQ(m.regs[80], 0x5214);
}

TEST_CASE("range checks reject values the manual does not allow, without bus traffic") {
  MockBasis2 m = makeController();
  al::Driver<MockBasis2> d(m, 1);
  CHECK(d.SetCommWatchdogMs(5001).error == al::DriverError::InvalidParameter);
  CHECK(d.SetFlowAveragingMs(2501).error == al::DriverError::InvalidParameter);
  CHECK(d.SetReferenceTemperatureC(30.5f).error == al::DriverError::InvalidParameter);
  CHECK(d.SetModbusAddress(0).error == al::DriverError::InvalidAddress);
  CHECK(d.SetModbusAddress(248).error == al::DriverError::InvalidAddress);
  CHECK(d.SetAsciiUnitId('a').error == al::DriverError::InvalidParameter);
  CHECK(m.requests.empty());
}

TEST_CASE("SetModbusAddress retargets the driver after the instrument acknowledges") {
  MockBasis2 m = makeController();
  m.regs[45] = 1;
  al::Driver<MockBasis2> d(m, 1);
  REQUIRE(d.SetModbusAddress(17).ok());
  CHECK_EQ(d.GetAddress(), 17);
  CHECK_EQ(m.address, 17);
  CHECK(d.ReadIdentity().ok());
}

TEST_CASE("errors: exception for a missing register, timeout for a silent address, bad CRC") {
  MockBasis2 m = makeController();
  m.regs.erase(514);  // a meter has no watchdog register
  m.regs.erase(4210);
  al::Driver<MockBasis2> d(m, 1);
  CHECK(d.ReadMeasurement().error == al::DriverError::ModbusException);
  CHECK(d.GetLastException() == al::ModbusExceptionCode::IllegalDataAddress);
  CHECK(d.SetCommWatchdogMs(100).error == al::DriverError::ModbusException);

  al::Driver<MockBasis2> nobody(m, 9);
  CHECK(nobody.ReadInstantaneous().error == al::DriverError::Timeout);

  m.corrupt_crc = true;
  CHECK(d.ReadInstantaneous().error == al::DriverError::BadCrc);
}

TEST_CASE("DiscoverPresentAddresses finds the responding address and restores the driver") {
  MockBasis2 m = makeController();
  m.address = 42;
  al::Driver<MockBasis2> d(m, 1, 150);
  uint8_t bitmap[32]{};
  const auto found = d.DiscoverPresentAddresses(bitmap, sizeof(bitmap), 20, 40, 45);
  REQUIRE(found.ok());
  CHECK_EQ(found.value, 1);
  CHECK((bitmap[42 / 8] & (1U << (42 % 8))) != 0);
  CHECK_EQ(d.GetAddress(), 1);
  CHECK_EQ(d.GetTimeoutMs(), 150);
  CHECK_EQ(m.requests.size(), 6U);
}

TEST_CASE("exception replies to writes are recognised by length and returned without a timeout") {
  // Modbus Application Protocol V1.1b3 section 7: function | 0x80, exception code, CRC (5 bytes).
  MockBasis2 m = makeController();
  m.regs.erase(514); // a meter: no control registers
  m.regs.erase(2053);
  al::Driver<MockBasis2> d(m, 1);

  CHECK(d.SetCommWatchdogMs(100).error == al::DriverError::ModbusException); // function 06
  CHECK(d.GetLastException() == al::ModbusExceptionCode::IllegalDataAddress);
  CHECK(d.SetSetpoint(1.0f).error == al::DriverError::ModbusException); // function 16
  CHECK(d.GetLastException() == al::ModbusExceptionCode::IllegalDataAddress);
  m.regs.erase(25);
  uint16_t fw = 0;
  CHECK(d.ReadHoldingRegisters(al::reg::FirmwareVersion, &fw, 1) ==
        al::DriverError::ModbusException); // function 03
  CHECK_EQ(m.timeouts, 0U);                // every exception came back without waiting

  CHECK(d.Tare().ok()); // a good request clears the last exception
  CHECK(d.GetLastException() == al::ModbusExceptionCode::None);

  m.corrupt_crc = true; // a damaged exception reply is a CRC error, still without a timeout
  CHECK(d.SetCommWatchdogMs(100).error == al::DriverError::BadCrc);
  CHECK(d.GetLastException() == al::ModbusExceptionCode::None);
  CHECK_EQ(m.timeouts, 0U);
}
