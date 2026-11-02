# =============================================================================
# Install check (run by CTest): install the driver to a temporary prefix, check that every public header is
# there, then configure and build a consumer that uses find_package(hf_alicat_basis2) and hf::alicat_basis2.
#
#   cmake -DBUILD_DIR=<driver build> -DSOURCE_DIR=<driver source> -DWORK_DIR=<scratch>
#         -DCXX_COMPILER=<c++> -P run_install_test.cmake
# =============================================================================
cmake_minimum_required(VERSION 3.16)

foreach(var BUILD_DIR SOURCE_DIR WORK_DIR CXX_COMPILER)
  if(NOT DEFINED ${var})
    message(FATAL_ERROR "run_install_test.cmake: ${var} is not set")
  endif()
endforeach()

set(prefix "${WORK_DIR}/prefix")
set(consumer_build "${WORK_DIR}/consumer-build")
file(REMOVE_RECURSE "${WORK_DIR}")

function(run)
  execute_process(COMMAND ${ARGN} RESULT_VARIABLE rc OUTPUT_VARIABLE out ERROR_VARIABLE err)
  if(NOT rc EQUAL 0)
    message(FATAL_ERROR "failed (${rc}): ${ARGN}\n${out}\n${err}")
  endif()
endfunction()

run("${CMAKE_COMMAND}" --install "${BUILD_DIR}" --prefix "${prefix}")

# Every header in inc/ and the generated version header must be installed.
file(GLOB headers RELATIVE "${SOURCE_DIR}/inc" "${SOURCE_DIR}/inc/*.hpp" "${SOURCE_DIR}/inc/*.h"
     "${SOURCE_DIR}/inc/*.ipp")
list(APPEND headers alicat_basis2_version.h)
foreach(h IN LISTS headers)
  if(NOT EXISTS "${prefix}/include/hf_alicat_basis2/${h}")
    message(FATAL_ERROR "not installed: include/hf_alicat_basis2/${h}")
  endif()
endforeach()

run("${CMAKE_COMMAND}" -S "${SOURCE_DIR}/tests/install/consumer" -B "${consumer_build}"
    "-DCMAKE_PREFIX_PATH=${prefix}" "-DCMAKE_CXX_COMPILER=${CXX_COMPILER}")
run("${CMAKE_COMMAND}" --build "${consumer_build}")
message(STATUS "install check passed: ${prefix}")
file(REMOVE_RECURSE "${WORK_DIR}")
