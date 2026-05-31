#include "ast.h"
#include <stdexcept>

Parser::Parser(const std::vector<Token>& tokens) : tokens(tokens) {}

const Token& Parser::peek() const { return tokens[current]; }
const Token& Parser::previous() const { return tokens[current - 1]; }
bool Parser::isAtEnd() const { return peek().type == TokenType::TOKEN_EOF; }

const Token& Parser::advance() {
    if (!isAtEnd()) current++;
    return previous();
}

bool Parser::check(TokenType type) const {
    return !isAtEnd() && peek().type == type;
}

bool Parser::match(TokenType type) {
    if (check(type)) { advance(); return true; }
    return false;
}

Token Parser::consume(TokenType type, const std::string& msg) {
    if (check(type)) return advance();
    error(msg);
    return peek(); // unreachable
}

void Parser::error(const std::string& msg) {
    throw std::runtime_error("[line " + std::to_string(peek().line) + "] " + msg);
}

std::vector<StmtPtr> Parser::parse() {
    std::vector<StmtPtr> stmts;
    while (!isAtEnd()) stmts.push_back(declaration());
    return stmts;
}

StmtPtr Parser::declaration() {
    if (match(TokenType::CLASS)) return classDeclaration();
    if (match(TokenType::FUN)) return funDeclaration();
    if (match(TokenType::VAR)) return varDeclaration();
    return statement();
}

StmtPtr Parser::classDeclaration() {
    Token name = consume(TokenType::IDENTIFIER, "Expect class name.");
    std::string superclass;
    if (match(TokenType::LESS)) {
        consume(TokenType::IDENTIFIER, "Expect superclass name.");
        superclass = previous().lexeme;
    }
    consume(TokenType::LEFT_BRACE, "Expect '{' before class body.");
    std::vector<std::unique_ptr<FunStmt>> methods;
    while (!check(TokenType::RIGHT_BRACE) && !isAtEnd()) {
        Token methodName = consume(TokenType::IDENTIFIER, "Expect method name.");
        consume(TokenType::LEFT_PAREN, "Expect '(' after method name.");
        std::vector<std::string> params;
        if (!check(TokenType::RIGHT_PAREN)) {
            do {
                params.push_back(consume(TokenType::IDENTIFIER, "Expect parameter name.").lexeme);
            } while (match(TokenType::COMMA));
        }
        consume(TokenType::RIGHT_PAREN, "Expect ')' after parameters.");
        consume(TokenType::LEFT_BRACE, "Expect '{' before method body.");
        auto bodyStmt = block();
        auto* blk = static_cast<BlockStmt*>(bodyStmt.get());
        std::vector<StmtPtr> body;
        for (auto& s : blk->statements) body.push_back(std::move(s));
        methods.push_back(std::make_unique<FunStmt>(
            methodName.lexeme, std::move(params), std::move(body), methodName.line));
    }
    consume(TokenType::RIGHT_BRACE, "Expect '}' after class body.");
    return std::make_unique<ClassStmt>(name.lexeme, superclass, std::move(methods), name.line);
}

StmtPtr Parser::funDeclaration() {
    Token name = consume(TokenType::IDENTIFIER, "Expect function name.");
    consume(TokenType::LEFT_PAREN, "Expect '(' after function name.");
    std::vector<std::string> params;
    if (!check(TokenType::RIGHT_PAREN)) {
        do {
            params.push_back(consume(TokenType::IDENTIFIER, "Expect parameter name.").lexeme);
        } while (match(TokenType::COMMA));
    }
    consume(TokenType::RIGHT_PAREN, "Expect ')' after parameters.");
    consume(TokenType::LEFT_BRACE, "Expect '{' before function body.");
    auto bodyStmt = block();
    auto* blk = static_cast<BlockStmt*>(bodyStmt.get());
    std::vector<StmtPtr> body;
    for (auto& s : blk->statements) body.push_back(std::move(s));
    return std::make_unique<FunStmt>(name.lexeme, std::move(params), std::move(body), name.line);
}

StmtPtr Parser::varDeclaration() {
    Token name = consume(TokenType::IDENTIFIER, "Expect variable name.");
    ExprPtr init = nullptr;
    if (match(TokenType::EQUAL)) init = expression();
    consume(TokenType::SEMICOLON, "Expect ';' after variable declaration.");
    return std::make_unique<VarStmt>(name.lexeme, std::move(init), name.line);
}

