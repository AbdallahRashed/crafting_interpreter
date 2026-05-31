#include "scanner.h"
#include <unordered_map>

static const std::unordered_map<std::string, TokenType> keywords = {
    {"and", TokenType::AND}, {"class", TokenType::CLASS},
    {"else", TokenType::ELSE}, {"false", TokenType::FALSE_},
    {"for", TokenType::FOR}, {"fun", TokenType::FUN},
    {"if", TokenType::IF}, {"nil", TokenType::NIL},
    {"or", TokenType::OR}, {"print", TokenType::PRINT},
    {"return", TokenType::RETURN}, {"super", TokenType::SUPER},
    {"this", TokenType::THIS}, {"true", TokenType::TRUE_},
    {"var", TokenType::VAR}, {"while", TokenType::WHILE},
};

Scanner::Scanner(const std::string& source) : source(source) {}

std::vector<Token> Scanner::scanTokens() {
    while (!isAtEnd()) {
        start = current;
        scanToken();
    }
    tokens.push_back({TokenType::TOKEN_EOF, "", line});
    return tokens;
}

bool Scanner::isAtEnd() const { return current >= (int)source.size(); }
char Scanner::advance() { return source[current++]; }
char Scanner::peek() const { return isAtEnd() ? '\0' : source[current]; }
char Scanner::peekNext() const {
    return (current + 1 >= (int)source.size()) ? '\0' : source[current + 1];
}

bool Scanner::match(char expected) {
    if (isAtEnd() || source[current] != expected) return false;
    current++;
    return true;
}

Token Scanner::makeToken(TokenType type) const {
    return {type, source.substr(start, current - start), line};
}

void Scanner::addToken(TokenType type) {
    tokens.push_back(makeToken(type));
}

void Scanner::scanToken() {
    char c = advance();
    switch (c) {
        case '(': addToken(TokenType::LEFT_PAREN); break;
        case ')': addToken(TokenType::RIGHT_PAREN); break;
        case '{': addToken(TokenType::LEFT_BRACE); break;
        case '}': addToken(TokenType::RIGHT_BRACE); break;
        case ',': addToken(TokenType::COMMA); break;
        case '.': addToken(TokenType::DOT); break;
        case '-': addToken(TokenType::MINUS); break;
        case '+': addToken(TokenType::PLUS); break;
        case ';': addToken(TokenType::SEMICOLON); break;
        case '*': addToken(TokenType::STAR); break;
        case '!': addToken(match('=') ? TokenType::BANG_EQUAL : TokenType::BANG); break;
        case '=': addToken(match('=') ? TokenType::EQUAL_EQUAL : TokenType::EQUAL); break;
        case '<': addToken(match('=') ? TokenType::LESS_EQUAL : TokenType::LESS); break;
        case '>': addToken(match('=') ? TokenType::GREATER_EQUAL : TokenType::GREATER); break;
        case '/':
            if (match('/')) {
                while (peek() != '\n' && !isAtEnd()) advance();
            } else {
                addToken(TokenType::SLASH);
            }
            break;
        case ' ': case '\r': case '\t': break;
        case '\n': line++; break;
        case '"': string(); break;
        default:
            if (c >= '0' && c <= '9') number();
            else if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_') identifier();
            else tokens.push_back({TokenType::TOKEN_ERROR, "Unexpected character.", line});
    }
}

void Scanner::string() {
    while (peek() != '"' && !isAtEnd()) {
        if (peek() == '\n') line++;
        advance();
    }
    if (isAtEnd()) {
        tokens.push_back({TokenType::TOKEN_ERROR, "Unterminated string.", line});
        return;
    }
    advance(); // closing "
    Token tok = makeToken(TokenType::STRING);
    tok.stringVal = source.substr(start + 1, current - start - 2);
    tokens.push_back(tok);
}

void Scanner::number() {
    while (peek() >= '0' && peek() <= '9') advance();
    if (peek() == '.' && peekNext() >= '0' && peekNext() <= '9') {
        advance();
        while (peek() >= '0' && peek() <= '9') advance();
    }
    Token tok = makeToken(TokenType::NUMBER);
    tok.numberVal = std::stod(tok.lexeme);
    tokens.push_back(tok);
}

void Scanner::identifier() {
    while ((peek() >= 'a' && peek() <= 'z') ||
           (peek() >= 'A' && peek() <= 'Z') ||
           (peek() >= '0' && peek() <= '9') ||
           peek() == '_') advance();
    std::string text = source.substr(start, current - start);
    auto it = keywords.find(text);
    addToken(it != keywords.end() ? it->second : TokenType::IDENTIFIER);
}
