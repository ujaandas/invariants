#include "parser.hpp"

#include <memory>

#include "expression.hpp"
#include "statements.hpp"
#include "token.hpp"

using namespace invariants::parser;
using namespace invariants::ast;

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

ExprPtr Parser::parseExpr() { return expression(); }

ModulePtr Parser::parseModule() { return module(); }

ExprPtr Parser::expression() { return implication(); }

ExprPtr Parser::implication() {
  ExprPtr left = disjunction();

  if (match(TT::ARROW)) {
    ExprPtr right = implication();

    return std::make_unique<Expr>(
        BinaryExpr{std::move(left), BinaryOp::Imply, std::move(right)});
  }

  return left;
}

ExprPtr Parser::disjunction() {
  ExprPtr left = conjunction();

  while (match(TT::LOGICAL_OR)) {
    ExprPtr right = conjunction();

    left = std::make_unique<Expr>(
        BinaryExpr{std::move(left), BinaryOp::Or, std::move(right)});
  }

  return left;
}

ExprPtr Parser::conjunction() {
  ExprPtr left = equality();

  while (match(TT::LOGICAL_AND)) {
    ExprPtr right = equality();

    left = std::make_unique<Expr>(
        BinaryExpr{std::move(left), BinaryOp::And, std::move(right)});
  }

  return left;
}

ExprPtr Parser::equality() {
  ExprPtr left = comparison();

  while (match(TT::EQUAL_EQUAL, TT::BANG_EQUAL)) {
    auto opToken = previous();

    ExprPtr right = comparison();

    auto op = tokenToBinaryOp(opToken.getType());

    left = std::make_unique<Expr>(
        BinaryExpr{std::move(left), op, std::move(right)});
  }

  return left;
}

ExprPtr Parser::comparison() {
  ExprPtr left = membership();

  while (match(TT::GREATER, TT::GREATER_EQUAL, TT::LESS, TT::LESS_EQUAL)) {
    auto opToken = previous();

    ExprPtr right = membership();

    auto op = tokenToBinaryOp(opToken.getType());

    left = std::make_unique<Expr>(
        BinaryExpr{std::move(left), op, std::move(right)});
  }

  return left;
}

ExprPtr Parser::membership() {
  ExprPtr left = term();

  if (match(TT::KW_IN, TT::KW_NOT_IN)) {
    auto opToken = previous();

    ExprPtr right = term();

    auto op = tokenToBinaryOp(opToken.getType());

    return std::make_unique<Expr>(
        BinaryExpr{std::move(left), op, std::move(right)});
  }

  return left;
}

ExprPtr Parser::term() {
  ExprPtr left = factor();

  while (match(TT::PLUS, TT::MINUS)) {
    auto opToken = previous();

    ExprPtr right = factor();

    auto op = tokenToBinaryOp(opToken.getType());

    left = std::make_unique<Expr>(
        BinaryExpr{std::move(left), op, std::move(right)});
  }

  return left;
}

ExprPtr Parser::factor() {
  ExprPtr left = unary();

  while (match(TT::STAR, TT::SLASH, TT::PERCENTAGE)) {
    auto opToken = previous();

    ExprPtr right = unary();

    auto op = tokenToBinaryOp(opToken.getType());

    left = std::make_unique<Expr>(
        BinaryExpr{std::move(left), op, std::move(right)});
  }

  return left;
}

ExprPtr Parser::unary() {
  if (match(TT::BANG, TT::MINUS)) {
    ExprPtr operand = unary();

    auto prevToken = previous();
    UnaryOp op =
        prevToken.getType() == TT::BANG ? UnaryOp::Not : UnaryOp::Negate;

    return std::make_unique<Expr>(UnaryExpr{op, std::move(operand)});
  }

  return postfix();
}

ExprPtr Parser::postfix() {
  ExprPtr base = primary();

  std::vector<PostfixOp> ops;

  while (true) {
    if (match(TT::DOT)) {
      auto token = advance();
      if (token.getType() != TT::LIT_IDENTIFIER) {
        throw std::runtime_error("Expected identifier after '.'");
      }
      ops.push_back(MemberAccessOp{previous().getLexeme()});
    } else if (match(TT::LEFT_BRACKET)) {
      ExprPtr index = expression();
      if (!match(TT::RIGHT_BRACKET)) {
        throw std::runtime_error("Expected ']' after index expression");
      }
      ops.push_back(IndexOp{std::move(index)});
    } else {
      break;
    }
  }

  if (ops.empty()) {
    return base;
  }

  return std::make_unique<Expr>(PostfixExpr{std::move(base), std::move(ops)});
}

