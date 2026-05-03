#include <gtest/gtest.h>

#include <cstddef>
#include <memory>
#include <string>
#include <utility>
#include <variant>

#include "expression.hpp"
#include "statements.hpp"
#include "types.hpp"

using namespace invariants::ast;

TEST(AstTest, LiteralNumberOk) {
  Expr e(LiteralExpr{3.5});
  const auto& number = std::get<LiteralExpr>(e.value);

  EXPECT_TRUE(std::holds_alternative<double>(number.value));
  EXPECT_EQ(std::get<double>(number.value), 3.5);
}

TEST(AstTest, LiteralStringOk) {
  Expr e(LiteralExpr{std::string("hi")});
  const auto& str = std::get<LiteralExpr>(e.value);

  EXPECT_TRUE(std::holds_alternative<std::string>(str.value));
  EXPECT_EQ(std::get<std::string>(str.value), "hi");
}

TEST(AstTest, LiteralBoolOk) {
  Expr e(LiteralExpr{true});
  const auto& flag = std::get<LiteralExpr>(e.value);

  EXPECT_TRUE(std::holds_alternative<bool>(flag.value));
  EXPECT_TRUE(std::get<bool>(flag.value));
}

TEST(AstTest, LiteralNullOk) {
  Expr e(LiteralExpr{nullptr});

  const auto& nil = std::get<LiteralExpr>(e.value);
  EXPECT_TRUE(std::holds_alternative<std::nullptr_t>(nil.value));
}

TEST(AstTest, IdentifierOk) {
  Expr e(IdentifierExpr{"foo"});

  const auto& ident = std::get<IdentifierExpr>(e.value);
  EXPECT_EQ(ident.name, "foo");
}

TEST(AstTest, ThisExprOk) {
  Expr e(ThisExpr{});

  EXPECT_TRUE(std::holds_alternative<ThisExpr>(e.value));
}

TEST(AstTest, ListExprOk) {
  std::vector<ExprPtr> elements;
  elements.emplace_back(std::make_unique<Expr>(LiteralExpr{1.0}));
  elements.emplace_back(std::make_unique<Expr>(LiteralExpr{2.0}));

  Expr e(ListExpr{std::move(elements)});

  const auto& list = std::get<ListExpr>(e.value);
  ASSERT_EQ(list.elements.size(), 2u);

  const auto& first = std::get<LiteralExpr>(list.elements[0]->value);
  const auto& second = std::get<LiteralExpr>(list.elements[1]->value);

  EXPECT_EQ(std::get<double>(first.value), 1.0);
  EXPECT_EQ(std::get<double>(second.value), 2.0);
}

TEST(AstTest, GroupingExprOk) {
  Expr e(GroupingExpr{std::make_unique<Expr>(LiteralExpr{42.0})});

  const auto& group = std::get<GroupingExpr>(e.value);
  const auto& inner = std::get<LiteralExpr>(group.expression->value);

  EXPECT_EQ(std::get<double>(inner.value), 42.0);
}

TEST(AstTest, PostfixExprMemberAccessOk) {
  std::vector<PostfixOp> ops;
  ops.emplace_back(MemberAccessOp{"length"});

  Expr e(PostfixExpr{std::make_unique<Expr>(IdentifierExpr{"list"}),
                     std::move(ops)});

  const auto& postfix = std::get<PostfixExpr>(e.value);
  ASSERT_EQ(postfix.ops.size(), 1u);

  const auto& op = std::get<MemberAccessOp>(postfix.ops[0]);
  EXPECT_EQ(op.member, "length");
}

TEST(AstTest, PostfixExprIndexOk) {
  std::vector<PostfixOp> ops;
  ops.emplace_back(IndexOp{std::make_unique<Expr>(LiteralExpr{0.0})});

  Expr e(PostfixExpr{std::make_unique<Expr>(IdentifierExpr{"list"}),
                     std::move(ops)});

  const auto& postfix = std::get<PostfixExpr>(e.value);
  ASSERT_EQ(postfix.ops.size(), 1u);

  const auto& op = std::get<IndexOp>(postfix.ops[0]);
  const auto& index = std::get<LiteralExpr>(op.index->value);

  EXPECT_EQ(std::get<double>(index.value), 0.0);
}

TEST(AstTest, UnaryExprOk) {
  Expr e(UnaryExpr{UnaryOp::Negate, std::make_unique<Expr>(LiteralExpr{1.0})});

  const auto& unary = std::get<UnaryExpr>(e.value);
  EXPECT_EQ(unary.op, UnaryOp::Negate);

  const auto& inner = std::get<LiteralExpr>(unary.operand->value);
  EXPECT_EQ(std::get<double>(inner.value), 1.0);
}

TEST(AstTest, BinaryExprOk) {
  Expr e(BinaryExpr{std::make_unique<Expr>(LiteralExpr{1.0}), BinaryOp::Add,
                    std::make_unique<Expr>(LiteralExpr{2.0})});

  const auto& binary = std::get<BinaryExpr>(e.value);
  EXPECT_EQ(binary.op, BinaryOp::Add);

  const auto& left = std::get<LiteralExpr>(binary.left->value);
  const auto& right = std::get<LiteralExpr>(binary.right->value);

  EXPECT_EQ(std::get<double>(left.value), 1.0);
  EXPECT_EQ(std::get<double>(right.value), 2.0);
}

