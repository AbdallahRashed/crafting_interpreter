#pragma once
#include <string>
#include <vector>

enum class TokenType {
    // Single char
    LEFT_PAREN, RIGHT_PAREN, LEFT_BRACE, RIGHT_BRACE,
    COMMA, DOT, MINUS, PLUS, SEMICOLON, SLASH, STAR,
    // One or two chars
    BANG, BANG_EQUAL, EQUAL, EQUAL_EQUAL,
    GREATER, GREATER_EQUAL, LESS, LESS_EQUAL,
    // Literals
    IDENTIFIER, STRING, NUMBER,
    // Keywords
    AND, CLASS, ELSE, FALSE_, FOR, FUN, IF, NIL,
    OR, PRINT, RETURN, SUPER, THIS, TRUE_, VAR, WHILE,
    // Special
    TOKEN_EOF, TOKEN_ERROR
};

struct Token {
    TokenType type;
    std::string lexeme;
    int line;
    double numberVal = 0;
    std::string stringVal;
};

class Scanner {
public:
    explicit Scanner(const std::string& source);
    std::vector<Token> scanTokens();

private:
    std::string source;
    std::vector<Token> tokens;
    int start = 0;
    int current = 0;
    int line = 1;

    bool isAtEnd() const;
    char advance();
    char peek() const;
    char peekNext() const;
    bool match(char expected);
    void scanToken();
    void string();
    void number();
    void identifier();
    Token makeToken(TokenType type) const;
    void addToken(TokenType type);
};
