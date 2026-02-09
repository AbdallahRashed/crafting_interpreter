/**
 * Parser.cpp - Implementation of recursive descent parser
 * 
 * STATEMENT PARSING FLOW:
 * ======================
 * 
 * 1. parse() - Entry point
 *    Loops through all tokens, calling declaration() for each top-level construct
 *    Returns vector of all statements
 * 
 * 2. declaration() - Top-level dispatcher
 *    Recognizes: class, fun, var declarations, or falls through to statement()
 *    Handles errors with synchronize() to continue parsing
 * 
 * 3. statement() - Statement dispatcher  
 *    Recognizes: for, if, print, return, while, blocks, or expression statements
 * 
 * 4. Specific statement parsers
 *    Each builds and returns appropriate Stmt subclass
 * 
 * DESUGARING:
 * ==========
 * For-loops are "desugared" (transformed) into while-loops:
 *   for (init; cond; incr) body
 * becomes:
 *   { init; while (cond) { body; incr; } }
 * 
 * This simplifies the interpreter - it only needs to handle while-loops.
 * 
 * ERROR RECOVERY:
 * ==============
 * When a parse error occurs:
 * 1. Error is reported with line number
 * 2. ParseError exception is thrown
 * 3. Caught in declaration() which calls synchronize()
 * 4. synchronize() skips tokens until statement boundary (semicolon or keyword)
 * 5. Parsing continues with next statement
 * 
 * This allows reporting multiple errors in one parse pass.
 */

#include "Parser.h"
#include <iostream>

Parser::Parser(const std::vector<Token>& tokens) : tokens(tokens) {}

// ==================== MAIN ENTRY POINT ====================

// Main parse method - returns vector of statements
std::vector<std::shared_ptr<Stmt>> Parser::parse() {
    std::vector<std::shared_ptr<Stmt>> statements;
    while (!isAtEnd()) {
        statements.push_back(declaration());
    }
    return statements;
}

// ==================== DECLARATION PARSING ====================

/**
 * declaration - Top-level parsing entry point
 * 
 * Handles all declarations and statements:
 * - class declarations
 * - function declarations  
 * - variable declarations
 * - all statement types
 * 
 * Error recovery: catches ParseError and synchronizes to continue parsing
 */
// declaration -> classDecl | funDecl | varDecl | statement
std::shared_ptr<Stmt> Parser::declaration() {
    try {
        if (match(CLASS)) return classDeclaration();
        if (match(FUN)) return functionDeclaration("function");
        if (match(VAR)) return varDeclaration();
        return statement();
    } catch (const ParseError& error) {
        synchronize();
        return nullptr;
    }
}

// varDecl -> "var" IDENTIFIER ( "=" expression )? ";"
std::shared_ptr<Stmt> Parser::varDeclaration() {
    Token name = consume(IDENTIFIER, "Expect variable name.");
    
    std::shared_ptr<Expr> initializer = nullptr;
    if (match(EQUAL)) {
        initializer = expression();
    }
    
    consume(SEMICOLON, "Expect ';' after variable declaration.");
    return std::make_shared<Var>(name, initializer);
}

/**
 * classDeclaration - Parses class with optional superclass and methods
 * 
 * Syntax: class Name < Superclass { method1() {} method2() {} }
 * Superclass is optional. Methods are parsed as function declarations.
 */
// classDecl -> "class" IDENTIFIER ( "<" IDENTIFIER )? "{" function* "}"
std::shared_ptr<Stmt> Parser::classDeclaration() {
    Token name = consume(IDENTIFIER, "Expect class name.");
    
    std::shared_ptr<Variable> superclass = nullptr;
    if (match(LESS)) {
        consume(IDENTIFIER, "Expect superclass name.");
        superclass = std::make_shared<Variable>(previous());
    }
    
    consume(LEFT_BRACE, "Expect '{' before class body.");
    
    std::vector<std::shared_ptr<Function>> methods;
    while (!check(RIGHT_BRACE) && !isAtEnd()) {
        methods.push_back(functionDeclaration("method"));
    }
    
    consume(RIGHT_BRACE, "Expect '}' after class body.");
    return std::make_shared<Class>(name, superclass, methods);
}

/**
 * functionDeclaration - Parses function/method with parameters and body
 * 
 * Used for both top-level functions and class methods.
 * kind parameter is "function" or "method" (for error messages)
 */
