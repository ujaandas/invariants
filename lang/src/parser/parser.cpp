#include "parser.hpp"

#include <memory>

#include "expression.hpp"
#include "token.hpp"

using namespace invariants::parser;
using invariants::ast::ExprPtr;

Parser::Parser(const std::vector<lexer::Token>& tokens) : tokens(tokens) {}

ExprPtr Parser::expression() { return implication(); }

ExprPtr Parser::implication() {
  ExprPtr left = disjunction();

  if (match(lexer::TokenType::ARROW)) {
    ExprPtr right = implication();

    return std::make_unique<ast::Expr>(ast::BinaryExpr{
        std::move(left), ast::BinaryOp::Imply, std::move(right)});
  }

  return left;
}

// ExprPtr Parser::disjunction() {
//   ExprPtr left = conjunction();

//   if (match(lexer::TokenType::)) }
// ExprPtr Parser::conjunction() {}
// ExprPtr Parser::equality() {}
// ExprPtr Parser::comparison() {}
// ExprPtr Parser::membership() {}
// ExprPtr Parser::term() {}
// ExprPtr Parser::factor() {}
// ExprPtr Parser::unary() {}
// ExprPtr Parser::postfix() {}
// ExprPtr Parser::postfixOp() {}
// ExprPtr Parser::primary() {}
// ExprPtr Parser::literal() {}
// ExprPtr Parser::list() {}