ExprPtr Parser::primary() {
  if (match(TT::LIT_BOOLEAN_T)) {
    return std::make_unique<Expr>(LiteralExpr{true});
  }

  if (match(TT::LIT_BOOLEAN_F)) {
    return std::make_unique<Expr>(LiteralExpr{false});
  }

  if (match(TT::LIT_NULL)) {
    return std::make_unique<Expr>(LiteralExpr{nullptr});
  }

  if (match(TT::LIT_NUMBER)) {
    auto token = previous();
    auto value = std::get<double>(token.getLiteral());
    return std::make_unique<Expr>(LiteralExpr{value});
  }

  if (match(TT::LIT_INTEGER)) {
    auto token = previous();
    auto value = static_cast<double>(std::get<int>(token.getLiteral()));
    return std::make_unique<Expr>(LiteralExpr{value});
  }

  if (match(TT::LIT_STRING)) {
    auto token = previous();
    auto value = std::get<std::string>(token.getLiteral());
    return std::make_unique<Expr>(LiteralExpr{value});
  }

  if (match(TT::LIT_IDENTIFIER)) {
    auto token = previous();
    auto name = std::get<std::string>(token.getLiteral());
    return std::make_unique<Expr>(IdentifierExpr{name});
  }

  if (match(TT::KW_THIS)) {
    return std::make_unique<Expr>(ThisExpr{});
  }

  if (check(TT::LEFT_BRACKET)) {
    return list();
  }

  if (match(TT::LEFT_PAREN)) {
    ExprPtr expr = expression();
    if (!match(TT::RIGHT_PAREN)) {
      throw std::runtime_error("Expected ')' after expression");
    }
    return std::make_unique<Expr>(GroupingExpr{std::move(expr)});
  }

  throw std::runtime_error("Expected primary expression");
}

ExprPtr Parser::list() {
  if (!match(TT::LEFT_BRACKET)) {
    throw std::runtime_error("Expected '['");
  }

  std::vector<ExprPtr> elements;

  if (!check(TT::RIGHT_BRACKET)) {
    do {
      elements.push_back(expression());
    } while (match(TT::COMMA));
  }

  if (!match(TT::RIGHT_BRACKET)) {
    throw std::runtime_error("Expected ']' after list");
  }

  return std::make_unique<Expr>(ListExpr{std::move(elements)});
}

// Statements
ModulePtr Parser::module() {
  std::vector<SpecPtr> specs;

  while (!isAtEnd() && check(TT::KW_SPEC)) {
    specs.push_back(spec());
  }

  return std::make_unique<ModuleStmt>(ModuleStmt{std::move(specs)});
}

SpecPtr Parser::spec() {
  if (!match(TT::KW_SPEC)) {
    throw std::runtime_error("Expected 'spec'");
  }

  if (!match(TT::LIT_IDENTIFIER)) {
    throw std::runtime_error("Expected identifier after 'spec'");
  }

  auto nameTok = previous();
  auto name = std::get<std::string>(nameTok.getLiteral());

  if (!match(TT::LEFT_BRACE)) {
    throw std::runtime_error("Expected '{' after spec identifier");
  }

  std::vector<SpecMember> members;

  while (!isAtEnd() && !check(TT::RIGHT_BRACE)) {
    members.push_back(specMember());
  }

  if (!match(TT::RIGHT_BRACE)) {
    throw std::runtime_error("Expected '}' after spec body");
  }

  return std::make_unique<SpecStmt>(SpecStmt{name, std::move(members)});
}

SpecMember Parser::specMember() {
  if (check(TT::KW_FIELD)) {
    auto fptr = field();
    return SpecMember{std::move(*fptr)};
  }

  if (check(TT::KW_INVARIANT)) {
    auto iptr = invariant();
    return SpecMember{std::move(*iptr)};
  }

  throw std::runtime_error("Expected spec member (field | invariant)");
}

