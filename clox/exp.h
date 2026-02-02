#pragma once

#include <iostream>
#include <memory>
#include <vector>
#include <any>
#include <string>
#include "Token.h"

// Forward declarations
class Assign; class Binary; class Call; class Get;
class Grouping; class Literal; class Logical; class Set;
class Super; class This; class Unary; class Variable;

class ExprVisitor {
public:
    virtual ~ExprVisitor() = default;
    virtual std::any visitAssignExpr(Assign* expr) = 0;
    virtual std::any visitBinaryExpr(Binary* expr) = 0;
    virtual std::any visitCallExpr(Call* expr) = 0;
    virtual std::any visitGetExpr(Get* expr) = 0;
    virtual std::any visitGroupingExpr(Grouping* expr) = 0;
    virtual std::any visitLiteralExpr(Literal* expr) = 0;
    virtual std::any visitLogicalExpr(Logical* expr) = 0;
    virtual std::any visitSetExpr(Set* expr) = 0;
    virtual std::any visitSuperExpr(Super* expr) = 0;
    virtual std::any visitThisExpr(This* expr) = 0;
    virtual std::any visitUnaryExpr(Unary* expr) = 0;
    virtual std::any visitVariableExpr(Variable* expr) = 0;
};

class Expr {
public:
    virtual ~Expr() = default;
    virtual std::any accept(ExprVisitor& visitor) = 0;
};

// --- Subclasses ---

class Assign : public Expr {
public:
    const Token name;
    const std::shared_ptr<Expr> value;
    Assign(Token name, std::shared_ptr<Expr> value) : name(name), value(value) {}
    std::any accept(ExprVisitor& visitor) override { 
        return visitor.visitAssignExpr(this); 
    }
};

class Binary : public Expr {
public:
    const std::shared_ptr<Expr> left;
    const Token op;
    const std::shared_ptr<Expr> right;
    Binary(std::shared_ptr<Expr> left, Token op, std::shared_ptr<Expr> right) 
        : left(left), op(op), right(right) {}
    std::any accept(ExprVisitor& visitor) override { 
        return visitor.visitBinaryExpr(this); 
    }
};

class Call : public Expr {
public:
    const std::shared_ptr<Expr> callee;
    const Token paren;
    const std::vector<std::shared_ptr<Expr>> arguments;
    Call(std::shared_ptr<Expr> callee, Token paren, std::vector<std::shared_ptr<Expr>> arguments) 
        : callee(callee), paren(paren), arguments(arguments) {}
    std::any accept(ExprVisitor& visitor) override { 
        return visitor.visitCallExpr(this); 
    }
};

class Get : public Expr {
public:
    const std::shared_ptr<Expr> object;
    const Token name;
    Get(std::shared_ptr<Expr> object, Token name) : object(object), name(name) {}
    std::any accept(ExprVisitor& visitor) override { 
        return visitor.visitGetExpr(this); 
    }
};

class Grouping : public Expr {
public:
    const std::shared_ptr<Expr> expression;
    Grouping(std::shared_ptr<Expr> expression) : expression(expression) {}
    std::any accept(ExprVisitor& visitor) override { 
        return visitor.visitGroupingExpr(this); 
    }
};

class Literal : public Expr {
public:
    const std::any value;
    Literal(std::any value) : value(value) {}
    std::any accept(ExprVisitor& visitor) override { 
        return visitor.visitLiteralExpr(this); 
    }
};

class Logical : public Expr {
public:
    const std::shared_ptr<Expr> left;
    const Token op;
    const std::shared_ptr<Expr> right;
    Logical(std::shared_ptr<Expr> left, Token op, std::shared_ptr<Expr> right) 
        : left(left), op(op), right(right) {}
    std::any accept(ExprVisitor& visitor) override { 
        return visitor.visitLogicalExpr(this); 
    }
};

class Set : public Expr {
public:
    const std::shared_ptr<Expr> object;
    const Token name;
    const std::shared_ptr<Expr> value;
    Set(std::shared_ptr<Expr> object, Token name, std::shared_ptr<Expr> value) 
        : object(object), name(name), value(value) {}
    std::any accept(ExprVisitor& visitor) override { 
        return visitor.visitSetExpr(this); 
    }
};

class Super : public Expr {
public:
    const Token keyword;
    const Token method;
    Super(Token keyword, Token method) : keyword(keyword), method(method) {}
    std::any accept(ExprVisitor& visitor) override { 
        return visitor.visitSuperExpr(this); 
    }
};

class This : public Expr {
public:
    const Token keyword;
    This(Token keyword) : keyword(keyword) {}
    std::any accept(ExprVisitor& visitor) override { 
        return visitor.visitThisExpr(this); 
    }
};

class Unary : public Expr {
public:
    const Token op;
    const std::shared_ptr<Expr> right;
    Unary(Token op, std::shared_ptr<Expr> right) : op(op), right(right) {}
    std::any accept(ExprVisitor& visitor) override { 
        return visitor.visitUnaryExpr(this); 
    }
};

class Variable : public Expr {
public:
    const Token name;
    Variable(Token name) : name(name) {}
    std::any accept(ExprVisitor& visitor) override { 
        return visitor.visitVariableExpr(this); 
    }
};