#include "Parser.h"
#include <iostream>

Parser::Parser(const std::vector<Token>& tokens) : tokens(tokens) {}

// Main parse method - returns expression AST or nullptr on error
std::vector<std::shared_ptr<Stmt>> Parser::parse() {
   std::vector<std::shared_ptr<Stmt>> statements;
   while(!isAtEnd())
   {
    statements.push_back(declaration());
   }

   return statements;
}



// expression -> assignment
std::shared_ptr<Expr> Parser::expression() {
    return assignment();
}
std::shared_ptr<Stmt> Parser::statement()
{
    if(match(PRINT))
    {
        return printStatement();
    }
    return expressionStatement();
} 

std::shared_ptr<Stmt> Parser::printStatement()
{
    std::shared_ptr<Expr> value=expression();
    consume(SEMICOLON,"Expect ';' after value.");
    return std::make_shared<Print>(value);
}       

std::shared_ptr<Stmt> Parser::expressionStatement()
{
    std::shared_ptr<Expr> expr=expression();
    consume(SEMICOLON,"Expect ';' after expression.");
    return std::make_shared<Expression>(expr);
}

std::shared_ptr<Stmt> Parser::declaration() {
    try {
        if (match(VAR)) return varDeclaration();
        return statement();
    } catch (const ParseError& error) {
        synchronize();
        return nullptr;
    }
}

std::shared_ptr<Stmt>Parser::varDeclaration()
{

  Token name = consume(IDENTIFIER, "Expect variable name.");
    std::shared_ptr<Expr> initializer = nullptr;
    if(match(EQUAL))
    {
     initializer=expression();   
    }
    consume(SEMICOLON, "Expect ';' after variable declaration.");
    return std::make_shared<Var>(name, initializer);   

}
// assignment -> IDENTIFIER "=" assignment | logicalOr
std::shared_ptr<Expr> Parser::assignment() {
    std::shared_ptr<Expr> expr = logicalOr();

    if (match(EQUAL)) {
        Token equals = previous();
        std::shared_ptr<Expr> value = assignment();

        if (auto varExpr = std::dynamic_pointer_cast<Variable>(expr)) {
            std::cout << "Parsed assignment to variable: " << varExpr->name.lexeme << std::endl;
            return std::make_shared<Assign>(varExpr->name, value);
        } else if (auto getExpr = std::dynamic_pointer_cast<Get>(expr)) {
            return std::make_shared<Set>(getExpr->object, getExpr->name, value);
        }

        error(equals, "Invalid assignment target.");
        // Don't throw, just continue parsing
    }

    return expr;
}

// logicalOr -> logicalAnd ( "or" logicalAnd )*
std::shared_ptr<Expr> Parser::logicalOr() {
    std::shared_ptr<Expr> expr = logicalAnd();

    while (match(OR)) {
        Token op = previous();
        std::shared_ptr<Expr> right = logicalAnd();
        expr = std::make_shared<Logical>(expr, op, right);
    }

    return expr;
}

// logicalAnd -> equality ( "and" equality )*
std::shared_ptr<Expr> Parser::logicalAnd() {
    std::shared_ptr<Expr> expr = equality();

    while (match(AND)) {
        Token op = previous();
        std::shared_ptr<Expr> right = equality();
        expr = std::make_shared<Logical>(expr, op, right);
    }

    return expr;
}

// equality -> comparison ( ( "!=" | "==" ) comparison )*
std::shared_ptr<Expr> Parser::equality() {
    std::shared_ptr<Expr> expr = comparison();

    while (match({BANG_EQUAL, EQUAL_EQUAL})) {
        Token op = previous();
        std::shared_ptr<Expr> right = comparison();
        expr = std::make_shared<Binary>(expr, op, right);
    }

    return expr;
}

// comparison -> term ( ( ">" | ">=" | "<" | "<=" ) term )*
std::shared_ptr<Expr> Parser::comparison() {
    std::shared_ptr<Expr> expr = term();

    while (match({GREATER, GREATER_EQUAL, LESS, LESS_EQUAL})) {
        Token op = previous();
        std::shared_ptr<Expr> right = term();
        expr = std::make_shared<Binary>(expr, op, right);
    }

    return expr;
}

// term -> factor ( ( "-" | "+" ) factor )*
std::shared_ptr<Expr> Parser::term() {
    std::shared_ptr<Expr> expr = factor();

    while (match({MINUS, PLUS})) {
        Token op = previous();
        std::shared_ptr<Expr> right = factor();
        expr = std::make_shared<Binary>(expr, op, right);
        std::cout << "Parsed binary expression: " << tokenTypeToString(op.type) << std::endl;       
    }

    return expr;
}