// funDecl -> "fun" function
// function -> IDENTIFIER "(" parameters? ")" block
std::shared_ptr<Function> Parser::functionDeclaration(const std::string& kind) {
    Token name = consume(IDENTIFIER, "Expect " + kind + " name.");
    consume(LEFT_PAREN, "Expect '(' after " + kind + " name.");
    
    std::vector<Token> parameters;
    if (!check(RIGHT_PAREN)) {
        do {
            if (parameters.size() >= 255) {
                error(peek(), "Can't have more than 255 parameters.");
            }
            parameters.push_back(consume(IDENTIFIER, "Expect parameter name."));
        } while (match(COMMA));
    }
    consume(RIGHT_PAREN, "Expect ')' after parameters.");
    
    consume(LEFT_BRACE, "Expect '{' before " + kind + " body.");
    std::vector<std::shared_ptr<Stmt>> body = block();
    
    return std::make_shared<Function>(name, parameters, body);
}

// ==================== STATEMENT PARSING ====================

/**
 * statement - Dispatches to specific statement parser based on keyword
 * 
 * Recognizes control flow and special statements, otherwise falls through
 * to expression statement (most common case).
 */
// statement -> exprStmt | forStmt | ifStmt | printStmt | returnStmt | whileStmt | block
std::shared_ptr<Stmt> Parser::statement() {
    if (match(FOR)) return forStatement();
    if (match(IF)) return ifStatement();
    if (match(PRINT)) return printStatement();
    if (match(RETURN)) return returnStatement();
    if (match(WHILE)) return whileStatement();
    if (match(LEFT_BRACE)) return std::make_shared<Block>(block());
    return expressionStatement();
}

// printStmt -> "print" expression ";"
std::shared_ptr<Stmt> Parser::printStatement() {
    std::shared_ptr<Expr> value = expression();
    consume(SEMICOLON, "Expect ';' after value.");
    return std::make_shared<Print>(value);
}

// returnStmt -> "return" expression? ";"
std::shared_ptr<Stmt> Parser::returnStatement() {
    Token keyword = previous();
    std::shared_ptr<Expr> value = nullptr;
    if (!check(SEMICOLON)) {
        value = expression();
    }
    consume(SEMICOLON, "Expect ';' after return value.");
    return std::make_shared<Return>(keyword, value);
}

// exprStmt -> expression ";"
std::shared_ptr<Stmt> Parser::expressionStatement() {
    std::shared_ptr<Expr> expr = expression();
    consume(SEMICOLON, "Expect ';' after expression.");
    return std::make_shared<Expression>(expr);
}

// ifStmt -> "if" "(" expression ")" statement ( "else" statement )?
std::shared_ptr<Stmt> Parser::ifStatement() {
    consume(LEFT_PAREN, "Expect '(' after 'if'.");
    std::shared_ptr<Expr> condition = expression();
    consume(RIGHT_PAREN, "Expect ')' after if condition.");
    
    std::shared_ptr<Stmt> thenBranch = statement();
    std::shared_ptr<Stmt> elseBranch = nullptr;
    if (match(ELSE)) {
        elseBranch = statement();
    }
    
    return std::make_shared<If>(condition, thenBranch, elseBranch);
}

// whileStmt -> "while" "(" expression ")" statement
std::shared_ptr<Stmt> Parser::whileStatement() {
    consume(LEFT_PAREN, "Expect '(' after 'while'.");
    std::shared_ptr<Expr> condition = expression();
    consume(RIGHT_PAREN, "Expect ')' after condition.");
    std::shared_ptr<Stmt> body = statement();
    
    return std::make_shared<While>(condition, body);
}

/**
 * forStatement - Parses for-loop and desugars it into a while-loop
 * 
 * Desugaring example:
 *   for (var i = 0; i < 10; i = i + 1) print i;
 * becomes:
 *   { var i = 0; while (i < 10) { print i; i = i + 1; } }
 * 
 * This simplifies the interpreter by reusing while-loop logic.
 */
