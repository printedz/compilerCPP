#include <gtest/gtest.h>
#include "lexer.h"
#include "parser.h"
#include "resolver.h"

namespace {
std::unique_ptr<Program> parseProgram(const std::string& source) {
    auto tokens = Lexer::tokenize(source);
    Parser parser(tokens);
    return parser.parseProgram();
}
}

TEST(ResolverTests, AllowsShadowingInNestedBlock) {
    const std::string source = R"(int main(void) {
        int x = 1;
        {
            int x = 2;
            return x;
        }
        return x;
    })";

    auto program = parseProgram(source);
    auto resolved = Resolver::resolve(*program);

    auto* outerDecl = dynamic_cast<Declaration*>(resolved->function->body->items[0].get());
    ASSERT_NE(outerDecl, nullptr);

    auto* compound = dynamic_cast<CompoundStatement*>(resolved->function->body->items[1].get());
    ASSERT_NE(compound, nullptr);

    auto* innerDecl = dynamic_cast<Declaration*>(compound->block->items[0].get());
    ASSERT_NE(innerDecl, nullptr);

    auto* innerReturn = dynamic_cast<Return*>(compound->block->items[1].get());
    ASSERT_NE(innerReturn, nullptr);
    auto* innerVar = dynamic_cast<Var*>(innerReturn->expr.get());
    ASSERT_NE(innerVar, nullptr);

    auto* finalReturn = dynamic_cast<Return*>(resolved->function->body->items[2].get());
    ASSERT_NE(finalReturn, nullptr);
    auto* finalVar = dynamic_cast<Var*>(finalReturn->expr.get());
    ASSERT_NE(finalVar, nullptr);

    EXPECT_NE(outerDecl->name, innerDecl->name);
    EXPECT_EQ(innerVar->name, innerDecl->name);
    EXPECT_EQ(finalVar->name, outerDecl->name);
}

TEST(ResolverTests, PropagatesOuterVariablesIntoInnerInitializers) {
    const std::string source = R"(int main(void) {
        int x = 5;
        {
            int y = x;
            return y;
        }
    })";

    auto program = parseProgram(source);
    auto resolved = Resolver::resolve(*program);

    auto* outerDecl = dynamic_cast<Declaration*>(resolved->function->body->items[0].get());
    ASSERT_NE(outerDecl, nullptr);

    auto* compound = dynamic_cast<CompoundStatement*>(resolved->function->body->items[1].get());
    ASSERT_NE(compound, nullptr);

    auto* innerDecl = dynamic_cast<Declaration*>(compound->block->items[0].get());
    ASSERT_NE(innerDecl, nullptr);
    ASSERT_NE(innerDecl->init, nullptr);

    auto* initVar = dynamic_cast<Var*>(innerDecl->init.get());
    ASSERT_NE(initVar, nullptr);
    EXPECT_EQ(initVar->name, outerDecl->name);
}

TEST(ResolverTests, RejectsDuplicateDeclarationInSameScope) {
    const std::string source = R"(int main(void) {
        int x = 1;
        int x = 2;
        return x;
    })";

    auto program = parseProgram(source);

    try {
        auto resolved = Resolver::resolve(*program);
        (void)resolved;
        FAIL() << "Expected duplicate declaration to throw";
    } catch (const std::runtime_error& err) {
        EXPECT_NE(std::string(err.what()).find("duplicate variable declaration"),
                  std::string::npos);
    }
}