TEST(AstTest, SimpleTypeBuiltinOk) {
  Type t(SimpleType{BuiltinType::String});

  const auto& simple = std::get<SimpleType>(t.value);

  EXPECT_TRUE(std::holds_alternative<BuiltinType>(simple.value));
  EXPECT_EQ(std::get<BuiltinType>(simple.value), BuiltinType::String);
}

TEST(AstTest, SimpleTypeNamedOk) {
  Type t(SimpleType{std::string("MyType")});

  const auto& simple = std::get<SimpleType>(t.value);

  EXPECT_TRUE(std::holds_alternative<std::string>(simple.value));
  EXPECT_EQ(std::get<std::string>(simple.value), "MyType");
}

TEST(AstTest, ArrayTypeOk) {
  Type t(ArrayType{std::make_unique<Type>(SimpleType{BuiltinType::Number})});

  const auto& array = std::get<ArrayType>(t.value);
  const auto& elem = std::get<SimpleType>(array.element->value);

  EXPECT_EQ(std::get<BuiltinType>(elem.value), BuiltinType::Number);
}

TEST(AstTest, MapTypeOk) {
  Type t(MapType{std::make_unique<Type>(SimpleType{BuiltinType::String}),
                 std::make_unique<Type>(SimpleType{BuiltinType::Boolean})});

  const auto& map = std::get<MapType>(t.value);
  const auto& key = std::get<SimpleType>(map.key->value);
  const auto& value = std::get<SimpleType>(map.value->value);

  EXPECT_EQ(std::get<BuiltinType>(key.value), BuiltinType::String);
  EXPECT_EQ(std::get<BuiltinType>(value.value), BuiltinType::Boolean);
}

TEST(AstTest, ConstraintStmtOk) {
  Stmt s(ConstraintStmt{std::make_unique<Expr>(LiteralExpr{true})});

  const auto& constraint = std::get<ConstraintStmt>(s.value);
  const auto& literal = std::get<LiteralExpr>(constraint.expression->value);

  EXPECT_EQ(std::get<bool>(literal.value), true);
}

TEST(AstTest, FieldStmtOk) {
  FieldStmt field;
  field.identifier = "count";
  field.type = std::make_unique<Type>(SimpleType{BuiltinType::Integer});
  field.constraints.emplace_back(std::make_unique<ConstraintStmt>(
      std::make_unique<Expr>(LiteralExpr{true})));

  Stmt s(std::move(field));

  const auto& stmt = std::get<FieldStmt>(s.value);
  EXPECT_EQ(stmt.identifier, "count");

  const auto& type = std::get<SimpleType>(stmt.type->value);
  EXPECT_EQ(std::get<BuiltinType>(type.value), BuiltinType::Integer);
  ASSERT_EQ(stmt.constraints.size(), 1u);

  const auto& c = stmt.constraints[0];
  const auto& literal = std::get<LiteralExpr>(c->expression->value);
  EXPECT_EQ(std::get<bool>(literal.value), true);
}

TEST(AstTest, InvariantStmtOk) {
  InvariantStmt inv;
  inv.identifier = "NonEmpty";
  inv.constraints.emplace_back(std::make_unique<ConstraintStmt>(
      std::make_unique<Expr>(LiteralExpr{true})));

  Stmt s(std::move(inv));

  const auto& stmt = std::get<InvariantStmt>(s.value);

  EXPECT_EQ(stmt.identifier, "NonEmpty");
  ASSERT_EQ(stmt.constraints.size(), 1u);
}

TEST(AstTest, SpecStmtOk) {
  SpecStmt spec;
  spec.identifier = "User";
  spec.members.emplace_back(FieldStmt{
      "name", std::make_unique<Type>(SimpleType{BuiltinType::String}), {}});

  Stmt s(std::move(spec));

  const auto& stmt = std::get<SpecStmt>(s.value);
  EXPECT_EQ(stmt.identifier, "User");
  ASSERT_EQ(stmt.members.size(), 1u);

  const auto& member = std::get<FieldStmt>(stmt.members[0]);
  EXPECT_EQ(member.identifier, "name");
}

TEST(AstTest, ModuleStmtOk) {
  SpecStmt spec;
  spec.identifier = "User";
  spec.members.emplace_back(FieldStmt{
      "name", std::make_unique<Type>(SimpleType{BuiltinType::String}), {}});

  ModuleStmt module;
  module.specs.emplace_back(std::make_unique<SpecStmt>(std::move(spec)));

  Stmt s(std::move(module));

  const auto& stmt = std::get<ModuleStmt>(s.value);

  ASSERT_EQ(stmt.specs.size(), 1u);
  EXPECT_EQ(stmt.specs[0]->identifier, "User");
}
