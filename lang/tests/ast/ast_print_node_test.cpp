#include <gtest/gtest.h>

#include <functional>
#include <memory>
#include <ostream>
#include <string>
#include <vector>

#include "ast_expr.hpp"
#include "ast_stmt.hpp"
#include "ast_types.hpp"
#include "printers/plain_printer.hpp"
#include "printers/tree_printer.hpp"

using namespace invariants::ast;

namespace {

using invariants::ast::printers::Plain;
using invariants::ast::printers::Tree;

ExprPtr expr_ptr(Expr expr) { return std::make_unique<Expr>(std::move(expr)); }

TypePtr type_ptr(Type type) { return std::make_unique<Type>(std::move(type)); }

ConstraintPtr constraint_ptr(Expr expr) {
  return std::make_unique<ConstraintStmt>(
      ConstraintStmt{expr_ptr(std::move(expr))});
}

FieldStmt field(const std::string& name, Type type,
                std::vector<Expr> constraints) {
  FieldStmt stmt;
  stmt.identifier = name;
  stmt.type = type_ptr(std::move(type));
  for (auto& constraint : constraints) {
    stmt.constraints.emplace_back(constraint_ptr(std::move(constraint)));
  }
  return stmt;
}

InvariantStmt invariant(const std::string& name,
                        std::vector<Expr> constraints) {
  InvariantStmt stmt;
  stmt.identifier = name;
  for (auto& constraint : constraints) {
    stmt.constraints.emplace_back(constraint_ptr(std::move(constraint)));
  }
  return stmt;
}

SpecStmt spec(const std::string& name, std::vector<SpecMember> members) {
  SpecStmt stmt;
  stmt.identifier = name;
  stmt.members = std::move(members);
  return stmt;
}

ModuleStmt module(std::vector<SpecStmt> specs) {
  ModuleStmt stmt;
  for (auto& s : specs) {
    stmt.specs.emplace_back(std::make_unique<SpecStmt>(std::move(s)));
  }
  return stmt;
}

struct ExprCase {
  std::string name;
  std::function<Expr()> build;
  std::string expected;
};

struct StmtCase {
  std::string name;
  std::function<Stmt()> build;
  std::string expected;
};

struct TypeCase {
  std::string name;
  std::function<Type()> build;
  std::string expected;
};

void PrintTo(const ExprCase& value, std::ostream* os) {
  *os << value.name << " expected=\"" << value.expected << "\"";
}

void PrintTo(const StmtCase& value, std::ostream* os) {
  *os << value.name << " expected=\"" << value.expected << "\"";
}

void PrintTo(const TypeCase& value, std::ostream* os) {
  *os << value.name << " expected=\"" << value.expected << "\"";
}

}  // namespace

class ExprPrettyPrintsTest : public testing::TestWithParam<ExprCase> {};

TEST_P(ExprPrettyPrintsTest, PrettyPrints) {
  const auto& param = GetParam();

  auto out = Plain{}.print(param.build());

  EXPECT_EQ(out, param.expected);
}

class StmtPrettyPrintsTest : public testing::TestWithParam<StmtCase> {};

TEST_P(StmtPrettyPrintsTest, PrettyPrints) {
  const auto& param = GetParam();

  auto out = Plain{}.print(param.build());

  EXPECT_EQ(out, param.expected);
}

class TypePrettyPrintsTest : public testing::TestWithParam<TypeCase> {};

TEST_P(TypePrettyPrintsTest, PrettyPrints) {
  const auto& param = GetParam();

  auto out = Plain{}.print(param.build());

  EXPECT_EQ(out, param.expected);
}

