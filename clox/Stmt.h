/**
 * Stmt.h - Statement AST Node Definitions
 * 
 * DESIGN OVERVIEW:
 * ===============
 * This file defines the statement node hierarchy for the Lox language AST.
 * Statements represent actions and declarations (unlike expressions which produce values).
 * 
 * VISITOR PATTERN:
 * ===============
 * Uses the Visitor pattern for AST traversal and interpretation:
 * - StmtVisitor interface defines visit methods for each statement type
 * - Each statement class implements accept() to dispatch to appropriate visitor method
 * - Returns std::any to allow flexible return types (e.g., void for execution, types for analysis)
 * 
 * STATEMENT TYPES:
 * ===============
 * 1. Block        - Scoped group of statements { ... }
 * 2. Class        - Class declaration with methods and optional superclass
 * 3. Expression   - Statement that wraps an expression (expr;)
 * 4. Function     - Function declaration with parameters and body
 * 5. If           - Conditional execution (if-then-else)
 * 6. Print        - Output statement (Lox-specific)
 * 7. Return       - Function return with optional value
 * 8. Var          - Variable declaration with optional initializer
 * 9. While        - Loop statement (for-loops desugar to while)
 * 
 * MEMORY MANAGEMENT:
 * =================
 * - Uses std::shared_ptr for ownership and automatic cleanup
 * - Statements can contain sub-statements (e.g., if body, while body)
 * - Expressions are stored as shared_ptr<Expr> from exp.h
 * - Token values are copied (lightweight structs)
 * 
 * RELATIONSHIP WITH EXPRESSIONS:
 * =============================
 * - Statements may contain expressions (e.g., print expr, if condition)
 * - Expression statements wrap a single expression as a statement
 * - See exp.h for expression node definitions
 * 
 * USAGE EXAMPLE:
 * =============
 * Parser creates statements:
 *   auto stmt = std::make_shared<Print>(expressionNode);
 * 
 * Interpreter visits statements:
 *   class Interpreter : public StmtVisitor {
 *     std::any visitPrintStmt(Print* stmt) override {
 *       // Execute print logic
 *     }
 *   };
 */

#pragma once

#include <iostream>
#include <memory>
#include <vector>
#include <any>
#include <string>
#include "Token.h"
#include "exp.h"

// Forward declarations
class Block; class Class; class Expression; class Function;
class If; class Print; class Return; class Var; class While;

/**
 * StmtVisitor - Visitor interface for statement processing
 * 
 * Implement this interface to traverse and process statement ASTs.
 * Each visit method receives a pointer to the specific statement type.
 * 
 * Common implementations:
 * - Interpreter: Execute statements (returns std::any wrapping void or control flow)
 * - Resolver: Perform semantic analysis (variable binding)
 * - Pretty-printer: Convert AST back to source code
 * - Code generator: Emit bytecode or machine code
 */
class StmtVisitor {
public:
    virtual ~StmtVisitor() = default;
    virtual std::any visitBlockStmt(Block* stmt) = 0;
    virtual std::any visitClassStmt(Class* stmt) = 0;
    virtual std::any visitExpressionStmt(Expression* stmt) = 0;
    virtual std::any visitFunctionStmt(Function* stmt) = 0;
    virtual std::any visitIfStmt(If* stmt) = 0;
    virtual std::any visitPrintStmt(Print* stmt) = 0;
    virtual std::any visitReturnStmt(Return* stmt) = 0;
    virtual std::any visitVarStmt(Var* stmt) = 0;
    virtual std::any visitWhileStmt(While* stmt) = 0;
};

/**
 * Stmt - Abstract base class for all statement nodes
 * 
 * All statement types inherit from this class and implement accept().
 * Pure virtual class - cannot be instantiated directly.
 */
class Stmt {
public:
    virtual ~Stmt() = default;
    virtual std::any accept(StmtVisitor& visitor) = 0;
};

// --- Subclasses ---

/**
 * Block - Represents a scope with multiple statements
 * 
 * Syntax: { stmt1; stmt2; ... }
 * 
 * Creates a new lexical scope. Variables declared inside are local to the block.
 * Used for function bodies, if/while bodies, and explicit scoping.
 */
class Block : public Stmt {
public:
    const std::vector<std::shared_ptr<Stmt>> statements;
    Block(std::vector<std::shared_ptr<Stmt>> statements) : statements(statements) {}
    std::any accept(StmtVisitor& visitor) override { 
        return visitor.visitBlockStmt(this); 
    }
};

/**
 * Class - Class declaration statement
 * 
 * Syntax: class Name < Superclass { methods... }
 * 
 * Defines a new class with:
 * - name: Identifier token for the class
 * - superclass: Optional parent class (Variable expression)
 * - methods: List of Function statements (method declarations)
 * 
 * Classes support inheritance and method binding.
 */
