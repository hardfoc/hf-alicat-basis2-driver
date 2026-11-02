# Installation

Add the driver to a CMake or ESP-IDF project. It is a header-only C++17 library with no dependencies.

## Requirements

- A C++17 compiler: GCC 8+, Clang 7+ or a current MSVC. The driver uses `if constexpr`, `std::string_view` and
  `std::void_t`.
- CMake 3.16 or newer, for the version header and the target.
- No RTOS, heap or exceptions: every call is `noexcept` and returns `DriverResult<T>` or `DriverError`.

## Get the source

A git submodule keeps the driver pinned to a version you chose:

```bash
git submodule add https://github.com/hardfoc/hf-alicat-basis2-driver.git external/hf-alicat-basis2-driver
git submodule update --init external/hf-alicat-basis2-driver
```

Only `inc/`, `cmake/` and `CMakeLists.txt` are needed to build. The examples have their own submodule
(`examples/esp32/scripts`); you do not need it to use the driver.

## CMake

```cmake
add_subdirectory(external/hf-alicat-basis2-driver)
target_link_libraries(my_app PRIVATE hf::alicat_basis2)
```

`hf::alicat_basis2` is an alias of the `INTERFACE` target `hf_alicat_basis2`. It carries the include directories
(`inc/` and the generated version header) and requires C++17 (`cxx_std_17`), so your target does not need to set
either.

| Option | Default | Effect |
| --- | --- | --- |
| `HF_ALICAT_BASIS2_ENABLE_WARNINGS` | `OFF` | Adds `-Wall -Wextra -Wpedantic` to targets that link the driver |
| `HF_ALICAT_BASIS2_BUILD_TESTS` | `OFF` | Builds the host unit tests in `tests/` ([Testing and CI](testing.md)) |

## ESP-IDF

The ESP-IDF component wrapper is
[`examples/esp32/components/hf_alicat_basis2/`](../examples/esp32/components/hf_alicat_basis2/CMakeLists.txt).
Copy that folder into your project's `components/` (or point `EXTRA_COMPONENT_DIRS` at it) and set the
`HF_ALICAT_BASIS2_ROOT` cache variable to the driver checkout; the wrapper stops with a clear message when the
path is wrong. Your component then declares `REQUIRES hf_alicat_basis2`.

## How the build is organised

| Layer | File | Role |
| --- | --- | --- |
| Settings | [`cmake/hf_alicat_basis2_build_settings.cmake`](../cmake/hf_alicat_basis2_build_settings.cmake) | Target name, include directories, version: the single source of truth |
| Library | [`CMakeLists.txt`](../CMakeLists.txt) | `INTERFACE` target, package config, host tests |
| ESP-IDF | [`examples/esp32/components/hf_alicat_basis2/CMakeLists.txt`](../examples/esp32/components/hf_alicat_basis2/CMakeLists.txt) | IDF component wrapper over the same settings |

The version is set once in the settings file and written into `alicat_basis2_version.h` (in the build tree, under
`hf_alicat_basis2_generated/`) at configure time. Read it at run time:

```cpp
const char* v = alicat_basis2::GetDriverVersion();   // "1.0.0"
```

The same string is the macro `HF_ALICAT_BASIS2_VERSION_STRING`.

## Headers

| Header | Contents |
| --- | --- |
| `alicat_basis2.hpp` | `Driver<UartT>`; include this one, it includes the rest |
| `alicat_basis2_uart_interface.hpp` | `UartInterface<Derived>`, the base of your port |
| `alicat_basis2_types.hpp` | `DriverError`, `DriverResult<T>`, gases, units, baud rates, decoded data |
| `alicat_basis2_registers.hpp` | Register addresses (`alicat_basis2::reg`) and function codes |
| `alicat_basis2_modbus.hpp` | Frame builders, parsers and the CRC-16 (`alicat_basis2::modbus`) |

## Next

[Quickstart](quickstart.md) wires an instrument and reads flow; [Porting](porting.md) writes the serial interface
for your MCU.
