#ifndef COMPILER_PARSER_H
#define COMPILER_PARSER_H

#include <vector>
#include <memory>
#include "ast.h"

class Parser {
public:
    explicit Parser(std::vector<Token> tokens);
    std::unique_ptr<Program> parseProgram();

private:
    std::vector<Token> tokens;
    size_t position = 0;

    // parsing helpers
    std::unique_ptr<Function> parseFunction();
    std::unique_ptr<BlockItem> parseBlockItem();
    std::unique_ptr<Declaration> parseDeclaration();
    std::unique_ptr<Typedef> parseTypedef();
    std::unique_ptr<Statement> parseStatement();
    std::unique_ptr<IfStatement> parseIfStatement();
    std::unique_ptr<Return> parseReturn();
    std::unique_ptr<Exp> parseExp();
    std::unique_ptr<Exp> parseExpWithPrecedence(int minPrec);
    std::unique_ptr<Exp> parseUnary();
    std::unique_ptr<Exp> parseFactor();
    std::unique_ptr<Exp> parsePrimary();

    void expect(TokenType expectedType);
    Token takeToken();
    Token peekToken();
};

#endif //COMPILER_PARSER_H
