# Alicat BASIS-2 Driver API reference {#mainpage}

The classes, functions and register constants of the HardFOC Alicat BASIS-2 driver, generated from the headers.

Start with alicat_basis2::Driver, one object per instrument on the bus, and alicat_basis2::UartInterface, the
interface your platform port implements. Results and decoded data are in alicat_basis2_types.hpp
(alicat_basis2::DriverResult, alicat_basis2::InstantaneousData); register addresses are in the
alicat_basis2::reg namespace and the Modbus-RTU framing in alicat_basis2::modbus.

The guides, with examples that compile, are on the documentation site: <a href="../">Alicat BASIS-2 Driver
documentation</a>.
