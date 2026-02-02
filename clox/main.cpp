#include <iostream>
#include "Scanner.h"

int main() {
    std::cout << "=== Scanner Test Cases ===" << std::endl << std::endl;

    // Test 1: Single character tokens
    std::cout << "Test 1: Single character tokens" << std::endl;
    Scanner scanner1("(){},.-+;*");
    std::vector<Token> tokens1 = scanner1.scanTokens();
    for (const auto& token : tokens1) {
        std::cout << token.toString() << std::endl;
    }
    std::cout << std::endl;

    // Test 2: Two character tokens
    std::cout << "Test 2: Two character tokens" << std::endl;
    Scanner scanner2("! != == = < <= > >=");
    std::vector<Token> tokens2 = scanner2.scanTokens();
    for (const auto& token : tokens2) {
        std::cout << token.toString() << std::endl;
    }
    std::cout << std::endl;

    // Test 3: Comments
    std::cout << "Test 3: Comments" << std::endl;
    Scanner scanner3("// this is a comment\n(\n// another comment\n)");
    std::vector<Token> tokens3 = scanner3.scanTokens();
    for (const auto& token : tokens3) {
        std::cout << token.toString() << std::endl;
    }
    std::cout << std::endl;

    // Test 4: Strings
    std::cout << "Test 4: Strings" << std::endl;
    Scanner scanner4("\"hello world\" \"test\"");
    std::vector<Token> tokens4 = scanner4.scanTokens();
    for (const auto& token : tokens4) {
        std::cout << token.toString() << std::endl;
    }
    std::cout << std::endl;

    // Test 5: Numbers
    std::cout << "Test 5: Numbers" << std::endl;
    Scanner scanner5("123 456.789 0.123");
    std::vector<Token> tokens5 = scanner5.scanTokens();
    for (const auto& token : tokens5) {
        std::cout << token.toString() << std::endl;
    }
    std::cout << std::endl;

    // Test 6: Identifiers and keywords
    std::cout << "Test 6: Identifiers and keywords" << std::endl;
    Scanner scanner6("var x = 10; if while class fun");
    std::vector<Token> tokens6 = scanner6.scanTokens();
    for (const auto& token : tokens6) {
        std::cout << token.toString() << std::endl;
    }
    std::cout << std::endl;

    // Test 7: Mixed expression
    std::cout << "Test 7: Mixed expression" << std::endl;
    Scanner scanner7("var average = (min + max) / 2;");
    std::vector<Token> tokens7 = scanner7.scanTokens();
    for (const auto& token : tokens7) {
        std::cout << token.toString() << std::endl;
    }
    std::cout << std::endl;

    // Test 8: Error case - unterminated string
    std::cout << "Test 8: Error case - unterminated string" << std::endl;
    Scanner scanner8("\"unterminated");
    std::vector<Token> tokens8 = scanner8.scanTokens();
    for (const auto& token : tokens8) {
        std::cout << token.toString() << std::endl;
    }
    std::cout << std::endl;

    // Test 9: Error case - unexpected character
    std::cout << "Test 9: Error case - unexpected character" << std::endl;
    Scanner scanner9("@ # $");
    std::vector<Token> tokens9 = scanner9.scanTokens();
    for (const auto& token : tokens9) {
        std::cout << token.toString() << std::endl;
    }
    std::cout << std::endl;

    // Test 10: Multi-line code
    std::cout << "Test 10: Multi-line code" << std::endl;
    Scanner scanner10("var x = 10;\nprint x;\nif (x > 5) {\n  print \"big\";\n}");
    std::vector<Token> tokens10 = scanner10.scanTokens();
    for (const auto& token : tokens10) {
        std::cout << token.toString() << std::endl;
    }

    return 0;
}
