#include "parser.h"
#include <stdexcept>

namespace {
    static int precedence(TokenType type) {
        switch (type) {
            case TokenType::EQUAL: return 1;
            case TokenType::QUESTION: return 2;
            case TokenType::DOUBLEBAR: return 3;
            case TokenType::DOUBLEAND: return 4;
            case TokenType::TWOEQUAL:
            case TokenType::NOTEQUAL: return 5;
            case TokenType::LESSTHAN:
            case TokenType::LESSEQUALTHAN:
            case TokenType::GREATERTHAN:
            case TokenType::GREATEREQUALTHAN: return 6;
            case TokenType::PLUS:
            case TokenType::HYPHEN: return 7;
            case TokenType::STAR:
            case TokenType::SLASH:
            case TokenType::PERCENT: return 8;
            default: return -1;
        }
    }

    static BinaryOperator tokenToBinaryOperator(TokenType type) {
        switch (type) {
            case TokenType::PLUS: return BinaryOperator::Add;
            case TokenType::HYPHEN: return BinaryOperator::Subtract;
            case TokenType::STAR: return BinaryOperator::Multiply;
            case TokenType::SLASH: return BinaryOperator::Divide;
            case TokenType::PERCENT: return BinaryOperator::Remainder;
            case TokenType::DOUBLEBAR: return BinaryOperator::Or;
            case TokenType::DOUBLEAND: return BinaryOperator::And;
            case TokenType::TWOEQUAL: return BinaryOperator::Equal;
            case TokenType::NOTEQUAL: return BinaryOperator::NotEqual;
            case TokenType::LESSTHAN: return BinaryOperator::LessThan;
            case TokenType::LESSEQUALTHAN: return BinaryOperator::LessOrEqual;
            case TokenType::GREATERTHAN: return BinaryOperator::GreaterThan;
            case TokenType::GREATEREQUALTHAN: return BinaryOperator::GreaterOrEqual;
            default: throw std::runtime_error("Syntax error: Unsupported binary operator");
        }
    }
}

Parser::Parser(std::vector<Token> tokens) : tokens(std::move(tokens)) {}

std::unique_ptr<Program> Parser::parseProgram() {
    auto function = parseFunction();
    if (position < tokens.size()) {
        throw std::runtime_error("Syntax error: Extra content at the end of file");
    }
    return std::make_unique<Program>(std::move(function));
}

std::unique_ptr<Function> Parser::parseFunction() {
    // Expect: int <identifier>(void) { { <block-item> } }
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

    std::vector<std::unique_ptr<BlockItem>> body;
    while (peekToken().type != TokenType::CLOSE_BRACE) {
        body.push_back(parseBlockItem());
    }

    expect(TokenType::CLOSE_BRACE);

    return std::make_unique<Function>(id.value, std::move(body));
}

std::unique_ptr<BlockItem> Parser::parseBlockItem() {
    if (peekToken().type == TokenType::TYPEDEF_KEYWORD) {
        return parseTypedef();
    }
    if (peekToken().type == TokenType::INT_KEYWORD) {
        return parseDeclaration();
    }
    return parseStatement();
}

std::unique_ptr<Typedef> Parser::parseTypedef() {
    expect(TokenType::TYPEDEF_KEYWORD);
    expect(TokenType::INT_KEYWORD);
    Token id = takeToken();
    if (id.type != TokenType::IDENTIFIER) {
        throw std::runtime_error("Error: Expected identifier in typedef");
    }
    expect(TokenType::SEMICOLON);
    return std::make_unique<Typedef>(id.value, "int");
}

std::unique_ptr<Declaration> Parser::parseDeclaration() {
    expect(TokenType::INT_KEYWORD);
    Token id = takeToken();
    if (id.type != TokenType::IDENTIFIER) {
        throw std::runtime_error("Error: Expected identifier in declaration");
    }
    std::unique_ptr<Exp> initExpr = nullptr;
    if (peekToken().type == TokenType::EQUAL) {
        takeToken(); // consume '='
        initExpr = parseExp();
    }
    expect(TokenType::SEMICOLON);
    return std::make_unique<Declaration>(id.value, std::move(initExpr));
}

std::unique_ptr<Statement> Parser::parseStatement() {
    if (peekToken().type == TokenType::RETURN_KEYWORD) {
        return parseReturn();
    }
    if (peekToken().type == TokenType::IF_KEYWORD) {
        return parseIfStatement();
    }
    if (peekToken().type == TokenType::SEMICOLON) {
        takeToken(); // consume ';'
        return std::make_unique<EmptyStatement>();
    }
    auto e = parseExp();
    expect(TokenType::SEMICOLON);
    return std::make_unique<ExpressionStatement>(std::move(e));
}

std::unique_ptr<IfStatement> Parser::parseIfStatement() {
    expect(TokenType::IF_KEYWORD);
    expect(TokenType::OPEN_PAREN);
    auto condition = parseExp();
    expect(TokenType::CLOSE_PAREN);
    auto thenStmt = parseStatement();
    std::unique_ptr<Statement> elseStmt = nullptr;
    if (peekToken().type == TokenType::ELSE_KEYWORD) {
        takeToken(); // consume 'else'
        elseStmt = parseStatement();
    }
    return std::make_unique<IfStatement>(std::move(condition), std::move(thenStmt), std::move(elseStmt));
}

std::unique_ptr<Return> Parser::parseReturn() {
    expect(TokenType::RETURN_KEYWORD);
    auto e = parseExp();
    expect(TokenType::SEMICOLON);
    return std::make_unique<Return>(std::move(e));
}

// Top-level entry for expressions
std::unique_ptr<Exp> Parser::parseExp() {
    return parseExpWithPrecedence(0);
}

std::unique_ptr<Exp> Parser::parseExpWithPrecedence(int minPrec) {
    auto left = parseUnary();
    Token next = peekToken();
    int prec = precedence(next.type);
    while (prec >= minPrec) {
        if (next.type == TokenType::EQUAL) {
            takeToken(); // consume '='
            auto right = parseExpWithPrecedence(prec);
            left = std::make_unique<Assignment>(std::move(left), std::move(right));
        } else if (next.type == TokenType::QUESTION) {
            takeToken(); // consume '?'
            auto middle = parseExpWithPrecedence(0);
            expect(TokenType::COLON);
            auto right = parseExpWithPrecedence(prec);
            left = std::make_unique<Conditional>(
                std::move(left),
                std::move(middle),
                std::move(right));
        } else {
            Token op = takeToken();
            auto right = parseExpWithPrecedence(prec + 1);
            left = std::make_unique<Binary>(
                tokenToBinaryOperator(op.type),
                std::move(left),
                std::move(right));
        }
        next = peekToken();
        prec = precedence(next.type);
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
    // <factor> ::= <int> | <identifier> | "(" <exp> ")"
    Token next = peekToken();

    // Integer literal
    if (next.type == TokenType::CONSTANT) {
        Token t = takeToken();
        return std::make_unique<Constant>(std::stoi(t.value));
    }

    // Identifier
    if (next.type == TokenType::IDENTIFIER) {
        Token t = takeToken();
        return std::make_unique<Var>(t.value);
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
