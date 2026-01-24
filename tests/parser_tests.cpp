#include <gtest/gtest.h>
#include "lexer.h"
#include "parser.h"

TEST(ParserTests, ParsesReturnConstant) {
    const std::string source = "int main(void) { return 5; }";
    auto tokens = Lexer::tokenize(source);
    Parser parser(tokens);
    auto program = parser.parseProgram();

    ASSERT_NE(program, nullptr);
    EXPECT_EQ(program->function->name, "main");
    ASSERT_EQ(program->function->body->items.size(), 1u);

    auto* ret = dynamic_cast<Return*>(program->function->body->items.front().get());
    ASSERT_NE(ret, nullptr);

    auto* constant = dynamic_cast<Constant*>(ret->expr.get());
    ASSERT_NE(constant, nullptr);
    EXPECT_EQ(constant->value, 5);
}

TEST(ParserTests, RespectsBinaryPrecedence) {
    const std::string source = "int main(void) { return 1 + 2 * 3; }";
    auto tokens = Lexer::tokenize(source);
    Parser parser(tokens);
    auto program = parser.parseProgram();

    ASSERT_EQ(program->function->body->items.size(), 1u);
    auto* ret = dynamic_cast<Return*>(program->function->body->items.front().get());
    ASSERT_NE(ret, nullptr);

    auto* add = dynamic_cast<Binary*>(ret->expr.get());
    ASSERT_NE(add, nullptr);
    EXPECT_EQ(add->op, BinaryOperator::Add);

    auto* leftConst = dynamic_cast<Constant*>(add->left.get());
    ASSERT_NE(leftConst, nullptr);
    EXPECT_EQ(leftConst->value, 1);

    auto* mul = dynamic_cast<Binary*>(add->right.get());
    ASSERT_NE(mul, nullptr);
    EXPECT_EQ(mul->op, BinaryOperator::Multiply);

    auto* rhsLeft = dynamic_cast<Constant*>(mul->left.get());
    auto* rhsRight = dynamic_cast<Constant*>(mul->right.get());
    ASSERT_NE(rhsLeft, nullptr);
    ASSERT_NE(rhsRight, nullptr);
    EXPECT_EQ(rhsLeft->value, 2);
    EXPECT_EQ(rhsRight->value, 3);
}
