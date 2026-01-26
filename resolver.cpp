#include "resolver.h"
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>
#include <optional>

namespace {
    static std::string makeTemporary() {
        static int counter = 0;
        return "t" + std::to_string(counter++);
    }
    static std::string makeLoopLabel() {
        static int counter = 0;
        return "loop" + std::to_string(counter++);
    }

    // Tracks scoped mappings from source variable names to unique lowered names.
    class ScopeStack {
    public:
        ScopeStack() {
            scopes.emplace_back();
        }

        void push() {
            scopes.emplace_back();
        }

        void pop() {
            scopes.pop_back();
        }

        bool declaredInCurrent(const std::string& name) const {
            return scopes.back().count(name) != 0;
        }

        void declare(const std::string& name, const std::string& unique) {
            scopes.back()[name] = unique;
        }

        std::string lookup(const std::string& name) const {
            for (auto it = scopes.rbegin(); it != scopes.rend(); ++it) {
                auto found = it->find(name);
                if (found != it->end()) {
                    return found->second;
                }
            }
            throw std::runtime_error("Undeclared variable!");
        }

    private:
        std::vector<std::unordered_map<std::string, std::string>> scopes;
    };

    static std::unique_ptr<Exp> resolveExp(
        const Exp& exp,
        ScopeStack& scopes);

    static std::unique_ptr<Block> resolveBlock(
        const Block& block,
        ScopeStack& scopes);

    static std::unique_ptr<Declaration> resolveDeclaration(
        const Declaration& decl,
        ScopeStack& scopes) {
        if (scopes.declaredInCurrent(decl.name)) {
            throw std::runtime_error("Resolver error: duplicate variable declaration");
        }
        std::string uniqueName = makeTemporary();
        scopes.declare(decl.name, uniqueName);
        std::unique_ptr<Exp> initExpr = nullptr;
        if (decl.init) {
            initExpr = resolveExp(*decl.init, scopes);
        }
        return std::make_unique<Declaration>(uniqueName, std::move(initExpr));
    }

    static std::unique_ptr<ForInit> resolveForInit(
        const ForInit& init,
        ScopeStack& scopes);

    static std::unique_ptr<Statement> resolveStatement(
        const Statement& stmt,
        ScopeStack& scopes) {
        if (auto* ret = dynamic_cast<const Return*>(&stmt)) {
            return std::make_unique<Return>(resolveExp(*ret->expr, scopes));
        }
        if (auto* ifStmt = dynamic_cast<const IfStatement*>(&stmt)) {
            auto condition = resolveExp(*ifStmt->condition, scopes);
            auto thenStmt = resolveStatement(*ifStmt->thenStmt, scopes);
            std::unique_ptr<Statement> elseStmt = nullptr;
            if (ifStmt->elseStmt) {
                elseStmt = resolveStatement(*ifStmt->elseStmt, scopes);
            }
            return std::make_unique<IfStatement>(
                std::move(condition),
                std::move(thenStmt),
                std::move(elseStmt));
        }
        if (auto* exprStmt = dynamic_cast<const ExpressionStatement*>(&stmt)) {
            return std::make_unique<ExpressionStatement>(resolveExp(*exprStmt->expr, scopes));
        }
        if (dynamic_cast<const BreakStatement*>(&stmt)) {
            return std::make_unique<BreakStatement>();
        }
        if (dynamic_cast<const ContinueStatement*>(&stmt)) {
            return std::make_unique<ContinueStatement>();
        }
        if (auto* whileStmt = dynamic_cast<const WhileStatement*>(&stmt)) {
            auto condition = resolveExp(*whileStmt->condition, scopes);
            auto body = resolveStatement(*whileStmt->body, scopes);
            return std::make_unique<WhileStatement>(std::move(condition), std::move(body), whileStmt->label);
        }
        if (auto* doWhile = dynamic_cast<const DoWhileStatement*>(&stmt)) {
            auto body = resolveStatement(*doWhile->body, scopes);
            auto condition = resolveExp(*doWhile->condition, scopes);
            return std::make_unique<DoWhileStatement>(std::move(body), std::move(condition), doWhile->label);
        }
        if (auto* forStmt = dynamic_cast<const ForStatement*>(&stmt)) {
            scopes.push();
            auto init = resolveForInit(*forStmt->init, scopes);
            std::unique_ptr<Exp> cond = nullptr;
            if (forStmt->condition) {
                cond = resolveExp(*forStmt->condition, scopes);
            }
            std::unique_ptr<Exp> post = nullptr;
            if (forStmt->post) {
                post = resolveExp(*forStmt->post, scopes);
            }
            auto body = resolveStatement(*forStmt->body, scopes);
            scopes.pop();
            return std::make_unique<ForStatement>(
                std::move(init),
                std::move(cond),
                std::move(post),
                std::move(body),
                forStmt->label);
        }
        if (dynamic_cast<const EmptyStatement*>(&stmt)) {
            return std::make_unique<EmptyStatement>();
        }
        if (auto* compound = dynamic_cast<const CompoundStatement*>(&stmt)) {
            return std::make_unique<CompoundStatement>(resolveBlock(*compound->block, scopes));
        }
        throw std::runtime_error("Resolver error: unsupported statement");
    }

    static std::unique_ptr<BlockItem> resolveBlockItem(
        const BlockItem& item,
        ScopeStack& scopes) {
        if (auto* decl = dynamic_cast<const Declaration*>(&item)) {
            return resolveDeclaration(*decl, scopes);
        }
        if (auto* td = dynamic_cast<const Typedef*>(&item)) {
            return std::make_unique<Typedef>(td->name, td->baseType);
        }
        if (auto* stmt = dynamic_cast<const Statement*>(&item)) {
            return resolveStatement(*stmt, scopes);
        }
        throw std::runtime_error("Resolver error: unsupported block item");
    }

