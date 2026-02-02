#pragma once

#include <string>
#include <unordered_map>

enum TokenType {
    // Single-character tokens
    LEFT_PAREN, RIGHT_PAREN,
    LEFT_BRACE, RIGHT_BRACE,
    COMMA, DOT, SEMICOLON,
    PLUS, MINUS, STAR, SLASH,
    
    // One or two character tokens
    BANG, BANG_EQUAL,
    EQUAL, EQUAL_EQUAL,
    GREATER, GREATER_EQUAL,
    LESS, LESS_EQUAL,
    
    // Literals
    IDENTIFIER, STRING, NUMBER,
    
    // Keywords
    AND, CLASS, ELSE, FALSE, FUN, FOR, IF, NIL, OR, PRINT,
    PRIVATE, RETURN, SUPER, THIS, TRUE, VAR, WHILE,
    
    // End of file
    TOKEN_EOF
};

// Helper function to convert TokenType to string
std::string tokenTypeToString(TokenType type);

// Keyword lookup table
extern std::unordered_map<std::string, TokenType> keywords;