// forStmt -> "for" "(" ( varDecl | exprStmt | ";" ) expression? ";" expression? ")" statement
std::shared_ptr<Stmt> Parser::forStatement() {
    consume(LEFT_PAREN, "Expect '(' after 'for'.");
    
    std::shared_ptr<Stmt> initializer;
    if (match(SEMICOLON)) {
        initializer = nullptr;
    } else if (match(VAR)) {
        initializer = varDeclaration();
    } else {
        initializer = expressionStatement();
    }
    
    std::shared_ptr<Expr> condition = nullptr;
    if (!check(SEMICOLON)) {
        condition = expression();
    }
    consume(SEMICOLON, "Expect ';' after loop condition.");
    
    std::shared_ptr<Expr> increment = nullptr;
    if (!check(RIGHT_PAREN)) {
        increment = expression();
    }
    consume(RIGHT_PAREN, "Expect ')' after for clauses.");
    
    std::shared_ptr<Stmt> body = statement();
    
    // Desugar for loop into while loop
    if (increment != nullptr) {
        std::vector<std::shared_ptr<Stmt>> bodyStatements;
        bodyStatements.push_back(body);
        bodyStatements.push_back(std::make_shared<Expression>(increment));
        body = std::make_shared<Block>(bodyStatements);
    }
    
    if (condition == nullptr) {
        condition = std::make_shared<Literal>(true);
    }
    body = std::make_shared<While>(condition, body);
    
    if (initializer != nullptr) {
        std::vector<std::shared_ptr<Stmt>> statements;
        statements.push_back(initializer);
        statements.push_back(body);
        body = std::make_shared<Block>(statements);
    }
    
    return body;
}

// block -> "{" declaration* "}"
std::vector<std::shared_ptr<Stmt>> Parser::block() {
    std::vector<std::shared_ptr<Stmt>> statements;
    
    while (!check(RIGHT_BRACE) && !isAtEnd()) {
        statements.push_back(declaration());
    }
    
    consume(RIGHT_BRACE, "Expect '}' after block.");
    return statements;
}

// expression -> assignment
std::shared_ptr<Expr> Parser::expression() {
    return assignment();
}

// assignment -> IDENTIFIER "=" assignment | logicalOr
std::shared_ptr<Expr> Parser::assignment() {
    std::shared_ptr<Expr> expr = logicalOr();

    if (match(EQUAL)) {
        Token equals = previous();
        std::shared_ptr<Expr> value = assignment();

        if (auto varExpr = std::dynamic_pointer_cast<Variable>(expr)) {
            std::cout << "Parsed assignment to variable: " << varExpr->name.lexeme << std::endl;
            return std::make_shared<Assign>(varExpr->name, value);
        } else if (auto getExpr = std::dynamic_pointer_cast<Get>(expr)) {
            return std::make_shared<Set>(getExpr->object, getExpr->name, value);
        }

        error(equals, "Invalid assignment target.");
        // Don't throw, just continue parsing
    }

    return expr;
}

// logicalOr -> logicalAnd ( "or" logicalAnd )*
std::shared_ptr<Expr> Parser::logicalOr() {
    std::shared_ptr<Expr> expr = logicalAnd();

    while (match(OR)) {
        Token op = previous();
        std::shared_ptr<Expr> right = logicalAnd();
        expr = std::make_shared<Logical>(expr, op, right);
    }

    return expr;
}

// logicalAnd -> equality ( "and" equality )*
std::shared_ptr<Expr> Parser::logicalAnd() {
    std::shared_ptr<Expr> expr = equality();

    while (match(AND)) {
        Token op = previous();
        std::shared_ptr<Expr> right = equality();
        expr = std::make_shared<Logical>(expr, op, right);
    }

    return expr;
}

// equality -> comparison ( ( "!=" | "==" ) comparison )*
std::shared_ptr<Expr> Parser::equality() {
    std::shared_ptr<Expr> expr = comparison();

    while (match({BANG_EQUAL, EQUAL_EQUAL})) {
        Token op = previous();
        std::shared_ptr<Expr> right = comparison();
        expr = std::make_shared<Binary>(expr, op, right);
    }

    return expr;
}

// comparison -> term ( ( ">" | ">=" | "<" | "<=" ) term )*
std::shared_ptr<Expr> Parser::comparison() {
    std::shared_ptr<Expr> expr = term();

    while (match({GREATER, GREATER_EQUAL, LESS, LESS_EQUAL})) {
        Token op = previous();
        std::shared_ptr<Expr> right = term();
        expr = std::make_shared<Binary>(expr, op, right);
    }

    return expr;
}

// term -> factor ( ( "-" | "+" ) factor )*
std::shared_ptr<Expr> Parser::term() {
    std::shared_ptr<Expr> expr = factor();

    while (match({MINUS, PLUS})) {
        Token op = previous();
        std::shared_ptr<Expr> right = factor();
        expr = std::make_shared<Binary>(expr, op, right);
        std::cout << "Parsed binary expression: " << tokenTypeToString(op.type) << std::endl;       
    }

    return expr;
}

