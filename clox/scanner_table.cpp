#include <string>
#include <vector>
#include <iostream>
#include <unordered_map>
#include <cctype>

enum TokenType {
    //single-character tokens
    LEFT_PAREN, RIGHT_PAREN,
    LEFT_BRACE, RIGHT_BRACE,
    COMMA, DOT, SEMICOLON,
    PLUS, MINUS, STAR, SLASH,
    //one or two character tokens
    BANG, BANG_EQUAL,
    EQUAL, EQUAL_EQUAL,
    GREATER, GREATER_EQUAL,
    LESS, LESS_EQUAL,
    //literals
    IDENTIFIER, STRING, NUMBER,
    //keywords
    AND, CLASS, ELSE, FALSE, FUN, FOR, IF, NIL, OR, PRINT,
    PRIVATE, RETURN, SUPER, THIS, TRUE, VAR, WHILE,
    //end of file       
    TOKEN_EOF,
    TOKEN_ERROR
};

// DFA States
enum State {
    START,
    // Single character states
    IN_LEFT_PAREN, IN_RIGHT_PAREN, IN_LEFT_BRACE, IN_RIGHT_BRACE,
    IN_COMMA, IN_DOT, IN_SEMICOLON, IN_PLUS, IN_MINUS, IN_STAR,
    // Multi-character states
    IN_BANG, IN_BANG_EQUAL,
    IN_EQUAL, IN_EQUAL_EQUAL,
    IN_GREATER, IN_GREATER_EQUAL,
    IN_LESS, IN_LESS_EQUAL,
    IN_SLASH, IN_COMMENT,
    // Literal states
    IN_STRING, STRING_END,
    IN_NUMBER, IN_NUMBER_DOT, IN_NUMBER_DECIMAL,
    IN_IDENTIFIER,
    // Special states
    ACCEPT,
    ERROR,
    NUM_STATES
};

// Character classes for transition table
enum CharClass {
    CHAR_LPAREN, CHAR_RPAREN, CHAR_LBRACE, CHAR_RBRACE,
    CHAR_COMMA, CHAR_DOT, CHAR_SEMICOLON,
    CHAR_PLUS, CHAR_MINUS, CHAR_STAR,
    CHAR_BANG, CHAR_EQUAL, CHAR_GREATER, CHAR_LESS,
    CHAR_SLASH, CHAR_QUOTE,
    CHAR_DIGIT, CHAR_ALPHA, CHAR_UNDERSCORE,
    CHAR_NEWLINE, CHAR_WHITESPACE,
    CHAR_OTHER,
    NUM_CHAR_CLASSES
};

std::unordered_map<std::string, TokenType> keywords = {
    {"and", AND},
    {"class", CLASS},
    {"else", ELSE},
    {"false", FALSE},
    {"for", FOR},
    {"fun", FUN},
    {"if", IF},
    {"nil", NIL},
    {"or", OR},
    {"print", PRINT},
    {"private", PRIVATE},
    {"return", RETURN},
    {"super", SUPER},
    {"this", THIS},
    {"true", TRUE},
    {"var", VAR},
    {"while", WHILE}
};

