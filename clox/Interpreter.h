#pragma once

#include "exp.h"
#include "Stmt.h"
#include "Environment.h"
#include <any>
#include <string>
#include <stdexcept>
#include <memory>

// Custom exception for runtime errors with token info
class RuntimeError : public std::runtime_error {
public:
    Token token;
    
    RuntimeError(const Token& token, const std::string& message)
        : std::runtime_error(message), token(token) {}
};

class Interpreter : public ExprVisitor, public StmtVisitor {
public:
    // Expression visitors
    std::any visitLiteralExpr(Literal* expr) override;
    std::any visitGroupingExpr(Grouping* expr) override;
    std::any visitUnaryExpr(Unary* expr) override;
    std::any visitBinaryExpr(Binary* expr) override;
    std::any visitAssignExpr(Assign* expr) override;
    std::any visitCallExpr(Call* expr) override;
    std::any visitGetExpr(Get* expr) override;
    std::any visitLogicalExpr(Logical* expr) override;
    std::any visitSetExpr(Set* expr) override;
    std::any visitSuperExpr(Super* expr) override;
    std::any visitThisExpr(This* expr) override;
    std::any visitVariableExpr(Variable* expr) override;

    // Statement visitors
    std::any visitExpressionStmt(Expression* stmt) override;
    std::any visitPrintStmt(Print* stmt) override;
    std::any visitVarStmt(Var* stmt) override;
    std::any visitBlockStmt(Block* stmt) override;
    std::any visitIfStmt(If* stmt) override;
    std::any visitWhileStmt(While* stmt) override;
    std::any visitFunctionStmt(Function* stmt) override;        
    std::any visitReturnStmt(Return* stmt) override;
    std::any visitClassStmt(Class* stmt) override;
            
    // Public interface
    void interpret(const std::vector<std::shared_ptr<Stmt>>& statements);  // NEW: interpret statements
    std::any interpret(Expr* expr);  // LEGACY: interpret single expression (for tests)
    std::string stringify(const std::any& value);  // Convert value to string for output

private:
    // Environment for variable storage (starts as global scope)
    std::shared_ptr<Environment> environment = std::make_shared<Environment>();
    
    // Expression evaluation
    std::any evaluate(Expr* expr);
    
    // Statement execution
    void execute(Stmt* stmt);
    void executeBlock(const std::vector<std::shared_ptr<Stmt>>& statements,
                      std::shared_ptr<Environment> env);
    
    // Helper methods
    bool isTruthy(const std::any& val);
    bool isEqual(const std::any& a, const std::any& b);
    void checkNumberOperand(const Token& op, const std::any& operand);
    void checkNumberOperands(const Token& op, const std::any& left, const std::any& right);
};
