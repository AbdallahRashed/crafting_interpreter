#include "Interpreter.h"
#include "TokenType.h"
#include <any>
#include <string>
#include <stdexcept>
#include <typeinfo>
#include <iostream>

  std::any Interpreter:: visitExpressionStmt(Expression* stmt)
  {

   evaluate(stmt->expression.get());
   return nullptr;

  }
 std::any Interpreter:: visitPrintStmt(Print* stmt)
 {

  std::any value=evaluate(stmt->expression.get());
  std::cout<<stringify(value)<<std::endl;
  return nullptr;  

 }

std::any Interpreter::visitLiteralExpr(Literal* expr) {
    std::cout << "Evaluating literal: " << stringify(expr->value) << std::endl;
    return expr->value;    
}

std::any Interpreter::visitGroupingExpr(Grouping* expr) {
    return evaluate(expr->expression.get());
}

std::any Interpreter::visitUnaryExpr(Unary* expr) {
    std::any right = evaluate(expr->right.get());

    switch (expr->op.type) {
        case MINUS:
            checkNumberOperand(expr->op, right);
            return -std::any_cast<double>(right);
        case BANG:
            return !isTruthy(right);
        default:
            break; // Unreachable
    }
    return nullptr;
}

std::any Interpreter::visitBinaryExpr(Binary* expr) {
    std::any left = evaluate(expr->left.get());
    std::any right = evaluate(expr->right.get());
    std::cout << "Evaluating binary expression: " << tokenTypeToString(expr->op.type) << std::endl;
    std::cout<< "Left operand: " << stringify(left) << ", Right operand: " << stringify(right) << std::endl;
    switch (expr->op.type) {
        case PLUS:
            if (left.type() == typeid(double) && right.type() == typeid(double)) {
                return std::any_cast<double>(left) + std::any_cast<double>(right);
            }
            if(left.type() == typeid(std::string) && right.type() == typeid(std::string)) {
                return std::any_cast<std::string>(left) + std::any_cast<std::string>(right);
            }
            throw RuntimeError(expr->op, "Operands must be two numbers or two strings.");
        case MINUS:
            checkNumberOperands(expr->op, left, right);
            return std::any_cast<double>(left) - std::any_cast<double>(right);
        case STAR:
            checkNumberOperands(expr->op, left, right);
            return std::any_cast<double>(left) * std::any_cast<double>(right);
        case SLASH:
            checkNumberOperands(expr->op, left, right);
            if (std::any_cast<double>(right) == 0) {
                throw RuntimeError(expr->op, "Division by zero.");
            }
            return std::any_cast<double>(left) / std::any_cast<double>(right);
        case GREATER:
            checkNumberOperands(expr->op, left, right);
            return std::any_cast<double>(left) > std::any_cast<double>(right);
        case GREATER_EQUAL:
            checkNumberOperands(expr->op, left, right);
            return std::any_cast<double>(left) >= std::any_cast<double>(right);
        case LESS:
            checkNumberOperands(expr->op, left, right);
            return std::any_cast<double>(left) < std::any_cast<double>(right);
        case LESS_EQUAL:
            checkNumberOperands(expr->op, left, right);
            return std::any_cast<double>(left) <= std::any_cast<double>(right);
        case EQUAL_EQUAL:
            return isEqual(left, right);
        case BANG_EQUAL:
            return !isEqual(left, right);   
        default:
            break; // Unreachable
    }
    return nullptr;
}

// Stub implementations for remaining visitor methods
std::any Interpreter::visitAssignExpr(Assign* expr) {
    throw std::runtime_error("Assign not yet implemented.");
}

std::any Interpreter::visitCallExpr(Call* expr) {
    throw std::runtime_error("Call not yet implemented.");
}

std::any Interpreter::visitGetExpr(Get* expr) {
    throw std::runtime_error("Get not yet implemented.");
}

std::any Interpreter::visitLogicalExpr(Logical* expr) {
    throw std::runtime_error("Logical not yet implemented.");
}

std::any Interpreter::visitSetExpr(Set* expr) {
    throw std::runtime_error("Set not yet implemented.");
}

std::any Interpreter::visitSuperExpr(Super* expr) {
    throw std::runtime_error("Super not yet implemented.");
}

std::any Interpreter::visitThisExpr(This* expr) {
    throw std::runtime_error("This not yet implemented.");
}

std::any Interpreter::visitVariableExpr(Variable* expr) {
    throw std::runtime_error("Variable not yet implemented.");
}

// Public interface

void Interpreter::interpret(const std::vector<std::shared_ptr<Stmt>>& statements) {
    try {
        for (const auto& statement : statements) {
            execute(statement.get());
        }
    } catch (const RuntimeError& error) {
        std::cerr << "[line " << error.token.line << "] Runtime error: " << error.what() << std::endl;
    }
}
std::any Interpreter::interpret(Expr* expr) {
    try {
        return evaluate(expr);
    } catch (const RuntimeError& error) {
        std::cerr << "[line " << error.token.line << "] Runtime error: " << error.what() << std::endl;
        return std::any();
    }
}

std::string Interpreter::stringify(const std::any& value) {
    if (!value.has_value()) return "nil";
    
    if (value.type() == typeid(bool)) {
        return std::any_cast<bool>(value) ? "true" : "false";
    }
    
    if (value.type() == typeid(double)) {
        std::string text = std::to_string(std::any_cast<double>(value));
        // Remove trailing zeros
        text.erase(text.find_last_not_of('0') + 1, std::string::npos);
        if (text.back() == '.') text.pop_back();
        return text;
    }
    
    if (value.type() == typeid(std::string)) {
        return std::any_cast<std::string>(value);
    }
    
    return "unknown";
}

// Private helper methods
std::any Interpreter::evaluate(Expr* expr) {
    return expr->accept(*this); 
}

void Interpreter::execute(Stmt* stmt) {
    stmt->accept(*this);
}

bool Interpreter::isTruthy(const std::any& val) {
    if (!val.has_value()) return false;
    if (val.type() == typeid(bool)) return std::any_cast<bool>(val);
    return true;
}

bool Interpreter::isEqual(const std::any& a, const std::any& b) {
    if (!a.has_value() && !b.has_value()) return true;
    if (!a.has_value() || !b.has_value()) return false;
    if (a.type() != b.type()) return false;

    try {
        if (a.type() == typeid(bool)) {
            return std::any_cast<bool>(a) == std::any_cast<bool>(b);
        } else if (a.type() == typeid(double)) {
            return std::any_cast<double>(a) == std::any_cast<double>(b);
        } else if (a.type() == typeid(std::string)) {
            return std::any_cast<std::string>(a) == std::any_cast<std::string>(b);
        }
    } catch (...) {
        return false; // If any cast fails, consider them not equal
    }
    return false; // For unsupported types, consider them not equal
}

void Interpreter::checkNumberOperand(const Token& op, const std::any& operand) {
    if (operand.type() == typeid(double)) return;
    throw RuntimeError(op, "Operand must be a number.");
}

void Interpreter::checkNumberOperands(const Token& op, const std::any& left, const std::any& right) {
    if (left.type() == typeid(double) && right.type() == typeid(double)) return;
    throw RuntimeError(op, "Operands must be numbers.");
}