FieldPtr Parser::field() {
  if (!match(TT::KW_FIELD)) {
    throw std::runtime_error("Expected 'field'");
  }

  if (!match(TT::LIT_IDENTIFIER)) {
    throw std::runtime_error("Expected identifier after 'field'");
  }

  auto idTok = previous();
  auto identifier = std::get<std::string>(idTok.getLiteral());

  if (!match(TT::COLON)) {
    throw std::runtime_error("Expected ':' after field identifier");
  }

  auto typeNode = type();

  if (!match(TT::LEFT_BRACE)) {
    throw std::runtime_error("Expected '{' to start field constraints");
  }

  std::vector<ConstraintPtr> constraints;

  while (!isAtEnd() && !check(TT::RIGHT_BRACE)) {
    constraints.push_back(constraint());
  }

  if (!match(TT::RIGHT_BRACE)) {
    throw std::runtime_error("Expected '}' after field constraints");
  }

  return std::make_unique<FieldStmt>(
      FieldStmt{identifier, std::move(typeNode), std::move(constraints)});
}

InvariantPtr Parser::invariant() {
  if (!match(TT::KW_INVARIANT)) {
    throw std::runtime_error("Expected 'invariant'");
  }

  if (!match(TT::LIT_IDENTIFIER)) {
    throw std::runtime_error("Expected identifier after 'invariant'");
  }

  auto idTok = previous();
  auto identifier = std::get<std::string>(idTok.getLiteral());

  if (!match(TT::LEFT_BRACE)) {
    throw std::runtime_error("Expected '{' to start invariant constraints");
  }

  std::vector<ConstraintPtr> constraints;

  while (!isAtEnd() && !check(TT::RIGHT_BRACE)) {
    constraints.push_back(constraint());
  }

  if (!match(TT::RIGHT_BRACE)) {
    throw std::runtime_error("Expected '}' after invariant constraints");
  }

  return std::make_unique<InvariantStmt>(
      InvariantStmt{identifier, std::move(constraints)});
}

ConstraintPtr Parser::constraint() {
  auto expr = expression();

  if (!match(TT::SEMICOLON)) {
    throw std::runtime_error("Expected ';' after constraint expression");
  }

  return std::make_unique<ConstraintStmt>(ConstraintStmt{std::move(expr)});
}

TypePtr Parser::type() { return primaryType(); }

TypePtr Parser::primaryType() {
  if (match(TT::LEFT_PAREN)) {
    auto inner = type();
    if (!match(TT::RIGHT_PAREN)) {
      throw std::runtime_error("Expected ')' after type");
    }
    return inner;
  }

  if (check(TT::KW_ARRAY)) {
    return arrayType();
  }

  if (check(TT::KW_MAP)) {
    return mapType();
  }

  return simpleType();
}

TypePtr Parser::simpleType() {
  if (match(TT::KW_NUMBER)) {
    return std::make_unique<Type>(SimpleType{BuiltinType::Number});
  }
  if (match(TT::KW_INTEGER)) {
    return std::make_unique<Type>(SimpleType{BuiltinType::Integer});
  }
  if (match(TT::KW_STRING)) {
    return std::make_unique<Type>(SimpleType{BuiltinType::String});
  }
  if (match(TT::KW_BOOLEAN)) {
    return std::make_unique<Type>(SimpleType{BuiltinType::Boolean});
  }

  if (match(TT::LIT_IDENTIFIER)) {
    auto token = previous();
    auto name = std::get<std::string>(token.getLiteral());
    return std::make_unique<Type>(SimpleType{std::move(name)});
  }

  throw std::runtime_error("Expected type");
}

TypePtr Parser::arrayType() {
  if (!match(TT::KW_ARRAY)) {
    throw std::runtime_error("Expected 'Array'");
  }
  if (!match(TT::LESS)) {
    throw std::runtime_error("Expected '<' after Array");
  }

  auto element = type();

  if (!match(TT::GREATER)) {
    throw std::runtime_error("Expected '>' after array element type");
  }

  return std::make_unique<Type>(ArrayType{std::move(element)});
}

TypePtr Parser::mapType() {
  if (!match(TT::KW_MAP)) {
    throw std::runtime_error("Expected 'Map'");
  }
  if (!match(TT::LESS)) {
    throw std::runtime_error("Expected '<' after Map");
  }

  auto key = type();

  if (!match(TT::COMMA)) {
    throw std::runtime_error("Expected ',' after map key type");
  }

  auto value = type();

  if (!match(TT::GREATER)) {
    throw std::runtime_error("Expected '>' after map value type");
  }

  return std::make_unique<Type>(MapType{std::move(key), std::move(value)});
}