    static std::unique_ptr<Exp> resolveExp(
        const Exp& exp,
        ScopeStack& scopes) {
        if (auto* c = dynamic_cast<const Constant*>(&exp)) {
            return std::make_unique<Constant>(c->value);
        }
        if (auto* v = dynamic_cast<const Var*>(&exp)) {
            return std::make_unique<Var>(scopes.lookup(v->name));
        }
        if (auto* u = dynamic_cast<const Unary*>(&exp)) {
            return std::make_unique<Unary>(u->op, resolveExp(*u->expr, scopes));
        }
        if (auto* b = dynamic_cast<const Binary*>(&exp)) {
            return std::make_unique<Binary>(
                b->op,
                resolveExp(*b->left, scopes),
                resolveExp(*b->right, scopes));
        }
        if (auto* a = dynamic_cast<const Assignment*>(&exp)) {
            if (dynamic_cast<const Var*>(a->lhs.get()) == nullptr) {
                throw std::runtime_error("Invalid lvalue!");
            }
            auto lhs = resolveExp(*a->lhs, scopes);
            auto rhs = resolveExp(*a->rhs, scopes);
            return std::make_unique<Assignment>(std::move(lhs), std::move(rhs));
        }
        if (auto* c = dynamic_cast<const Conditional*>(&exp)) {
            auto condition = resolveExp(*c->condition, scopes);
            auto thenExpr = resolveExp(*c->thenExpr, scopes);
            auto elseExpr = resolveExp(*c->elseExpr, scopes);
            return std::make_unique<Conditional>(
                std::move(condition),
                std::move(thenExpr),
                std::move(elseExpr));
        }
        throw std::runtime_error("Resolver error: unsupported expression");
    }

    static std::unique_ptr<Block> resolveBlock(
        const Block& block,
        ScopeStack& scopes) {
        scopes.push();
        std::vector<std::unique_ptr<BlockItem>> items;
        items.reserve(block.items.size());
        for (const auto& item : block.items) {
            items.push_back(resolveBlockItem(*item, scopes));
        }
        scopes.pop();
        return std::make_unique<Block>(std::move(items));
    }

    static std::unique_ptr<ForInit> resolveForInit(
        const ForInit& init,
        ScopeStack& scopes) {
        if (auto* d = dynamic_cast<const InitDecl*>(&init)) {
            auto decl = resolveDeclaration(*d->decl, scopes);
            return std::make_unique<InitDecl>(std::move(decl));
        }
        if (auto* e = dynamic_cast<const InitExp*>(&init)) {
            if (e->expr) {
                return std::make_unique<InitExp>(resolveExp(*e->expr, scopes));
            }
            return std::make_unique<InitExp>(nullptr);
        }
        throw std::runtime_error("Resolver error: unsupported for-init");
    }

    static void annotateStatement(
        Statement& stmt,
        std::optional<std::string> currentLabel);

    static void annotateBlock(Block& block, std::optional<std::string> currentLabel) {
        for (auto& item : block.items) {
            if (auto* stmt = dynamic_cast<Statement*>(item.get())) {
                annotateStatement(*stmt, currentLabel);
            } else if (auto* decl = dynamic_cast<Declaration*>(item.get())) {
                (void)decl; // no-op
            } else if (auto* td = dynamic_cast<Typedef*>(item.get())) {
                (void)td; // no-op
            }
        }
    }

    static void annotateStatement(
        Statement& stmt,
        std::optional<std::string> currentLabel) {
        if (auto* br = dynamic_cast<BreakStatement*>(&stmt)) {
            if (!currentLabel) {
                throw std::runtime_error("Loop annotation error: break outside loop");
            }
            br->label = *currentLabel;
            return;
        }
        if (auto* cont = dynamic_cast<ContinueStatement*>(&stmt)) {
            if (!currentLabel) {
                throw std::runtime_error("Loop annotation error: continue outside loop");
            }
            cont->label = *currentLabel;
            return;
        }
        if (auto* w = dynamic_cast<WhileStatement*>(&stmt)) {
            std::string newLabel = makeLoopLabel();
            w->label = newLabel;
            annotateStatement(*w->body, newLabel);
            return;
        }
        if (auto* dw = dynamic_cast<DoWhileStatement*>(&stmt)) {
            std::string newLabel = makeLoopLabel();
            dw->label = newLabel;
            annotateStatement(*dw->body, newLabel);
            return;
        }
        if (auto* f = dynamic_cast<ForStatement*>(&stmt)) {
            std::string newLabel = makeLoopLabel();
            f->label = newLabel;
            annotateStatement(*f->body, newLabel);
            return;
        }
        if (auto* compound = dynamic_cast<CompoundStatement*>(&stmt)) {
            annotateBlock(*compound->block, currentLabel);
            return;
        }
        if (auto* ifs = dynamic_cast<IfStatement*>(&stmt)) {
            annotateStatement(*ifs->thenStmt, currentLabel);
            if (ifs->elseStmt) {
                annotateStatement(*ifs->elseStmt, currentLabel);
            }
            return;
        }
        if (dynamic_cast<Return*>(&stmt)) return;
        if (dynamic_cast<ExpressionStatement*>(&stmt)) return;
        if (dynamic_cast<EmptyStatement*>(&stmt)) return;
        throw std::runtime_error("Loop annotation error: unsupported statement");
    }
}

std::unique_ptr<Program> Resolver::resolve(const Program& program) {
    ScopeStack scopes;
    auto body = resolveBlock(*program.function->body, scopes);
    auto function = std::make_unique<Function>(program.function->name, std::move(body));
    annotateBlock(*function->body, std::nullopt);
    return std::make_unique<Program>(std::move(function));
}
