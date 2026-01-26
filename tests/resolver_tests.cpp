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

std::unique_ptr<Program> parseAndResolve(const std::string& source) {
    auto program = parseProgram(source);
    return Resolver::resolve(*program);
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

TEST(ResolverTests, AnnotatesWhileBreakContinueWithSameLoopId) {
    const std::string source = R"(
        int main(void) {
            int x = 1;
            int y = 0;
            while (x) {
                if (y) break;
                continue;
            }
            return 0;
        }
    )";
    auto resolved = parseAndResolve(source);
    WhileStatement* whileStmt = nullptr;
    for (const auto& item : resolved->function->body->items) {
        whileStmt = dynamic_cast<WhileStatement*>(item.get());
        if (whileStmt) break;
    }
    ASSERT_NE(whileStmt, nullptr);
    ASSERT_FALSE(whileStmt->label.empty());

    auto* compound = dynamic_cast<CompoundStatement*>(whileStmt->body.get());
    ASSERT_NE(compound, nullptr);
    auto* ifStmt = dynamic_cast<IfStatement*>(compound->block->items[0].get());
    ASSERT_NE(ifStmt, nullptr);
    auto* br = dynamic_cast<BreakStatement*>(ifStmt->thenStmt.get());
    ASSERT_NE(br, nullptr);
    auto* cont = dynamic_cast<ContinueStatement*>(compound->block->items[1].get());
    ASSERT_NE(cont, nullptr);

    EXPECT_EQ(br->label, whileStmt->label);
    EXPECT_EQ(cont->label, whileStmt->label);
}

TEST(ResolverTests, AnnotatesNestedLoopsWithDistinctIds) {
    const std::string source = R"(
        int main(void) {
            int c = 1;
            for (;;){
                do {
                    continue;
                } while (c);
                break;
            }
            return 0;
        }
    )";
    auto resolved = parseAndResolve(source);
    ForStatement* outerFor = nullptr;
    for (const auto& item : resolved->function->body->items) {
        outerFor = dynamic_cast<ForStatement*>(item.get());
        if (outerFor) break;
    }
    ASSERT_NE(outerFor, nullptr);
    ASSERT_FALSE(outerFor->label.empty());

    auto* outerBody = dynamic_cast<CompoundStatement*>(outerFor->body.get());
    ASSERT_NE(outerBody, nullptr);

    auto* innerDo = dynamic_cast<DoWhileStatement*>(outerBody->block->items[0].get());
    ASSERT_NE(innerDo, nullptr);
    ASSERT_FALSE(innerDo->label.empty());

    ContinueStatement* cont = dynamic_cast<ContinueStatement*>(innerDo->body.get());
    if (!cont) {
        if (auto* innerCompound = dynamic_cast<CompoundStatement*>(innerDo->body.get())) {
            cont = dynamic_cast<ContinueStatement*>(innerCompound->block->items[0].get());
        }
    }
    ASSERT_NE(cont, nullptr);

    auto* outerBreak = dynamic_cast<BreakStatement*>(outerBody->block->items[1].get());
    ASSERT_NE(outerBreak, nullptr);

    EXPECT_NE(outerFor->label, innerDo->label);
    EXPECT_EQ(cont->label, innerDo->label);
    EXPECT_EQ(outerBreak->label, outerFor->label);
}
