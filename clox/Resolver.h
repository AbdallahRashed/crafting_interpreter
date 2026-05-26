
#pragma once
#include "exp.h"
#include "Stmt.h"
#include "Interpreter.h"
#include <unordered_map>
#include <vector>
#include <string>
#include <any>
#include <iostream>

class Resolver : public StmtVisitor, public ExprVisitor {
public:
    Resolver(Interpreter& interpreter) : interpreter(interpreter) {}

    std::any visitBlockStmt(Block* stmt) override;
    std::any visitClassStmt(Class* stmt) override;
    std::any visitExpressionStmt(Expression* stmt) override;
    std::any visitFunctionStmt(Function* stmt) override;
    std::any visitIfStmt(If* stmt) override;
    std::any visitPrintStmt(Print* stmt) override;
    std::any visitReturnStmt(Return* stmt) override;
    std::any visitVarStmt(Var* stmt) override;
    std::any visitWhileStmt(While* stmt) override;
    std::any visitAssignExpr(Assign* expr) override;
    std::any visitBinaryExpr(Binary* expr) override;
    std::any visitCallExpr(Call* expr) override;
    std::any visitGetExpr(Get* expr) override;
    std::any visitGroupingExpr(Grouping* expr) override;
    std::any visitLiteralExpr(Literal* expr) override;
    std::any visitLogicalExpr(Logical* expr) override;
    std::any visitSetExpr(Set* expr) override;
    std::any visitSuperExpr(Super* expr) override;
    std::any visitThisExpr(This* expr) override;
    std::any visitUnaryExpr(Unary* expr) override;
    std::any visitVariableExpr(Variable* expr) override;

    void resolve(const std::vector<std::shared_ptr<Stmt>>& statements);

private:
    Interpreter& interpreter;
    // vector instead of stack — indexed access needed in resolveLocal
    std::vector<std::unordered_map<std::string, bool>> scopes;

    void resolve(Stmt* stmt);
    void resolve(Expr* expr);
    void beginScope();
    void endScope();
    void declare(const Token& name);
    void define(const Token& name);
    void resolveLocal(Expr* expr, const Token& name);
    void resolveFunction(Function* stmt);
};