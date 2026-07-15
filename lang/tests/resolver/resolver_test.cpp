#include "resolver.hpp"

#include <gtest/gtest.h>

#include <memory>
#include <string>

#include "lexer.hpp"
#include "parser.hpp"

using namespace invariants::lexer;
using namespace invariants::parser;
using namespace invariants::resolver;
using namespace invariants::ast;

#ifndef EXPECT_SUBSTR
#define EXPECT_SUBSTR(needle, haystack) \
  EXPECT_PRED_FORMAT2(::testing::IsSubstring, needle, haystack)
#endif

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

class ResolverIntegrationTest : public ::testing::Test {};

TEST_F(ResolverIntegrationTest, ResolvesValidBulkOrderAndSymbolTable) {
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

  auto module = parseSource(source);
  Resolver resolver;
  EXPECT_NO_THROW(resolver(*module));

  const auto& table = resolver.getSt();

  // Verify Specification registration
  const auto* spec = table.lookup_spec("BulkOrder");
  ASSERT_NE(spec, nullptr);
  EXPECT_EQ(spec->name, "BulkOrder");

  // Verify all field symbol mapping registrations
  const auto* priceField = table.lookup_field("BulkOrder", "unit_price");
  ASSERT_NE(priceField, nullptr);
  EXPECT_EQ(priceField->name, "unit_price");

  const auto* qtyField = table.lookup_field("BulkOrder", "quantity");
  ASSERT_NE(qtyField, nullptr);
  EXPECT_EQ(qtyField->name, "quantity");

  const auto* totalField = table.lookup_field("BulkOrder", "total_price");
  ASSERT_NE(totalField, nullptr);
  EXPECT_EQ(totalField->name, "total_price");
}

TEST_F(ResolverIntegrationTest, ResolvesOrderedCustomTypeCrossReferences) {
  // Address is declared first, so User can resolve its field type safely
  std::string source = R"(
    spec Address {
      field zip: String { }
    }
    spec User {
      field location: Address { }
    }
  )";

  auto module = parseSource(source);
  Resolver resolver;
  EXPECT_NO_THROW(resolver(*module));

  const auto& table = resolver.getSt();
  EXPECT_NE(table.lookup_spec("Address"), nullptr);
  EXPECT_NE(table.lookup_spec("User"), nullptr);
  EXPECT_NE(table.lookup_field("User", "location"), nullptr);
}

TEST_F(ResolverIntegrationTest, ResolvesExistentThisMemberAccess) {
  std::string source = R"(
    spec Product {
      field price: Number { }
      invariant bad_member {
        this.price == 50;
      }
    }
  )";

  auto module = parseSource(source);
  Resolver resolver;
  EXPECT_NO_THROW(resolver(*module));
}

TEST_F(ResolverIntegrationTest, ResolvesParenthesizedThisMemberAccess) {
  std::string source = R"(
    spec Product {
      field price: Number { }
      invariant grouped_this_member {
        (this).price == 50;
      }
    }
  )";

  auto module = parseSource(source);
  Resolver resolver;
  EXPECT_NO_THROW(resolver(*module));
}

TEST_F(ResolverIntegrationTest,
       ResolvesNestedParenthesizedThisChainedMemberAccess) {
  std::string source = R"(
    spec Address {
      field zip: String { }
    }
    spec User {
      field location: Address { }
      invariant nested_grouping_member {
        ((this)).location.zip == "10001";
      }
    }
  )";

  auto module = parseSource(source);
  Resolver resolver;
  EXPECT_NO_THROW(resolver(*module));
}

TEST_F(ResolverIntegrationTest, RejectsUnorderedCustomTypeCrossReferences) {
  std::string source = R"(
    spec User {
      field location: Address { }
    }
    spec Address {
      field zip: String { }
    }
  )";

  auto module = parseSource(source);
  Resolver resolver;
  EXPECT_THROW(resolver(*module), std::runtime_error);
}

TEST_F(ResolverIntegrationTest, RejectsInvalidValueScopeInInvariants) {
  try {
    std::string source = R"(
      spec User {
        field age: Integer { }
        invariant bad_scope {
          value > 18;
          }
          }
          )";
    auto module = parseSource(source);
    Resolver resolver;
    resolver(*module);
    FAIL() << "Expected std::runtime_error due to 'value' being used outside a "
              "field scope.";
  } catch (const std::runtime_error& err) {
    EXPECT_SUBSTR(
        "The 'value' keyword can only be used inside field constraints.",
        err.what());
  }
}

TEST_F(ResolverIntegrationTest, RejectsNakedVariableIdentifiers) {
  std::string source = R"(
    spec Product {
      field price: Number { }
      invariant bad_reference {
        price > 0.0;
      }
    }
  )";

  auto module = parseSource(source);
  Resolver resolver;
  EXPECT_THROW(resolver(*module), std::runtime_error);
}

TEST_F(ResolverIntegrationTest, RejectsNonExistentThisMemberAccess) {
  std::string source = R"(
    spec Product {
      field price: Number { }
      invariant bad_member {
        this.sku == "unknown";
      }
    }
  )";

  auto module = parseSource(source);
  Resolver resolver;
  EXPECT_THROW(resolver(*module), std::runtime_error);
}

TEST_F(ResolverIntegrationTest, RejectsParenthesizedThisMissingMemberAccess) {
  std::string source = R"(
    spec Product {
      field price: Number { }
      invariant bad_grouped_member {
        (this).sku == "unknown";
      }
    }
  )";

  auto module = parseSource(source);
  Resolver resolver;
  EXPECT_THROW(resolver(*module), std::runtime_error);
}