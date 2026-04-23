
#include "interpreter.hpp"

#include <print>

#include "lexer.hpp"

namespace invariants::interpreter {

// TODO: implement and wire up centralized interp error reporting

void Interpreter::run(std::string_view src) {
  lexer::Lexer lexer(src);
  auto tokens = lexer.scanTokens();

  for (const auto& token : tokens) {
    std::println("{}", token.toString());
  }
}

}  // namespace invariants::interpreter