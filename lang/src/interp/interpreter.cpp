
#include "interpreter.hpp"

#include <print>
// #include <string_view>

#include "lexer.hpp"

namespace invariants::interpreter {

// void Interpreter::report(std::size_t line, std::string_view where,
//                          std::string_view msg) {
//   std::println("[line %d] Error %s : %s", line, where, msg);
//   hadErr = true;
// }

void Interpreter::run(std::string_view src) {
  lexer::Lexer lexer(src);
  auto tokens = lexer.scanTokens();

  for (const auto& token : tokens) {
    std::println("{}", token.toString());
  }
}

}  // namespace invariants::interpreter