// factor -> unary ( ( "/" | "*" ) unary )*
std::shared_ptr<Expr> Parser::factor() {
    std::shared_ptr<Expr> expr = unary();

    while (match({SLASH, STAR})) {
        Token op = previous();
        std::shared_ptr<Expr> right = unary();
        expr = std::make_shared<Binary>(expr, op, right);
    }

    return expr;
}

// unary -> ( "!" | "-" ) unary | call
std::shared_ptr<Expr> Parser::unary() {
    if (match({BANG, MINUS})) {
        Token op = previous();
        std::shared_ptr<Expr> right = unary();
        return std::make_shared<Unary>(op, right);
    }

    return call();
}

// call -> primary ( "(" arguments? ")" | "." IDENTIFIER )*
std::shared_ptr<Expr> Parser::call() {
    std::shared_ptr<Expr> expr = primary();

    while (true) {
        if (match(LEFT_PAREN)) {
            expr = finishCall(expr);
        } else if (match(DOT)) {
            Token name = consume(IDENTIFIER, "Expect property name after '.'.");
            expr = std::make_shared<Get>(expr, name);
        } else {
            break;
        }
    }

    return expr;
}

// finishCall -> parse function call arguments
std::shared_ptr<Expr> Parser::finishCall(std::shared_ptr<Expr> callee) {
    std::vector<std::shared_ptr<Expr>> arguments;

    if (!check(RIGHT_PAREN)) {
        do {
            if (arguments.size() >= 255) {
                error(peek(), "Can't have more than 255 arguments.");
            }
            arguments.push_back(expression());
        } while (match(COMMA));
    }

    Token paren = consume(RIGHT_PAREN, "Expect ')' after arguments.");
    return std::make_shared<Call>(callee, paren, arguments);
}

// primary -> NUMBER | STRING | "true" | "false" | "nil" 
//          | "(" expression ")" | IDENTIFIER | "this" | "super" "." IDENTIFIER
std::shared_ptr<Expr> Parser::primary() {
    if (match(FALSE)) return std::make_shared<Literal>(false);
    if (match(TRUE)) return std::make_shared<Literal>(true);
    if (match(NIL)) return std::make_shared<Literal>(nullptr);

    if (match(NUMBER)) {
        return std::make_shared<Literal>(previous().literal);
    }

    if (match(STRING)) {
        return std::make_shared<Literal>(previous().literal);
    }

    if (match(THIS)) {
        return std::make_shared<This>(previous());
    }

    if (match(SUPER)) {
        Token keyword = previous();
        consume(DOT, "Expect '.' after 'super'.");
        Token method = consume(IDENTIFIER, "Expect superclass method name.");
        return std::make_shared<Super>(keyword, method);
    }

    if (match(IDENTIFIER)) {
        return std::make_shared<Variable>(previous());
    }

    if (match(LEFT_PAREN)) {
        std::shared_ptr<Expr> expr = expression();
        consume(RIGHT_PAREN, "Expect ')' after expression.");
        return std::make_shared<Grouping>(expr);
    }

    throw error(peek(), "Expect expression.");
}

// Utility methods

bool Parser::match(TokenType type) {
    if (check(type)) {
        advance();
        return true;
    }
    return false;
}

bool Parser::match(std::initializer_list<TokenType> types) {
    for (TokenType type : types) {
        if (check(type)) {
            advance();
            return true;
        }
    }
    return false;
}

Token Parser::consume(TokenType type, const std::string& message) {
    if (check(type)) return advance();
    throw error(peek(), message);
}

bool Parser::check(TokenType type) const {
    if (isAtEnd()) return false;
    return peek().type == type;
}

Token Parser::advance() {
    if (!isAtEnd()) current++;
    return previous();
}

bool Parser::isAtEnd() const {
    return peek().type == TOKEN_EOF;
}

Token Parser::peek() const {
    return tokens[current];
}

Token Parser::previous() const {
    return tokens[current - 1];
}

// Error handling

ParseError Parser::error(const Token& token, const std::string& message) {
    if (token.type == TOKEN_EOF) {
        std::cerr << "[Line " << token.line << "] Error at end: " 
                  << message << std::endl;
    } else {
        std::cerr << "[Line " << token.line << "] Error at '" 
                  << token.lexeme << "': " << message << std::endl;
    }
    return ParseError(message);
}

void Parser::synchronize() {
    advance();

    while (!isAtEnd()) {
        if (previous().type == SEMICOLON) return;

        switch (peek().type) {
            case CLASS:
            case FUN:
            case VAR:
            case FOR:
            case IF:
            case WHILE:
            case PRINT:
            case RETURN:
                return;
            default:
                break;
        }

        advance();
    }
}
