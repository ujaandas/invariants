#include <pybind11/numpy.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "dependency_analyzer.hpp"
#include "runtime.hpp"
#include "symbol_table.hpp"

using invariants::analysis::ExecutionSchedule;
using invariants::ast::BuiltinType;
using invariants::binder::BoundModule;
using invariants::binder::ResolvedType;

namespace py = pybind11;
using namespace invariants::runtime;

PYBIND11_MODULE(invariants_cpp, m) {
  m.doc() = "Invariants LLM constrained execution runtime engine";

  py::enum_<ValidationStatus>(m, "ValidationStatus")
      .value("Valid", ValidationStatus::Valid)
      .value("PartialValid", ValidationStatus::PartialValid)
      .value("Invalid", ValidationStatus::Invalid)
      .export_values();

  py::class_<Runtime>(m, "Runtime")
      // Initialize with bound module and schedule
      .def(py::init<const BoundModule&, const ExecutionSchedule&>())

      .def("has_more_fields", &Runtime::hasMoreFields)
      .def("get_active_field_name", &Runtime::getActiveFieldName)
      .def("is_active_field_deterministic",
           &Runtime::isActiveFieldDeterministic)
      .def("solve_deterministic", &Runtime::solveDeterministic)

      // The single-string submission (used after a token completes)
      .def("submit_val_str", &Runtime::submitValStr, py::arg("name"),
           py::arg("raw_str"))

      // Python passes the entire vocabulary of strings, C++ loops over them
      // internally and returns a list of boolean values (the mask)
      .def(
          "validate_vocabulary_batch",
          [](const Runtime& rt, const std::vector<std::string>& vocab) {
            std::vector<bool> mask;
            mask.reserve(vocab.size());
            for (const auto& token : vocab) {
              // Consider rewriting validate partial
              auto status = rt.validatePartial(token);
              mask.push_back(status != ValidationStatus::Invalid);
            }
            return mask;
          },
          "Filters a list of candidate tokens, returning a boolean mask.");

  m.def(
      "process_logits_batch",
      [](const Runtime& rt, py::array_t<float> logits,
         const std::vector<int>& top_k_indices,
         const std::vector<std::string>& top_k_strings,
         const std::string& current_buffer) {
        py::buffer_info buf = logits.request();
        float* ptr = static_cast<float*>(buf.ptr);

        ResolvedType field_type = rt.getActiveFieldSymbol()->resType;
        bool isString =
            field_type.isBuiltin() &&
            std::get<BuiltinType>(field_type.type) == BuiltinType::String;

        for (size_t i = 0; i < top_k_indices.size(); ++i) {
          std::string proposed = current_buffer + top_k_strings[i];

          // Check for exit tokens BEFORE trimming whitespace!
          bool isExit = false;
          if (!proposed.empty() &&
              (proposed.back() == ',' || proposed.back() == '\n' ||
               proposed.back() == '}')) {
            isExit = true;
            proposed.pop_back();  // Remove the structural char for validation
          }

          // Trim remaining whitespace
          if (!proposed.empty()) {
            size_t first = proposed.find_first_not_of(" \t\r");  // Excluded \n
            if (first == std::string::npos) {
              proposed.clear();
            } else {
              proposed.erase(0, first);
              proposed.erase(proposed.find_last_not_of(" \t\r") + 1);
            }
          }

          // Handle JSON string quotes
          if (isString) {
            if (!proposed.empty() &&
                (proposed.front() == '"' || proposed.front() == '\''))
              proposed.erase(0, 1);
            if (!proposed.empty() &&
                (proposed.back() == '"' || proposed.back() == '\''))
              proposed.pop_back();
          }

          // Validate
          ValidationStatus status;
          if (isString && proposed.empty()) {
            status = ValidationStatus::PartialValid;
          } else {
            status = rt.validatePartial(proposed);
          }

          // Mask logits in-memory
          if (status == ValidationStatus::Invalid ||
              (isExit && status != ValidationStatus::Valid)) {
            int tokenId = top_k_indices[i];
            ptr[tokenId] = -std::numeric_limits<float>::infinity();
          }
        }
      },
      "Mutates logits in-place based on C++ constraint validation.");
}