#pragma once

#include <concepts>
#include <vector>

#include "expression.hpp"
#include "statements.hpp"
#include "token.hpp"

namespace invariants::parser {

class Parser {
 private:
  int curr = 0;
  std::vector<lexer::Token> tokens;

  template <typename... Ts>
    requires(std::same_as<Ts, lexer::TokenType> && ...)
  bool match(Ts... types) {
    auto matches = [this](lexer::TokenType type) { return check(type); };

    if ((matches(types) || ...)) {
      advance();
      return true;
    }
    return false;
  }

  lexer::Token previous();
  lexer::Token peek() const;
  lexer::Token advance();
  bool check(lexer::TokenType type) const;
  bool isAtEnd() const;

  // Types
  ast::TypePtr type();
  ast::TypePtr primaryType();
  ast::TypePtr simpleType();
  ast::TypePtr arrayType();
  ast::TypePtr mapType();

  // Statements
  ast::ModulePtr module();
  ast::SpecPtr spec();
  ast::SpecMember specMember();
  ast::FieldPtr field();
  ast::InvariantPtr invariant();
  ast::ConstraintPtr constraint();

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
  ast::ExprPtr primary();
  ast::ExprPtr list();

 public:
  explicit Parser(const std::vector<lexer::Token>& tokens);
  ast::ExprPtr parseExpr();
  ast::ModulePtr parseModule();
  ast::TypePtr parseType();
};

}  // namespace invariants::parser