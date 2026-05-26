#include "Resolver.h"

void Resolver::resolve(const std::vector<std::shared_ptr<Stmt>>& statements) {
    for (const auto& statement : statements) {
        resolve(statement.get());
    }
}

void Resolver::resolve(Stmt* stmt) {
    stmt->accept(*this);
}

void Resolver::resolve(Expr* expr) {
    expr->accept(*this);
}

void Resolver::beginScope() {
    scopes.push_back({});
}

void Resolver::endScope() {
    scopes.pop_back();
}

void Resolver::declare(const Token& name) {
    if (scopes.empty()) return;
    auto& scope = scopes.back();
    if (scope.find(name.lexeme) != scope.end()) {
        std::cerr << "Error: Variable '" << name.lexeme << "' already declared in this scope." << std::endl;
    }
    scope[name.lexeme] = false; // declared but not yet defined
}

void Resolver::define(const Token& name) {
    if (scopes.empty()) return;
    scopes.back()[name.lexeme] = true;
}

void Resolver::resolveLocal(Expr* expr, const Token& name) {
    // Walk from innermost scope outward
    for (int i = static_cast<int>(scopes.size()) - 1; i >= 0; --i) {
        if (scopes[i].find(name.lexeme) != scopes[i].end()) {
            // depth = distance from current scope to where it was found
            interpreter.resolve(expr, static_cast<int>(scopes.size()) - 1 - i);
            return;
        }
    }
    // Not found in any local scope — it's global, no resolve() call needed
}

void Resolver::resolveFunction(Function* stmt) {
    beginScope();
    for (const Token& param : stmt->params) {
        declare(param);
        define(param);
    }
    resolve(stmt->body);
    endScope();
}

// ==================== STATEMENT VISITORS ====================

std::any Resolver::visitBlockStmt(Block* stmt) {
    beginScope();
    resolve(stmt->statements);
    endScope();
    return std::any();
}

std::any Resolver::visitVarStmt(Var* stmt) {
    declare(stmt->name);
    if (stmt->initializer != nullptr) {
        resolve(stmt->initializer.get());
    }
    define(stmt->name);
    return std::any();
}

std::any Resolver::visitFunctionStmt(Function* stmt) {
    declare(stmt->name);
    define(stmt->name);
    resolveFunction(stmt);
    return std::any();
}

std::any Resolver::visitExpressionStmt(Expression* stmt) {
    resolve(stmt->expression.get());
    return std::any();
}

std::any Resolver::visitIfStmt(If* stmt) {
    resolve(stmt->condition.get());
    resolve(stmt->thenBranch.get());
    if (stmt->elseBranch != nullptr) resolve(stmt->elseBranch.get());
    return std::any();
}

std::any Resolver::visitPrintStmt(Print* stmt) {
    resolve(stmt->expression.get());
    return std::any();
}

std::any Resolver::visitReturnStmt(Return* stmt) {
    if (stmt->value != nullptr) resolve(stmt->value.get());
    return std::any();
}

std::any Resolver::visitWhileStmt(While* stmt) {
    resolve(stmt->condition.get());
    resolve(stmt->body.get());
    return std::any();
}

std::any Resolver::visitClassStmt(Class* stmt) {
    declare(stmt->name);
    define(stmt->name);

    if (stmt->superclass != nullptr) {
        if (stmt->superclass->name.lexeme == stmt->name.lexeme) {
            std::cerr << "Error: A class can't inherit from itself." << std::endl;
        }
        resolve(stmt->superclass.get());

        // Create a scope for 'super'
        beginScope();
        scopes.back()["super"] = true;
    }

    beginScope();
    scopes.back()["this"] = true;

    for (const auto& method : stmt->methods) {
        resolveFunction(method.get());
    }

    endScope();

    if (stmt->superclass != nullptr) endScope();

    return std::any();
}

// ==================== EXPRESSION VISITORS ====================

std::any Resolver::visitVariableExpr(Variable* expr) {
    if (!scopes.empty()) {
        auto& scope = scopes.back();
        auto it = scope.find(expr->name.lexeme);
        if (it != scope.end() && it->second == false) {
            std::cerr << "Error: Can't read local variable '" << expr->name.lexeme
                      << "' in its own initializer." << std::endl;
        }
    }
    resolveLocal(expr, expr->name);
    return std::any();
}

std::any Resolver::visitAssignExpr(Assign* expr) {
    resolve(expr->value.get());
    resolveLocal(expr, expr->name);
    return std::any();
}

std::any Resolver::visitBinaryExpr(Binary* expr) {
    resolve(expr->left.get());
    resolve(expr->right.get());
    return std::any();
}

std::any Resolver::visitCallExpr(Call* expr) {
    resolve(expr->callee.get());
    for (const auto& arg : expr->arguments) resolve(arg.get());
    return std::any();
}

std::any Resolver::visitGroupingExpr(Grouping* expr) {
    resolve(expr->expression.get());
    return std::any();
}

std::any Resolver::visitLiteralExpr(Literal* expr) {
    return std::any();
}

std::any Resolver::visitLogicalExpr(Logical* expr) {
    resolve(expr->left.get());
    resolve(expr->right.get());
    return std::any();
}

std::any Resolver::visitUnaryExpr(Unary* expr) {
    resolve(expr->right.get());
    return std::any();
}

std::any Resolver::visitGetExpr(Get* expr) {
    resolve(expr->object.get());
    return std::any();
}

std::any Resolver::visitSetExpr(Set* expr) {
    resolve(expr->value.get());
    resolve(expr->object.get());
    return std::any();
}

std::any Resolver::visitThisExpr(This* expr) {
    resolveLocal(expr, expr->keyword);
    return std::any();
}

std::any Resolver::visitSuperExpr(Super* expr) {
    resolveLocal(expr, expr->keyword);
    return std::any();
}
