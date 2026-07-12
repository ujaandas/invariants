#include <gtest/gtest.h>

#include <functional>
#include <memory>
#include <ostream>
#include <string>
#include <utility>
#include <vector>

#include "ast_expr.hpp"
#include "ast_stmt.hpp"
#include "ast_types.hpp"

using namespace invariants::ast;

namespace {

ExprPtr expr_ptr(Expr expr) { return std::make_unique<Expr>(std::move(expr)); }

TypePtr type_ptr(Type type) { return std::make_unique<Type>(std::move(type)); }

ConstraintPtr constraint_ptr(Expr expr) {
  return std::make_unique<ConstraintStmt>(
      ConstraintStmt{expr_ptr(std::move(expr))});
}

SpecPtr spec_ptr(SpecStmt spec) {
  return std::make_unique<SpecStmt>(std::move(spec));
}

}  // namespace

struct EqCase {
  std::string name;
  std::function<bool()> evaluate;
  bool expected;
};

void PrintTo(const EqCase& value, std::ostream* os) { *os << value.name; }

class AstEqTest : public testing::TestWithParam<EqCase> {};

TEST_P(AstEqTest, Works) {
  const auto& param = GetParam();
  EXPECT_EQ(param.evaluate(), param.expected);
}

INSTANTIATE_TEST_SUITE_P(
    AstEq, AstEqTest,
    testing::ValuesIn(std::vector<EqCase>{
        {"literal_equal", [] { return LiteralExpr{1.0} == LiteralExpr{1.0}; },
         true},
        {"literal_not_equal",
         [] { return LiteralExpr{1.0} == LiteralExpr{2.0}; }, false},
        {"identifier_not_equal",
         [] { return IdentifierExpr{"a"} == IdentifierExpr{"b"}; }, false},
        {"this_equal", [] { return ThisExpr{} == ThisExpr{}; }, true},

        {"list_equal",
         [] {
           ListExpr a;
           a.elements.emplace_back(expr_ptr(Expr(LiteralExpr{1.0})));
           a.elements.emplace_back(expr_ptr(Expr(IdentifierExpr{"x"})));

           ListExpr b;
           b.elements.emplace_back(expr_ptr(Expr(LiteralExpr{1.0})));
           b.elements.emplace_back(expr_ptr(Expr(IdentifierExpr{"x"})));

           return a == b;
         },
         true},
        {"list_size_mismatch",
         [] {
           ListExpr a;
           a.elements.emplace_back(expr_ptr(Expr(LiteralExpr{1.0})));

           ListExpr b;
           b.elements.emplace_back(expr_ptr(Expr(LiteralExpr{1.0})));
           b.elements.emplace_back(expr_ptr(Expr(LiteralExpr{2.0})));

           return a == b;
         },
         false},
        {"list_element_mismatch",
         [] {
           ListExpr a;
           a.elements.emplace_back(expr_ptr(Expr(LiteralExpr{1.0})));

           ListExpr b;
           b.elements.emplace_back(expr_ptr(Expr(LiteralExpr{2.0})));

           return a == b;
         },
         false},

        {"grouping_both_null",
         [] { return GroupingExpr{nullptr} == GroupingExpr{nullptr}; }, true},
        {"grouping_one_null",
         [] {
           return GroupingExpr{expr_ptr(Expr(LiteralExpr{1.0}))} ==
                  GroupingExpr{nullptr};
         },
         false},

        {"member_access_not_equal",
         [] { return MemberAccessOp{"left"} == MemberAccessOp{"right"}; },
         false},
        {"index_equal",
         [] {
           return IndexOp{expr_ptr(Expr(LiteralExpr{0.0}))} ==
                  IndexOp{expr_ptr(Expr(LiteralExpr{0.0}))};
         },
         true},
        {"index_not_equal",
         [] {
           return IndexOp{expr_ptr(Expr(LiteralExpr{1.0}))} ==
                  IndexOp{expr_ptr(Expr(LiteralExpr{2.0}))};
         },
         false},

        {"postfix_equal",
         [] {
           PostfixExpr a;
           a.base = expr_ptr(Expr(IdentifierExpr{"list"}));
           a.ops.emplace_back(MemberAccessOp{"size"});
           a.ops.emplace_back(IndexOp{expr_ptr(Expr(LiteralExpr{0.0}))});

           PostfixExpr b;
           b.base = expr_ptr(Expr(IdentifierExpr{"list"}));
           b.ops.emplace_back(MemberAccessOp{"size"});
           b.ops.emplace_back(IndexOp{expr_ptr(Expr(LiteralExpr{0.0}))});

           return a == b;
         },
         true},
        {"postfix_base_not_equal",
         [] {
           PostfixExpr a;
           a.base = expr_ptr(Expr(IdentifierExpr{"left"}));

           PostfixExpr b;
           b.base = expr_ptr(Expr(IdentifierExpr{"right"}));

           return a == b;
         },
         false},

        {"unary_equal",
         [] {
           return UnaryExpr{UnaryOp::Not,
                            expr_ptr(Expr(IdentifierExpr{"x"}))} ==
                  UnaryExpr{UnaryOp::Not, expr_ptr(Expr(IdentifierExpr{"x"}))};
         },
         true},
        {"unary_op_not_equal",
         [] {
           return UnaryExpr{UnaryOp::Not,
                            expr_ptr(Expr(IdentifierExpr{"x"}))} ==
                  UnaryExpr{UnaryOp::Negate,
                            expr_ptr(Expr(IdentifierExpr{"x"}))};
         },
         false},

        {"binary_equal",
         [] {
           return BinaryExpr{expr_ptr(Expr(LiteralExpr{1.0})), BinaryOp::Add,
                             expr_ptr(Expr(LiteralExpr{2.0}))} ==
                  BinaryExpr{expr_ptr(Expr(LiteralExpr{1.0})), BinaryOp::Add,
                             expr_ptr(Expr(LiteralExpr{2.0}))};
         },
         true},
        {"binary_right_not_equal",
         [] {
           return BinaryExpr{expr_ptr(Expr(LiteralExpr{1.0})), BinaryOp::Add,
                             expr_ptr(Expr(LiteralExpr{2.0}))} ==
                  BinaryExpr{expr_ptr(Expr(LiteralExpr{1.0})), BinaryOp::Add,
                             expr_ptr(Expr(LiteralExpr{3.0}))};
         },
         false},

        {"expr_wrapper_equal",
         [] { return Expr(ThisExpr{}) == Expr(ThisExpr{}); }, true},
        {"expr_wrapper_variant_mismatch",
         [] { return Expr(LiteralExpr{1.0}) == Expr(IdentifierExpr{"1"}); },
         false},

        {"simple_type_builtin_equal",
         [] {
           return SimpleType{BuiltinType::String} ==
                  SimpleType{BuiltinType::String};
         },
         true},
        {"simple_type_named_not_equal",
         [] { return SimpleType{"A"} == SimpleType{"B"}; }, false},
        {"array_type_equal",
         [] {
           return ArrayType{type_ptr(Type(SimpleType{BuiltinType::Integer}))} ==
                  ArrayType{type_ptr(Type(SimpleType{BuiltinType::Integer}))};
         },
         true},
        {"array_type_one_null",
         [] {
           return ArrayType{type_ptr(Type(SimpleType{BuiltinType::Integer}))} ==
                  ArrayType{nullptr};
         },
         false},
        {"map_type_equal",
         [] {
           return MapType{type_ptr(Type(SimpleType{BuiltinType::String})),
                          type_ptr(Type(SimpleType{BuiltinType::Boolean}))} ==
                  MapType{type_ptr(Type(SimpleType{BuiltinType::String})),
                          type_ptr(Type(SimpleType{BuiltinType::Boolean}))};
         },
         true},
        {"map_type_value_not_equal",
         [] {
           return MapType{type_ptr(Type(SimpleType{BuiltinType::String})),
                          type_ptr(Type(SimpleType{BuiltinType::Boolean}))} ==
                  MapType{type_ptr(Type(SimpleType{BuiltinType::String})),
                          type_ptr(Type(SimpleType{BuiltinType::Integer}))};
         },
         false},
        {"type_wrapper_variant_mismatch",
         [] {
           return Type(SimpleType{BuiltinType::Number}) ==
                  Type(ArrayType{
                      type_ptr(Type(SimpleType{BuiltinType::Number}))});
         },
         false},

        {"constraint_equal",
         [] {
           return ConstraintStmt{expr_ptr(Expr(LiteralExpr{true}))} ==
                  ConstraintStmt{expr_ptr(Expr(LiteralExpr{true}))};
         },
         true},
        {"constraint_one_null",
         [] {
           return ConstraintStmt{expr_ptr(Expr(LiteralExpr{true}))} ==
                  ConstraintStmt{nullptr};
         },
         false},

        {"field_equal",
         [] {
           FieldStmt a;
           a.identifier = "count";
           a.type = type_ptr(Type(SimpleType{BuiltinType::Integer}));
           a.constraints.emplace_back(constraint_ptr(Expr(LiteralExpr{true})));

           FieldStmt b;
           b.identifier = "count";
           b.type = type_ptr(Type(SimpleType{BuiltinType::Integer}));
           b.constraints.emplace_back(constraint_ptr(Expr(LiteralExpr{true})));

           return a == b;
         },
         true},
        {"field_constraint_not_equal",
         [] {
           FieldStmt a;
           a.identifier = "count";
           a.type = type_ptr(Type(SimpleType{BuiltinType::Integer}));
           a.constraints.emplace_back(constraint_ptr(Expr(LiteralExpr{true})));

           FieldStmt b;
           b.identifier = "count";
           b.type = type_ptr(Type(SimpleType{BuiltinType::Integer}));
           b.constraints.emplace_back(constraint_ptr(Expr(LiteralExpr{false})));

           return a == b;
         },
         false},

        {"invariant_equal",
         [] {
           InvariantStmt a;
           a.identifier = "Valid";
           a.constraints.emplace_back(constraint_ptr(Expr(LiteralExpr{true})));

           InvariantStmt b;
           b.identifier = "Valid";
           b.constraints.emplace_back(constraint_ptr(Expr(LiteralExpr{true})));

           return a == b;
         },
         true},
        {"invariant_identifier_not_equal",
         [] {
           InvariantStmt a;
           a.identifier = "A";

           InvariantStmt b;
           b.identifier = "B";

           return a == b;
         },
         false},

        {"spec_equal",
         [] {
           SpecStmt a;
           a.identifier = "User";
           a.members.emplace_back(FieldStmt{
               "name", type_ptr(Type(SimpleType{BuiltinType::String})), {}});

           SpecStmt b;
           b.identifier = "User";
           b.members.emplace_back(FieldStmt{
               "name", type_ptr(Type(SimpleType{BuiltinType::String})), {}});

           return a == b;
         },
         true},
        {"spec_members_not_equal",
         [] {
           SpecStmt a;
           a.identifier = "User";
           a.members.emplace_back(FieldStmt{
               "name", type_ptr(Type(SimpleType{BuiltinType::String})), {}});

           SpecStmt b;
           b.identifier = "User";
           b.members.emplace_back(FieldStmt{
               "age", type_ptr(Type(SimpleType{BuiltinType::Integer})), {}});

           return a == b;
         },
         false},

        {"module_equal",
         [] {
           SpecStmt spec_a;
           spec_a.identifier = "User";

           SpecStmt spec_b;
           spec_b.identifier = "User";

           ModuleStmt a;
           a.specs.emplace_back(spec_ptr(std::move(spec_a)));

           ModuleStmt b;
           b.specs.emplace_back(spec_ptr(std::move(spec_b)));

           return a == b;
         },
         true},
        {"module_size_not_equal",
         [] {
           ModuleStmt a;

           SpecStmt spec;
           spec.identifier = "User";

           ModuleStmt b;
           b.specs.emplace_back(spec_ptr(std::move(spec)));

           return a == b;
         },
         false},

        {"stmt_wrapper_equal",
         [] { return Stmt(ModuleStmt{}) == Stmt(ModuleStmt{}); }, true},
        {"stmt_wrapper_variant_mismatch",
         [] { return Stmt(ModuleStmt{}) == Stmt(SpecStmt{}); }, false},
    }),
    [](const testing::TestParamInfo<EqCase>& info) { return info.param.name; });
