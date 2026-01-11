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

    std::unique_ptr<Function> parseFunction();
    void expect(TokenType expectedType);
    Token takeToken();
    Token peekToken();
};

#endif //COMPILER_PARSER_H
