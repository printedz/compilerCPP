#include <gtest/gtest.h>
#include "lexer.h"

namespace {
std::vector<TokenType> typesFrom(const std::vector<Token>& tokens) {
    std::vector<TokenType> result;
    result.reserve(tokens.size());
    for (const auto& t : tokens) {
        result.push_back(t.type);
    }
    return result;
}
}

TEST(LexerTests, TokenizesSimpleFunction) {
    const std::string source = "int main(void) { return 42; }";
    auto tokens = Lexer::tokenize(source);

    std::vector<TokenType> expected = {
        TokenType::INT_KEYWORD,
        TokenType::IDENTIFIER,
        TokenType::OPEN_PAREN,
        TokenType::VOID_KEYWORD,
        TokenType::CLOSE_PAREN,
        TokenType::OPEN_BRACE,
        TokenType::RETURN_KEYWORD,
        TokenType::CONSTANT,
        TokenType::SEMICOLON,
        TokenType::CLOSE_BRACE};

    EXPECT_EQ(typesFrom(tokens), expected);
    ASSERT_EQ(tokens.size(), expected.size());
    EXPECT_EQ(tokens[1].value, "main");
    EXPECT_EQ(tokens[7].value, "42");
}

TEST(LexerTests, SkipsCommentsAndWhitespace) {
    const std::string source = R"(
        // leading comment
        int main(void) {
            /* block comment */
            return 7; // trailing comment
        }
    )";

    auto tokens = Lexer::tokenize(source);

    std::vector<TokenType> expected = {
        TokenType::INT_KEYWORD,
        TokenType::IDENTIFIER,
        TokenType::OPEN_PAREN,
        TokenType::VOID_KEYWORD,
        TokenType::CLOSE_PAREN,
        TokenType::OPEN_BRACE,
        TokenType::RETURN_KEYWORD,
        TokenType::CONSTANT,
        TokenType::SEMICOLON,
        TokenType::CLOSE_BRACE};

    EXPECT_EQ(typesFrom(tokens), expected);
    ASSERT_EQ(tokens.size(), expected.size());
    EXPECT_EQ(tokens[7].value, "7");
}

TEST(LexerTests, TokenizesLoopKeywords) {
    const std::string source = R"(
        int main(void) {
            do { continue; } while (cond);
            for (i = 0; i < 3; i = i + 1) { break; }
        }
    )";

    auto tokens = Lexer::tokenize(source);

    std::vector<TokenType> expected = {
        TokenType::INT_KEYWORD,
        TokenType::IDENTIFIER,
        TokenType::OPEN_PAREN,
        TokenType::VOID_KEYWORD,
        TokenType::CLOSE_PAREN,
        TokenType::OPEN_BRACE,

        TokenType::DO_KEYWORD,
        TokenType::OPEN_BRACE,
        TokenType::CONTINUE_KEYWORD,
        TokenType::SEMICOLON,
        TokenType::CLOSE_BRACE,
        TokenType::WHILE_KEYWORD,
        TokenType::OPEN_PAREN,
        TokenType::IDENTIFIER, // cond
        TokenType::CLOSE_PAREN,
        TokenType::SEMICOLON,

        TokenType::FOR_KEYWORD,
        TokenType::OPEN_PAREN,
        TokenType::IDENTIFIER, // i
        TokenType::EQUAL,
        TokenType::CONSTANT,
        TokenType::SEMICOLON,
        TokenType::IDENTIFIER, // i
        TokenType::LESSTHAN,
        TokenType::CONSTANT,
        TokenType::SEMICOLON,
        TokenType::IDENTIFIER, // i
        TokenType::EQUAL,
        TokenType::IDENTIFIER, // i
        TokenType::PLUS,
        TokenType::CONSTANT,
        TokenType::CLOSE_PAREN,
        TokenType::OPEN_BRACE,
        TokenType::BREAK_KEYWORD,
        TokenType::SEMICOLON,
        TokenType::CLOSE_BRACE,

        TokenType::CLOSE_BRACE};

    EXPECT_EQ(typesFrom(tokens), expected);
}
