#include "lexer.h"
#include <cctype>
#include <stdexcept>

const std::vector<Lexer::TokenDefinition>& Lexer::getTokenDefinitions() {
    static const std::vector<TokenDefinition> definitions = {
        {TokenType::INT_KEYWORD, std::regex("^int\\b")},
        {TokenType::VOID_KEYWORD, std::regex("^void\\b")},
        {TokenType::RETURN_KEYWORD, std::regex("^return\\b")},
        {TokenType::IDENTIFIER, std::regex("^[a-zA-Z_]\\w*\\b")},
        {TokenType::CONSTANT, std::regex("^[0-9]+\\b")},
        {TokenType::TILDE, std::regex("^~")},
        {TokenType::DECREMENT, std::regex("^--")},
        {TokenType::HYPHEN, std::regex("^-")},
        {TokenType::PLUS, std::regex("^\\+")},
        {TokenType::STAR, std::regex("^\\*")},
        {TokenType::SLASH, std::regex("^/")},
        {TokenType::OPEN_PAREN, std::regex("^\\(")},
        {TokenType::CLOSE_PAREN, std::regex("^\\)")},
        {TokenType::OPEN_BRACE, std::regex("^\\{")},
        {TokenType::CLOSE_BRACE, std::regex("^\\}")},
        {TokenType::SEMICOLON, std::regex("^;")},
    };
    return definitions;
}

std::vector<Token> Lexer::tokenize(const std::string& input) {
    std::vector<Token> tokens;
    size_t position = 0;

    while (position < input.length()) {
        if (std::isspace(input[position])) {
            position++;
            continue;
        }

        // Single-line comments
        if (position + 1 < input.length() && input[position] == '/' && input[position + 1] == '/') {
            while (position < input.length() && input[position] != '\n') {
                position++;
            }
            continue;
        }

        // Multi-line comments
        if (position + 1 < input.length() && input[position] == '/' && input[position + 1] == '*') {
            position += 2;
            while (position + 1 < input.length()) {
                if (input[position] == '*' && input[position + 1] == '/') {
                    position += 2;
                    break;
                }
                position++;
            }
            continue;
        }

        std::string remaining = input.substr(position);
        bool matched = false;
        Token bestMatch;
        bestMatch.value = "";

        for (const auto& def : getTokenDefinitions()) {
            std::smatch match;
            if (std::regex_search(remaining, match, def.pattern)) {
                std::string matchStr = match.str();
                if (matchStr.length() > bestMatch.value.length()) {
                    bestMatch = {def.type, matchStr};
                    matched = true;
                }
            }
        }

        if (!matched) {
            throw std::runtime_error("Lexical error: Unexpected character at position " + std::to_string(position));
        }

        tokens.push_back(bestMatch);
        position += bestMatch.value.length();
    }

    return tokens;
}
