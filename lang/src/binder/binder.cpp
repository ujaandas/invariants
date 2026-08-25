#include "binder.hpp"

#include <cstddef>
#include <stdexcept>

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

  // Bind fields
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
  ResolvedType type = bindType(*fieldAst.type);

  activeField =
      table.add_field(activeSpec->name, fieldAst.identifier, type, &fieldAst);
  if (!activeField) {
    throw std::runtime_error("Duplicate field name: " + fieldAst.identifier);
  }

  BoundField boundField{.symbol = activeField};

  for (const auto& constraintAst : fieldAst.constraints) {
    if (!constraintAst) continue;
    boundField.local_constraints.push_back(bindConstraint(*constraintAst));
  }

  activeField = nullptr;
  return boundField;
}

BoundConstraint Binder::bindConstraint(
    const ast::ConstraintStmt& constraintAst) {
  BoundExprPtr expr = bindExpr(*constraintAst.expression);

  // Validate it returns a boolean
  if (!expr->type.isBuiltin() || std::get<ast::BuiltinType>(expr->type.type) !=
                                     ast::BuiltinType::Boolean) {
    throw std::runtime_error(
        "Constraint expression must evaluate to a Boolean.");
  }

  return BoundConstraint{.expr = std::move(expr)};
}

BoundInvariant Binder::bindInvariant(const ast::InvariantStmt& invariantAst) {
  // Assuming invariant has a single root expression constraint for now
  if (invariantAst.constraints.empty()) {
    throw std::runtime_error("Invariant must have an expression.");
  }

  BoundExprPtr expr = bindExpr(*invariantAst.constraints.front()->expression);

  BoundInvariant boundInv{
      .name = invariantAst.identifier,
      .expression = std::move(expr),
      .isDeterministicPossible =
          false,  // TODO: Add detection for target fields later
      .target = nullptr};

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
        } else if constexpr (std::is_same_v<T, ast::ThisExpr>) {
          throw std::runtime_error("'this' cannot be evaluated on its own.");
        } else if constexpr (std::is_same_v<T, ast::ListExpr>) {
          throw std::runtime_error("List expr binding not implemented yet.");
        }
      },
      exprAst.value);
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
  // For now, only handling simple `this.fieldName`
  if (std::holds_alternative<ast::ThisExpr>(expr.base->value)) {
    if (expr.ops.size() != 1 ||
        !std::holds_alternative<ast::MemberAccessOp>(expr.ops[0])) {
      throw std::runtime_error(
          "Only simple member access on 'this' is supported.");
    }

    const std::string& fieldName =
        std::get<ast::MemberAccessOp>(expr.ops[0]).member;
    const FieldSymbol* field = table.lookup_field(activeSpec->name, fieldName);

    if (!field) {
      throw std::runtime_error("Unknown field: " + fieldName);
    }

    return std::make_unique<BoundExpr>(BoundFieldAccessExpr{field},
                                       field->resType);
  }
  throw std::runtime_error("Complex postfix expressions not implemented yet.");
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

  // Basic type deduction
  ResolvedType outType;
  if (expr.op == ast::BinaryOp::Equal || expr.op == ast::BinaryOp::NotEqual ||
      expr.op == ast::BinaryOp::Greater || expr.op == ast::BinaryOp::Less ||
      expr.op == ast::BinaryOp::GreaterEqual ||
      expr.op == ast::BinaryOp::LessEqual || expr.op == ast::BinaryOp::And ||
      expr.op == ast::BinaryOp::Or || expr.op == ast::BinaryOp::In ||
      expr.op == ast::BinaryOp::Imply) {
    outType = ResolvedType{ast::BuiltinType::Boolean};
  } else {
    // Math ops return the type of the left operand for now
    outType = left->type;
  }

  return std::make_unique<BoundExpr>(
      BoundBinaryExpr{std::move(left), expr.op, std::move(right)}, outType);
}

}  // namespace invariants::binder