INSTANTIATE_TEST_SUITE_P(
    AstTest, ExprPrettyPrintsTest,
    testing::ValuesIn(std::vector<ExprCase>{
        {"literal_number", [] { return Expr(LiteralExpr{3.5}); }, "3.500000"},
        {"literal_string", [] { return Expr(LiteralExpr{std::string("hi")}); },
         "\"hi\""},
        {"literal_bool", [] { return Expr(LiteralExpr{true}); }, "true"},
        {"literal_null", [] { return Expr(LiteralExpr{nullptr}); }, "null"},
        {"identifier", [] { return Expr(IdentifierExpr{"foo"}); }, "foo"},
        {"this_expr", [] { return Expr(ThisExpr{}); }, "this"},
        {"list_empty",
         [] {
           std::vector<ExprPtr> elements;
           return Expr(ListExpr{std::move(elements)});
         },
         "[]"},
        {"list_mixed",
         [] {
           std::vector<ExprPtr> elements;
           elements.emplace_back(expr_ptr(Expr(LiteralExpr{1.0})));
           elements.emplace_back(expr_ptr(Expr(LiteralExpr{std::string("a")})));
           elements.emplace_back(expr_ptr(Expr(LiteralExpr{true})));
           return Expr(ListExpr{std::move(elements)});
         },
         "[1.000000, \"a\", true]"},
        {"grouping",
         [] {
           return Expr(GroupingExpr{expr_ptr(Expr(IdentifierExpr{"foo"}))});
         },
         "(foo)"},
        {"postfix_member_index",
         [] {
           std::vector<PostfixOp> ops;
           ops.emplace_back(MemberAccessOp{"length"});
           ops.emplace_back(IndexOp{expr_ptr(Expr(LiteralExpr{0.0}))});
           return Expr(PostfixExpr{expr_ptr(Expr(IdentifierExpr{"list"})),
                                   std::move(ops)});
         },
         "list.length[0.000000]"},
        {"unary_not",
         [] {
           return Expr(
               UnaryExpr{UnaryOp::Not, expr_ptr(Expr(IdentifierExpr{"ok"}))});
         },
         "(!ok)"},
        {"binary_add",
         [] {
           return Expr(BinaryExpr{expr_ptr(Expr(LiteralExpr{1.0})),
                                  BinaryOp::Add,
                                  expr_ptr(Expr(LiteralExpr{2.0}))});
         },
         "(1.000000 + 2.000000)"},
        {"binary_not_in",
         [] {
           return Expr(BinaryExpr{expr_ptr(Expr(IdentifierExpr{"item"})),
                                  BinaryOp::NotIn,
                                  expr_ptr(Expr(IdentifierExpr{"list"}))});
         },
         "(item not in list)"},
        {"binary_imply",
         [] {
           return Expr(BinaryExpr{expr_ptr(Expr(IdentifierExpr{"a"})),
                                  BinaryOp::Imply,
                                  expr_ptr(Expr(IdentifierExpr{"b"}))});
         },
         "(a -> b)"},
    }),
    [](const testing::TestParamInfo<ExprCase>& info) {
      return info.param.name;
    });

INSTANTIATE_TEST_SUITE_P(
    AstTest, StmtPrettyPrintsTest,
    testing::ValuesIn(std::vector<StmtCase>{
        {"constraint_true",
         [] { return Stmt(ConstraintStmt{expr_ptr(Expr(LiteralExpr{true}))}); },
         "true;"},
        {"field_no_constraints",
         [] {
           return Stmt(
               field("count", Type(SimpleType{BuiltinType::Integer}), {}));
         },
         "field count: Integer {}"},
        {"field_with_constraints",
         [] {
           std::vector<Expr> constraints;
           constraints.emplace_back(Expr(BinaryExpr{
               expr_ptr(Expr(IdentifierExpr{"count"})), BinaryOp::Greater,
               expr_ptr(Expr(LiteralExpr{0.0}))}));
           constraints.emplace_back(Expr(
               BinaryExpr{expr_ptr(Expr(IdentifierExpr{"count"})),
                          BinaryOp::Less, expr_ptr(Expr(LiteralExpr{10.0}))}));
           return Stmt(field("count", Type(SimpleType{BuiltinType::Integer}),
                             std::move(constraints)));
         },
         "field count: Integer { (count > 0.000000); (count < 10.000000); }"},
        {"invariant_with_constraint",
         [] {
           std::vector<Expr> constraints;
           constraints.emplace_back(Expr(BinaryExpr{
               expr_ptr(Expr(IdentifierExpr{"len"})), BinaryOp::Greater,
               expr_ptr(Expr(LiteralExpr{0.0}))}));
           return Stmt(invariant("NonEmpty", std::move(constraints)));
         },
         "invariant NonEmpty { (len > 0.000000); }"},
        {"spec_empty", [] { return Stmt(spec("User", {})); }, "spec User {}"},
        {"spec_with_members",
         [] {
           std::vector<SpecMember> members;
           members.emplace_back(
               field("name", Type(SimpleType{BuiltinType::String}), {}));
           std::vector<Expr> constraints;
           constraints.emplace_back(Expr(LiteralExpr{true}));
           members.emplace_back(invariant("NonEmpty", std::move(constraints)));
           return Stmt(spec("User", std::move(members)));
         },
         "spec User { field name: String {} invariant NonEmpty { true; } }"},
        {"module_two_specs",
         [] {
           std::vector<SpecStmt> specs;
           specs.emplace_back(spec("User", {}));
           specs.emplace_back(spec("Order", {}));
           return Stmt(module(std::move(specs)));
         },
         "spec User {} spec Order {}"},
    }),
    [](const testing::TestParamInfo<StmtCase>& info) {
      return info.param.name;
    });

