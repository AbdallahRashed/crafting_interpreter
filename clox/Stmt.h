
#pragma once

#include <iostream>
#include <memory>
#include <vector>
#include <any>
#include <string>
#include "Token.h"
#include "exp.h"


// Forward declarations so StmtVisitor can reference these classes
class Expression;
class Print;
class Var;

class StmtVisitor{
public:
virtual std::any visitExpressionStmt(Expression* stmt)=0;
virtual std::any visitPrintStmt(Print* stmt)=0;
virtual std::any visitVarStmt(Var* stmt)=0;
virtual ~StmtVisitor() = default;
};


class Stmt {
public:
    virtual ~Stmt() = default;
    virtual std::any accept(StmtVisitor& visitor) = 0;
};

class Expression : public Stmt {
public:
const std::shared_ptr<Expr> expression;
Expression(std::shared_ptr<Expr> expression):expression(expression){}

std::any accept(StmtVisitor&visitor) override {
   return visitor.visitExpressionStmt(this); 
}

};
class Print : public Stmt {
public:
    const std::shared_ptr<Expr> expression;
    Print(std::shared_ptr<Expr> expression) : expression(expression) {}
    std::any accept(StmtVisitor& visitor) override {
        return visitor.visitPrintStmt(this);
    }
};

class Var : public Stmt {
public:
    const Token name;
    std::shared_ptr<Expr>initializer;
   Var(Token name, std::shared_ptr<Expr> initializer) 
        : name(name), initializer(initializer) {}
   std::any accept(StmtVisitor& visitor) override { 
        return visitor.visitVarStmt(this); 
    }
};  