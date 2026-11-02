# Testing and CI

How the driver is checked: host unit tests against a model of the instrument, compiled documentation, ESP32 builds
and static analysis, on every pull request.

## Host unit tests

The tests build the real driver against a mock UART that behaves like a BASIS-2 Modbus slave: holding registers
in a map, an echo for function 06, address and count for function 16, exception 02 for a register the model does
not have, silence for another address, and a switch to corrupt the CRC.

```bash
cmake -S . -B build-host -DHF_ALICAT_BASIS2_BUILD_TESTS=ON
cmake --build build-host
ctest --test-dir build-host --output-on-failure
```

| File | Covers |
| --- | --- |
| [`tests/test_modbus.cpp`](../tests/test_modbus.cpp) | CRC-16 against the Modbus specification example, request builders, reply parsing |
| [`tests/test_driver.cpp`](../tests/test_driver.cpp) | Identity, live data scaling, setpoint encoding, magic values, range checks, address change, errors, discovery |
| [`tests/test_hooks.cpp`](../tests/test_hooks.cpp) | The optional port hooks `delay_ms_impl()` and `log_impl()`, the driver's failure logs |
| [`tests/test_timing.cpp`](../tests/test_timing.cpp) | The 3.5-character inter-frame gap before every request |
| [`tests/mock_basis2.hpp`](../tests/mock_basis2.hpp) | The instrument model |
| [`tests/install/`](../tests/install/run_install_test.cmake) | CTest `hf_alicat_basis2_install_consumer`: `cmake --install` to a temporary prefix, every header present, a `find_package()` consumer builds |

## Documentation snippets

Every C++ block on this site is included from [`tests/docs/`](../tests/docs/my_uart.hpp), which the host build
compiles (target `hf_alicat_basis2_doc_snippets`) with the driver instantiated over the documented port. A change
to the API that breaks a page breaks the build first.

## CI workflows

| Workflow | What it checks |
| --- | --- |
| Host unit tests | GCC and Clang build and run the tests and the doc snippets |
| ESP32 examples build | Every app in `examples/esp32/app_config.yml`, for each listed ESP-IDF version and build type |
| C++ lint, C++ analysis | clang-format, clang-tidy and cppcheck |
| Docs | Strict site build, figure and generated-table drift, Doxygen, link check, Lighthouse budget |
| Markdown lint, YAML lint, link check | Repository text files |

The workflows call the shared, versioned workflows of
[hf-general-ci-tools](https://github.com/hardfoc/hf-general-ci-tools) and
[hf-espidf-ci-tools](https://github.com/hardfoc/hf-espidf-ci-tools).

## On hardware

The [ESP32 examples](examples.md) are the hardware test: `alicat_basis2_full_example` walks every feature and logs
the result of each call over the serial monitor. Run it on a meter and on a controller after any change to the
driver.
