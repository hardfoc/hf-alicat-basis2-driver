/**
 * @file api_tasks.cpp
 * @brief The API guide's task snippets, compiled by the host build (never run).
 */
#include "my_uart.hpp"

#include <array>
#include <cstdio>

using Driver = alicat_basis2::Driver<MyUart>;
namespace al = alicat_basis2;

void set_host_baud(uint32_t bps) noexcept;  // your UART driver's baud-rate call

void bus(MyUart& uart) {
  // --8<-- [start:bus]
  Driver flow_in(uart, 1);             // one Driver per instrument, all on the same port
  Driver flow_out(uart, 2, 100);       // a 100 ms response timeout for this one

  flow_out.SetTimeoutMs(250);          // change it later
  // --8<-- [end:bus]
  (void)flow_in;
}

bool identity(Driver& meter, al::InstrumentIdentity& id) {
  // --8<-- [start:identity]
  const auto r = meter.ReadIdentity();  // registers 25, 26..31 and 45..49: three requests
  if (!r.ok()) {
    return false;
  }
  id = r.value;
  const double full_scale = id.full_scale_flow_raw / 1000.0;  // registers 47-48: value / 1000
  std::printf("FW %u.%u.%u  SN %s  full scale %.3f %s\n", id.fw_major, id.fw_minor, id.fw_patch,
              id.serial_number, full_scale, al::ToString(id.flow_units).data());
  // --8<-- [end:identity]
  return true;
}

void read(Driver& meter) {
  // --8<-- [start:read]
  const auto r = meter.ReadInstantaneous(3);  // 3: the instrument's factory-set flow decimals
  if (r.ok()) {
    const al::InstantaneousData& d = r.value;
    std::printf("%s %.2f C flow %.3f total %.3f setpoint %.3f valve %.1f %%\n",
                al::GasShortName(d.gas).data(), static_cast<double>(d.temperature_c),
                static_cast<double>(d.flow), static_cast<double>(d.total_volume),
                static_cast<double>(d.setpoint), static_cast<double>(d.valve_drive_pct));
    if (d.mass_flow_overrange || d.temperature_overrange || d.totalizer_overrange) {
      std::printf("over range\n");
    }
  }
  // --8<-- [end:read]
}

bool setpoint(Driver& controller) {
  // --8<-- [start:setpoint]
  // Controllers only. Order: source, watchdog, setpoint.
  if (!controller.SetSetpointSource(al::SetpointSource::DigitalUnsaved).ok()) {
    return false;                      // a meter answers ModbusException
  }
  if (!controller.SetCommWatchdogMs(1000).ok()) {  // zero setpoint after 1 s without traffic
    return false;
  }
  return controller.SetSetpoint(2.5f).ok();        // 2.5 in the instrument's flow units
  // --8<-- [end:setpoint]
}

void tare(Driver& meter) {
  // --8<-- [start:tare]
  // With the process gas in the line and no flow:
  if (const auto r = meter.Tare(); !r.ok()) {
    std::printf("tare: %s\n", al::ToString(r.error).data());
  }
  meter.ResetTotalizer();                                  // total volume back to 0
  meter.SetTotalizerLimitMode(al::TotalizerLimitMode::SaturateWithOvr);
  // --8<-- [end:tare]
}

void measurement(Driver& controller) {
  // --8<-- [start:measurement]
  controller.ConfigureMeasurementTrigger(al::MeasurementTrigger::OnDigitalSetpointChange);
  controller.StartMeasurementSamples(400);   // 400 samples at about 2.5 ms: about 1 s
  // ... wait at least as long as the window ...
  const auto m = controller.ReadMeasurement(3); // 3: the same flow decimals as live data
  if (m.ok()) {
    std::printf("%u samples: min %d max %d (raw flow), avg flow %.3f, avg %.2f C\n",
                m.value.num_samples_collected, m.value.min_flow_raw, m.value.max_flow_raw,
                static_cast<double>(m.value.avg_flow),
                static_cast<double>(m.value.avg_temperature_c));
    std::printf("previous window: avg flow %ld (raw)\n",
                static_cast<long>(m.value.prev_avg_flow_raw));
  }
  // --8<-- [end:measurement]
}

void discover(Driver& probe) {
  // --8<-- [start:discover]
  std::array<uint8_t, 32> present{};   // bit N set: an instrument answered at address N
  const auto r = probe.DiscoverPresentAddresses(present.data(), present.size());
  if (r.ok()) {
    for (unsigned a = 1; a <= al::kMaxModbusAddress; ++a) {
      if (present[a / 8] & (1U << (a % 8))) {
        std::printf("instrument at %u\n", a);
      }
    }
  }
  // --8<-- [end:discover]
}

bool readdress(Driver& meter) {
  // --8<-- [start:address]
  // Only this instrument on the bus, or only one at the old address.
  if (!meter.SetModbusAddress(12).ok()) {
    return false;
  }
  return meter.GetAddress() == 12;     // the driver now talks to address 12
  // --8<-- [end:address]
}

bool rebaud(Driver& meter) {
  // --8<-- [start:baud]
  // The instrument switches as soon as it receives the write, so its reply may be garbled.
  meter.SetBaudRate(al::BaudRate::Bps_115200);
  set_host_baud(al::BaudRateToBps(al::BaudRate::Bps_115200));
  return meter.ReadIdentity().ok();    // confirm at the new rate
  // --8<-- [end:baud]
}

void raw(Driver& meter) {
  // --8<-- [start:raw]
  uint16_t full_scale_sccm[2]{};       // registers 35-36: full scale in SCCM, unsigned 32-bit
  if (meter.ReadHoldingRegisters(al::reg::FullScaleFlowSccm, full_scale_sccm, 2) == al::DriverError::None) {
    const uint32_t sccm = (static_cast<uint32_t>(full_scale_sccm[0]) << 16) | full_scale_sccm[1];
    std::printf("full scale %lu SCCM\n", static_cast<unsigned long>(sccm));
  }
  meter.WriteSingleRegister(al::reg::TareSampleCount, 400);  // register 51: samples per tare
  // --8<-- [end:raw]
}

void errors(Driver& meter) {
  // --8<-- [start:errors]
  const auto r = meter.ReadInstantaneous();
  if (!r.ok()) {
    switch (r.error) {
    case al::DriverError::Timeout:         // no reply: wiring, baud rate, address, timeout
    case al::DriverError::BadCrc:          // noise, termination, a second slave answering
      break;
    case al::DriverError::ModbusException: // the instrument refused: register, value, meter vs controller
      std::printf("exception %s\n", al::ToString(meter.GetLastException()).data());
      break;
    default:
      std::printf("%s\n", al::ToString(r.error).data());
      break;
    }
  }
  // --8<-- [end:errors]
}
