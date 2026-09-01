#pragma once

#include <string>
#include <string_view>

#include "dependency_analyzer.hpp"
#include "evaluator.hpp"

namespace invariants::runtime {

enum class ValidationStatus { Valid, PartialValid, Invalid };

class Runtime {
 public:
  Runtime(const binder::BoundModule& module,
          const analysis::ExecutionSchedule& schedule);

  // State machine lifecycle
  void reset();
  bool hasMoreFields() const;

  // Returns the name of the field the LLM is currently supposed to generate
  std::string getActiveFieldName() const;

  // Zero-token bypass logic
  bool isActiveFieldDeterministic() const;
  std::string solveDeterministic();

  // Evaluates an incomplete/complete LLM string against the field's triggers
  ValidationStatus validatePartial(std::string_view proposedChars) const;

  // Commits a validated LLM generation to the C++ Environment and advances the
  // step
  void submitValStr(std::string_view name, std::string_view raw_str);

  const binder::FieldSymbol* getActiveFieldSymbol() const;

  const Environment& getEnvironment() const;

 private:
  const binder::BoundModule& boundModule;
  const analysis::ExecutionSchedule& schedule;

  Environment environment;
  Evaluator evaluator;
  size_t currStepIdx = 0;

  // Internal helpers
  void submitVal(const std::string& name, const Value& val);
};

}  // namespace invariants::runtime