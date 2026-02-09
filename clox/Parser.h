/**
 * Parser.h - Recursive Descent Parser for Lox Language
 * 
 * DESIGN OVERVIEW:
 * ===============
 * Implements a recursive descent parser that converts tokens into an Abstract Syntax Tree (AST).
 * Now supports both expressions AND statements for complete program parsing.
 * 
 * PARSING ARCHITECTURE:
 * ====================
 * The parser uses two main hierarchies:
 * 
 * 1. STATEMENT PARSING (Top-level)
 *    parse() → declaration() → statement() → specific statement methods
 *    
 *    Grammar hierarchy:
 *    - declaration:  classDecl | funDecl | varDecl | statement
 *    - statement:    exprStmt | forStmt | ifStmt | printStmt | returnStmt | whileStmt | block
 *    
 * 2. EXPRESSION PARSING (Precedence climbing)
 *    expression() → assignment() → logicalOr() → ... → primary()
 *    
 *    Precedence (lowest to highest):
 *    - assignment:   =
 *    - logicalOr:    or
 *    - logicalAnd:   and
 *    - equality:     == !=
 *    - comparison:   > >= < <=
 *    - term:         + -
 *    - factor:       * /
 *    - unary:        ! -
 *    - call:         () .
 *    - primary:      literals, identifiers, grouping
 * 
 * HOW IT WORKS:
 * ============
 * 1. Scanner produces tokens from source code
 * 2. Parser consumes tokens to build AST:
 *    - Maintains current token position
 *    - Uses recursive methods matching grammar rules
 *    - Each method consumes tokens and returns AST nodes
 * 3. Returns vector of statement AST nodes representing the program
 * 
 * ERROR HANDLING:
 * ==============
 * - Throws ParseError on syntax errors
 * - Reports errors with line numbers
 * - Uses synchronize() to recover and continue parsing
 * - Allows multiple errors to be reported in one pass
 * 
 * KEY CHANGES FROM EXPRESSION-ONLY PARSER:
 * ========================================
 * - parse() now returns vector<shared_ptr<Stmt>> instead of shared_ptr<Expr>
 * - Added declaration() as top-level entry point
 * - Added statement parsing methods for all statement types
 * - Expression parsing remains unchanged (used within statements)
 * 
 * USAGE:
 * =====
 *   Scanner scanner(source);
 *   vector<Token> tokens = scanner.scanTokens();
 *   Parser parser(tokens);
 *   vector<shared_ptr<Stmt>> statements = parser.parse();
 *   // Pass statements to interpreter or other backend
 */

#pragma once

#include <vector>
#include <memory>
#include <stdexcept>
#include "exp.h"
#include "Stmt.h"
#include "Token.h"

// Custom exception for parse errors
class ParseError : public std::runtime_error {
public:
    ParseError(const std::string& message) : std::runtime_error(message) {}
};

class Parser {
private:
    std::vector<Token> tokens;
    int current = 0;

    // ==================== STATEMENT PARSING ====================
    // Top-level entry point and declaration handlers
    
    /**
     * declaration - Parses top-level declarations
     * Grammar: classDecl | funDecl | varDecl | statement
     * Entry point for each statement in the program
     */
    std::shared_ptr<Stmt> declaration();
    
    /**
     * statement - Dispatches to specific statement parsers
     * Grammar: exprStmt | forStmt | ifStmt | printStmt | returnStmt | whileStmt | block
     */
    std::shared_ptr<Stmt> statement();
    
    // Specific statement parsers
    std::shared_ptr<Stmt> printStatement();        // print expr;
    std::shared_ptr<Stmt> returnStatement();       // return expr?;
    std::shared_ptr<Stmt> expressionStatement();   // expr;
    std::shared_ptr<Stmt> ifStatement();           // if (expr) stmt (else stmt)?
    std::shared_ptr<Stmt> whileStatement();        // while (expr) stmt
    std::shared_ptr<Stmt> forStatement();          // for (init; cond; incr) stmt (desugars to while)
    
    // Declaration parsers
    std::shared_ptr<Stmt> varDeclaration();                           // var name = expr?;
    std::shared_ptr<Function> functionDeclaration(const std::string& kind);  // fun name(params) block
    std::shared_ptr<Stmt> classDeclaration();                         // class name (< super)? { methods }
    
    std::vector<std::shared_ptr<Stmt>> block();    // { declarations* }

    // ==================== EXPRESSION PARSING ====================
    // Precedence climbing - each method handles one precedence level
    // ==================== EXPRESSION PARSING ====================
    // Precedence climbing - each method handles one precedence level
    
    std::shared_ptr<Expr> expression();     // Entry point: assignment
    std::shared_ptr<Expr> assignment();     // = (right-associative)
    std::shared_ptr<Expr> logicalOr();      // or
    std::shared_ptr<Expr> logicalAnd();     // and
    std::shared_ptr<Expr> equality();       // == !=
    std::shared_ptr<Expr> comparison();     // > >= < <=
    std::shared_ptr<Expr> term();           // + -
    std::shared_ptr<Expr> factor();         // * /
    std::shared_ptr<Expr> unary();          // ! - (prefix)
    std::shared_ptr<Expr> call();           // () . (postfix)
    std::shared_ptr<Expr> finishCall(std::shared_ptr<Expr> callee);
    std::shared_ptr<Expr> primary();        // literals, identifiers, (expr)

    // ==================== UTILITY METHODS ====================
    // Token consumption and checking
    // ==================== UTILITY METHODS ====================
    // Token consumption and checking
    
    bool match(TokenType type);                           // Consume if matches
    bool match(std::initializer_list<TokenType> types);   // Consume if any match
    Token consume(TokenType type, const std::string& message);  // Consume or error
    bool check(TokenType type) const;                     // Peek without consuming
    Token advance();                                      // Move to next token
    bool isAtEnd() const;                                 // At EOF?
    Token peek() const;                                   // Current token
    Token previous() const;                               // Last consumed token

    // ==================== ERROR HANDLING ====================
    // ==================== ERROR HANDLING ====================
    
    ParseError error(const Token& token, const std::string& message);  // Report error
    void synchronize();  // Panic mode recovery - skip to next statement boundary

public:
    Parser(const std::vector<Token>& tokens);
    
    /**
     * parse - Main entry point
     * 
     * Parses all declarations/statements in the token stream.
     * Returns vector of statement AST nodes representing the complete program.
     * Returns empty vector if parsing fails (errors already reported).
     */
    std::vector<std::shared_ptr<Stmt>> parse();
};
