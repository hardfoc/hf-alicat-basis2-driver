/**
 * @file test_harness.hpp
 * @brief Minimal dependency-free test registry and check macros for the host unit tests.
 */
#pragma once

#include <cstdio>
#include <functional>
#include <string>
#include <vector>

namespace basis2_test {

struct TestCase {
  const char* name;
  std::function<void()> fn;
};

inline std::vector<TestCase>& registry() {
  static std::vector<TestCase> tests;
  return tests;
}

inline int& failureCount() {
  static int failures = 0;
  return failures;
}

struct Registrar {
  Registrar(const char* name, std::function<void()> fn) {
    registry().push_back({name, std::move(fn)});
  }
};

inline void reportFailure(const char* file, int line, const std::string& what) {
  ++failureCount();
  std::printf("    FAIL %s:%d: %s\n", file, line, what.c_str());
}

} // namespace basis2_test

#define BASIS2_TEST_CONCAT2(a, b) a##b
#define BASIS2_TEST_CONCAT(a, b) BASIS2_TEST_CONCAT2(a, b)

/// Define a test case.
#define TEST_CASE(name)                                                                            \
  static void BASIS2_TEST_CONCAT(test_fn_, __LINE__)();                                               \
  static const ::basis2_test::Registrar BASIS2_TEST_CONCAT(test_reg_, __LINE__)(                     \
      name, &BASIS2_TEST_CONCAT(test_fn_, __LINE__));                                                 \
  static void BASIS2_TEST_CONCAT(test_fn_, __LINE__)()

/// Non-fatal boolean check.
#define CHECK(cond)                                                                                \
  do {                                                                                             \
    if (!(cond)) {                                                                                 \
      ::basis2_test::reportFailure(__FILE__, __LINE__, #cond);                                    \
    }                                                                                              \
  } while (0)

/// Non-fatal equality check with values printed as unsigned long long.
#define CHECK_EQ(a, b)                                                                             \
  do {                                                                                             \
    const auto basis2_test_a = (a);                                                                   \
    const auto basis2_test_b = (b);                                                                   \
    if (!(basis2_test_a == basis2_test_b)) {                                                             \
      ::basis2_test::reportFailure(                                                               \
          __FILE__, __LINE__,                                                                      \
          std::string(#a " == " #b " (") +                                                         \
              std::to_string(static_cast<unsigned long long>(basis2_test_a)) + " vs " +               \
              std::to_string(static_cast<unsigned long long>(basis2_test_b)) + ")");                  \
    }                                                                                              \
  } while (0)

/// Fatal check: stop the current test case.
#define REQUIRE(cond)                                                                              \
  do {                                                                                             \
    if (!(cond)) {                                                                                 \
      ::basis2_test::reportFailure(__FILE__, __LINE__, #cond);                                    \
      return;                                                                                      \
    }                                                                                              \
  } while (0)
