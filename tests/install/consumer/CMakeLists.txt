# A downstream project that uses the installed driver through find_package, as the Installation page shows.
cmake_minimum_required(VERSION 3.16)
project(hf_alicat_basis2_install_consumer LANGUAGES CXX)

find_package(hf_alicat_basis2 1.0 REQUIRED CONFIG)
if(NOT TARGET hf::alicat_basis2)
  message(FATAL_ERROR "find_package(hf_alicat_basis2) did not define hf::alicat_basis2")
endif()

add_executable(consumer main.cpp)
target_link_libraries(consumer PRIVATE hf::alicat_basis2)
target_compile_options(consumer PRIVATE -Wall -Wextra)
