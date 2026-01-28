#include <string>
#include <list>
#include <iostream>
#include <unordered_map>        
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
    TOKEN_EOF
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
        default: return "UNKNOWN";
    }
}

class Token
{
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
class Scanner
{
private:
    std::string source;
    std::list<Token> tokens;
    int start = 0;
    int current = 0;
    int line = 1;

    char advance();
    char peek() const;
    char peekNext() const;
    bool match(char expected);
    void addToken(TokenType type);
    void addToken(TokenType type, const std::string& literal);
    void scanToken();
    void string();
    void number();
    void identifier();
    bool isDigit(char c) const;
    bool isAlpha(char c) const;
    bool isAlphaNumeric(char c) const;
    bool isAtEnd() const;

public:
    Scanner(const std::string& source) : source(source) {}
    std::list<Token> scanTokens();
};  
std::list<Token> Scanner::scanTokens() {
    while(!isAtEnd()) {
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
          if(match('/'))
          {
          while(peek()!='\n' && !isAtEnd()) advance(); 
          }
          else {
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
         if(isDigit(c))
            number();
         else if(isAlpha(c))
            identifier();

            else {    
            std::cerr << "[Line " << line << "] Error: Unexpected character '" << c << "'." << std::endl;
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
void Scanner::addToken(TokenType type)
{
  std::string text= source.substr(start, current - start);
  tokens.push_back(Token(type, text, line));  
}

void Scanner::addToken(TokenType type, const std::string& literal)
{
  std::string text= source.substr(start, current - start);
  tokens.push_back(Token(type, literal, line));  
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
    if(peek()=='.' && isDigit(peekNext())) {
        // Consume the "."
        advance();

        while (isDigit(peek())) advance();
    }
    std::string text = source.substr(start, current - start);
    addToken(NUMBER, text);
}
bool Scanner::isAlpha(char c) const {
    return (c >= 'a' && c <= 'z') ||
           (c >= 'A' && c <= 'Z') ||        
           c == '_';
}
void Scanner::identifier() {
  while(isAlphaNumeric(peek()))
  {
    advance();
  }  

std::string text=source.substr(start,current-start);
TokenType type;
if(keywords.find(text)==keywords.end())
{
type=IDENTIFIER;
}
else{
  type=keywords[text];  
}
addToken(type);
}

bool Scanner::isAlphaNumeric(char c) const
{
   return isAlpha(c)||isDigit(c); 
}
int main(){
    std::cout << "=== Scanner Test Cases ===" << std::endl << std::endl;

    // Test 1: Single character tokens
    std::cout << "Test 1: Single character tokens" << std::endl;
    Scanner scanner1("(){},.-+;*");
    std::list<Token> tokens1 = scanner1.scanTokens();
    for (const auto& token : tokens1) {
        std::cout << token.toString() << std::endl;
    }
    std::cout << std::endl;

    // Test 2: Two character tokens
    std::cout << "Test 2: Two character tokens" << std::endl;
    Scanner scanner2("! != == = < <= > >=");
    std::list<Token> tokens2 = scanner2.scanTokens();
    for (const auto& token : tokens2) {
        std::cout << token.toString() << std::endl;
    }
    std::cout << std::endl;

    // Test 3: Comments
    std::cout << "Test 3: Comments" << std::endl;
    Scanner scanner3("// this is a comment\n(\n// another comment\n)");
    std::list<Token> tokens3 = scanner3.scanTokens();
    for (const auto& token : tokens3) {
        std::cout << token.toString() << std::endl;
    }
    std::cout << std::endl;

    // Test 4: Strings
    std::cout << "Test 4: Strings" << std::endl;
    Scanner scanner4("\"hello world\" \"test\"");
    std::list<Token> tokens4 = scanner4.scanTokens();
    for (const auto& token : tokens4) {
        std::cout << token.toString() << std::endl;
    }
    std::cout << std::endl;

    // Test 5: Numbers
    std::cout << "Test 5: Numbers" << std::endl;
    Scanner scanner5("123 456.789 0.123");
    std::list<Token> tokens5 = scanner5.scanTokens();
    for (const auto& token : tokens5) {
        std::cout << token.toString() << std::endl;
    }
    std::cout << std::endl;

    // Test 6: Identifiers and keywords
    std::cout << "Test 6: Identifiers and keywords" << std::endl;
    Scanner scanner6("var x = 10; if while class fun");
    std::list<Token> tokens6 = scanner6.scanTokens();
    for (const auto& token : tokens6) {
        std::cout << token.toString() << std::endl;
    }
    std::cout << std::endl;

    // Test 7: Mixed expression
    std::cout << "Test 7: Mixed expression" << std::endl;
    Scanner scanner7("var average = (min + max) / 2;");
    std::list<Token> tokens7 = scanner7.scanTokens();
    for (const auto& token : tokens7) {
        std::cout << token.toString() << std::endl;
    }
    std::cout << std::endl;

    // Test 8: Error case - unterminated string
    std::cout << "Test 8: Error case - unterminated string" << std::endl;
    Scanner scanner8("\"unterminated");
    std::list<Token> tokens8 = scanner8.scanTokens();
    for (const auto& token : tokens8) {
        std::cout << token.toString() << std::endl;
    }
    std::cout << std::endl;

    // Test 9: Error case - unexpected character
    std::cout << "Test 9: Error case - unexpected character" << std::endl;
    Scanner scanner9("@ # $");
    std::list<Token> tokens9 = scanner9.scanTokens();
    for (const auto& token : tokens9) {
        std::cout << token.toString() << std::endl;
    }
    std::cout << std::endl;

    // Test 10: Multi-line code
    std::cout << "Test 10: Multi-line code" << std::endl;
    Scanner scanner10("var x = 10;\nprint x;\nif (x > 5) {\n  print \"big\";\n}");
    std::list<Token> tokens10 = scanner10.scanTokens();
    for (const auto& token : tokens10) {
        std::cout << token.toString() << std::endl;
    }

    return 0;
}