// factor -> unary ( ( "/" | "*" ) unary )*
std::shared_ptr<Expr> Parser::factor() {
    std::shared_ptr<Expr> expr = unary();

    while (match({SLASH, STAR})) {
        Token op = previous();
        std::shared_ptr<Expr> right = unary();
        expr = std::make_shared<Binary>(expr, op, right);
    }

    return expr;
}

// unary -> ( "!" | "-" ) unary | call
std::shared_ptr<Expr> Parser::unary() {
    if (match({BANG, MINUS})) {
        Token op = previous();
        std::shared_ptr<Expr> right = unary();
        return std::make_shared<Unary>(op, right);
    }

    return call();
}

// call -> primary ( "(" arguments? ")" | "." IDENTIFIER )*
std::shared_ptr<Expr> Parser::call() {
    std::shared_ptr<Expr> expr = primary();

    while (true) {
        if (match(LEFT_PAREN)) {
            expr = finishCall(expr);
        } else if (match(DOT)) {
            Token name = consume(IDENTIFIER, "Expect property name after '.'.");
            expr = std::make_shared<Get>(expr, name);
        } else {
            break;
        }
    }

    return expr;
}

// finishCall -> parse function call arguments
std::shared_ptr<Expr> Parser::finishCall(std::shared_ptr<Expr> callee) {
    std::vector<std::shared_ptr<Expr>> arguments;

    if (!check(RIGHT_PAREN)) {
        do {
            if (arguments.size() >= 255) {
                error(peek(), "Can't have more than 255 arguments.");
            }
            arguments.push_back(expression());
        } while (match(COMMA));
    }

    Token paren = consume(RIGHT_PAREN, "Expect ')' after arguments.");
    return std::make_shared<Call>(callee, paren, arguments);
}

// primary -> NUMBER | STRING | "true" | "false" | "nil" 
//          | "(" expression ")" | IDENTIFIER | "this" | "super" "." IDENTIFIER
std::shared_ptr<Expr> Parser::primary() {
    if (match(FALSE)) return std::make_shared<Literal>(false);
    if (match(TRUE)) return std::make_shared<Literal>(true);
    if (match(NIL)) return std::make_shared<Literal>(nullptr);

    if (match(NUMBER)) {
        return std::make_shared<Literal>(previous().literal);
    }

    if (match(STRING)) {
        return std::make_shared<Literal>(previous().literal);
    }

    if (match(THIS)) {
        return std::make_shared<This>(previous());
    }

    if (match(SUPER)) {
        Token keyword = previous();
        consume(DOT, "Expect '.' after 'super'.");
        Token method = consume(IDENTIFIER, "Expect superclass method name.");
        return std::make_shared<Super>(keyword, method);
    }

    if (match(IDENTIFIER)) {
        return std::make_shared<Variable>(previous());
    }

    if (match(LEFT_PAREN)) {
        std::shared_ptr<Expr> expr = expression();
        consume(RIGHT_PAREN, "Expect ')' after expression.");
        return std::make_shared<Grouping>(expr);
    }

    throw error(peek(), "Expect expression.");
}

// Utility methods

bool Parser::match(TokenType type) {
    if (check(type)) {
        advance();
        return true;
    }
    return false;
}

bool Parser::match(std::initializer_list<TokenType> types) {
    for (TokenType type : types) {
        if (check(type)) {
            advance();
            return true;
        }
    }
    return false;
}

Token Parser::consume(TokenType type, const std::string& message) {
    if (check(type)) return advance();
    throw error(peek(), message);
}

bool Parser::check(TokenType type) const {
    if (isAtEnd()) return false;
    return peek().type == type;
}

Token Parser::advance() {
    if (!isAtEnd()) current++;
    return previous();
}

bool Parser::isAtEnd() const {
    return peek().type == TOKEN_EOF;
}

Token Parser::peek() const {
    return tokens[current];
}

Token Parser::previous() const {
    return tokens[current - 1];
}

// Error handling

ParseError Parser::error(const Token& token, const std::string& message) {
    if (token.type == TOKEN_EOF) {
        std::cerr << "[Line " << token.line << "] Error at end: " 
                  << message << std::endl;
    } else {
        std::cerr << "[Line " << token.line << "] Error at '" 
                  << token.lexeme << "': " << message << std::endl;
    }
    return ParseError(message);
}

void Parser::synchronize() {
    advance();

    while (!isAtEnd()) {
        if (previous().type == SEMICOLON) return;

        switch (peek().type) {
            case CLASS:
            case FUN:
            case VAR:
            case FOR:
            case IF:
            case WHILE:
            case PRINT:
            case RETURN:
                return;
            default:
                break;
        }

        advance();
    }
}
