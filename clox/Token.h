#pragma once

#include <string>
#include <any>
#include "TokenType.h"

struct Token {
    TokenType type;
    std::string lexeme;
    std::any literal;  // Can hold string, double, or be empty
    int line;

    Token(TokenType type, const std::string& lexeme, int line)
        : type(type), lexeme(lexeme), literal(), line(line) {}

    Token(TokenType type, const std::string& lexeme, std::any literal, int line)
        : type(type), lexeme(lexeme), literal(literal), line(line) {}

    std::string toString() const {
        return tokenTypeToString(type) + " " + lexeme + " " + std::to_string(line);
    }
};
