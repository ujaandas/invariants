#include "parser.hpp"

#include <memory>

#include "expression.hpp"
#include "token.hpp"

using namespace invariants::parser;
using invariants::ast::ExprPtr;

namespace {
invariants::ast::BinaryOp tokenToBinaryOp(invariants::lexer::TokenType type) {
  using TT = invariants::lexer::TokenType;
  using BO = invariants::ast::BinaryOp;

  switch (type) {
    case TT::PLUS:
      return BO::Add;
    case TT::MINUS:
      return BO::Subtract;
    case TT::STAR:
      return BO::Multiply;
    case TT::SLASH:
      return BO::Divide;
    case TT::PERCENTAGE:
      return BO::Modulo;

    case TT::LOGICAL_AND:
      return BO::And;
    case TT::LOGICAL_OR:
      return BO::Or;

    case TT::EQUAL_EQUAL:
      return BO::Equal;
    case TT::BANG_EQUAL:
      return BO::NotEqual;

    case TT::GREATER:
      return BO::Greater;
    case TT::GREATER_EQUAL:
      return BO::GreaterEqual;
    case TT::LESS:
      return BO::Less;
    case TT::LESS_EQUAL:
      return BO::LessEqual;

    case TT::KW_IN:
      return BO::In;
    case TT::KW_NOT_IN:
      return BO::NotIn;

    default:
      // TODO: make this nicer!
      throw std::runtime_error("Invalid binary operator token");
  }
}
}  // namespace

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

ExprPtr Parser::disjunction() {
  ExprPtr left = conjunction();

  while (match(lexer::TokenType::LOGICAL_OR)) {
    ExprPtr right = conjunction();

    left = std::make_unique<ast::Expr>(
        ast::BinaryExpr{std::move(left), ast::BinaryOp::Or, std::move(right)});
  }

  return left;
}

ExprPtr Parser::conjunction() {
  ExprPtr left = equality();

  while (match(lexer::TokenType::LOGICAL_AND)) {
    ExprPtr right = equality();

    left = std::make_unique<ast::Expr>(
        ast::BinaryExpr{std::move(left), ast::BinaryOp::And, std::move(right)});
  }

  return left;
}

ExprPtr Parser::equality() {
  ExprPtr left = comparison();

  while (match(lexer::TokenType::EQUAL_EQUAL, lexer::TokenType::BANG_EQUAL)) {
    ExprPtr right = comparison();

    auto prevToken = previous();
    auto op = tokenToBinaryOp(prevToken.getType());

    left = std::make_unique<ast::Expr>(
        ast::BinaryExpr{std::move(left), op, std::move(right)});
  }

  return left;
}

ExprPtr Parser::comparison() {
  ExprPtr left = membership();

  while (match(lexer::TokenType::GREATER, lexer::TokenType::GREATER_EQUAL,
               lexer::TokenType::LESS, lexer::TokenType::LESS_EQUAL)) {
    ExprPtr right = membership();

    auto prevToken = previous();
    auto op = tokenToBinaryOp(prevToken.getType());

    left = std::make_unique<ast::Expr>(
        ast::BinaryExpr{std::move(left), op, std::move(right)});
  }

  return left;
}

ExprPtr Parser::membership() {
  ExprPtr left = term();

  if (match(lexer::TokenType::KW_IN, lexer::TokenType::KW_NOT_IN)) {
    ExprPtr right = term();

    auto prevToken = previous();
    auto op = tokenToBinaryOp(prevToken.getType());

    return std::make_unique<ast::Expr>(
        ast::BinaryExpr{std::move(left), op, std::move(right)});
  }

  return left;
}

// ExprPtr Parser::term() {}
// ExprPtr Parser::factor() {}
// ExprPtr Parser::unary() {}
// ExprPtr Parser::postfix() {}
// ExprPtr Parser::postfixOp() {}
// ExprPtr Parser::primary() {}
// ExprPtr Parser::literal() {}
// ExprPtr Parser::list() {}