class Class : public Stmt {
public:
    const Token name;
    const std::shared_ptr<Variable> superclass;
    const std::vector<std::shared_ptr<Function>> methods;
    Class(Token name, std::shared_ptr<Variable> superclass, std::vector<std::shared_ptr<Function>> methods) 
        : name(name), superclass(superclass), methods(methods) {}
    std::any accept(StmtVisitor& visitor) override { 
        return visitor.visitClassStmt(this); 
    }
};

/**
 * Expression - Statement that evaluates an expression
 * 
 * Syntax: expression;
 * 
 * Wraps an expression as a statement. The expression is evaluated for side effects,
 * but its value is discarded. Examples:
 * - foo();           // Function call
 * - x = 10;          // Assignment
 * - 1 + 2;           // Valid but useless
 */
class Expression : public Stmt {
public:
    const std::shared_ptr<Expr> expression;
    Expression(std::shared_ptr<Expr> expression) : expression(expression) {}
    std::any accept(StmtVisitor& visitor) override { 
        return visitor.visitExpressionStmt(this); 
    }
};

/**
 * Function - Function declaration statement
 * 
 * Syntax: fun name(param1, param2, ...) { body }
 * 
 * Defines a named function with:
 * - name: Function identifier
 * - params: List of parameter tokens (names)
 * - body: List of statements forming the function body
 * 
 * Functions are first-class values and can capture closures.
 */
class Function : public Stmt {
public:
    const Token name;
    const std::vector<Token> params;
    const std::vector<std::shared_ptr<Stmt>> body;
    Function(Token name, std::vector<Token> params, std::vector<std::shared_ptr<Stmt>> body) 
        : name(name), params(params), body(body) {}
    std::any accept(StmtVisitor& visitor) override { 
        return visitor.visitFunctionStmt(this); 
    }
};

/**
 * If - Conditional statement
 * 
 * Syntax: if (condition) thenBranch else elseBranch
 * 
 * Executes thenBranch if condition is truthy, otherwise executes elseBranch.
 * - condition: Expression to evaluate (truthy check)
 * - thenBranch: Statement to execute if true
 * - elseBranch: Optional statement to execute if false (can be nullptr)
 */
class If : public Stmt {
public:
    const std::shared_ptr<Expr> condition;
    const std::shared_ptr<Stmt> thenBranch;
    const std::shared_ptr<Stmt> elseBranch;
    If(std::shared_ptr<Expr> condition, std::shared_ptr<Stmt> thenBranch, std::shared_ptr<Stmt> elseBranch) 
        : condition(condition), thenBranch(thenBranch), elseBranch(elseBranch) {}
    std::any accept(StmtVisitor& visitor) override { 
        return visitor.visitIfStmt(this); 
    }
};

/**
 * Print - Output statement (Lox-specific)
 * 
 * Syntax: print expression;
 * 
 * Evaluates expression and outputs its value to stdout.
 * This is a language-level statement in Lox (not a library function).
 */
class Print : public Stmt {
public:
    const std::shared_ptr<Expr> expression;
    Print(std::shared_ptr<Expr> expression) : expression(expression) {}
    std::any accept(StmtVisitor& visitor) override { 
        return visitor.visitPrintStmt(this); 
    }
};

/**
 * Return - Function return statement
 * 
 * Syntax: return value;
 * 
 * Returns from a function with an optional value:
 * - keyword: The 'return' token (for error reporting)
 * - value: Expression to return (nullptr for empty return)
 * 
 * Implemented as an exception-based control flow mechanism.
 */
class Return : public Stmt {
public:
    const Token keyword;
    const std::shared_ptr<Expr> value;
    Return(Token keyword, std::shared_ptr<Expr> value) 
        : keyword(keyword), value(value) {}
    std::any accept(StmtVisitor& visitor) override { 
        return visitor.visitReturnStmt(this); 
    }
};

/**
 * Var - Variable declaration statement
 * 
 * Syntax: var name = initializer;
 * 
 * Declares a variable in current scope:
 * - name: Variable identifier
 * - initializer: Optional initialization expression (nullptr = uninitialized)
 * 
 * Uninitialized variables have value 'nil'.
 */
class Var : public Stmt {
public:
    const Token name;
    const std::shared_ptr<Expr> initializer;
    Var(Token name, std::shared_ptr<Expr> initializer) 
        : name(name), initializer(initializer) {}
    std::any accept(StmtVisitor& visitor) override { 
        return visitor.visitVarStmt(this); 
    }
};

/**
 * While - Loop statement
 * 
 * Syntax: while (condition) body
 * 
 * Repeatedly executes body while condition is truthy:
 * - condition: Expression evaluated before each iteration
 * - body: Statement to execute each iteration
 * 
 * For-loops are desugared into while-loops by the parser.
 */
class While : public Stmt {
public:
    const std::shared_ptr<Expr> condition;
    const std::shared_ptr<Stmt> body;
    While(std::shared_ptr<Expr> condition, std::shared_ptr<Stmt> body) 
        : condition(condition), body(body) {}
    std::any accept(StmtVisitor& visitor) override { 
        return visitor.visitWhileStmt(this); 
    }
};
