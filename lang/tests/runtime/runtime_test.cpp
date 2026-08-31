#include "runtime.hpp"

#include <gtest/gtest.h>

#include <string>
#include <vector>

using namespace invariants::runtime;

TEST(RuntimeTest, InitialStateIsCorrect) {
  Runtime rt;

  EXPECT_TRUE(rt.hasMoreFields());
  EXPECT_EQ(rt.getActiveFieldName(), "unit_price");
  EXPECT_EQ(rt.getActiveFieldType(), FieldType::Number);

  std::vector<std::string> expected_order = {"unit_price", "quantity",
                                             "currency", "total_price"};
  EXPECT_EQ(rt.getGenOrder(), expected_order);
}

TEST(RuntimeTest, ValidatesUnitPricePrefixesAndBoundaries) {
  Runtime rt;

  // Partial prefixes should not be rejected
  EXPECT_EQ(rt.validate_active_field_partial(""),
            ValidationStatus::PartialValid);
  EXPECT_EQ(rt.validate_active_field_partial("1"), ValidationStatus::Valid);
  EXPECT_EQ(rt.validate_active_field_partial("12."),
            ValidationStatus::PartialValid);
  EXPECT_EQ(rt.validate_active_field_partial("10.5"), ValidationStatus::Valid);

  // Actively invalid inputs
  EXPECT_EQ(rt.validate_active_field_partial("-5.0"),
            ValidationStatus::Invalid);  // value > 0.0
  EXPECT_EQ(rt.validate_active_field_partial("0.0"),
            ValidationStatus::Invalid);  // value > 0.0
  EXPECT_EQ(rt.validate_active_field_partial("abc"), ValidationStatus::Invalid);
}

TEST(RuntimeTest, ValidatesQuantityPrefixesAndBoundaries) {
  Runtime rt;
  rt.submitValStr("unit_price", "10.00");  // Advance to quantity

  ASSERT_EQ(rt.getActiveFieldName(), "quantity");

  // Prefix checks
  EXPECT_EQ(rt.validate_active_field_partial(""),
            ValidationStatus::PartialValid);
  EXPECT_EQ(rt.validate_active_field_partial("5"), ValidationStatus::Valid);

  // Numerical constraints (value >= 1 && value <= 1000)
  EXPECT_EQ(rt.validate_active_field_partial("0"), ValidationStatus::Invalid);
  EXPECT_EQ(rt.validate_active_field_partial("1000"), ValidationStatus::Valid);
  EXPECT_EQ(rt.validate_active_field_partial("1001"),
            ValidationStatus::Invalid);
  EXPECT_EQ(rt.validate_active_field_partial("12.5"),
            ValidationStatus::Invalid);  // Not an int
}

TEST(RuntimeTest, ValidatesCurrencyExactAndPrefixes) {
  Runtime rt;
  rt.submitValStr("unit_price", "10.00");
  rt.submitValStr("quantity", "5");

  ASSERT_EQ(rt.getActiveFieldName(), "currency");

  // Prefixes of currencies
  EXPECT_EQ(rt.validate_active_field_partial("U"),
            ValidationStatus::PartialValid);
  EXPECT_EQ(rt.validate_active_field_partial("EU"),
            ValidationStatus::PartialValid);

  // Exact matches
  EXPECT_EQ(rt.validate_active_field_partial("USD"), ValidationStatus::Valid);
  EXPECT_EQ(rt.validate_active_field_partial("GBP"), ValidationStatus::Valid);

  // Out of domain
  EXPECT_EQ(rt.validate_active_field_partial("CAD"), ValidationStatus::Invalid);
  EXPECT_EQ(rt.validate_active_field_partial("USDE"),
            ValidationStatus::Invalid);
}

TEST(RuntimeTest, RejectsInvalidSubmissions) {
  Runtime rt;

  // Submitting invalid values should throw runtime_error
  EXPECT_THROW(rt.submitValStr("unit_price", "-10.00"), std::runtime_error);
  EXPECT_THROW(rt.submitValStr("unit_price", "abc"), std::runtime_error);
}

TEST(RuntimeTest, StepsThroughCompletePipelineAndSolvesDeterministically) {
  Runtime rt;

  // Submit unit_price
  rt.submitValStr("unit_price", "15.50");
  EXPECT_EQ(rt.getActiveFieldName(), "quantity");

  // Submit quantity
  rt.submitValStr("quantity", "100");
  EXPECT_EQ(rt.getActiveFieldName(), "currency");

  // Submit currency
  rt.submitValStr("currency", "EUR");
  EXPECT_EQ(rt.getActiveFieldName(), "total_price");

  // Solve total_price mathematically
  // total_price = 15.50 * 100 = 1550.0
  ASSERT_TRUE(rt.isAciveFieldDeterministic());

  std::string total_str = rt.solveDeterministic();
  EXPECT_DOUBLE_EQ(std::stod(total_str), 1550.0);

  // Ensure state machine is fully completed
  EXPECT_FALSE(rt.hasMoreFields());

  // Verify internal Environment Map State
  const auto& env = rt.get_environment();
  EXPECT_DOUBLE_EQ(std::get<double>(env.at("unit_price")), 15.50);
  EXPECT_EQ(std::get<int>(env.at("quantity")), 100);
  EXPECT_EQ(std::get<std::string>(env.at("currency")), "EUR");
  EXPECT_DOUBLE_EQ(std::get<double>(env.at("total_price")), 1550.0);
}

TEST(RuntimeTest, ResetClearsEnvironmentAndSteps) {
  Runtime rt;
  rt.submitValStr("unit_price", "5.00");

  EXPECT_EQ(rt.getActiveFieldName(), "quantity");
  EXPECT_FALSE(rt.get_environment().empty());

  rt.reset();

  EXPECT_EQ(rt.getActiveFieldName(), "unit_price");
  EXPECT_TRUE(rt.get_environment().empty());
  EXPECT_TRUE(rt.hasMoreFields());
}