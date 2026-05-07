#include "parser.hpp"

#include <memory>

#include "expression.hpp"
#include "token.hpp"

using namespace invariants::parser;
using ExprPtr = invariants::ast::ExprPtr;
using Token = invariants::lexer::Token;
using TT = invariants::lexer::TokenType;

namespace {
invariants::ast::BinaryOp tokenToBinaryOp(TT type) {
  using TT = TT;
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

Parser::Parser(const std::vector<Token>& tokens) : tokens(tokens) {}

Token Parser::peek() const {
  if (curr >= tokens.size()) {
    throw std::runtime_error("Unexpected end of token stream");
  }

  return tokens[curr];
}
Token Parser::advance() {
  if (!isAtEnd()) {
    curr++;
  }
  return previous();
}

Token Parser::previous() { return tokens[curr - 1]; }

bool Parser::check(TT type) const {
  if (isAtEnd()) {
    return false;
  }
  return peek().getType() == type;
}

bool Parser::isAtEnd() const {
  return curr >= tokens.size() || tokens[curr].getType() == TT::EOF_TOKEN;
}
ExprPtr Parser::parse() { return expression(); }

ExprPtr Parser::expression() { return implication(); }

ExprPtr Parser::implication() {
  ExprPtr left = disjunction();

  if (match(TT::ARROW)) {
    ExprPtr right = implication();

    return std::make_unique<ast::Expr>(ast::BinaryExpr{
        std::move(left), ast::BinaryOp::Imply, std::move(right)});
  }

  return left;
}

ExprPtr Parser::disjunction() {
  ExprPtr left = conjunction();

  while (match(TT::LOGICAL_OR)) {
    ExprPtr right = conjunction();

    left = std::make_unique<ast::Expr>(
        ast::BinaryExpr{std::move(left), ast::BinaryOp::Or, std::move(right)});
  }

  return left;
}

ExprPtr Parser::conjunction() {
  ExprPtr left = equality();

  while (match(TT::LOGICAL_AND)) {
    ExprPtr right = equality();

    left = std::make_unique<ast::Expr>(
        ast::BinaryExpr{std::move(left), ast::BinaryOp::And, std::move(right)});
  }

  return left;
}

ExprPtr Parser::equality() {
  ExprPtr left = comparison();

  while (match(TT::EQUAL_EQUAL, TT::BANG_EQUAL)) {
    auto opToken = previous();

    ExprPtr right = comparison();

    auto op = tokenToBinaryOp(opToken.getType());

    left = std::make_unique<ast::Expr>(
        ast::BinaryExpr{std::move(left), op, std::move(right)});
  }

  return left;
}

ExprPtr Parser::comparison() {
  ExprPtr left = membership();

  while (match(TT::GREATER, TT::GREATER_EQUAL, TT::LESS, TT::LESS_EQUAL)) {
    auto opToken = previous();

    ExprPtr right = membership();

    auto op = tokenToBinaryOp(opToken.getType());

    left = std::make_unique<ast::Expr>(
        ast::BinaryExpr{std::move(left), op, std::move(right)});
  }

  return left;
}

ExprPtr Parser::membership() {
  ExprPtr left = term();

  if (match(TT::KW_IN, TT::KW_NOT_IN)) {
    ExprPtr right = term();

    auto prevToken = previous();
    auto op = tokenToBinaryOp(prevToken.getType());

    return std::make_unique<ast::Expr>(
        ast::BinaryExpr{std::move(left), op, std::move(right)});
  }

  return left;
}

ExprPtr Parser::term() {
  ExprPtr left = factor();

  while (match(TT::PLUS, TT::MINUS)) {
    auto opToken = previous();

    ExprPtr right = factor();

    auto op = tokenToBinaryOp(opToken.getType());

    left = std::make_unique<ast::Expr>(
        ast::BinaryExpr{std::move(left), op, std::move(right)});
  }

  return left;
}

ExprPtr Parser::factor() {
  ExprPtr left = unary();

  while (match(TT::STAR, TT::SLASH, TT::PERCENTAGE)) {
    ExprPtr right = unary();

    auto prevToken = previous();
    auto op = tokenToBinaryOp(prevToken.getType());

    left = std::make_unique<ast::Expr>(
        ast::BinaryExpr{std::move(left), op, std::move(right)});
  }

  return left;
}

ExprPtr Parser::unary() {
  if (match(TT::BANG, TT::MINUS)) {
    ExprPtr operand = unary();

    auto prevToken = previous();
    ast::UnaryOp op = prevToken.getType() == TT::BANG ? ast::UnaryOp::Not
                                                      : ast::UnaryOp::Negate;

    return std::make_unique<ast::Expr>(ast::UnaryExpr{op, std::move(operand)});
  }

  return postfix();
}

ExprPtr Parser::postfix() {
  ExprPtr base = primary();

  std::vector<ast::PostfixOp> ops;

  while (true) {
    if (match(TT::DOT)) {
      auto token = advance();
      if (token.getType() != TT::LIT_IDENTIFIER) {
        throw std::runtime_error("Expected identifier after '.'");
      }
      ops.push_back(ast::MemberAccessOp{previous().getLexeme()});
    } else if (match(TT::LEFT_BRACKET)) {
      ExprPtr index = expression();
      if (!match(TT::RIGHT_BRACKET)) {
        throw std::runtime_error("Expected ']' after index expression");
      }
      ops.push_back(ast::IndexOp{std::move(index)});
    } else {
      break;
    }
  }

  if (ops.empty()) {
    return base;
  }

  return std::make_unique<ast::Expr>(
      ast::PostfixExpr{std::move(base), std::move(ops)});
}

ExprPtr Parser::primary() {
  if (match(TT::LIT_BOOLEAN_T)) {
    return std::make_unique<ast::Expr>(ast::LiteralExpr{true});
  }

  if (match(TT::LIT_BOOLEAN_F)) {
    return std::make_unique<ast::Expr>(ast::LiteralExpr{false});
  }

  if (match(TT::LIT_NULL)) {
    return std::make_unique<ast::Expr>(ast::LiteralExpr{nullptr});
  }

  if (match(TT::LIT_NUMBER)) {
    auto token = previous();
    auto value = std::get<double>(token.getLiteral());
    return std::make_unique<ast::Expr>(ast::LiteralExpr{value});
  }

  if (match(TT::LIT_INTEGER)) {
    auto token = previous();
    auto value = static_cast<double>(std::get<int>(token.getLiteral()));
    return std::make_unique<ast::Expr>(ast::LiteralExpr{value});
  }

  if (match(TT::LIT_STRING)) {
    auto token = previous();
    auto value = std::get<std::string>(token.getLiteral());
    return std::make_unique<ast::Expr>(ast::LiteralExpr{value});
  }

  if (match(TT::LIT_IDENTIFIER)) {
    auto token = previous();
    auto name = std::get<std::string>(token.getLiteral());
    return std::make_unique<ast::Expr>(ast::IdentifierExpr{name});
  }

  if (match(TT::KW_THIS)) {
    return std::make_unique<ast::Expr>(ast::ThisExpr{});
  }

  if (check(TT::LEFT_BRACKET)) {
    return list();
  }

  if (match(TT::LEFT_PAREN)) {
    ExprPtr expr = expression();
    if (!match(TT::RIGHT_PAREN)) {
      throw std::runtime_error("Expected ')' after expression");
    }
    return std::make_unique<ast::Expr>(ast::GroupingExpr{std::move(expr)});
  }

  throw std::runtime_error("Expected primary expression");
}

ExprPtr Parser::list() {
  if (!match(TT::LEFT_BRACKET)) {
    throw std::runtime_error("Expected '['");
  }

  std::vector<ast::ExprPtr> elements;

  if (!check(TT::RIGHT_BRACKET)) {
    do {
      elements.push_back(expression());
    } while (match(TT::COMMA));
  }

  if (!match(TT::RIGHT_BRACKET)) {
    throw std::runtime_error("Expected ']' after list");
  }

  return std::make_unique<ast::Expr>(ast::ListExpr{std::move(elements)});
}