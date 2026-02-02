#include "Scanner.h"
#include <iostream>

Scanner::Scanner(const std::string& source) : source(source) {}

std::vector<Token> Scanner::scanTokens() {
    while (!isAtEnd()) {
        // We are at the beginning of the next lexeme.
        start = current;
        scanToken();
    }
    tokens.push_back(Token(TOKEN_EOF, "", line));
    return tokens;
}

bool Scanner::isAtEnd() const {
    return current >= source.length();
}

void Scanner::scanToken() {
    char c = advance();
    switch (c) {
        case '(': addToken(LEFT_PAREN); break;
        case ')': addToken(RIGHT_PAREN); break;
        case '{': addToken(LEFT_BRACE); break;
        case '}': addToken(RIGHT_BRACE); break;
        case ',': addToken(COMMA); break;
        case '.': addToken(DOT); break;
        case '-': addToken(MINUS); break;
        case '+': addToken(PLUS); break;
        case ';': addToken(SEMICOLON); break;
        case '*': addToken(STAR); break;
        case '!':
            addToken(match('=') ? BANG_EQUAL : BANG);
            break;
        case '=':
            addToken(match('=') ? EQUAL_EQUAL : EQUAL);
            break;
        case '>':
            addToken(match('=') ? GREATER_EQUAL : GREATER);
            break;
        case '<':
            addToken(match('=') ? LESS_EQUAL : LESS);
            break;
        case '/':
            if (match('/')) {
                // A comment goes until the end of the line.
                while (peek() != '\n' && !isAtEnd()) advance();
            } else {
                addToken(SLASH);
            }
            break;
        case ' ':
        case '\r':
        case '\t':
            // Ignore whitespace.
            break;
        case '\n':
            line++;
            break;
        case '"':
            string();
            break;
        default:
            if (isDigit(c)) {
                number();
            } else if (isAlpha(c)) {
                identifier();
            } else {
                std::cerr << "[Line " << line << "] Error: Unexpected character '" 
                          << c << "'." << std::endl;
            }
            break;
    }
}

char Scanner::advance() {
    return source[current++];
}

char Scanner::peek() const {
    if (isAtEnd()) return '\0';
    return source[current];
}

char Scanner::peekNext() const {
    if (current + 1 >= source.length()) return '\0';
    return source[current + 1];
}

bool Scanner::match(char expected) {
    if (isAtEnd()) return false;
    if (source[current] != expected) return false;
    current++;
    return true;
}

void Scanner::addToken(TokenType type) {
    std::string text = source.substr(start, current - start);
    tokens.push_back(Token(type, text, line));
}

void Scanner::addToken(TokenType type, std::any literal) {
    std::string text = source.substr(start, current - start);
    tokens.push_back(Token(type, text, literal, line));
}

void Scanner::string() {
    while (peek() != '"' && !isAtEnd()) {
        if (peek() == '\n') line++;
        advance();
    }

    if (isAtEnd()) {
        std::cerr << "[Line " << line << "] Error: Unterminated string." << std::endl;
        return;
    }

    // The closing ".
    advance();

    // Trim the surrounding quotes.
    std::string value = source.substr(start + 1, current - start - 2);
    addToken(STRING, value);
}

bool Scanner::isDigit(char c) const {
    return c >= '0' && c <= '9';
}

void Scanner::number() {
    while (isDigit(peek())) advance();

    // Look for a fractional part.
    if (peek() == '.' && isDigit(peekNext())) {
        // Consume the "."
        advance();
        while (isDigit(peek())) advance();
    }

    std::string text = source.substr(start, current - start);
    addToken(NUMBER, std::stod(text));
}

bool Scanner::isAlpha(char c) const {
    return (c >= 'a' && c <= 'z') ||
           (c >= 'A' && c <= 'Z') ||
           c == '_';
}

void Scanner::identifier() {
    while (isAlphaNumeric(peek())) {
        advance();
    }

    std::string text = source.substr(start, current - start);
    TokenType type;
    
    auto it = keywords.find(text);
    if (it == keywords.end()) {
        type = IDENTIFIER;
    } else {
        type = it->second;
    }
    
    addToken(type);
}

bool Scanner::isAlphaNumeric(char c) const {
    return isAlpha(c) || isDigit(c);
}
