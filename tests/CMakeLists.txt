# =============================================================================
# hf-alicat-basis2-driver — host unit tests
# =============================================================================
# Builds the driver headers against a mock BASIS-2 Modbus slave and runs the
# tests with CTest. No hardware and no ESP-IDF needed:
#
#   cmake -S . -B build-host -DHF_ALICAT_BASIS2_BUILD_TESTS=ON
#   cmake --build build-host
#   ctest --test-dir build-host --output-on-failure
# =============================================================================

add_executable(hf_alicat_basis2_host_tests
  test_main.cpp
  test_modbus.cpp
  test_driver.cpp
  test_hooks.cpp
  test_timing.cpp
)

target_link_libraries(hf_alicat_basis2_host_tests PRIVATE ${HF_ALICAT_BASIS2_TARGET_NAME})
target_include_directories(hf_alicat_basis2_host_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_features(hf_alicat_basis2_host_tests PRIVATE cxx_std_17)
target_compile_options(hf_alicat_basis2_host_tests PRIVATE -Wall -Wextra)

add_test(NAME hf_alicat_basis2_host_tests COMMAND hf_alicat_basis2_host_tests)

# The code shown on the documentation site (docs/ includes these files with pymdownx.snippets), built so a
# page cannot show code that does not compile against the current headers. Compiled, never linked or run.
add_library(hf_alicat_basis2_doc_snippets OBJECT
  docs/quickstart.cpp
  docs/api_tasks.cpp
  docs/ports.cpp
  docs/configuration.cpp
)
target_link_libraries(hf_alicat_basis2_doc_snippets PRIVATE ${HF_ALICAT_BASIS2_TARGET_NAME})
target_include_directories(hf_alicat_basis2_doc_snippets PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/docs)
target_compile_features(hf_alicat_basis2_doc_snippets PRIVATE cxx_std_17)
target_compile_options(hf_alicat_basis2_doc_snippets PRIVATE -Wall -Wextra)

# Install the driver to a temporary prefix and build a find_package() consumer against it (the documented way to
# use an installed driver). Needs the install rules (HF_ALICAT_BASIS2_INSTALL, on for a top-level build).
if(HF_ALICAT_BASIS2_INSTALL)
  add_test(NAME hf_alicat_basis2_install_consumer
    COMMAND ${CMAKE_COMMAND}
      -DBUILD_DIR=${PROJECT_BINARY_DIR}
      -DSOURCE_DIR=${PROJECT_SOURCE_DIR}
      -DWORK_DIR=${CMAKE_CURRENT_BINARY_DIR}/install-check
      -DCXX_COMPILER=${CMAKE_CXX_COMPILER}
      -P ${CMAKE_CURRENT_SOURCE_DIR}/install/run_install_test.cmake)
endif()
