#include "parser.h"
#include <stdexcept>

Parser::Parser(std::vector<Token> tokens) : tokens(std::move(tokens)) {}

std::unique_ptr<Program> Parser::parseProgram() {
    auto function = parseFunction();
    if (position < tokens.size()) {
        throw std::runtime_error("Syntax error: Extra content at the end of file");
    }
    return std::make_unique<Program>(std::move(function));
}

std::unique_ptr<Function> Parser::parseFunction() {
    // Expect: int <identifier>(void) { <statement> }
    expect(TokenType::INT_KEYWORD);

    Token id = takeToken();
    if (id.type != TokenType::IDENTIFIER) {
        throw std::runtime_error("Error: Expected function name");
    }

    expect(TokenType::OPEN_PAREN);
    if (peekToken().type != TokenType::VOID_KEYWORD) {
        throw std::runtime_error("Syntax error: expected 'void' in parameter list");
    }
    takeToken(); // consume 'void'
    expect(TokenType::CLOSE_PAREN);
    expect(TokenType::OPEN_BRACE);

    auto stmt = parseStatement();

    expect(TokenType::CLOSE_BRACE);

    return std::make_unique<Function>(id.value, std::move(stmt));
}

std::unique_ptr<Statement> Parser::parseStatement() {
    // Only Return(exp); for now
    return parseReturn();
}

std::unique_ptr<Return> Parser::parseReturn() {
    expect(TokenType::RETURN_KEYWORD);
    auto e = parseExp();
    expect(TokenType::SEMICOLON);
    return std::make_unique<Return>(std::move(e));
}

std::unique_ptr<Exp> Parser::parseExp() {
    // Grammar: <exp> ::= <int> | <unop> <exp> | '(' <exp> ')'
    // We implement as: unary -> primary | ('~' | '-') unary, and primary -> constant | '(' exp ')'
    return parseUnary();
}

std::unique_ptr<Exp> Parser::parseUnary() {
    if (peekToken().type == TokenType::TILDE) {
        takeToken();
        return std::make_unique<Unary>(UnaryOperator::Complement, parseUnary());
    } else if (peekToken().type == TokenType::HYPHEN) {
        takeToken();
        return std::make_unique<Unary>(UnaryOperator::Negate, parseUnary());
    }

    // primary
    Token t = peekToken();
    if (t.type == TokenType::OPEN_PAREN) {
        takeToken(); // '('
        auto inner = parseExp();
        expect(TokenType::CLOSE_PAREN);
        return inner;
    }

    // constant
    t = takeToken();
    if (t.type == TokenType::CONSTANT) {
        return std::make_unique<Constant>(std::stoi(t.value));
    }

    throw std::runtime_error("Syntax error: expected expression (constant, unary, or parenthesized)");
}

void Parser::expect(TokenType expectedType) {
    Token actual = takeToken();
    if (actual.type != expectedType) {
        throw std::runtime_error("Syntax error: Expected token type " + std::to_string(static_cast<int>(expectedType)));
    }
}

Token Parser::takeToken() {
    if (position >= tokens.size()) {
        throw std::runtime_error("Syntax error: Unexpected end of file");
    }
    return tokens[position++];
}

Token Parser::peekToken() {
    if (position >= tokens.size()) {
        throw std::runtime_error("Syntax error: Unexpected end of file");
    }
    return tokens[position];
}
