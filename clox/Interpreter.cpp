#include "Interpreter.h"
#include "TokenType.h"
#include <any>
#include <string>
#include <stdexcept>
#include <typeinfo>
#include <iostream>

// ==================== EXPRESSION VISITORS ====================

std::any Interpreter::visitLiteralExpr(Literal* expr) {
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

// ==================== EXPRESSION VISITORS (Updated) ====================

std::any Interpreter::visitAssignExpr(Assign* expr) {
    std::any value = evaluate(expr->value.get());
    environment->assign(expr->name, value);
    return value;
}

std::any Interpreter::visitCallExpr(Call* expr) {
    std::any callee = evaluate(expr->callee.get());
    std::vector<std::any> arguments;
    for (const auto& arg : expr->arguments) {
        arguments.push_back(evaluate(arg.get()));
    }

    if (callee.type() != typeid(std::shared_ptr<LoxCallable>)) {
        throw RuntimeError(expr->paren, "Can only call functions and classes.");
    }
    std::shared_ptr<LoxCallable> function = std::any_cast<std::shared_ptr<LoxCallable>>(callee);

    if (static_cast<int>(arguments.size()) != function->arity()) {
        throw RuntimeError(expr->paren, "Expected " + std::to_string(function->arity()) +
                          " arguments but got " + std::to_string(arguments.size()) + ".");
    }
    return function->call(*this, arguments);
}

std::any Interpreter::visitGetExpr(Get* expr) {
    throw std::runtime_error("Get not yet implemented.");
}

std::any Interpreter::visitLogicalExpr(Logical* expr) {
    std::any left = evaluate(expr->left.get());
    
    // Short-circuit evaluation
    if (expr->op.type == OR) {
        if (isTruthy(left)) return left;
    } else { // AND
        if (!isTruthy(left)) return left;
    }
    
    return evaluate(expr->right.get());
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
    return environment->get(expr->name);
}

// ==================== STATEMENT VISITORS ====================

std::any Interpreter::visitExpressionStmt(Expression* stmt) {
    evaluate(stmt->expression.get());
    return std::any();  // No return value
}

std::any Interpreter::visitPrintStmt(Print* stmt) {
    std::any value = evaluate(stmt->expression.get());
    std::cout << stringify(value) << std::endl;
    return std::any();
}

std::any Interpreter::visitVarStmt(Var* stmt) {
    std::any value;
    if (stmt->initializer != nullptr) {
        value = evaluate(stmt->initializer.get());
    }
    // If no initializer, value is default-constructed std::any (empty = nil)
    
    environment->define(stmt->name.lexeme, value);
    return std::any();
}

std::any Interpreter::visitBlockStmt(Block* stmt) {
    executeBlock(stmt->statements, std::make_shared<Environment>(environment));
    return std::any();
}

std::any Interpreter::visitIfStmt(If* stmt) {
    if (isTruthy(evaluate(stmt->condition.get()))) {
        execute(stmt->thenBranch.get());
    } else if (stmt->elseBranch != nullptr) {
        execute(stmt->elseBranch.get());
    }
    return std::any();
}

std::any Interpreter::visitWhileStmt(While* stmt) {
    while (isTruthy(evaluate(stmt->condition.get()))) {
        execute(stmt->body.get());
    }
    return std::any();
}

std::any Interpreter::visitFunctionStmt(Function* stmt) {
    std::shared_ptr<LoxCallable> function = std::make_shared<LoxFunction>(stmt);
    environment->define(stmt->name.lexeme, function);
    return std::any();
}

std::any LoxFunction::call(Interpreter& interpreter, const std::vector<std::any>& arguments) {
    std::shared_ptr<Environment> env = std::make_shared<Environment>(interpreter.globals);
    for (size_t i = 0; i < declaration->params.size(); ++i) {
        env->define(declaration->params[i].lexeme, arguments[i]);
    }
    try {
        interpreter.executeBlock(declaration->body, env);
    } catch (const ReturnException& ret) {
        return ret.value;
    }
    return std::any();  // implicit nil return
}

std::any Interpreter::visitReturnStmt(Return* stmt) {
    std::any value;
    if (stmt->value != nullptr) {
        value = evaluate(stmt->value.get());
    }
    throw ReturnException(value);
}

std::any Interpreter::visitClassStmt(Class* stmt) {
    throw std::runtime_error("Class not yet implemented.");
}

// ==================== PUBLIC INTERFACE ====================

// NEW: Interpret a list of statements (main entry point)
void Interpreter::interpret() {
    globals->define("clock", std::shared_ptr<LoxCallable>(std::make_shared<ClockCallable>()));
}
void Interpreter::interpret(const std::vector<std::shared_ptr<Stmt>>& statements) {
    try {
        for (const auto& statement : statements) {
            execute(statement.get());
        }
    } catch (const RuntimeError& error) {
        std::cerr << "[line " << error.token.line << "] Runtime error: " << error.what() << std::endl;
    } catch (const std::runtime_error& error) {
        std::cerr << "Runtime error: " << error.what() << std::endl;
    }
}

// LEGACY: Interpret single expression (for existing tests)
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

// ==================== PRIVATE HELPER METHODS ====================

std::any Interpreter::evaluate(Expr* expr) {
    return expr->accept(*this); 
}

void Interpreter::execute(Stmt* stmt) {
    stmt->accept(*this);
}

void Interpreter::executeBlock(const std::vector<std::shared_ptr<Stmt>>& statements,
                                std::shared_ptr<Environment> env) {
    // Save current environment
    std::shared_ptr<Environment> previous = environment;
    
    try {
        // Switch to new environment
        environment = env;
        
        // Execute all statements in the block
        for (const auto& statement : statements) {
            execute(statement.get());
        }
        
        // Restore previous environment
        environment = previous;
    } catch (...) {
        // Ensure environment is restored even on error
        environment = previous;
        throw;
    }
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