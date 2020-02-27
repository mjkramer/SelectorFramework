#pragma once

#include <stdexcept>
#include <string>

#define ASSERT(cond)                                            \
  if (not (cond)) {                                             \
    const auto msg = std::string("Assertion failed: ") + #cond; \
    throw std::runtime_error(msg);                              \
  }
