#pragma once

#include <cstddef>
#include <string_view>

namespace invariants::interpreter {

class Interpreter {
 private:
  bool hadErr = false;
  void report(std::size_t line, std::string_view where, std::string_view msg);

 public:
  void run(const std::string_view src);
};

}  // namespace invariants::interpreter