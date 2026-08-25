#include "symbol_table.hpp"

#include <gtest/gtest.h>

using namespace invariants::binder;

TEST(SymbolTableTest, LookupNonExistentSpecReturnsNullptr) {
  SymbolTable table;

  EXPECT_EQ(table.lookup_spec("User"), nullptr);
}

TEST(SymbolTableTest, AddAndLookupSpec) {
  SymbolTable table;

  const auto* spec = table.add_spec("User", nullptr);
  ASSERT_NE(spec, nullptr);
  EXPECT_EQ(spec->id, 0);
  EXPECT_EQ(spec->name, "User");

  const auto* found = table.lookup_spec("User");
  ASSERT_NE(found, nullptr);
  EXPECT_EQ(found->id, 0);
  EXPECT_EQ(found->name, "User");
}

TEST(SymbolTableTest, AddDuplicateSpecFails) {
  SymbolTable table;

  EXPECT_NE(table.add_spec("User", nullptr), nullptr);
  EXPECT_EQ(table.add_spec("User", nullptr), nullptr);

  const auto* found = table.lookup_spec("User");
  ASSERT_NE(found, nullptr);
  EXPECT_EQ(found->id, 0);
}

TEST(SymbolTableTest, LookupFieldInNonExistentSpecReturnsNullptr) {
  SymbolTable table;

  EXPECT_EQ(table.lookup_field("User", "age"), nullptr);
}

TEST(SymbolTableTest, AddAndLookupField) {
  SymbolTable table;

  ASSERT_NE(table.add_spec("User", nullptr), nullptr);

  ResolvedType intType{invariants::ast::BuiltinType::Integer};
  const auto* field = table.add_field("User", "age", intType, nullptr);
  ASSERT_NE(field, nullptr);
  EXPECT_EQ(field->id, 0);
  EXPECT_EQ(field->name, "age");
  EXPECT_TRUE(field->resType.isBuiltin());

  const auto* found = table.lookup_field("User", "age");
  ASSERT_NE(found, nullptr);
  EXPECT_EQ(found->id, 0);
  EXPECT_EQ(found->name, "age");
  EXPECT_EQ(table.get_total_field_count(), 1);
}

TEST(SymbolTableTest, AddFieldToNonExistentSpecFails) {
  SymbolTable table;

  ResolvedType intType{invariants::ast::BuiltinType::Integer};
  EXPECT_EQ(table.add_field("MissingSpec", "age", intType, nullptr), nullptr);
  EXPECT_EQ(table.lookup_field("MissingSpec", "age"), nullptr);
}

TEST(SymbolTableTest, AddDuplicateFieldFails) {
  SymbolTable table;

  ASSERT_NE(table.add_spec("User", nullptr), nullptr);

  ResolvedType intType{invariants::ast::BuiltinType::Integer};
  EXPECT_NE(table.add_field("User", "age", intType, nullptr), nullptr);
  EXPECT_EQ(table.add_field("User", "age", intType, nullptr), nullptr);

  const auto* field = table.lookup_field("User", "age");
  ASSERT_NE(field, nullptr);
  EXPECT_EQ(field->id, 0);
}

TEST(SymbolTableTest, FieldsWithSameNameInDifferentSpecsDoNotCollide) {
  SymbolTable table;

  ASSERT_NE(table.add_spec("User", nullptr), nullptr);
  ASSERT_NE(table.add_spec("Admin", nullptr), nullptr);

  ResolvedType intType{invariants::ast::BuiltinType::Integer};
  EXPECT_NE(table.add_field("User", "id", intType, nullptr), nullptr);
  EXPECT_NE(table.add_field("Admin", "id", intType, nullptr), nullptr);

  const auto* userField = table.lookup_field("User", "id");
  const auto* adminField = table.lookup_field("Admin", "id");

  ASSERT_NE(userField, nullptr);
  ASSERT_NE(adminField, nullptr);
  EXPECT_EQ(userField->id, 0);
  EXPECT_EQ(adminField->id, 1);
  EXPECT_EQ(table.get_total_field_count(), 2);
}

TEST(SymbolTableTest, ResolvesCustomSpecFieldType) {
  SymbolTable table;

  const auto* addressSpec = table.add_spec("Address", nullptr);
  ASSERT_NE(addressSpec, nullptr);
  ASSERT_NE(table.add_spec("User", nullptr), nullptr);

  ResolvedType customType{addressSpec};
  const auto* locationField =
      table.add_field("User", "location", customType, nullptr);
  ASSERT_NE(locationField, nullptr);
  EXPECT_TRUE(locationField->resType.isCustom());

  const auto* found = table.lookup_field("User", "location");
  ASSERT_NE(found, nullptr);
  EXPECT_EQ(std::get<const SpecSymbol*>(found->resType.type), addressSpec);
}

TEST(SymbolTableTest, ResolvesArrayFieldType) {
  SymbolTable table;
  ASSERT_NE(table.add_spec("User", nullptr), nullptr);

  ResolvedType elementType{invariants::ast::BuiltinType::String};
  ResolvedType arrayType{
      std::make_shared<ResolvedArrayType>(ResolvedArrayType{elementType})};

  const auto* tagsField = table.add_field("User", "tags", arrayType, nullptr);
  ASSERT_NE(tagsField, nullptr);
  EXPECT_TRUE(tagsField->resType.isArray());

  const auto* found = table.lookup_field("User", "tags");
  ASSERT_NE(found, nullptr);
  auto ptr = std::get<std::shared_ptr<ResolvedArrayType>>(found->resType.type);
  EXPECT_TRUE(ptr->element.isBuiltin());
}

TEST(SymbolTableTest, ResolvesMapFieldType) {
  SymbolTable table;
  ASSERT_NE(table.add_spec("User", nullptr), nullptr);

  ResolvedType keyType{invariants::ast::BuiltinType::String};
  ResolvedType valType{invariants::ast::BuiltinType::Integer};
  ResolvedType mapType{
      std::make_shared<ResolvedMapType>(ResolvedMapType{keyType, valType})};

  const auto* scoresField = table.add_field("User", "scores", mapType, nullptr);
  ASSERT_NE(scoresField, nullptr);
  EXPECT_TRUE(scoresField->resType.isMap());

  const auto* found = table.lookup_field("User", "scores");
  ASSERT_NE(found, nullptr);
  auto ptr = std::get<std::shared_ptr<ResolvedMapType>>(found->resType.type);
  EXPECT_TRUE(ptr->key.isBuiltin());
  EXPECT_TRUE(ptr->value.isBuiltin());
}