std::string tokenTypeToString(TokenType type) {
    switch(type) {
        case LEFT_PAREN: return "LEFT_PAREN";
        case RIGHT_PAREN: return "RIGHT_PAREN";
        case LEFT_BRACE: return "LEFT_BRACE";
        case RIGHT_BRACE: return "RIGHT_BRACE";
        case COMMA: return "COMMA";
        case DOT: return "DOT";
        case SEMICOLON: return "SEMICOLON";
        case PLUS: return "PLUS";
        case MINUS: return "MINUS";
        case STAR: return "STAR";
        case SLASH: return "SLASH";
        case BANG: return "BANG";
        case BANG_EQUAL: return "BANG_EQUAL";
        case EQUAL: return "EQUAL";
        case EQUAL_EQUAL: return "EQUAL_EQUAL";
        case GREATER: return "GREATER";
        case GREATER_EQUAL: return "GREATER_EQUAL";
        case LESS: return "LESS";
        case LESS_EQUAL: return "LESS_EQUAL";
        case IDENTIFIER: return "IDENTIFIER";
        case STRING: return "STRING";
        case NUMBER: return "NUMBER";
        case AND: return "AND";
        case CLASS: return "CLASS";
        case ELSE: return "ELSE";
        case FALSE: return "FALSE";
        case FUN: return "FUN";
        case FOR: return "FOR";
        case IF: return "IF";
        case NIL: return "NIL";
        case OR: return "OR";
        case PRINT: return "PRINT";
        case PRIVATE: return "PRIVATE";
        case RETURN: return "RETURN";
        case SUPER: return "SUPER";
        case THIS: return "THIS";
        case TRUE: return "TRUE";
        case VAR: return "VAR";
        case WHILE: return "WHILE";
        case TOKEN_EOF: return "TOKEN_EOF";
        case TOKEN_ERROR: return "TOKEN_ERROR";
        default: return "UNKNOWN";
    }
}

class Token {
public:
    TokenType type;
    std::string lexeme;
    int line;
    
    Token(TokenType type, const std::string& lexeme, int line)
        : type(type), lexeme(lexeme), line(line) {}
    
    std::string toString() const {
        return tokenTypeToString(type) + " " + lexeme + " " + std::to_string(line);
    }
};

class TableDrivenScanner {
private:
    std::string source;
    std::vector<Token> tokens;
    int start = 0;
    int current = 0;
    int line = 1;
    
    // Transition Table: [current_state][character_class] -> next_state
    State transitionTable[NUM_STATES][NUM_CHAR_CLASSES];
    
    // Accepting states map to token types
    std::unordered_map<State, TokenType> acceptingStates;
    
    void initializeTransitionTable();
    void initializeAcceptingStates();
    CharClass getCharClass(char c) const;
    bool isAtEnd() const;
    char peek() const;
    char advance();
    void addToken(TokenType type);
    void addToken(TokenType type, const std::string& literal);
    void scanToken();
    
public:
    TableDrivenScanner(const std::string& source);
    std::vector<Token> scanTokens();
    void printTransitionTable();
};

TableDrivenScanner::TableDrivenScanner(const std::string& source) : source(source) {
    // Initialize all transitions to ERROR state
    for (int i = 0; i < NUM_STATES; i++) {
        for (int j = 0; j < NUM_CHAR_CLASSES; j++) {
            transitionTable[i][j] = ERROR;
        }
    }
    initializeTransitionTable();
    initializeAcceptingStates();
}

