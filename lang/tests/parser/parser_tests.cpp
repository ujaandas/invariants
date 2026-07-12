#include <gtest/gtest.h>

#include "ast_stmt.hpp"
#include "lexer.hpp"
#include "parser.hpp"

using namespace invariants::ast;
using namespace invariants::parser;
using namespace invariants::lexer;

std::vector<Token> lexSource(const std::string& source) {
  Lexer lexer(source);
  return lexer.scanTokens();
}

class ParserIntegrationTest : public ::testing::Test {};

TEST_F(ParserIntegrationTest, ParsesSimpleField) {
  std::string source = R"(
    spec Box {
      field width: Number { }
    }
  )";

  auto tokens = lexSource(source);
  Parser parser(tokens);
  auto module = parser.parseModule();

  ASSERT_NE(module, nullptr);
  EXPECT_EQ(module->specs.size(), 1);
  EXPECT_EQ(module->specs[0]->identifier, "Box");
  EXPECT_EQ(module->specs[0]->members.size(), 1);

  auto& field = std::get<FieldStmt>(module->specs[0]->members[0]);
  EXPECT_EQ(field.identifier, "width");
  EXPECT_EQ(field.constraints.size(), 0);
}

TEST_F(ParserIntegrationTest, ParsesFieldWithConstraints) {
  std::string source = R"(
    spec Account {
      field balance: Number {
        value >= 0.0;
        value < 1000000.0;
      }
    }
  )";

  auto tokens = lexSource(source);
  Parser parser(tokens);
  auto module = parser.parseModule();

  ASSERT_NE(module, nullptr);
  EXPECT_EQ(module->specs.size(), 1);

  auto& field = std::get<FieldStmt>(module->specs[0]->members[0]);
  EXPECT_EQ(field.identifier, "balance");
  EXPECT_EQ(field.constraints.size(), 2);
}

TEST_F(ParserIntegrationTest, ParsesInvariant) {
  std::string source = R"(
    spec Simple {
      invariant always_true {
        true;
      }
    }
  )";

  auto tokens = lexSource(source);
  Parser parser(tokens);
  auto module = parser.parseModule();

  ASSERT_NE(module, nullptr);
  EXPECT_EQ(module->specs.size(), 1);
  EXPECT_EQ(module->specs[0]->members.size(), 1);

  auto& inv = std::get<InvariantStmt>(module->specs[0]->members[0]);
  EXPECT_EQ(inv.identifier, "always_true");
  EXPECT_EQ(inv.constraints.size(), 1);
}

TEST_F(ParserIntegrationTest, ParsesMultipleMembers) {
  std::string source = R"(
    spec Mixed {
      field x: Integer { }
      field y: String { }
      invariant constraint_1 {
        true;
      }
    }
  )";

  auto tokens = lexSource(source);
  Parser parser(tokens);
  auto module = parser.parseModule();

  ASSERT_NE(module, nullptr);
  EXPECT_EQ(module->specs.size(), 1);
  EXPECT_EQ(module->specs[0]->members.size(), 3);

  auto& f1 = std::get<FieldStmt>(module->specs[0]->members[0]);
  EXPECT_EQ(f1.identifier, "x");

  auto& f2 = std::get<FieldStmt>(module->specs[0]->members[1]);
  EXPECT_EQ(f2.identifier, "y");

  auto& inv = std::get<InvariantStmt>(module->specs[0]->members[2]);
  EXPECT_EQ(inv.identifier, "constraint_1");
}

TEST_F(ParserIntegrationTest, ParsesArrayAndMapTypes) {
  std::string source = R"(
    spec Collections {
      field items: Array<Number> { }
      field mapping: Map<String, Integer> { }
    }
  )";

  auto tokens = lexSource(source);
  Parser parser(tokens);
  auto module = parser.parseModule();

  ASSERT_NE(module, nullptr);
  EXPECT_EQ(module->specs[0]->members.size(), 2);

  auto& f1 = std::get<FieldStmt>(module->specs[0]->members[0]);
  EXPECT_EQ(f1.identifier, "items");

  auto& f2 = std::get<FieldStmt>(module->specs[0]->members[1]);
  EXPECT_EQ(f2.identifier, "mapping");
}

TEST_F(ParserIntegrationTest, ParsesComplexConstraints) {
  std::string source = R"(
    spec Math {
      field a: Number {
        value > 0.0 -> value < 100.0;
      }
    }
  )";

  auto tokens = lexSource(source);
  Parser parser(tokens);
  auto module = parser.parseModule();

  ASSERT_NE(module, nullptr);
  auto& field = std::get<FieldStmt>(module->specs[0]->members[0]);
  EXPECT_EQ(field.constraints.size(), 1);
}

TEST_F(ParserIntegrationTest, ParsesBulkOrderFromDocs) {
  // Simplified version of sample.inv from docs
  std::string source = R"(
    spec BulkOrder {
      field unit_price: Number {
        value > 0.0;
      }
      field quantity: Integer {
        value >= 1;
      }
      field total_price: Number { }
      invariant valid_total_price {
        this.total_price == this.unit_price * this.quantity;
      }
    }
  )";

  auto tokens = lexSource(source);
  Parser parser(tokens);
  auto module = parser.parseModule();

  ASSERT_NE(module, nullptr);
  EXPECT_EQ(module->specs.size(), 1);

  auto& spec = module->specs[0];
  EXPECT_EQ(spec->identifier, "BulkOrder");
  EXPECT_EQ(spec->members.size(), 4);

  // Check fields
  auto& f1 = std::get<FieldStmt>(spec->members[0]);
  EXPECT_EQ(f1.identifier, "unit_price");
  EXPECT_EQ(f1.constraints.size(), 1);

  auto& f2 = std::get<FieldStmt>(spec->members[1]);
  EXPECT_EQ(f2.identifier, "quantity");
  EXPECT_EQ(f2.constraints.size(), 1);

  // Check invariant
  auto& inv = std::get<InvariantStmt>(spec->members[3]);
  EXPECT_EQ(inv.identifier, "valid_total_price");
  EXPECT_EQ(inv.constraints.size(), 1);
}

TEST_F(ParserIntegrationTest, ParsesMultipleSpecs) {
  std::string source = R"(
    spec First {
      field x: Number { }
    }
    spec Second {
      field y: String { }
    }
  )";

  auto tokens = lexSource(source);
  Parser parser(tokens);
  auto module = parser.parseModule();

  ASSERT_NE(module, nullptr);
  EXPECT_EQ(module->specs.size(), 2);
  EXPECT_EQ(module->specs[0]->identifier, "First");
  EXPECT_EQ(module->specs[1]->identifier, "Second");
}

TEST_F(ParserIntegrationTest, ThrowsOnExtraTokensAfterModule) {
  std::string source = R"(
    spec Single {
      field x: Number { }
    }
    true
  )";

  auto tokens = lexSource(source);
  Parser parser(tokens);

  EXPECT_THROW(parser.parseModule(), std::runtime_error);
}