INSTANTIATE_TEST_SUITE_P(
    AstTest, TypePrettyPrintsTest,
    testing::ValuesIn(std::vector<TypeCase>{
        {"builtin_number", [] { return Type(SimpleType{BuiltinType::Number}); },
         "Number"},
        {"builtin_string", [] { return Type(SimpleType{BuiltinType::String}); },
         "String"},
        {"named_type", [] { return Type(SimpleType{std::string("User")}); },
         "User"},
        {"array_integer",
         [] {
           return Type(
               ArrayType{type_ptr(Type(SimpleType{BuiltinType::Integer}))});
         },
         "Array<Integer>"},
        {"map_simple",
         [] {
           return Type(
               MapType{type_ptr(Type(SimpleType{BuiltinType::String})),
                       type_ptr(Type(SimpleType{BuiltinType::Boolean}))});
         },
         "Map<String, Boolean>"},
        {"map_nested",
         [] {
           return Type(MapType{type_ptr(Type(SimpleType{BuiltinType::String})),
                               type_ptr(Type(ArrayType{type_ptr(
                                   Type(SimpleType{BuiltinType::Integer}))}))});
         },
         "Map<String, Array<Integer>>"},
    }),
    [](const testing::TestParamInfo<TypeCase>& info) {
      return info.param.name;
    });

TEST(AstTest, PrintsSimpleTree1) {
  std::vector<ExprPtr> elements;
  elements.emplace_back(expr_ptr(Expr(LiteralExpr{1.0})));
  elements.emplace_back(expr_ptr(Expr(LiteralExpr{2.0})));
  elements.emplace_back(expr_ptr(Expr(LiteralExpr{3.0})));

  std::vector<Expr> constraints;
  constraints.emplace_back(
      Expr(BinaryExpr{expr_ptr(Expr(IdentifierExpr{"count"})), BinaryOp::In,
                      expr_ptr(Expr(ListExpr{std::move(elements)}))}));

  auto out =
      Plain{}.print(Stmt(field("count", Type(SimpleType{BuiltinType::Integer}),
                               std::move(constraints))));

  EXPECT_EQ(
      out,
      "field count: Integer { (count in [1.000000, 2.000000, 3.000000]); }");
}

TEST(AstTest, PrintsSimpleTree2) {
  std::vector<PostfixOp> role_ops;
  role_ops.emplace_back(MemberAccessOp{"role"});

  std::vector<Expr> constraints;
  constraints.emplace_back(Expr(BinaryExpr{
      expr_ptr(Expr(
          UnaryExpr{UnaryOp::Not, expr_ptr(Expr(IdentifierExpr{"active"}))})),
      BinaryOp::Or,
      expr_ptr(Expr(BinaryExpr{
          expr_ptr(Expr(PostfixExpr{expr_ptr(Expr(IdentifierExpr{"user"})),
                                    std::move(role_ops)})),
          BinaryOp::Equal,
          expr_ptr(Expr(LiteralExpr{std::string("admin")}))}))}));

  auto out = Plain{}.print(Stmt(invariant("Access", std::move(constraints))));

  EXPECT_EQ(out,
            "invariant Access { ((!active) || (user.role == \"admin\")); }");
}