void TableDrivenScanner::initializeTransitionTable() {
    // Single character tokens - direct transitions from START to accepting state
    transitionTable[START][CHAR_LPAREN] = IN_LEFT_PAREN;
    transitionTable[START][CHAR_RPAREN] = IN_RIGHT_PAREN;
    transitionTable[START][CHAR_LBRACE] = IN_LEFT_BRACE;
    transitionTable[START][CHAR_RBRACE] = IN_RIGHT_BRACE;
    transitionTable[START][CHAR_COMMA] = IN_COMMA;
    transitionTable[START][CHAR_SEMICOLON] = IN_SEMICOLON;
    transitionTable[START][CHAR_PLUS] = IN_PLUS;
    transitionTable[START][CHAR_MINUS] = IN_MINUS;
    transitionTable[START][CHAR_STAR] = IN_STAR;
    
    // Two character tokens - need intermediate states
    transitionTable[START][CHAR_BANG] = IN_BANG;
    transitionTable[IN_BANG][CHAR_EQUAL] = IN_BANG_EQUAL;
    
    transitionTable[START][CHAR_EQUAL] = IN_EQUAL;
    transitionTable[IN_EQUAL][CHAR_EQUAL] = IN_EQUAL_EQUAL;
    
    transitionTable[START][CHAR_GREATER] = IN_GREATER;
    transitionTable[IN_GREATER][CHAR_EQUAL] = IN_GREATER_EQUAL;
    
    transitionTable[START][CHAR_LESS] = IN_LESS;
    transitionTable[IN_LESS][CHAR_EQUAL] = IN_LESS_EQUAL;
    
    // Slash and comments
    transitionTable[START][CHAR_SLASH] = IN_SLASH;
    transitionTable[IN_SLASH][CHAR_SLASH] = IN_COMMENT;
    // In comment, stay in comment until newline
    for (int i = 0; i < NUM_CHAR_CLASSES; i++) {
        if (i != CHAR_NEWLINE) {
            transitionTable[IN_COMMENT][i] = IN_COMMENT;
        }
    }
    
    // Strings
    transitionTable[START][CHAR_QUOTE] = IN_STRING;
    for (int i = 0; i < NUM_CHAR_CLASSES; i++) {
        if (i != CHAR_QUOTE) {
            transitionTable[IN_STRING][i] = IN_STRING;
        }
    }
    transitionTable[IN_STRING][CHAR_QUOTE] = STRING_END;
    
    // Numbers
    transitionTable[START][CHAR_DIGIT] = IN_NUMBER;
    transitionTable[IN_NUMBER][CHAR_DIGIT] = IN_NUMBER;
    transitionTable[IN_NUMBER][CHAR_DOT] = IN_NUMBER_DOT;
    transitionTable[IN_NUMBER_DOT][CHAR_DIGIT] = IN_NUMBER_DECIMAL;
    transitionTable[IN_NUMBER_DECIMAL][CHAR_DIGIT] = IN_NUMBER_DECIMAL;
    
    // Identifiers
    transitionTable[START][CHAR_ALPHA] = IN_IDENTIFIER;
    transitionTable[START][CHAR_UNDERSCORE] = IN_IDENTIFIER;
    transitionTable[IN_IDENTIFIER][CHAR_ALPHA] = IN_IDENTIFIER;
    transitionTable[IN_IDENTIFIER][CHAR_DIGIT] = IN_IDENTIFIER;
    transitionTable[IN_IDENTIFIER][CHAR_UNDERSCORE] = IN_IDENTIFIER;
    
    // Whitespace (stays in START state - ignored)
    transitionTable[START][CHAR_WHITESPACE] = START;
    transitionTable[START][CHAR_NEWLINE] = START;
}

void TableDrivenScanner::initializeAcceptingStates() {
    acceptingStates[IN_LEFT_PAREN] = LEFT_PAREN;
    acceptingStates[IN_RIGHT_PAREN] = RIGHT_PAREN;
    acceptingStates[IN_LEFT_BRACE] = LEFT_BRACE;
    acceptingStates[IN_RIGHT_BRACE] = RIGHT_BRACE;
    acceptingStates[IN_COMMA] = COMMA;
    acceptingStates[IN_DOT] = DOT;
    acceptingStates[IN_SEMICOLON] = SEMICOLON;
    acceptingStates[IN_PLUS] = PLUS;
    acceptingStates[IN_MINUS] = MINUS;
    acceptingStates[IN_STAR] = STAR;
    acceptingStates[IN_BANG] = BANG;
    acceptingStates[IN_BANG_EQUAL] = BANG_EQUAL;
    acceptingStates[IN_EQUAL] = EQUAL;
    acceptingStates[IN_EQUAL_EQUAL] = EQUAL_EQUAL;
    acceptingStates[IN_GREATER] = GREATER;
    acceptingStates[IN_GREATER_EQUAL] = GREATER_EQUAL;
    acceptingStates[IN_LESS] = LESS;
    acceptingStates[IN_LESS_EQUAL] = LESS_EQUAL;
    acceptingStates[IN_SLASH] = SLASH;
    acceptingStates[STRING_END] = STRING;
    acceptingStates[IN_NUMBER] = NUMBER;
    acceptingStates[IN_NUMBER_DECIMAL] = NUMBER;
    acceptingStates[IN_IDENTIFIER] = IDENTIFIER;
}

