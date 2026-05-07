#pragma once

#include "exp.h"
#include "Stmt.h"
#include <any>
#include <string>
#include <stdexcept>
#include <vector>
#include "Env.h"
#include "Token.h"
// Custom exception for runtime errors with token info
class RuntimeError : public std::runtime_error {
public:
    Token token;
    
    RuntimeError(const Token& token, const std::string& message)
        : std::runtime_error(message), token(token) {}
};

class Interpreter : public ExprVisitor, public StmtVisitor {
public:
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
    std::any visitExpressionStmt(Expression* stmt) override;
    std::any visitPrintStmt(Print* stmt) override;
    std::any visitVarStmt(Var* stmt) override;
    // Public interface
    void interpret(const std::vector<std::shared_ptr<Stmt>>& statements);
    std::any interpret(Expr* expr);  // legacy: expression-only
    std::string stringify(const std::any& value);

private:
    std::any evaluate(Expr* expr);
    bool isTruthy(const std::any& val);
    bool isEqual(const std::any& a, const std::any& b);
    void checkNumberOperand(const Token& op, const std::any& operand);
    void checkNumberOperands(const Token& op, const std::any& left, const std::any& right);
    void execute(Stmt* stmt);
    void executeBlock(const std::vector<std::shared_ptr<Stmt>>& statements, std::shared_ptr<Environment> env);

    std::shared_ptr<Environment> environment = std::make_shared<Environment>();

};
