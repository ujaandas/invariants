#include "binder.hpp"

#include <gtest/gtest.h>

#include <string>
#include <variant>

#include "lexer.hpp"
#include "parser.hpp"

using namespace invariants::lexer;
using namespace invariants::parser;
using namespace invariants::binder;
using namespace invariants::ast;

namespace {

ModulePtr parseSource(const std::string& source) {
  Lexer lexer(source);
  auto tokens = lexer.scanTokens();

  Parser parser(tokens);
  auto module = parser.parseModule();

  EXPECT_NE(module, nullptr);
  return module;
}

}  // namespace

TEST(BinderTest, BindsValidModuleAndPopulatesSymbolTable) {
  std::string source = R"(
    spec BulkOrder {
      field unit_price: Number {
        value > 0.0;
      }
      field quantity: Integer { }
      
      invariant valid_total {
        this.quantity > 0;
      }
    }
  )";

  auto module = parseSource(source);
  Binder binder;
  BoundModule bound;
  EXPECT_NO_THROW(bound = binder.bind(*module));

  // Verify Bound Tree Structure
  ASSERT_EQ(bound.specs.size(), 1);
  const auto& spec = bound.specs[0];
  EXPECT_EQ(spec.symbol->name, "BulkOrder");

  ASSERT_EQ(spec.fields.size(), 2);
  EXPECT_EQ(spec.fields[0].symbol->name, "unit_price");
  EXPECT_TRUE(spec.fields[0].symbol->resType.isBuiltin());

  ASSERT_EQ(spec.fields[0].constraints.size(), 1);
  ASSERT_EQ(spec.invariants.size(), 1);

  // Verify ST state
  const auto& table = binder.getSt();
  EXPECT_EQ(table.get_total_field_count(), 2);
  EXPECT_NE(table.lookup_spec("BulkOrder"), nullptr);
  EXPECT_NE(table.lookup_field("BulkOrder", "unit_price"), nullptr);
}

TEST(BinderTest, ResolvesValueKeywordInsideFieldConstraints) {
  std::string source = R"(
    spec Product {
      field price: Number {
        value > 10.0;
      }
    }
  )";

  auto module = parseSource(source);
  Binder binder;
  BoundModule bound = binder.bind(*module);

  // Check the constraint expression
  const auto& constraint = bound.specs[0].fields[0].constraints[0];
  const auto& binExpr = std::get<BoundBinaryExpr>(constraint.expr->value);

  // Left side should be BoundValueAccessExpr
  EXPECT_TRUE(
      std::holds_alternative<BoundValueAccessExpr>(binExpr.left->value));

  // Right side should be Literal
  EXPECT_TRUE(std::holds_alternative<BoundLiteralExpr>(binExpr.right->value));
}

TEST(BinderTest, RejectsValueKeywordOutsideFieldScope) {
  std::string source = R"(
    spec Product {
      field price: Number { }
      invariant bad_scope {
        value > 10.0;
      }
    }
  )";

  auto module = parseSource(source);
  Binder binder;
  EXPECT_THROW(binder.bind(*module), std::runtime_error);
}

TEST(BinderTest, RejectsNakedIdentifiers) {
  std::string source = R"(
    spec Product {
      field price: Number { }
      invariant bad_ref {
        price > 10.0;
      }
    }
  )";

  auto module = parseSource(source);
  Binder binder;
  // Must use 'this.price', naked identifiers are rejected
  EXPECT_THROW(binder.bind(*module), std::runtime_error);
}

TEST(BinderTest, AllowsMultipleInvariantConstraints) {
  std::string source = R"(
    spec Product {
      field price: Number {
        value > 10.0;
        value < 10000.0;
      }
    }
  )";

  auto module = parseSource(source);
  Binder binder;
  BoundModule bound = binder.bind(*module);

  // Check the constraint expression
  const auto& constraint1 = bound.specs[0].fields[0].constraints[0];
  const auto& constraint2 = bound.specs[0].fields[0].constraints[1];
  const auto& binExpr1 = std::get<BoundBinaryExpr>(constraint1.expr->value);
  const auto& binExpr2 = std::get<BoundBinaryExpr>(constraint2.expr->value);

  // Left side should be BoundValueAccessExpr
  EXPECT_TRUE(
      std::holds_alternative<BoundValueAccessExpr>(binExpr1.left->value));
  EXPECT_TRUE(
      std::holds_alternative<BoundValueAccessExpr>(binExpr2.left->value));

  // Right side should be Literal
  EXPECT_TRUE(std::holds_alternative<BoundLiteralExpr>(binExpr1.right->value));
  EXPECT_TRUE(std::holds_alternative<BoundLiteralExpr>(binExpr2.right->value));
}

