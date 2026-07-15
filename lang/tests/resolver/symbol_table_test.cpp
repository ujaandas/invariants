#include "symbol_table.hpp"

#include <gtest/gtest.h>

#include <memory>
#include <string>
#include <utility>

using namespace invariants::resolver;

namespace {

std::unique_ptr<SpecSymbol> makeSpec(SpecId id, const std::string& name) {
  return std::make_unique<SpecSymbol>(
      SpecSymbol{.id = id, .name = name, .decl = nullptr, .fields = {}});
}

std::unique_ptr<FieldSymbol> makeField(FieldId id, const std::string& name) {
  static const invariants::ast::Type dummyType(invariants::ast::SimpleType{
      .value = invariants::ast::BuiltinType::Number});

  return std::make_unique<FieldSymbol>(
      FieldSymbol{.id = id, .name = name, .type = &dummyType, .decl = nullptr});
}

}  // namespace

TEST(SymbolTableTest, LookupNonExistentSpecReturnsNullptr) {
  SymbolTable table;

  EXPECT_EQ(table.lookup_spec("User"), nullptr);
}

TEST(SymbolTableTest, AddAndLookupSpec) {
  SymbolTable table;

  auto spec = makeSpec(1, "User");
  EXPECT_TRUE(table.add_spec("User", std::move(spec)));

  const auto* found = table.lookup_spec("User");
  ASSERT_NE(found, nullptr);
  EXPECT_EQ(found->id, 1);
  EXPECT_EQ(found->name, "User");
}

TEST(SymbolTableTest, AddDuplicateSpecFails) {
  SymbolTable table;

  EXPECT_TRUE(table.add_spec("User", makeSpec(1, "User")));
  EXPECT_FALSE(table.add_spec("User", makeSpec(2, "UserDuplicate")));

  const auto* found = table.lookup_spec("User");
  ASSERT_NE(found, nullptr);
  EXPECT_EQ(found->id, 1);
}

TEST(SymbolTableTest, AddNullSpecReturnsFalse) {
  SymbolTable table;

  EXPECT_FALSE(table.add_spec("User", nullptr));
}

TEST(SymbolTableTest, LookupFieldInNonExistentSpecReturnsNullptr) {
  SymbolTable table;

  EXPECT_EQ(table.lookup_field("User", "age"), nullptr);
}

TEST(SymbolTableTest, AddAndLookupField) {
  SymbolTable table;

  ASSERT_TRUE(table.add_spec("User", makeSpec(1, "User")));
  EXPECT_TRUE(table.add_field("User", "age", makeField(100, "age")));

  const auto* field = table.lookup_field("User", "age");
  ASSERT_NE(field, nullptr);
  EXPECT_EQ(field->id, 100);
  EXPECT_EQ(field->name, "age");
}

TEST(SymbolTableTest, AddFieldToNonExistentSpecFails) {
  SymbolTable table;

  EXPECT_FALSE(table.add_field("MissingSpec", "age", makeField(100, "age")));
  EXPECT_EQ(table.lookup_field("MissingSpec", "age"), nullptr);
}

TEST(SymbolTableTest, AddDuplicateFieldFails) {
  SymbolTable table;

  ASSERT_TRUE(table.add_spec("User", makeSpec(1, "User")));
  EXPECT_TRUE(table.add_field("User", "age", makeField(100, "age")));

  EXPECT_FALSE(table.add_field("User", "age", makeField(101, "ageDuplicate")));

  const auto* field = table.lookup_field("User", "age");
  ASSERT_NE(field, nullptr);
  EXPECT_EQ(field->id, 100);
}

TEST(SymbolTableTest, AddNullFieldReturnsFalse) {
  SymbolTable table;

  ASSERT_TRUE(table.add_spec("User", makeSpec(1, "User")));
  EXPECT_FALSE(table.add_field("User", "age", nullptr));
}

TEST(SymbolTableTest, FieldsWithSameNameInDifferentSpecsDoNotCollide) {
  SymbolTable table;

  ASSERT_TRUE(table.add_spec("User", makeSpec(1, "User")));
  ASSERT_TRUE(table.add_spec("Admin", makeSpec(2, "Admin")));

  EXPECT_TRUE(table.add_field("User", "id", makeField(10, "id")));
  EXPECT_TRUE(table.add_field("Admin", "id", makeField(20, "id")));

  const auto* userField = table.lookup_field("User", "id");
  const auto* adminField = table.lookup_field("Admin", "id");

  ASSERT_NE(userField, nullptr);
  ASSERT_NE(adminField, nullptr);
  EXPECT_EQ(userField->id, 10);
  EXPECT_EQ(adminField->id, 20);
}