#pragma once
#include <memory>
#include <string>
#include <vector>
#include "scanner.h"

// ===================== AST Nodes =====================

struct Expr;
struct Stmt;

using ExprPtr = std::unique_ptr<Expr>;
using StmtPtr = std::unique_ptr<Stmt>;

// --- Expressions ---
struct Expr {
    virtual ~Expr() = default;
    int line = 0;
};

struct NumberExpr : Expr {
    double value;
    NumberExpr(double v, int line) : value(v) { this->line = line; }
};

struct StringExpr : Expr {
    std::string value;
    StringExpr(std::string v, int line) : value(std::move(v)) { this->line = line; }
};

struct BoolExpr : Expr {
    bool value;
    BoolExpr(bool v, int line) : value(v) { this->line = line; }
};

struct NilExpr : Expr {
    NilExpr(int line) { this->line = line; }
};

struct IdentifierExpr : Expr {
    std::string name;
    IdentifierExpr(std::string n, int line) : name(std::move(n)) { this->line = line; }
};

struct AssignExpr : Expr {
    std::string name;
    ExprPtr value;
    AssignExpr(std::string n, ExprPtr v, int line)
        : name(std::move(n)), value(std::move(v)) { this->line = line; }
};

struct BinaryExpr : Expr {
    ExprPtr left, right;
    TokenType op;
    BinaryExpr(ExprPtr l, TokenType o, ExprPtr r, int line)
        : left(std::move(l)), right(std::move(r)), op(o) { this->line = line; }
};

struct UnaryExpr : Expr {
    TokenType op;
    ExprPtr operand;
    UnaryExpr(TokenType o, ExprPtr e, int line)
        : op(o), operand(std::move(e)) { this->line = line; }
};

struct CallExpr : Expr {
    ExprPtr callee;
    std::vector<ExprPtr> args;
    CallExpr(ExprPtr c, std::vector<ExprPtr> a, int line)
        : callee(std::move(c)), args(std::move(a)) { this->line = line; }
};

struct GetExpr : Expr {
    ExprPtr object;
    std::string name;
    GetExpr(ExprPtr o, std::string n, int line)
        : object(std::move(o)), name(std::move(n)) { this->line = line; }
};

struct SetExpr : Expr {
    ExprPtr object;
    std::string name;
    ExprPtr value;
    SetExpr(ExprPtr o, std::string n, ExprPtr v, int line)
        : object(std::move(o)), name(std::move(n)), value(std::move(v)) { this->line = line; }
};

struct ThisExpr : Expr {
    ThisExpr(int line) { this->line = line; }
};

struct SuperExpr : Expr {
    std::string method;
    SuperExpr(std::string m, int line) : method(std::move(m)) { this->line = line; }
};

struct LogicalExpr : Expr {
    ExprPtr left, right;
    TokenType op;
    LogicalExpr(ExprPtr l, TokenType o, ExprPtr r, int line)
        : left(std::move(l)), right(std::move(r)), op(o) { this->line = line; }
};

// --- Statements ---
struct Stmt {
    virtual ~Stmt() = default;
    int line = 0;
};

struct ExprStmt : Stmt {
    ExprPtr expression;
    ExprStmt(ExprPtr e, int line) : expression(std::move(e)) { this->line = line; }
};

struct PrintStmt : Stmt {
    ExprPtr expression;
    PrintStmt(ExprPtr e, int line) : expression(std::move(e)) { this->line = line; }
};

struct VarStmt : Stmt {
    std::string name;
    ExprPtr initializer;
    VarStmt(std::string n, ExprPtr init, int line)
        : name(std::move(n)), initializer(std::move(init)) { this->line = line; }
};

struct BlockStmt : Stmt {
    std::vector<StmtPtr> statements;
    BlockStmt(std::vector<StmtPtr> stmts, int line)
        : statements(std::move(stmts)) { this->line = line; }
};

struct IfStmt : Stmt {
    ExprPtr condition;
    StmtPtr thenBranch;
    StmtPtr elseBranch;
    IfStmt(ExprPtr c, StmtPtr t, StmtPtr e, int line)
        : condition(std::move(c)), thenBranch(std::move(t)), elseBranch(std::move(e)) { this->line = line; }
};

struct WhileStmt : Stmt {
    ExprPtr condition;
    StmtPtr body;
    WhileStmt(ExprPtr c, StmtPtr b, int line)
        : condition(std::move(c)), body(std::move(b)) { this->line = line; }
};

struct FunStmt : Stmt {
    std::string name;
    std::vector<std::string> params;
    std::vector<StmtPtr> body;
    FunStmt(std::string n, std::vector<std::string> p, std::vector<StmtPtr> b, int line)
        : name(std::move(n)), params(std::move(p)), body(std::move(b)) { this->line = line; }
};

struct ReturnStmt : Stmt {
    ExprPtr value;
    ReturnStmt(ExprPtr v, int line) : value(std::move(v)) { this->line = line; }
};

struct ClassStmt : Stmt {
    std::string name;
    std::string superclass;
    std::vector<std::unique_ptr<FunStmt>> methods;
    ClassStmt(std::string n, std::string sc, std::vector<std::unique_ptr<FunStmt>> m, int line)
        : name(std::move(n)), superclass(std::move(sc)), methods(std::move(m)) { this->line = line; }
};

// ===================== Parser =====================

class Parser {
public:
    explicit Parser(const std::vector<Token>& tokens);
    std::vector<StmtPtr> parse();

private:
    std::vector<Token> tokens;
    int current = 0;

    // Utilities
    const Token& peek() const;
    const Token& previous() const;
    bool isAtEnd() const;
    const Token& advance();
    bool check(TokenType type) const;
    bool match(TokenType type);
    Token consume(TokenType type, const std::string& msg);
    void error(const std::string& msg);

    // Grammar
    StmtPtr declaration();
    StmtPtr classDeclaration();
    StmtPtr funDeclaration();
    StmtPtr varDeclaration();
    StmtPtr statement();
    StmtPtr printStatement();
    StmtPtr ifStatement();
    StmtPtr whileStatement();
    StmtPtr forStatement();
    StmtPtr returnStatement();
    StmtPtr block();

    ExprPtr expression();
    ExprPtr assignment();
    ExprPtr logicOr();
    ExprPtr logicAnd();
    ExprPtr equality();
    ExprPtr comparison();
    ExprPtr term();
    ExprPtr factor();
    ExprPtr unary();
    ExprPtr call();
    ExprPtr primary();
    ExprPtr finishCall(ExprPtr callee);
};
