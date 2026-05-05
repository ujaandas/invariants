#pragma once

#include <vector>

#include "expression.hpp"
#include "token.hpp"

namespace invariants::parser {

class Parser {
 private:
  int curr = 0;
  std::vector<lexer::Token> tokens;

  template <typename... Ts>
    requires(std::same_as<Ts, lexer::TokenType> && ...)
  bool match(Ts... tokens);

  lexer::Token previous();

  // Expressions
  ast::ExprPtr expression();
  ast::ExprPtr implication();
  ast::ExprPtr disjunction();
  ast::ExprPtr conjunction();
  ast::ExprPtr equality();
  ast::ExprPtr comparison();
  ast::ExprPtr membership();
  ast::ExprPtr term();
  ast::ExprPtr factor();
  ast::ExprPtr unary();
  ast::ExprPtr postfix();
  ast::ExprPtr postfixOp();
  ast::ExprPtr primary();
  ast::ExprPtr literal();
  ast::ExprPtr list();

 public:
  explicit Parser(const std::vector<lexer::Token>& tokens);
  ast::ExprPtr parse();
};

}  // namespace invariants::parser