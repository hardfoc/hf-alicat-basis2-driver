# =============================================================================
# hf-alicat-basis2-driver — Root CMakeLists.txt
# =============================================================================
# Hardware-agnostic header-only driver for Alicat BASIS-2 mass-flow meters
# and controllers (B-Series / BC-Series). Provides a Modbus-RTU + ASCII
# serial command interface bridged to any byte-level transport via a CRTP
# adapter (see inc/alicat_basis2_uart_interface.hpp).
# For ESP-IDF integration, register the driver as a component using the
# pattern under examples/esp32 or via cmake/hf_alicat_basis2_build_settings.cmake.
# =============================================================================
cmake_minimum_required(VERSION 3.16)

project(hf_alicat_basis2
  VERSION 1.0.0
  LANGUAGES CXX
)

include(${CMAKE_CURRENT_LIST_DIR}/cmake/hf_alicat_basis2_build_settings.cmake)

include(GNUInstallDirs)

# Installed headers go to <prefix>/include/hf_alicat_basis2/ (consumers still write #include "alicat_basis2.hpp").
set(HF_ALICAT_BASIS2_INSTALL_INCLUDEDIR "${CMAKE_INSTALL_INCLUDEDIR}/hf_alicat_basis2")

add_library(${HF_ALICAT_BASIS2_TARGET_NAME} INTERFACE)
foreach(dir IN LISTS HF_ALICAT_BASIS2_PUBLIC_INCLUDE_DIRS)
  target_include_directories(${HF_ALICAT_BASIS2_TARGET_NAME} INTERFACE "$<BUILD_INTERFACE:${dir}>")
endforeach()
target_include_directories(${HF_ALICAT_BASIS2_TARGET_NAME}
    INTERFACE "$<INSTALL_INTERFACE:${HF_ALICAT_BASIS2_INSTALL_INCLUDEDIR}>")
target_compile_features(${HF_ALICAT_BASIS2_TARGET_NAME} INTERFACE cxx_std_17)
# Exported as hf::alicat_basis2, the same name as the alias below.
set_target_properties(${HF_ALICAT_BASIS2_TARGET_NAME} PROPERTIES EXPORT_NAME alicat_basis2)

add_library(hf::alicat_basis2 ALIAS ${HF_ALICAT_BASIS2_TARGET_NAME})

option(HF_ALICAT_BASIS2_ENABLE_WARNINGS "Enable compiler warnings for BASIS-2 consumers" OFF)
if(HF_ALICAT_BASIS2_ENABLE_WARNINGS)
  target_compile_options(${HF_ALICAT_BASIS2_TARGET_NAME}
      INTERFACE -Wall -Wextra -Wpedantic)
endif()

include(CMakePackageConfigHelpers)

write_basic_package_version_file(
  "${CMAKE_CURRENT_BINARY_DIR}/hf_alicat_basis2ConfigVersion.cmake"
  VERSION ${PROJECT_VERSION}
  COMPATIBILITY SameMajorVersion
)

set(HF_ALICAT_BASIS2_INSTALL_CMAKEDIR "${CMAKE_INSTALL_LIBDIR}/cmake/hf_alicat_basis2")

configure_package_config_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/cmake/hf_alicat_basis2Config.cmake.in"
  "${CMAKE_CURRENT_BINARY_DIR}/hf_alicat_basis2Config.cmake"
  INSTALL_DESTINATION "${HF_ALICAT_BASIS2_INSTALL_CMAKEDIR}"
)

# ── Install: headers, generated version header, exported target, package config ─
# On by default only when this is the top-level project, so add_subdirectory() users do not install it.
if(CMAKE_SOURCE_DIR STREQUAL PROJECT_SOURCE_DIR)
  set(HF_ALICAT_BASIS2_INSTALL_DEFAULT ON)
else()
  set(HF_ALICAT_BASIS2_INSTALL_DEFAULT OFF)
endif()
option(HF_ALICAT_BASIS2_INSTALL "Generate the install rules" ${HF_ALICAT_BASIS2_INSTALL_DEFAULT})

if(HF_ALICAT_BASIS2_INSTALL)
  # Every public header, so a header added to inc/ cannot be left out of the install.
  file(GLOB HF_ALICAT_BASIS2_PUBLIC_HEADERS CONFIGURE_DEPENDS
       "${CMAKE_CURRENT_SOURCE_DIR}/inc/*.hpp"
       "${CMAKE_CURRENT_SOURCE_DIR}/inc/*.h"
       "${CMAKE_CURRENT_SOURCE_DIR}/inc/*.ipp")
  install(FILES ${HF_ALICAT_BASIS2_PUBLIC_HEADERS} "${HF_ALICAT_BASIS2_VERSION_HEADER}"
          DESTINATION "${HF_ALICAT_BASIS2_INSTALL_INCLUDEDIR}")
  install(TARGETS ${HF_ALICAT_BASIS2_TARGET_NAME} EXPORT hf_alicat_basis2Targets)
  install(EXPORT hf_alicat_basis2Targets
          NAMESPACE hf::
          FILE hf_alicat_basis2Targets.cmake
          DESTINATION "${HF_ALICAT_BASIS2_INSTALL_CMAKEDIR}")
  install(FILES
            "${CMAKE_CURRENT_BINARY_DIR}/hf_alicat_basis2Config.cmake"
            "${CMAKE_CURRENT_BINARY_DIR}/hf_alicat_basis2ConfigVersion.cmake"
          DESTINATION "${HF_ALICAT_BASIS2_INSTALL_CMAKEDIR}")
endif()

# ── Host unit tests (mock UART; no hardware) ─────────────────────────────────
option(HF_ALICAT_BASIS2_BUILD_TESTS "Build the host unit tests in tests/" OFF)
if(HF_ALICAT_BASIS2_BUILD_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif()
