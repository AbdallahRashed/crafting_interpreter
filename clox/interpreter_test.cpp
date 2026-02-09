/**
 * Interpreter Test Suite
 * 
 * This file tests the complete pipeline:
 * 1. Scanner: Tokenizes source code
 * 2. Parser: Builds AST from tokens (now returns statements)
 * 3. Interpreter: Evaluates the AST and produces results
 * 
 * Tests cover:
 * - Basic arithmetic operations
 * - Operator precedence and associativity
 * - Unary operators (negation, logical NOT)
 * - Comparison and equality operators
 * - String operations
 * - Runtime error handling
 * - Complex nested expressions
 */

#include "Scanner.h"
#include "Parser.h"
#include "Interpreter.h"
#include <iostream>
#include <memory>

/**
 * AstPrinter - Visitor pattern implementation for visualizing expression AST
 * 
 * Converts an expression AST into a human-readable S-expression format.
 * Example: "1 + 2" becomes "(+ 1.000000 2.000000)"
 */
class AstPrinter : public ExprVisitor {
public:
    /**
     * Converts a binary expression to S-expression format
     * Example: left + right => "(+ left right)"
     */
    std::any visitBinaryExpr(Binary* expr) override {
        return parenthesize(expr->op.lexeme, {expr->left.get(), expr->right.get()});
    }

    /**
     * Wraps grouped expressions in parentheses
     */
    std::any visitGroupingExpr(Grouping* expr) override {
        return parenthesize("group", {expr->expression.get()});
    }

    /**
     * Converts literal values to string representation
     * Handles: nil, boolean, double, and string types
     */
    std::any visitLiteralExpr(Literal* expr) override {
        if (!expr->value.has_value()) return std::string("nil");
        if (expr->value.type() == typeid(bool)) {
            return std::string(std::any_cast<bool>(expr->value) ? "true" : "false");
        } else if (expr->value.type() == typeid(double)) {
            return std::to_string(std::any_cast<double>(expr->value));
        } else if (expr->value.type() == typeid(std::string)) {
            return std::any_cast<std::string>(expr->value);
        }
        return std::string("nil");
    }

    /**
     * Formats unary expressions (e.g., -5 or !true)
     */
    std::any visitUnaryExpr(Unary* expr) override {
        return parenthesize(expr->op.lexeme, {expr->right.get()});
    }

    // Visitor methods for other expression types
    std::any visitVariableExpr(Variable* expr) override { return expr->name.lexeme; }
    std::any visitAssignExpr(Assign* expr) override { return parenthesize("=", {expr->value.get()}); }
    std::any visitLogicalExpr(Logical* expr) override {
        return parenthesize(expr->op.lexeme, {expr->left.get(), expr->right.get()});
    }
    std::any visitCallExpr(Call* expr) override { return std::string("call"); }
    std::any visitGetExpr(Get* expr) override { return std::string("get"); }
    std::any visitSetExpr(Set* expr) override { return std::string("set"); }
    std::any visitThisExpr(This* expr) override { return std::string("this"); }
    std::any visitSuperExpr(Super* expr) override { return std::string("super"); }

    /**
     * Entry point to print an expression tree
     */
    std::string print(Expr* expr) {
        return std::any_cast<std::string>(expr->accept(*this));
    }

private:
    /**
     * Helper to format expressions in S-expression style
     * Example: parenthesize("+", {left, right}) => "(+ left right)"
     */
    std::string parenthesize(const std::string& name, std::vector<Expr*> exprs) {
        std::string result = "(" + name;
        for (Expr* expr : exprs) {
            result += " " + std::any_cast<std::string>(expr->accept(*this));
        }
        result += ")";
        return result;
    }
};

/**
 * runTest - Execute a single test case
 * 
 * Process:
 * 1. Scan the source code into tokens
 * 2. Parse tokens into statement AST
 * 3. Extract and print the expression from expression statements
 * 4. Interpret the expression and display result
 * 
 * @param source - Lox source code to test
 * @param description - Human-readable test description
 */