TEST(AstTest, PrintsComplexTree1) {
  std::vector<PostfixOp> index_ops;
  index_ops.emplace_back(IndexOp{expr_ptr(Expr(LiteralExpr{0.0}))});
  index_ops.emplace_back(IndexOp{expr_ptr(Expr(LiteralExpr{0.0}))});

  std::vector<Expr> row_constraints;
  row_constraints.emplace_back(Expr(BinaryExpr{
      expr_ptr(Expr(PostfixExpr{expr_ptr(Expr(IdentifierExpr{"rows"})),
                                std::move(index_ops)})),
      BinaryOp::GreaterEqual, expr_ptr(Expr(LiteralExpr{0.0}))}));

  std::vector<PostfixOp> length_ops;
  length_ops.emplace_back(MemberAccessOp{"length"});

  std::vector<Expr> invariant_constraints;
  invariant_constraints.emplace_back(Expr(BinaryExpr{
      expr_ptr(Expr(PostfixExpr{expr_ptr(Expr(IdentifierExpr{"rows"})),
                                std::move(length_ops)})),
      BinaryOp::GreaterEqual, expr_ptr(Expr(LiteralExpr{1.0}))}));

  std::vector<SpecMember> members;
  members.emplace_back(field("rows",
                             Type(ArrayType{type_ptr(Type(ArrayType{type_ptr(
                                 Type(SimpleType{BuiltinType::Integer}))}))}),
                             std::move(row_constraints)));
  members.emplace_back(invariant("Shape", std::move(invariant_constraints)));

  auto out = Plain{}.print(Stmt(spec("Matrix", std::move(members))));

  EXPECT_EQ(out,
            "spec Matrix { field rows: Array<Array<Integer>> { "
            "(rows[0.000000][0.000000] >= 0.000000); } invariant Shape { "
            "(rows.length >= 1.000000); } }");
}

TEST(AstTest, PrintsComplexTree2) {
  std::vector<Expr> tag_constraints;
  tag_constraints.emplace_back(Expr(
      BinaryExpr{expr_ptr(Expr(LiteralExpr{std::string("admin")})),
                 BinaryOp::NotIn, expr_ptr(Expr(IdentifierExpr{"tags"}))}));

  std::vector<PostfixOp> items_length_ops;
  items_length_ops.emplace_back(MemberAccessOp{"length"});

  std::vector<Expr> item_constraints;
  item_constraints.emplace_back(Expr(BinaryExpr{
      expr_ptr(Expr(PostfixExpr{expr_ptr(Expr(IdentifierExpr{"items"})),
                                std::move(items_length_ops)})),
      BinaryOp::GreaterEqual, expr_ptr(Expr(LiteralExpr{1.0}))}));

  std::vector<Expr> owner_constraints;
  owner_constraints.emplace_back(Expr(
      BinaryExpr{expr_ptr(Expr(IdentifierExpr{"owner"})), BinaryOp::NotEqual,
                 expr_ptr(Expr(LiteralExpr{nullptr}))}));

  std::vector<SpecMember> user_members;
  user_members.emplace_back(
      field("name", Type(SimpleType{BuiltinType::String}), {}));
  user_members.emplace_back(field(
      "tags", Type(ArrayType{type_ptr(Type(SimpleType{BuiltinType::String}))}),
      std::move(tag_constraints)));

  std::vector<SpecMember> order_members;
  order_members.emplace_back(field(
      "items", Type(ArrayType{type_ptr(Type(SimpleType{std::string("Item")}))}),
      std::move(item_constraints)));
  order_members.emplace_back(
      invariant("HasOwner", std::move(owner_constraints)));

  std::vector<SpecStmt> specs;
  specs.emplace_back(spec("User", std::move(user_members)));
  specs.emplace_back(spec("Order", std::move(order_members)));

  auto out = Plain{}.print(Stmt(module(std::move(specs))));

  EXPECT_EQ(out,
            "spec User { field name: String {} field tags: Array<String> { "
            "(\"admin\" not in tags); } } spec Order { field items: "
            "Array<Item> { (items.length >= 1.000000); } invariant HasOwner { "
            "(owner != null); } }");
}

TEST(AstTest, TreePrinterRendersBranches) {
  auto out = Tree{}.print(Expr(BinaryExpr{
      expr_ptr(
          Expr(UnaryExpr{UnaryOp::Not, expr_ptr(Expr(IdentifierExpr{"ok"}))})),
      BinaryOp::Or, expr_ptr(Expr(LiteralExpr{std::string("ready")}))}));

  EXPECT_EQ(out,
            "Binary ||\n/-- left: Unary !\n|   \\-- operand: Identifier ok\n"
            "\\-- right: Literal \"ready\"");
}