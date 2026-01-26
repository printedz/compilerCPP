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

TEST(ParserTests, ParsesWhileAndDoWhile) {
    const std::string source = R"(
        int main(void) {
            while (x) return y;
            do return z; while (w);
        }
    )";
    auto tokens = Lexer::tokenize(source);
    Parser parser(tokens);
    auto program = parser.parseProgram();

    ASSERT_EQ(program->function->body->items.size(), 2u);

    auto* whileStmt = dynamic_cast<WhileStatement*>(program->function->body->items[0].get());
    ASSERT_NE(whileStmt, nullptr);
    EXPECT_NE(dynamic_cast<Var*>(whileStmt->condition.get()), nullptr);
    auto* whileBody = dynamic_cast<Return*>(whileStmt->body.get());
    ASSERT_NE(whileBody, nullptr);

    auto* doWhile = dynamic_cast<DoWhileStatement*>(program->function->body->items[1].get());
    ASSERT_NE(doWhile, nullptr);
    auto* doReturn = dynamic_cast<Return*>(doWhile->body.get());
    ASSERT_NE(doReturn, nullptr);
    EXPECT_NE(dynamic_cast<Var*>(doReturn->expr.get()), nullptr);
    EXPECT_NE(dynamic_cast<Var*>(doWhile->condition.get()), nullptr);
}

TEST(ParserTests, ParsesForWithDeclarationInit) {
    const std::string source = R"(
        int main(void) {
            for (int i = 0; i < 3; i = i + 1) continue;
        }
    )";
    auto tokens = Lexer::tokenize(source);
    Parser parser(tokens);
    auto program = parser.parseProgram();

    ASSERT_EQ(program->function->body->items.size(), 1u);

    auto* forStmt = dynamic_cast<ForStatement*>(program->function->body->items[0].get());
    ASSERT_NE(forStmt, nullptr);
    auto* initDecl = dynamic_cast<InitDecl*>(forStmt->init.get());
    ASSERT_NE(initDecl, nullptr);
    ASSERT_NE(initDecl->decl->init, nullptr);
    EXPECT_NE(dynamic_cast<Binary*>(forStmt->condition.get()), nullptr);
    EXPECT_NE(dynamic_cast<Assignment*>(forStmt->post.get()), nullptr);

    auto* cont = dynamic_cast<ContinueStatement*>(forStmt->body.get());
    ASSERT_NE(cont, nullptr);
}

TEST(ParserTests, ParsesBreakStatement) {
    const std::string source = "int main(void) { break; }";
    auto tokens = Lexer::tokenize(source);
    Parser parser(tokens);
    auto program = parser.parseProgram();

    ASSERT_EQ(program->function->body->items.size(), 1u);
    auto* br = dynamic_cast<BreakStatement*>(program->function->body->items[0].get());
    ASSERT_NE(br, nullptr);
}
