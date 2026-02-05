#include "Scanner.h"
#include "Parser.h"
#include "Interpreter.h"
#include <iostream>
#include <memory>

// Simple AST printer to visualize expression structure
class AstPrinter : public ExprVisitor {
public:
    std::any visitBinaryExpr(Binary* expr) override {
        return parenthesize(expr->op.lexeme, {expr->left.get(), expr->right.get()});
    }

    std::any visitGroupingExpr(Grouping* expr) override {
        return parenthesize("group", {expr->expression.get()});
    }

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

    std::any visitUnaryExpr(Unary* expr) override {
        return parenthesize(expr->op.lexeme, {expr->right.get()});
    }

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

    std::string print(Expr* expr) {
        return std::any_cast<std::string>(expr->accept(*this));
    }

private:
    std::string parenthesize(const std::string& name, std::vector<Expr*> exprs) {
        std::string result = "(" + name;
        for (Expr* expr : exprs) {
            result += " " + std::any_cast<std::string>(expr->accept(*this));
        }
        result += ")";
        return result;
    }
};

void runTest(const std::string& source, const std::string& description) {
    std::cout << "\n=== " << description << " ===" << std::endl;
    std::cout << "Source: " << source << std::endl;
    
    Scanner scanner(source);
    std::vector<Token> tokens = scanner.scanTokens();
    
    Parser parser(tokens);
    auto expression = parser.parse();
    
    if (expression == nullptr) {
        std::cout << "Parse error occurred." << std::endl;
        return;
    }
    
    // Print the AST structure
    AstPrinter printer;
    std::cout << "AST: " << printer.print(expression.get()) << std::endl;
    
    Interpreter interpreter;
    std::any result = interpreter.interpret(expression.get());
    
    if (result.has_value()) {
        std::cout << "Result: " << interpreter.stringify(result) << std::endl;
    }
}

int main() {
    std::cout << "Interpreter Test Suite" << std::endl;
    std::cout << "======================" << std::endl;
    
    // Test 1: Basic arithmetic
    runTest("1 + 2", "Basic addition");
    runTest("10 - 3", "Basic subtraction");
    runTest("4 * 5", "Basic multiplication");
    runTest("20 / 4", "Basic division");
    
    // Test 2: Operator precedence
    runTest("1 + 2 * 3", "Precedence: multiplication before addition");
    runTest("(1 + 2) * 3", "Grouping with parentheses");
    runTest("10 - 2 - 3", "Left associativity");
    
    // Test 3: Unary operators
    runTest("-5", "Unary negation");
    runTest("!true", "Logical NOT on true");
    runTest("!false", "Logical NOT on false");
    runTest("!!true", "Double negation");
    
    // Test 4: Comparison operators
    runTest("5 > 3", "Greater than (true)");
    runTest("3 > 5", "Greater than (false)");
    runTest("5 >= 5", "Greater or equal");
    runTest("3 < 5", "Less than");
    runTest("5 <= 5", "Less or equal");
    
    // Test 5: Equality operators
    runTest("5 == 5", "Equality (true)");
    runTest("5 == 3", "Equality (false)");
    runTest("5 != 3", "Inequality (true)");
    runTest("5 != 5", "Inequality (false)");
    runTest("true == true", "Boolean equality");
    
    // Test 6: String concatenation
    runTest("\"Hello\" + \" \" + \"World\"", "String concatenation");
    
    // Test 7: Mixed types
    runTest("true == false", "Boolean comparison");
    runTest("nil == nil", "Nil equality");
    
    // Test 8: Runtime errors
    runTest("-\"string\"", "ERROR: Negating a string");
    runTest("\"hello\" - \"world\"", "ERROR: Subtracting strings");
    runTest("5 + \"string\"", "ERROR: Adding number and string");
    runTest("5 / 0", "ERROR: Division by zero");
    
    // Test 9: Complex expressions
    runTest("(5 + 3) * 2 - 4 / 2", "Complex arithmetic");
    runTest("!(5 > 3)", "Negation of comparison");
    runTest("(10 + 5) / 3 > 4", "Comparison with arithmetic");
    
    std::cout << "\n=== All tests completed ===" << std::endl;
    
    return 0;
}
