#pragma once

#include <string_view>

namespace invariants::interpreter {

class Interpreter {
 public:
  void run(const std::string_view src);
};

}  // namespace invariants::interpreter