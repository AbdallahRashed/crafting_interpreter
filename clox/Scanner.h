#pragma once

#include <string>
#include <vector>
#include "Token.h"

class Scanner {
private:
    std::string source;
    std::vector<Token> tokens;
    int start = 0;
    int current = 0;
    int line = 1;

    char advance();
    char peek() const;
    char peekNext() const;
    bool match(char expected);
    void addToken(TokenType type);
    void addToken(TokenType type, std::any literal);
    void scanToken();
    void string();
    void number();
    void identifier();
    bool isDigit(char c) const;
    bool isAlpha(char c) const;
    bool isAlphaNumeric(char c) const;
    bool isAtEnd() const;

public:
    Scanner(const std::string& source);
    std::vector<Token> scanTokens();
};
