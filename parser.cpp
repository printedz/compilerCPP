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
    expect(TokenType::INT_KEYWORD);

    Token id = takeToken();
    if (id.type != TokenType::IDENTIFIER) {
        throw std::runtime_error("Error: Expected function name");
    }

    expect(TokenType::OPEN_PAREN);
    if (peekToken().type == TokenType::VOID_KEYWORD) {
        takeToken();
    }
    expect(TokenType::CLOSE_PAREN);
    expect(TokenType::OPEN_BRACE);

    std::vector<std::unique_ptr<Instruction>> instructions;

    // Simplified: assume only "return <const>;"
    expect(TokenType::RETURN_KEYWORD);
    Token constToken = takeToken();
    if (constToken.type != TokenType::CONSTANT) {
        throw std::runtime_error("Error: expected constant");
    }
    expect(TokenType::SEMICOLON);

    instructions.push_back(std::make_unique<Mov>(
        std::make_unique<Imm>(std::stoi(constToken.value)),
        std::make_unique<Register>()
    ));
    instructions.push_back(std::make_unique<Ret>());

    expect(TokenType::CLOSE_BRACE);

    return std::make_unique<Function>(id.value, std::move(instructions));
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
