#include <iostream>
#include "Scanner.h"
#include "Parser.h"

// Simple AST printer (visitor pattern demonstration)
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
        
        try {
            if (expr->value.type() == typeid(bool)) {
                return std::string(std::any_cast<bool>(expr->value) ? "true" : "false");
            } else if (expr->value.type() == typeid(double)) {
                return std::to_string(std::any_cast<double>(expr->value));
            } else if (expr->value.type() == typeid(std::string)) {
                return std::any_cast<std::string>(expr->value);
            }
        } catch (...) {
            return std::string("nil");
        }
        return std::string("nil");
    }

    std::any visitUnaryExpr(Unary* expr) override {
        return parenthesize(expr->op.lexeme, {expr->right.get()});
    }

    std::any visitVariableExpr(Variable* expr) override {
        return expr->name.lexeme;
    }

    std::any visitAssignExpr(Assign* expr) override {
        return parenthesize("=", {expr->value.get()});
    }

    std::any visitLogicalExpr(Logical* expr) override {
        return parenthesize(expr->op.lexeme, {expr->left.get(), expr->right.get()});
    }

    std::any visitCallExpr(Call* expr) override {
        std::vector<Expr*> args;
        args.push_back(expr->callee.get());
        for (const auto& arg : expr->arguments) {
            args.push_back(arg.get());
        }
        return parenthesize("call", args);
    }

    std::any visitGetExpr(Get* expr) override {
        return parenthesize(".", {expr->object.get()});
    }

    std::any visitSetExpr(Set* expr) override {
        return parenthesize("set", {expr->object.get(), expr->value.get()});
    }

    std::any visitThisExpr(This* expr) override {
        return std::string("this");
    }

    std::any visitSuperExpr(Super* expr) override {
        return std::string("super." + expr->method.lexeme);
    }

    std::string print(Expr* expr) {
        try {
            return std::any_cast<std::string>(expr->accept(*this));
        } catch (...) {
            return "error";
        }
    }

private:
    std::string parenthesize(const std::string& name, const std::vector<Expr*>& exprs) {
        std::string result = "(" + name;
        for (Expr* expr : exprs) {
            result += " ";
            result += print(expr);
        }
        result += ")";
        return result;
    }
};

void testParser(const std::string& source) {
    std::cout << "Source: " << source << std::endl;
    
    Scanner scanner(source);
    std::vector<Token> tokens = scanner.scanTokens();
    
    Parser parser(tokens);
    auto statements = parser.parse();
    
    if (!statements.empty() && statements[0] != nullptr) {
        // Extract expression from ExpressionStmt for display
        AstPrinter printer;
        if (auto exprStmt = dynamic_cast<Expression*>(statements[0].get())) {
            std::cout << "AST: " << printer.print(exprStmt->expression.get()) << std::endl;
        } else {
            std::cout << "Parsed as statement." << std::endl;
        }
    } else {
        std::cout << "Parse failed." << std::endl;
    }
    std::cout << std::endl;
}

int main() {
    std::cout << "=== Parser Expression Tests ===" << std::endl << std::endl;

    // Test 1: Simple arithmetic
    testParser("1 + 2 * 3");

    // Test 2: Grouping
    testParser("(1 + 2) * 3");

    // Test 3: Unary
    testParser("-123");

    // Test 4: Comparison
    testParser("5 > 3");

    // Test 5: Equality
    testParser("5 == 5");

    // Test 6: Logical operators
    testParser("true and false or true");

    // Test 7: Variable
    testParser("x + y");

    // Test 8: Assignment
    testParser("x = 10");

    // Test 9: Function call
    testParser("foo(1, 2, 3)");

    // Test 10: Property access
    testParser("object.property");

    // Test 11: Complex expression
    testParser("(a + b) * c - d / e");

    // Test 12: Error case - missing closing paren
    testParser("(1 + 2");

    // Test 13: Error case - unexpected token
    testParser("1 +");

    return 0;
}
