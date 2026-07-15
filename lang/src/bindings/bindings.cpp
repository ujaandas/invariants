#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "runtime.hpp"

namespace py = pybind11;
using namespace invariants::runtime;

PYBIND11_MODULE(invariants, m) {
  m.doc() = "Invariants LLM constrained execution runtime engine";

  // Export FieldType enum
  py::enum_<FieldType>(m, "FieldType")
      .value("Integer", FieldType::Integer)
      .value("Number", FieldType::Number)
      .value("String", FieldType::String)
      .export_values();

  // Export ValidationStatus enum
  py::enum_<ValidationStatus>(m, "ValidationStatus")
      .value("Valid", ValidationStatus::Valid)
      .value("PartialValid", ValidationStatus::PartialValid)
      .value("Invalid", ValidationStatus::Invalid)
      .export_values();

  // Export Runtime class
  py::class_<Runtime>(m, "Runtime")
      .def(py::init<>())
      .def("has_more_fields", &Runtime::hasMoreFields,
           "Returns true if there are more fields to generate in the DAG.")

      .def("get_active_field_name", &Runtime::getActiveFieldName,
           "Gets the name of the currently active field.")

      .def("get_active_field_type", &Runtime::getActiveFieldType,
           "Gets the FieldType of the currently active field.")

      .def("get_gen_order", &Runtime::getGenOrder,
           "Gets the pre-sorted topological generation order of fields.")

      .def("submit_val", &Runtime::submitVal,
           "Submits a resolved Python Value (int, float, or str) directly.",
           py::arg("name"), py::arg("val"))

      .def("submit_val_str", &Runtime::submitValStr,
           "Submits a raw string to be parsed and validated by C++.",
           py::arg("name"), py::arg("raw_str"))

      .def("is_active_field_deterministic", &Runtime::isAciveFieldDeterministic,
           "Returns true if the active field is wholly solved by constraints.")

      .def("solve_deterministic", &Runtime::solveDeterministic,
           "Computes, commits, and returns the string representation of a "
           "deterministic field.")

      .def("validate_active_field_partial",
           &Runtime::validate_active_field_partial,
           "Evaluates if appending a proposed string maintains safety "
           "constraints.",
           py::arg("proposed_chars"))

      .def("get_environment", &Runtime::get_environment,
           "Returns the current map of resolved environment variables.")

      .def("reset", &Runtime::reset,
           "Resets the state machine and environment.");
}