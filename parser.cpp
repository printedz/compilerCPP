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

// Top-level entry for expressions
std::unique_ptr<Exp> Parser::parseExp() {
    return parseOr();
}

// Precedence climbing by layered parsing functions:
// OR (||) -> AND (&&) -> Equality (==, !=) -> Relational (<, <=, >, >=)
// -> Add/Sub -> Mul/Div/Mod -> Unary -> Primary

std::unique_ptr<Exp> Parser::parseOr() {
    auto left = parseAnd();
    while (peekToken().type == TokenType::DOUBLEBAR) {
        takeToken(); // consume '||'
        auto right = parseAnd();
        left = std::make_unique<Binary>(BinaryOperator::Or, std::move(left), std::move(right));
    }
    return left;
}

std::unique_ptr<Exp> Parser::parseAnd() {
    auto left = parseEquality();
    while (peekToken().type == TokenType::DOUBLEAND) {
        takeToken(); // consume '&&'
        auto right = parseEquality();
        left = std::make_unique<Binary>(BinaryOperator::And, std::move(left), std::move(right));
    }
    return left;
}

std::unique_ptr<Exp> Parser::parseEquality() {
    auto left = parseRelational();
    while (peekToken().type == TokenType::TWOEQUAL || peekToken().type == TokenType::NOTEQUAL) {
        Token op = takeToken();
        auto right = parseRelational();
        BinaryOperator binOp = (op.type == TokenType::TWOEQUAL) ? BinaryOperator::Equal : BinaryOperator::NotEqual;
        left = std::make_unique<Binary>(binOp, std::move(left), std::move(right));
    }
    return left;
}

std::unique_ptr<Exp> Parser::parseRelational() {
    auto left = parseAddSub();
    while (peekToken().type == TokenType::LESSTHAN
        || peekToken().type == TokenType::LESSEQUALTHAN
        || peekToken().type == TokenType::GREATERTHAN
        || peekToken().type == TokenType::GREATEREQUALTHAN) {
        Token op = takeToken();
        auto right = parseAddSub();
        BinaryOperator binOp = BinaryOperator::LessThan;
        if (op.type == TokenType::LESSTHAN) {
            binOp = BinaryOperator::LessThan;
        } else if (op.type == TokenType::LESSEQUALTHAN) {
            binOp = BinaryOperator::LessOrEqual;
        } else if (op.type == TokenType::GREATERTHAN) {
            binOp = BinaryOperator::GreaterThan;
        } else if (op.type == TokenType::GREATEREQUALTHAN) {
            binOp = BinaryOperator::GreaterOrEqual;
        }
        left = std::make_unique<Binary>(binOp, std::move(left), std::move(right));
    }
    return left;
}

std::unique_ptr<Exp> Parser::parseAddSub() {
    auto left = parseMulDiv();
    while (peekToken().type == TokenType::PLUS || peekToken().type == TokenType::HYPHEN) {
        Token op = takeToken();
        auto right = parseMulDiv();
        BinaryOperator binOp = (op.type == TokenType::PLUS) ? BinaryOperator::Add : BinaryOperator::Subtract;
        left = std::make_unique<Binary>(binOp, std::move(left), std::move(right));
    }
    return left;
}

std::unique_ptr<Exp> Parser::parseMulDiv() {
    auto left = parseUnary();
    while (peekToken().type == TokenType::STAR
        || peekToken().type == TokenType::SLASH
        || peekToken().type == TokenType::PERCENT) {
        Token op = takeToken();
        auto right = parseUnary();
        BinaryOperator binOp = BinaryOperator::Divide;
        if (op.type == TokenType::STAR) {
            binOp = BinaryOperator::Multiply;
        } else if (op.type == TokenType::SLASH) {
            binOp = BinaryOperator::Divide;
        } else {
            binOp = BinaryOperator::Remainder;
        }
        left = std::make_unique<Binary>(binOp, std::move(left), std::move(right));
    }
    return left;
}

std::unique_ptr<Exp> Parser::parseUnary() {
    Token next = peekToken();
    if (next.type == TokenType::HYPHEN || next.type == TokenType::TILDE || next.type == TokenType::BANG || next.type == TokenType::EXCLAMATION) {
        Token opTok = takeToken(); // consume operator
        auto inner = parseUnary(); // right-associative unary
        if (opTok.type == TokenType::HYPHEN) {
            return std::make_unique<Unary>(UnaryOperator::Negate, std::move(inner));
        } else if (opTok.type == TokenType::TILDE) {
            return std::make_unique<Unary>(UnaryOperator::Complement, std::move(inner));
        } else {
            return std::make_unique<Unary>(UnaryOperator::Not, std::move(inner));
        }
    }
    return parseFactor();
}

std::unique_ptr<Exp> Parser::parseFactor() {
    // parse_factor according to grammar:
    // <factor> ::= <int> | "(" <exp> ")"
    Token next = peekToken();

    // Integer literal
    if (next.type == TokenType::CONSTANT) {
        Token t = takeToken();
        return std::make_unique<Constant>(std::stoi(t.value));
    }

    // Parenthesized expression
    if (next.type == TokenType::OPEN_PAREN) {
        takeToken(); // consume '('
        auto inner = parseExp();
        expect(TokenType::CLOSE_PAREN);
        return inner;
    }

    // Otherwise, it's a malformed factor
    throw std::runtime_error("Syntax error: Malformed factor");
}

std::unique_ptr<Exp> Parser::parsePrimary() {
    return parseFactor();
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
