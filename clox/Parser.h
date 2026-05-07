#pragma once

#include <vector>
#include <memory>
#include <stdexcept>
#include "exp.h"
#include "Token.h"
#include "Stmt.h"

// Custom exception for parse errors
class ParseError : public std::runtime_error {
public:
    ParseError(const std::string& message) : std::runtime_error(message) {}
};

class Parser {
private:
    std::vector<Token> tokens;
    int current = 0;


        std::shared_ptr<Stmt> statement();
    
    // Specific statement parsers
    std::shared_ptr<Stmt> printStatement();        // print expr;
    std::shared_ptr<Stmt> expressionStatement();   // expr;
     std::shared_ptr<Stmt> declaration();
      std::shared_ptr<Stmt> varDeclaration();     
    // Expression parsing methods (precedence climbing)
    std::shared_ptr<Expr> expression();
    std::shared_ptr<Expr> assignment();
    std::shared_ptr<Expr> logicalOr();
    std::shared_ptr<Expr> logicalAnd();
    std::shared_ptr<Expr> equality();
    std::shared_ptr<Expr> comparison();
    std::shared_ptr<Expr> term();
    std::shared_ptr<Expr> factor();
    std::shared_ptr<Expr> unary();
    std::shared_ptr<Expr> call();
    std::shared_ptr<Expr> finishCall(std::shared_ptr<Expr> callee);
    std::shared_ptr<Expr> primary();

    // Utility methods
    bool match(TokenType type);
    bool match(std::initializer_list<TokenType> types);
    Token consume(TokenType type, const std::string& message);
    bool check(TokenType type) const;
    Token advance();
    bool isAtEnd() const;
    Token peek() const;
    Token previous() const;

    // Error handling
    ParseError error(const Token& token, const std::string& message);
    void synchronize();

public:
    Parser(const std::vector<Token>& tokens);
    std::vector<std::shared_ptr<Stmt>> parse();
};