CharClass TableDrivenScanner::getCharClass(char c) const {
    switch(c) {
        case '(': return CHAR_LPAREN;
        case ')': return CHAR_RPAREN;
        case '{': return CHAR_LBRACE;
        case '}': return CHAR_RBRACE;
        case ',': return CHAR_COMMA;
        case '.': return CHAR_DOT;
        case ';': return CHAR_SEMICOLON;
        case '+': return CHAR_PLUS;
        case '-': return CHAR_MINUS;
        case '*': return CHAR_STAR;
        case '!': return CHAR_BANG;
        case '=': return CHAR_EQUAL;
        case '>': return CHAR_GREATER;
        case '<': return CHAR_LESS;
        case '/': return CHAR_SLASH;
        case '"': return CHAR_QUOTE;
        case '\n': return CHAR_NEWLINE;
        case ' ':
        case '\r':
        case '\t': return CHAR_WHITESPACE;
        default:
            if (isdigit(c)) return CHAR_DIGIT;
            if (isalpha(c)) return CHAR_ALPHA;
            if (c == '_') return CHAR_UNDERSCORE;
            return CHAR_OTHER;
    }
}

bool TableDrivenScanner::isAtEnd() const {
    return current >= source.length();
}

char TableDrivenScanner::peek() const {
    if (isAtEnd()) return '\0';
    return source[current];
}

char TableDrivenScanner::advance() {
    return source[current++];
}

void TableDrivenScanner::addToken(TokenType type) {
    std::string text = source.substr(start, current - start);
    tokens.push_back(Token(type, text, line));
}

void TableDrivenScanner::addToken(TokenType type, const std::string& literal) {
    tokens.push_back(Token(type, literal, line));
}

void TableDrivenScanner::scanToken() {
    State state = START;
    State lastAcceptingState = ERROR;
    int lastAcceptingPos = start;
    
    while (!isAtEnd() && state != ERROR) {
        char c = peek();
        CharClass charClass = getCharClass(c);
        State nextState = transitionTable[state][charClass];
        
        if (nextState == ERROR) {
            // No valid transition, check if we're in accepting state
            if (acceptingStates.find(state) != acceptingStates.end()) {
                // Process the token
                if (state == STRING_END) {
                    std::string value = source.substr(start + 1, current - start - 2);
                    addToken(STRING, value);
                } else if (state == IN_NUMBER || state == IN_NUMBER_DECIMAL) {
                    std::string text = source.substr(start, current - start);
                    addToken(NUMBER, text);
                } else if (state == IN_IDENTIFIER) {
                    std::string text = source.substr(start, current - start);
                    TokenType type = IDENTIFIER;
                    if (keywords.find(text) != keywords.end()) {
                        type = keywords[text];
                    }
                    addToken(type);
                } else if (state != IN_COMMENT && state != START) {
                    addToken(acceptingStates[state]);
                }
                return;
            } else if (state == START && charClass == CHAR_OTHER) {
                // Unexpected character
                std::cerr << "[Line " << line << "] Error: Unexpected character '" 
                         << c << "'." << std::endl;
                advance();
                return;
            }
            return;
        }
        
        // Special handling for newlines in strings and comments
        if (c == '\n') {
            line++;
            if (state == IN_STRING) {
                // Multi-line string
            } else if (state == IN_COMMENT) {
                // End comment on newline
                current++;
                return;
            }
        }
        
        advance();
        state = nextState;
        
        // Track last accepting state for maximal munch
        if (acceptingStates.find(state) != acceptingStates.end()) {
            lastAcceptingState = state;
            lastAcceptingPos = current;
        }
    }
    
    // End of input - check if in accepting state
    if (acceptingStates.find(state) != acceptingStates.end()) {
        if (state == IN_IDENTIFIER) {
            std::string text = source.substr(start, current - start);
            TokenType type = IDENTIFIER;
            if (keywords.find(text) != keywords.end()) {
                type = keywords[text];
            }
            addToken(type);
        } else if (state == IN_NUMBER || state == IN_NUMBER_DECIMAL) {
            std::string text = source.substr(start, current - start);
            addToken(NUMBER, text);
        } else if (state == IN_STRING) {
            std::cerr << "[Line " << line << "] Error: Unterminated string." << std::endl;
        } else if (state != IN_COMMENT && state != START) {
            addToken(acceptingStates[state]);
        }
    }
}

