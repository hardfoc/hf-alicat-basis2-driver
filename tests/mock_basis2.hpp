/**
 * @file mock_basis2.hpp
 * @brief A model of a BASIS-2 Modbus-RTU slave behind a UART, for the host unit tests.
 *
 * The mock answers function codes 03, 06 and 16 the way the operation manual (Rev 4) describes:
 * holding registers in a map, the write-single reply echoes the request, the write-multiple reply
 * carries address and count, an unknown register gets exception 02 (illegal data address) and a
 * frame for another address gets no reply at all. Every request the driver sends is recorded.
 */
#pragma once

#include "alicat_basis2.hpp"

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <map>
#include <vector>

class MockBasis2 : public alicat_basis2::UartInterface<MockBasis2> {
public:
  explicit MockBasis2(uint8_t addr = 1) noexcept : address(addr) {}

  // --- UartInterface ----------------------------------------------------------------------------

  void write(const uint8_t* data, std::size_t length) noexcept {
    std::vector<uint8_t> frame(data, data + length);
    requests.push_back(frame);
    respond(frame);
  }

  // Like a real UART read: returns at once when `max` bytes are there, otherwise after the whole
  // timeout with what arrived. Every expired timeout is counted, and its length added to
  // `waited_ms`.
  std::size_t read(uint8_t* out, std::size_t max, uint32_t timeout_ms) noexcept {
    const std::size_t n = std::min(max, rx_.size());
    std::copy_n(rx_.begin(), n, out);
    rx_.erase(rx_.begin(), rx_.begin() + static_cast<std::ptrdiff_t>(n));
    if (n < max) {
      ++timeouts;
      waited_ms += timeout_ms;
    }
    return n;
  }

  void flush_rx() noexcept { rx_.clear(); }

  // --- Test controls ----------------------------------------------------------------------------

  std::map<uint16_t, uint16_t> regs;          ///< Holding registers the instrument implements.
  std::vector<std::vector<uint8_t>> requests; ///< Every frame the driver wrote.
  bool corrupt_crc = false;                   ///< Flip a CRC bit in the next replies.
  unsigned timeouts = 0;                      ///< read() calls that waited the full timeout.
  unsigned long waited_ms = 0;                ///< Total time read() spent waiting for bytes.
  uint8_t address;                            ///< Modbus address the mock answers to.

private:
  void respond(const std::vector<uint8_t>& f) {
    if (f.size() < 8 || !alicat_basis2::modbus::CheckCrc(f.data(), f.size())) {
      return;  // a slave ignores a frame with a bad CRC
    }
    if (f[0] != address) {
      return;  // another slave's frame, or a broadcast: no reply
    }
    const uint8_t fn = f[1];
    const uint16_t start = static_cast<uint16_t>((f[2] << 8) | f[3]);
    std::vector<uint8_t> r{f[0], fn};

    if (fn == 0x03) {
      const uint16_t count = static_cast<uint16_t>((f[4] << 8) | f[5]);
      for (uint16_t i = 0; i < count; ++i) {
        if (regs.find(static_cast<uint16_t>(start + i)) == regs.end()) {
          return exception(f[0], fn, 0x02);
        }
      }
      r.push_back(static_cast<uint8_t>(count * 2));
      for (uint16_t i = 0; i < count; ++i) {
        const uint16_t v = regs[static_cast<uint16_t>(start + i)];
        r.push_back(static_cast<uint8_t>(v >> 8));
        r.push_back(static_cast<uint8_t>(v & 0xFF));
      }
    } else if (fn == 0x06) {
      if (regs.find(start) == regs.end()) {
        return exception(f[0], fn, 0x02);
      }
      regs[start] = static_cast<uint16_t>((f[4] << 8) | f[5]);
      if (start == alicat_basis2::reg::ModbusAddress) {
        address = static_cast<uint8_t>(regs[start]);
      }
      r.assign(f.begin(), f.begin() + 6);  // the reply echoes the request
    } else if (fn == 0x10) {
      const uint16_t count = static_cast<uint16_t>((f[4] << 8) | f[5]);
      for (uint16_t i = 0; i < count; ++i) {
        if (regs.find(static_cast<uint16_t>(start + i)) == regs.end()) {
          return exception(f[0], fn, 0x02);
        }
        regs[static_cast<uint16_t>(start + i)] =
            static_cast<uint16_t>((f[7 + i * 2] << 8) | f[8 + i * 2]);
      }
      r.insert(r.end(), f.begin() + 2, f.begin() + 6);
    } else {
      return exception(f[0], fn, 0x01);
    }
    send(r);
  }

  void exception(uint8_t addr, uint8_t fn, uint8_t code) {
    send({addr, static_cast<uint8_t>(fn | 0x80), code});
  }

  void send(std::vector<uint8_t> r) {
    const std::size_t len = r.size();
    r.resize(len + 2);
    alicat_basis2::modbus::AppendCrc(r.data(), len);
    if (corrupt_crc) {
      r.back() ^= 0x01;
    }
    rx_ = r;
  }

  std::vector<uint8_t> rx_;
};
