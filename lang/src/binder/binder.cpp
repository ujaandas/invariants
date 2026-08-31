#include "binder.hpp"

#include <cstddef>
#include <optional>
#include <stdexcept>
#include <variant>

#include "bound_expr.hpp"
#include "expression.hpp"

namespace invariants::binder {

BoundModule Binder::bind(const ast::ModuleStmt& moduleAst) {
  BoundModule boundModule;
  for (const auto& specAst : moduleAst.specs) {
    if (!specAst) continue;
    boundModule.specs.push_back(bindSpec(*specAst));
  }
  return boundModule;
}

BoundSpec Binder::bindSpec(const ast::SpecStmt& specAst) {
  // Register spec
  activeSpec = table.add_spec(specAst.identifier, &specAst);
  if (!activeSpec) {
    throw std::runtime_error("Duplicate specification name: " +
                             specAst.identifier);
  }

  BoundSpec boundSpec{.symbol = activeSpec};

  // PASS 1: Register all fields in the symbol table to allow forward references
  for (const auto& member : specAst.members) {
    if (std::holds_alternative<ast::FieldStmt>(member)) {
      const auto& fieldAst = std::get<ast::FieldStmt>(member);
      ResolvedType type = bindType(*fieldAst.type);
      if (!table.add_field(activeSpec->name, fieldAst.identifier, type,
                           &fieldAst)) {
        throw std::runtime_error("Duplicate field name: " +
                                 fieldAst.identifier);
      }
    }
  }

  // PASS 2: Bind constraints now that all fields are globally visible
  for (const auto& member : specAst.members) {
    if (std::holds_alternative<ast::FieldStmt>(member)) {
      boundSpec.fields.push_back(bindField(std::get<ast::FieldStmt>(member)));
    }
  }

  // Bind invariants
  for (const auto& member : specAst.members) {
    if (std::holds_alternative<ast::InvariantStmt>(member)) {
      boundSpec.invariants.push_back(
          bindInvariant(std::get<ast::InvariantStmt>(member)));
    }
  }

  activeSpec = nullptr;
  return boundSpec;
}

BoundField Binder::bindField(const ast::FieldStmt& fieldAst) {
  // Fetch the field we already registered in Pass 1 and cast away the const
  activeField = const_cast<FieldSymbol*>(
      table.lookup_field(activeSpec->name, fieldAst.identifier));

  if (!activeField) {
    throw std::runtime_error(
        "Fatal: Field not found in symbol table during pass 2: " +
        fieldAst.identifier);
  }

  BoundField boundField{.symbol = activeField};

  for (const auto& constraintAst : fieldAst.constraints) {
    if (!constraintAst) continue;
    boundField.constraints.push_back(bindConstraint(*constraintAst));
  }

  activeField = nullptr;
  return boundField;
}

BoundConstraint Binder::bindConstraint(
    const ast::ConstraintStmt& constraintAst) {
  BoundExprPtr expr = bindExpr(*constraintAst.expression);
  if (!expr->type.isBuiltin() || std::get<ast::BuiltinType>(expr->type.type) !=
                                     ast::BuiltinType::Boolean) {
    throw std::runtime_error("Constraint expression must evaluate to Boolean.");
  }

  bool isDeterministic = false;
  std::string target = "";

  if (std::holds_alternative<BoundBinaryExpr>(expr->value)) {
    const auto& bin = std::get<BoundBinaryExpr>(expr->value);
    if (bin.op == ast::BinaryOp::Equal) {
      if (std::holds_alternative<BoundFieldAccessExpr>(bin.left->value)) {
        isDeterministic = true;
        target = std::get<BoundFieldAccessExpr>(bin.left->value).flattenedPath;
      } else if (std::holds_alternative<BoundFieldAccessExpr>(
                     bin.right->value)) {
        isDeterministic = true;
        target = std::get<BoundFieldAccessExpr>(bin.right->value).flattenedPath;
      }
      // Catch 'value' on either side. Leave target empty so the Analyzer
      // maps it to the owner field
      else if (std::holds_alternative<BoundValueAccessExpr>(bin.left->value) ||
               std::holds_alternative<BoundValueAccessExpr>(bin.right->value)) {
        isDeterministic = true;
        target = "";
      }
    }
  }

  return BoundConstraint{
      .expr = std::move(expr),
      .isDeterministicPossible = isDeterministic,
      .target = target,
  };
}

BoundInvariant Binder::bindInvariant(const ast::InvariantStmt& invariantAst) {
  BoundInvariant boundInv{
      .name = invariantAst.identifier,
      .constraints = {},
  };

  for (const auto& constraintAst : invariantAst.constraints) {
    if (!constraintAst || !constraintAst->expression) continue;

    BoundExprPtr expr = bindExpr(*constraintAst->expression);
    if (!expr->type.isBuiltin() ||
        std::get<ast::BuiltinType>(expr->type.type) !=
            ast::BuiltinType::Boolean) {
      throw std::runtime_error(
          "Invariant constraint expression must evaluate to a Boolean.");
    }

    bool isDet = false;
    std::string target = "";

    if (std::holds_alternative<BoundBinaryExpr>(expr->value)) {
      const auto& binExpr = std::get<BoundBinaryExpr>(expr->value);

      // Look for an equality check (==)
      if (binExpr.op == ast::BinaryOp::Equal) {
        // Is the left side `this.some_field`?
        if (std::holds_alternative<BoundFieldAccessExpr>(binExpr.left->value)) {
          isDet = true;
          target =
              std::get<BoundFieldAccessExpr>(binExpr.left->value).flattenedPath;
        }
        // Is the right side `this.some_field`?
        else if (std::holds_alternative<BoundFieldAccessExpr>(
                     binExpr.right->value)) {
          isDet = true;
          target = std::get<BoundFieldAccessExpr>(binExpr.right->value)
                       .flattenedPath;
        }
      }
    }

    boundInv.constraints.push_back(BoundConstraint{
        .expr = std::move(expr),
        .isDeterministicPossible = isDet,
        .target = target,
    });
  }

  if (boundInv.constraints.empty()) {
    throw std::runtime_error("Invariant must have an expression.");
  }

  return boundInv;
}

ResolvedType Binder::bindType(const ast::Type& typeAst) {
  return std::visit(
      [this](auto&& arg) -> ResolvedType {
        using T = std::decay_t<decltype(arg)>;
        if constexpr (std::is_same_v<T, ast::SimpleType>) {
          if (std::holds_alternative<ast::BuiltinType>(arg.value)) {
            return ResolvedType{std::get<ast::BuiltinType>(arg.value)};
          } else {
            const std::string& customName = std::get<std::string>(arg.value);
            const SpecSymbol* spec = table.lookup_spec(customName);
            if (!spec) throw std::runtime_error("Unknown type: " + customName);
            return ResolvedType{spec};
          }
        } else if constexpr (std::is_same_v<T, ast::ArrayType>) {
          return ResolvedType{std::make_shared<ResolvedArrayType>(
              ResolvedArrayType{bindType(*arg.element)})};
        } else if constexpr (std::is_same_v<T, ast::MapType>) {
          return ResolvedType{std::make_shared<ResolvedMapType>(
              ResolvedMapType{bindType(*arg.key), bindType(*arg.value)})};
        }
      },
      typeAst.value);
}

BoundExprPtr Binder::bindExpr(const ast::Expr& exprAst) {
  return std::visit(
      [this](auto&& arg) -> BoundExprPtr {
        using T = std::decay_t<decltype(arg)>;

        // GroupingExpr is skipped entirely! We just bind the inner expression.
        if constexpr (std::is_same_v<T, ast::GroupingExpr>) {
          return bindExpr(*arg.expression);
        } else if constexpr (std::is_same_v<T, ast::LiteralExpr>) {
          return bindLiteral(arg);
        } else if constexpr (std::is_same_v<T, ast::IdentifierExpr>) {
          return bindIdentifier(arg);
        } else if constexpr (std::is_same_v<T, ast::PostfixExpr>) {
          return bindPostfix(arg);
        } else if constexpr (std::is_same_v<T, ast::BinaryExpr>) {
          return bindBinary(arg);
        } else if constexpr (std::is_same_v<T, ast::UnaryExpr>) {
          return bindUnary(arg);
        } else if constexpr (std::is_same_v<T, ast::ListExpr>) {
          return bindListExpr(arg);
        } else if constexpr (std::is_same_v<T, ast::ThisExpr>) {
          throw std::runtime_error("'this' cannot be evaluated on its own.");
        }
      },
      exprAst.value);
}

BoundExprPtr Binder::bindListExpr(const ast::ListExpr& ast) {
  if (ast.elements.empty()) {
    throw std::runtime_error(
        "Empty lists are not supported yet (cannot infer type).");
  }

  std::vector<BoundExprPtr> boundElements;
  std::optional<ResolvedType> elementType;

  for (const auto& el : ast.elements) {
    auto boundEl = bindExpr(*el);

    // Ensure all elements in the list are the exact same type
    if (!elementType) {
      elementType = boundEl->type;
    } else {
      bool isEqual = false;

      // TODO: Assume lists only contain built-in primitives
      if (elementType->isBuiltin() && boundEl->type.isBuiltin()) {
        isEqual = std::get<ast::BuiltinType>(elementType->type) ==
                  std::get<ast::BuiltinType>(boundEl->type.type);
      }

      if (!isEqual) {
        throw std::runtime_error(
            "All elements in a list must be of the same type.");
      }
    }

    boundElements.push_back(std::move(boundEl));
  }

  // Create the array type using the shared_ptr expected by TypeVar
  auto resolvedArray = std::make_shared<binder::ResolvedArrayType>(
      binder::ResolvedArrayType{*elementType});

  return std::make_unique<BoundExpr>(BoundListExpr{std::move(boundElements)},
                                     ResolvedType{std::move(resolvedArray)});
}

BoundExprPtr Binder::bindLiteral(const ast::LiteralExpr& expr) {
  return std::visit(
      [](auto&& val) -> BoundExprPtr {
        using T = std::decay_t<decltype(val)>;

        ast::BuiltinType type = {};

        if constexpr (std::is_same_v<T, double>) {
          type = ast::BuiltinType::Number;
        } else if constexpr (std::is_same_v<T, int>) {
          type = ast::BuiltinType::Number;
        } else if constexpr (std::is_same_v<T, std::string>) {
          type = ast::BuiltinType::String;
        } else if constexpr (std::is_same_v<T, bool>) {
          type = ast::BuiltinType::Boolean;
        } else if constexpr (std::is_same_v<T, std::nullptr_t>) {
          throw std::runtime_error("Nullptr literal not supported yet.");
        }

        return std::make_unique<BoundExpr>(BoundLiteralExpr{val},
                                           ResolvedType{type});
      },
      expr.value);
}

BoundExprPtr Binder::bindIdentifier(const ast::IdentifierExpr& expr) {
  if (expr.name == "value") {
    if (!activeField) {
      throw std::runtime_error(
          "The 'value' keyword can only be used inside field constraints.");
    }
    return std::make_unique<BoundExpr>(
        BoundValueAccessExpr{activeField->resType}, activeField->resType);
  }
  throw std::runtime_error("Naked identifiers are not allowed. Use 'this." +
                           expr.name + "'.");
}

BoundExprPtr Binder::bindPostfix(const ast::PostfixExpr& expr) {
  // For now, only handling member access originating from `this`
  if (!std::holds_alternative<ast::ThisExpr>(expr.base->value)) {
    throw std::runtime_error(
        "Only member access on 'this' is supported currently.");
  }

  const SpecSymbol* currentSpec = activeSpec;
  const FieldSymbol* finalField = nullptr;
  std::string flattenedPath = "";

  for (size_t i = 0; i < expr.ops.size(); ++i) {
    if (!std::holds_alternative<ast::MemberAccessOp>(expr.ops[i])) {
      throw std::runtime_error("Only dot member access is supported.");
    }

    const std::string& memberName =
        std::get<ast::MemberAccessOp>(expr.ops[i]).member;

    // Look up the member in the current scope
    finalField = table.lookup_field(currentSpec->name, memberName);
    if (!finalField) {
      throw std::runtime_error("Unknown field '" + memberName + "' in spec '" +
                               currentSpec->name + "'.");
    }

    // Append to our dot-flattened runtime string
    flattenedPath += (i > 0 ? "." : "") + memberName;

    // If this is NOT the final operation, the current field MUST be a custom
    // nested Spec
    if (i < expr.ops.size() - 1) {
      if (finalField->resType.isBuiltin() || finalField->resType.isArray() ||
          finalField->resType.isMap()) {
        throw std::runtime_error(
            "Cannot access member on primitive or collection field '" +
            memberName + "'.");
      }
      // Jump into the nested spec for the next loop iteration
      currentSpec = std::get<const SpecSymbol*>(finalField->resType.type);
    }
  }

  return std::make_unique<BoundExpr>(
      BoundFieldAccessExpr{finalField, flattenedPath}, finalField->resType);
}

BoundExprPtr Binder::bindUnary(const ast::UnaryExpr& expr) {
  auto operand = bindExpr(*expr.operand);
  // Simplify type deduction: Unary operations generally return the same type,
  // or boolean for Not
  ResolvedType outType = (expr.op == ast::UnaryOp::Not)
                             ? ResolvedType{ast::BuiltinType::Boolean}
                             : operand->type;
  return std::make_unique<BoundExpr>(
      BoundUnaryExpr{expr.op, std::move(operand)}, outType);
}

BoundExprPtr Binder::bindBinary(const ast::BinaryExpr& expr) {
  auto left = bindExpr(*expr.left);
  auto right = bindExpr(*expr.right);

  // Check array types
  if (expr.op == ast::BinaryOp::In || expr.op == ast::BinaryOp::NotIn) {
    // Check that the right side is an Array
    if (!std::holds_alternative<std::shared_ptr<binder::ResolvedArrayType>>(
            right->type.type)) {
      throw std::runtime_error("Right side of 'IN' operator must be an Array.");
    }

    // Check that the left side matches the array's inner element type
    auto arrayType =
        std::get<std::shared_ptr<binder::ResolvedArrayType>>(right->type.type);
    bool isMatch = false;

    if (left->type.isBuiltin() && arrayType->element.isBuiltin()) {
      isMatch = std::get<ast::BuiltinType>(left->type.type) ==
                std::get<ast::BuiltinType>(arrayType->element.type);
    }

    if (!isMatch) {
      throw std::runtime_error(
          "Left side of 'IN' operator must match the array's element type.");
    }
  }

  // Basic type deduction
  ResolvedType outType;
  if (expr.op == ast::BinaryOp::Equal || expr.op == ast::BinaryOp::NotEqual ||
      expr.op == ast::BinaryOp::Greater || expr.op == ast::BinaryOp::Less ||
      expr.op == ast::BinaryOp::GreaterEqual ||
      expr.op == ast::BinaryOp::LessEqual || expr.op == ast::BinaryOp::And ||
      expr.op == ast::BinaryOp::Or || expr.op == ast::BinaryOp::In ||
      expr.op == ast::BinaryOp::Imply || expr.op == ast::BinaryOp::NotIn) {
    outType = ResolvedType{ast::BuiltinType::Boolean};
  } else {
    // Math ops return the type of the left operand for now
    outType = left->type;
  }

  return std::make_unique<BoundExpr>(
      BoundBinaryExpr{std::move(left), expr.op, std::move(right)}, outType);
}

}  // namespace invariants::binder