std::vector<Token> TableDrivenScanner::scanTokens() {
    while (!isAtEnd()) {
        start = current;
        scanToken();
    }
    tokens.push_back(Token(TOKEN_EOF, "", line));
    return tokens;
}

void TableDrivenScanner::printTransitionTable() {
    std::cout << "\n=== DFA Transition Table ===\n" << std::endl;
    std::cout << "Sample transitions (non-ERROR states):\n" << std::endl;
    
    const char* stateNames[] = {
        "START", "IN_LEFT_PAREN", "IN_RIGHT_PAREN", "IN_LEFT_BRACE", "IN_RIGHT_BRACE",
        "IN_COMMA", "IN_DOT", "IN_SEMICOLON", "IN_PLUS", "IN_MINUS", "IN_STAR",
        "IN_BANG", "IN_BANG_EQUAL", "IN_EQUAL", "IN_EQUAL_EQUAL",
        "IN_GREATER", "IN_GREATER_EQUAL", "IN_LESS", "IN_LESS_EQUAL",
        "IN_SLASH", "IN_COMMENT", "IN_STRING", "STRING_END",
        "IN_NUMBER", "IN_NUMBER_DOT", "IN_NUMBER_DECIMAL", "IN_IDENTIFIER"
    };
    
    const char* charClassNames[] = {
        "'('", "')'", "'{'", "'}'", "','", "'.'", "';'",
        "'+'", "'-'", "'*'", "'!'", "'='", "'>'", "'<'",
        "'/'", "'\"'", "DIGIT", "ALPHA", "'_'", "NEWLINE", "WHITESPACE", "OTHER"
    };
    
    for (int s = 0; s < 27; s++) {
        for (int c = 0; c < NUM_CHAR_CLASSES; c++) {
            State next = transitionTable[s][c];
            if (next != ERROR && next < 27) {
                std::cout << stateNames[s] << " + " << charClassNames[c] 
                         << " -> " << stateNames[next] << std::endl;
            }
        }
    }
}

int main() {
    std::cout << "=== Table-Driven Scanner Test ===" << std::endl << std::endl;
    
    // Print transition table
    TableDrivenScanner demo("x");
    demo.printTransitionTable();
    std::cout << "\n=== Test Cases ===\n" << std::endl;
    
    // Test 1
    std::cout << "Test 1: Single character tokens" << std::endl;
    TableDrivenScanner scanner1("(){},;+-*");
    std::vector<Token> tokens1 = scanner1.scanTokens();
    for (const auto& token : tokens1) {
        std::cout << token.toString() << std::endl;
    }
    std::cout << std::endl;
    
    // Test 2
    std::cout << "Test 2: Two character tokens" << std::endl;
    TableDrivenScanner scanner2("! != == = < <= > >=");
    std::vector<Token> tokens2 = scanner2.scanTokens();
    for (const auto& token : tokens2) {
        std::cout << token.toString() << std::endl;
    }
    std::cout << std::endl;
    
    // Test 3
    std::cout << "Test 3: Numbers" << std::endl;
    TableDrivenScanner scanner3("123 456.789");
    std::vector<Token> tokens3 = scanner3.scanTokens();
    for (const auto& token : tokens3) {
        std::cout << token.toString() << std::endl;
    }
    std::cout << std::endl;
    
    // Test 4
    std::cout << "Test 4: Identifiers and keywords" << std::endl;
    TableDrivenScanner scanner4("var x = 10; if while");
    std::vector<Token> tokens4 = scanner4.scanTokens();
    for (const auto& token : tokens4) {
        std::cout << token.toString() << std::endl;
    }
    std::cout << std::endl;
    
    // Test 5
    std::cout << "Test 5: Strings" << std::endl;
    TableDrivenScanner scanner5("\"hello world\"");
    std::vector<Token> tokens5 = scanner5.scanTokens();
    for (const auto& token : tokens5) {
        std::cout << token.toString() << std::endl;
    }
    std::cout << std::endl;
    
    return 0;
}