StmtPtr Parser::statement() {
    if (match(TokenType::PRINT)) return printStatement();
    if (match(TokenType::IF)) return ifStatement();
    if (match(TokenType::WHILE)) return whileStatement();
    if (match(TokenType::FOR)) return forStatement();
    if (match(TokenType::RETURN)) return returnStatement();
    if (match(TokenType::LEFT_BRACE)) return block();
    ExprPtr expr = expression();
    int ln = expr->line;
    consume(TokenType::SEMICOLON, "Expect ';' after expression.");
    return std::make_unique<ExprStmt>(std::move(expr), ln);
}

StmtPtr Parser::printStatement() {
    int ln = previous().line;
    ExprPtr val = expression();
    consume(TokenType::SEMICOLON, "Expect ';' after value.");
    return std::make_unique<PrintStmt>(std::move(val), ln);
}

StmtPtr Parser::ifStatement() {
    int ln = previous().line;
    consume(TokenType::LEFT_PAREN, "Expect '(' after 'if'.");
    ExprPtr cond = expression();
    consume(TokenType::RIGHT_PAREN, "Expect ')' after if condition.");
    StmtPtr thenBr = statement();
    StmtPtr elseBr = nullptr;
    if (match(TokenType::ELSE)) elseBr = statement();
    return std::make_unique<IfStmt>(std::move(cond), std::move(thenBr), std::move(elseBr), ln);
}

StmtPtr Parser::whileStatement() {
    int ln = previous().line;
    consume(TokenType::LEFT_PAREN, "Expect '(' after 'while'.");
    ExprPtr cond = expression();
    consume(TokenType::RIGHT_PAREN, "Expect ')' after while condition.");
    StmtPtr body = statement();
    return std::make_unique<WhileStmt>(std::move(cond), std::move(body), ln);
}

StmtPtr Parser::forStatement() {
    int ln = previous().line;
    consume(TokenType::LEFT_PAREN, "Expect '(' after 'for'.");

    StmtPtr init = nullptr;
    if (match(TokenType::SEMICOLON)) { /* no init */ }
    else if (match(TokenType::VAR)) init = varDeclaration();
    else {
        ExprPtr e = expression();
        consume(TokenType::SEMICOLON, "Expect ';'.");
        init = std::make_unique<ExprStmt>(std::move(e), ln);
    }

    ExprPtr cond = nullptr;
    if (!check(TokenType::SEMICOLON)) cond = expression();
    consume(TokenType::SEMICOLON, "Expect ';' after loop condition.");

    ExprPtr incr = nullptr;
    if (!check(TokenType::RIGHT_PAREN)) incr = expression();
    consume(TokenType::RIGHT_PAREN, "Expect ')' after for clauses.");

    StmtPtr body = statement();

    // Desugar: for (init; cond; incr) body → { init; while (cond) { body; incr; } }
    if (incr) {
        std::vector<StmtPtr> bodyStmts;
        bodyStmts.push_back(std::move(body));
        bodyStmts.push_back(std::make_unique<ExprStmt>(std::move(incr), ln));
        body = std::make_unique<BlockStmt>(std::move(bodyStmts), ln);
    }
    if (!cond) cond = std::make_unique<BoolExpr>(true, ln);
    body = std::make_unique<WhileStmt>(std::move(cond), std::move(body), ln);

    if (init) {
        std::vector<StmtPtr> stmts;
        stmts.push_back(std::move(init));
        stmts.push_back(std::move(body));
        body = std::make_unique<BlockStmt>(std::move(stmts), ln);
    }
    return body;
}

StmtPtr Parser::returnStatement() {
    int ln = previous().line;
    ExprPtr val = nullptr;
    if (!check(TokenType::SEMICOLON)) val = expression();
    consume(TokenType::SEMICOLON, "Expect ';' after return value.");
    return std::make_unique<ReturnStmt>(std::move(val), ln);
}

StmtPtr Parser::block() {
    int ln = previous().line;
    std::vector<StmtPtr> stmts;
    while (!check(TokenType::RIGHT_BRACE) && !isAtEnd())
        stmts.push_back(declaration());
    consume(TokenType::RIGHT_BRACE, "Expect '}' after block.");
    return std::make_unique<BlockStmt>(std::move(stmts), ln);
}

ExprPtr Parser::expression() { return assignment(); }

ExprPtr Parser::assignment() {
    ExprPtr expr = logicOr();
    if (match(TokenType::EQUAL)) {
        ExprPtr val = assignment();
        if (auto* id = dynamic_cast<IdentifierExpr*>(expr.get()))
            return std::make_unique<AssignExpr>(id->name, std::move(val), id->line);
        if (auto* get = dynamic_cast<GetExpr*>(expr.get())) {
            return std::make_unique<SetExpr>(
                std::move(get->object), get->name, std::move(val), get->line);
        }
        error("Invalid assignment target.");
    }
    return expr;
}

