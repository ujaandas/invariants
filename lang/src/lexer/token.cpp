#include "token.hpp"

#include <string>
#include <variant>

using namespace invariants::lexer;

namespace {
std::string literalToString(const Literal& lit) {
  return std::visit(
      [](const auto& value) -> std::string {
        using T = std::decay_t<decltype(value)>;

        if constexpr (std::is_same_v<T, std::monostate>) {
          return "nil";
        } else if constexpr (std::is_same_v<T, std::string>) {
          return value;
        } else if constexpr (std::is_same_v<T, double>) {
          return std::to_string(value);
        } else if constexpr (std::is_same_v<T, bool>) {
          return value ? "true" : "false";
        }
      },
      lit);
}
}  // namespace

Token::Token(TokenType type, std::string lexeme, Literal literal,
             std::size_t line)
    : type(type),
      lexeme(std::move(lexeme)),
      literal(std::move(literal)),
      line(line) {}

std::string Token::toString() const {
  return std::to_string(static_cast<int>(type)) + " " + lexeme + " " +
         literalToString(literal);
}