void runTest(const std::string& source, const std::string& description) {
    std::cout << "\n=== " << description << " ===" << std::endl;
    std::cout << "Source: " << source << std::endl;
    
    Scanner scanner(source);
    std::vector<Token> tokens = scanner.scanTokens();
    
    // Parse returns a vector of statements (new in statement-based parser)
    Parser parser(tokens);
    auto statements = parser.parse();
    
    if (statements.empty() || !statements[0]) {
        std::cout << "Parse error occurred." << std::endl;
        return;
    }
    
    // For expression statements, print and evaluate the expression
    // (Most test cases are simple expression statements like "1 + 2;")
    if (auto exprStmt = std::dynamic_pointer_cast<Expression>(statements[0])) {
        AstPrinter printer;
        std::cout << "AST: " << printer.print(exprStmt->expression.get()) << std::endl;
        
        Interpreter interpreter;
        std::any result = interpreter.interpret(exprStmt->expression.get());
        
        if (result.has_value()) {
            std::cout << "Result: " << interpreter.stringify(result) << std::endl;
        }
    } else {
        std::cout << "Not an expression statement." << std::endl;
    }
}

/**
 * Main test driver
 * 
 * Runs comprehensive test suite covering:
 * - Arithmetic operations (Test 1)
 * - Operator precedence and associativity (Test 2)
 * - Unary operators (Test 3)
 * - Comparison operators (Test 4)
 * - Equality operators (Test 5)
 * - String operations (Test 6)
 * - Type comparisons (Test 7)
 * - Runtime error detection (Test 8)
 * - Complex nested expressions (Test 9)
 */
int main() {
    std::cout << "Interpreter Test Suite" << std::endl;
    std::cout << "======================" << std::endl;
    
    // Test 1: Basic arithmetic
    runTest("1 + 2;", "Basic addition");
    runTest("10 - 3;", "Basic subtraction");
    runTest("4 * 5;", "Basic multiplication");
    runTest("20 / 4;", "Basic division");
    
    // Test 2: Operator precedence
    runTest("1 + 2 * 3;", "Precedence: multiplication before addition");
    runTest("(1 + 2) * 3;", "Grouping with parentheses");
    runTest("10 - 2 - 3;", "Left associativity");
    
    // Test 3: Unary operators
    runTest("-5;", "Unary negation");
    runTest("!true;", "Logical NOT on true");
    runTest("!false;", "Logical NOT on false");
    runTest("!!true;", "Double negation");
    
    // Test 4: Comparison operators
    runTest("5 > 3;", "Greater than (true)");
    runTest("3 > 5;", "Greater than (false)");
    runTest("5 >= 5;", "Greater or equal");
    runTest("3 < 5;", "Less than");
    runTest("5 <= 5;", "Less or equal");
    
    // Test 5: Equality operators
    runTest("5 == 5;", "Equality (true)");
    runTest("5 == 3;", "Equality (false)");
    runTest("5 != 3;", "Inequality (true)");
    runTest("5 != 5;", "Inequality (false)");
    runTest("true == true;", "Boolean equality");
    
    // Test 6: String concatenation
    runTest("\"Hello\" + \" \" + \"World\";", "String concatenation");
    
    // Test 7: Mixed types
    runTest("true == false;", "Boolean comparison");
    runTest("nil == nil;", "Nil equality");
    
    // Test 8: Runtime errors
    runTest("-\"string\";", "ERROR: Negating a string");
    runTest("\"hello\" - \"world\";", "ERROR: Subtracting strings");
    runTest("5 + \"string\";", "ERROR: Adding number and string");
    runTest("5 / 0;", "ERROR: Division by zero");
    
    // Test 9: Complex expressions
    runTest("(5 + 3) * 2 - 4 / 2;", "Complex arithmetic");
    runTest("!(5 > 3);", "Negation of comparison");
    runTest("(10 + 5) / 3 > 4;", "Comparison with arithmetic");
    
    std::cout << "\n=== All tests completed ===" << std::endl;
    
    return 0;
}