ExprPtr Parser::logicOr() {
    ExprPtr left = logicAnd();
    while (match(TokenType::OR)) {
        int ln = previous().line;
        ExprPtr right = logicAnd();
        left = std::make_unique<LogicalExpr>(std::move(left), TokenType::OR, std::move(right), ln);
    }
    return left;
}

ExprPtr Parser::logicAnd() {
    ExprPtr left = equality();
    while (match(TokenType::AND)) {
        int ln = previous().line;
        ExprPtr right = equality();
        left = std::make_unique<LogicalExpr>(std::move(left), TokenType::AND, std::move(right), ln);
    }
    return left;
}

ExprPtr Parser::equality() {
    ExprPtr left = comparison();
    while (match(TokenType::BANG_EQUAL) || match(TokenType::EQUAL_EQUAL)) {
        TokenType op = previous().type;
        int ln = previous().line;
        ExprPtr right = comparison();
        left = std::make_unique<BinaryExpr>(std::move(left), op, std::move(right), ln);
    }
    return left;
}

ExprPtr Parser::comparison() {
    ExprPtr left = term();
    while (match(TokenType::GREATER) || match(TokenType::GREATER_EQUAL) ||
           match(TokenType::LESS) || match(TokenType::LESS_EQUAL)) {
        TokenType op = previous().type;
        int ln = previous().line;
        ExprPtr right = term();
        left = std::make_unique<BinaryExpr>(std::move(left), op, std::move(right), ln);
    }
    return left;
}

ExprPtr Parser::term() {
    ExprPtr left = factor();
    while (match(TokenType::MINUS) || match(TokenType::PLUS)) {
        TokenType op = previous().type;
        int ln = previous().line;
        ExprPtr right = factor();
        left = std::make_unique<BinaryExpr>(std::move(left), op, std::move(right), ln);
    }
    return left;
}

ExprPtr Parser::factor() {
    ExprPtr left = unary();
    while (match(TokenType::SLASH) || match(TokenType::STAR)) {
        TokenType op = previous().type;
        int ln = previous().line;
        ExprPtr right = unary();
        left = std::make_unique<BinaryExpr>(std::move(left), op, std::move(right), ln);
    }
    return left;
}

ExprPtr Parser::unary() {
    if (match(TokenType::BANG) || match(TokenType::MINUS)) {
        TokenType op = previous().type;
        int ln = previous().line;
        ExprPtr right = unary();
        return std::make_unique<UnaryExpr>(op, std::move(right), ln);
    }
    return call();
}

ExprPtr Parser::call() {
    ExprPtr expr = primary();
    while (true) {
        if (match(TokenType::LEFT_PAREN)) {
            expr = finishCall(std::move(expr));
        } else if (match(TokenType::DOT)) {
            Token name = consume(TokenType::IDENTIFIER, "Expect property name after '.'.");
            expr = std::make_unique<GetExpr>(std::move(expr), name.lexeme, name.line);
        } else {
            break;
        }
    }
    return expr;
}

ExprPtr Parser::finishCall(ExprPtr callee) {
    int ln = previous().line;
    std::vector<ExprPtr> args;
    if (!check(TokenType::RIGHT_PAREN)) {
        do { args.push_back(expression()); } while (match(TokenType::COMMA));
    }
    consume(TokenType::RIGHT_PAREN, "Expect ')' after arguments.");
    return std::make_unique<CallExpr>(std::move(callee), std::move(args), ln);
}

ExprPtr Parser::primary() {
    int ln = peek().line;
    if (match(TokenType::NUMBER)) return std::make_unique<NumberExpr>(previous().numberVal, ln);
    if (match(TokenType::STRING)) return std::make_unique<StringExpr>(previous().stringVal, ln);
    if (match(TokenType::TRUE_)) return std::make_unique<BoolExpr>(true, ln);
    if (match(TokenType::FALSE_)) return std::make_unique<BoolExpr>(false, ln);
    if (match(TokenType::NIL)) return std::make_unique<NilExpr>(ln);
    if (match(TokenType::THIS)) return std::make_unique<ThisExpr>(ln);
    if (match(TokenType::SUPER)) {
        consume(TokenType::DOT, "Expect '.' after 'super'.");
        Token method = consume(TokenType::IDENTIFIER, "Expect superclass method name.");
        return std::make_unique<SuperExpr>(method.lexeme, ln);
    }
    if (match(TokenType::IDENTIFIER)) return std::make_unique<IdentifierExpr>(previous().lexeme, ln);
    if (match(TokenType::LEFT_PAREN)) {
        ExprPtr expr = expression();
        consume(TokenType::RIGHT_PAREN, "Expect ')' after expression.");
        return expr;
    }
    error("Expect expression.");
    return nullptr; // unreachable
}
