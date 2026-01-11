#ifndef COMPILER_LEXER_H
#define COMPILER_LEXER_H

#include <vector>
#include <string>
#include <regex>
#include "ast.h"

class Lexer {
public:
    static std::vector<Token> tokenize(const std::string& input);

private:
    struct TokenDefinition {
        TokenType type;
        std::regex pattern;
    };

    static const std::vector<TokenDefinition>& getTokenDefinitions();
};

#endif //COMPILER_LEXER_H