TEST(BinderTest, BindsThisMemberAccessAndResolvesDirectPointer) {
  std::string source = R"(
    spec Product {
      field price: Number { }
      invariant check_price {
        this.price > 0.0;
      }
    }
  )";

  auto module = parseSource(source);
  Binder binder;
  BoundModule bound = binder.bind(*module);

  const auto& inv = bound.specs[0].invariants[0];
  const auto& constraint = inv.constraints[0];
  const auto& binExpr = std::get<BoundBinaryExpr>(constraint.expr->value);

  // Left side should be BoundFieldAccessExpr pointing to price
  ASSERT_TRUE(
      std::holds_alternative<BoundFieldAccessExpr>(binExpr.left->value));
  auto access = std::get<BoundFieldAccessExpr>(binExpr.left->value);

  EXPECT_EQ(access.field->name, "price");
  EXPECT_EQ(access.field->id, 0);
  EXPECT_TRUE(access.field->resType.isBuiltin());
}

TEST(BinderTest, RejectsUnknownThisMemberAccess) {
  std::string source = R"(
    spec Product {
      field price: Number { }
      invariant check_price {
        this.fake_field > 0.0;
      }
    }
  )";

  auto module = parseSource(source);
  Binder binder;
  EXPECT_THROW(binder.bind(*module), std::runtime_error);
}

TEST(BinderTest, StripsParenthesesSyntacticNoise) {
  std::string source = R"(
    spec Product {
      field price: Number {
        (((value))) > (0.0);
      }
    }
  )";

  auto module = parseSource(source);
  Binder binder;
  BoundModule bound = binder.bind(*module);

  // Despite the parentheses in the AST, the BoundTree should just have a direct
  // BinaryExpr
  const auto& constraint = bound.specs[0].fields[0].constraints[0];
  const auto& binExpr = std::get<BoundBinaryExpr>(constraint.expr->value);

  EXPECT_TRUE(
      std::holds_alternative<BoundValueAccessExpr>(binExpr.left->value));
  EXPECT_TRUE(std::holds_alternative<BoundLiteralExpr>(binExpr.right->value));
}

TEST(BinderTest, InfersTypesForBinaryExpressions) {
  std::string source = R"(
    spec Math {
      field x: Number { }
      field y: Number { }
      invariant logic {
        this.x == this.y * 2.0;
      }
    }
  )";

  auto module = parseSource(source);
  Binder binder;
  BoundModule bound = binder.bind(*module);

  const auto& inv = bound.specs[0].invariants[0];
  const auto& constraint = inv.constraints[0];

  // The root expression is `==`, so its type should be Boolean
  EXPECT_TRUE(constraint.expr->type.isBuiltin());
  EXPECT_EQ(std::get<BuiltinType>(constraint.expr->type.type),
            invariants::ast::BuiltinType::Boolean);

  const auto& equalityExpr = std::get<BoundBinaryExpr>(constraint.expr->value);

  // The right side is `*`, so its type should evaluate to Number
  EXPECT_TRUE(equalityExpr.right->type.isBuiltin());
  EXPECT_EQ(std::get<BuiltinType>(equalityExpr.right->type.type),
            invariants::ast::BuiltinType::Number);
}

TEST(BinderTest, BindsComplexArrayAndMapTypes) {
  std::string source = R"(
    spec User {
      field tags: Array<String> { }
      field scores: Map<String, Integer> { }
    }
  )";

  auto module = parseSource(source);
  Binder binder;
  BoundModule bound;
  EXPECT_NO_THROW(bound = binder.bind(*module));

  const auto& tagsField = bound.specs[0].fields[0];
  EXPECT_TRUE(tagsField.symbol->resType.isArray());

  const auto& scoresField = bound.specs[0].fields[1];
  EXPECT_TRUE(scoresField.symbol->resType.isMap());
}

TEST(BinderTest, BindsHomogeneousListsAndInOperator) {
  std::string source = R"(
    spec Config {
      field currency: String {
        value IN ["USD", "EUR", "GBP"];
      }
    }
  )";
  auto ast = parseSource(source);
  Binder binder;
  EXPECT_NO_THROW(binder.bind(*ast));  // Should type-check perfectly
}

TEST(BinderTest, ThrowsOnMixedTypeList) {
  std::string source = R"(
    spec Config {
      field mixed: String {
        value IN ["USD", 5.0, "GBP"]; // Mixed String and Number
      }
    }
  )";
  auto ast = parseSource(source);
  Binder binder;
  // Should throw because "USD" is String but 5.0 is Number
  EXPECT_THROW(binder.bind(*ast), std::runtime_error);
}

TEST(BinderTest, ThrowsOnInOperatorTypeMismatch) {
  std::string source = R"(
    spec Config {
      field count: Integer {
        value IN ["A", "B"]; // count is Integer, list is Array<String>
      }
    }
  )";
  auto ast = parseSource(source);
  Binder binder;
  // Should throw because Left side (Integer) doesn't match Array Element
  // (String)
  EXPECT_THROW(binder.bind(*ast), std::runtime_error);
}

TEST(BinderTest, ThrowsOnEmptyList) {
  std::string source = R"(
    spec Config {
      field empty: String {
        value IN []; 
      }
    }
  )";
  auto ast = parseSource(source);
  Binder binder;
  // Should throw because we cannot infer the type of an empty list
  EXPECT_THROW(binder.bind(*ast), std::runtime_error);
}