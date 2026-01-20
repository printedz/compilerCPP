#include "resolver.h"
#include <stdexcept>
#include <string>
#include <unordered_map>

namespace {
    static std::string makeTemporary() {
        static int counter = 0;
        return "t" + std::to_string(counter++);
    }

    static std::unique_ptr<Exp> resolveExp(
        const Exp& exp,
        std::unordered_map<std::string, std::string>& variableMap);

    static std::unique_ptr<Declaration> resolveDeclaration(
        const Declaration& decl,
        std::unordered_map<std::string, std::string>& variableMap) {
        if (variableMap.count(decl.name) != 0) {
            throw std::runtime_error("Resolver error: duplicate variable declaration");
        }
        std::string uniqueName = makeTemporary();
        variableMap[decl.name] = uniqueName;
        std::unique_ptr<Exp> initExpr = nullptr;
        if (decl.init) {
            initExpr = resolveExp(*decl.init, variableMap);
        }
        return std::make_unique<Declaration>(uniqueName, std::move(initExpr));
    }

    static std::unique_ptr<Statement> resolveStatement(
        const Statement& stmt,
        std::unordered_map<std::string, std::string>& variableMap) {
        if (auto* ret = dynamic_cast<const Return*>(&stmt)) {
            return std::make_unique<Return>(resolveExp(*ret->expr, variableMap));
        }
        if (auto* exprStmt = dynamic_cast<const ExpressionStatement*>(&stmt)) {
            return std::make_unique<ExpressionStatement>(resolveExp(*exprStmt->expr, variableMap));
        }
        if (dynamic_cast<const EmptyStatement*>(&stmt)) {
            return std::make_unique<EmptyStatement>();
        }
        throw std::runtime_error("Resolver error: unsupported statement");
    }

    static std::unique_ptr<BlockItem> resolveBlockItem(
        const BlockItem& item,
        std::unordered_map<std::string, std::string>& variableMap) {
        if (auto* decl = dynamic_cast<const Declaration*>(&item)) {
            return resolveDeclaration(*decl, variableMap);
        }
        if (auto* td = dynamic_cast<const Typedef*>(&item)) {
            return std::make_unique<Typedef>(td->name, td->baseType);
        }
        if (auto* stmt = dynamic_cast<const Statement*>(&item)) {
            return resolveStatement(*stmt, variableMap);
        }
        throw std::runtime_error("Resolver error: unsupported block item");
    }

    static std::unique_ptr<Exp> resolveExp(
        const Exp& exp,
        std::unordered_map<std::string, std::string>& variableMap) {
        if (auto* c = dynamic_cast<const Constant*>(&exp)) {
            return std::make_unique<Constant>(c->value);
        }
        if (auto* v = dynamic_cast<const Var*>(&exp)) {
            auto it = variableMap.find(v->name);
            if (it == variableMap.end()) {
                throw std::runtime_error("Undeclared variable!");
            }
            return std::make_unique<Var>(it->second);
        }
        if (auto* u = dynamic_cast<const Unary*>(&exp)) {
            return std::make_unique<Unary>(u->op, resolveExp(*u->expr, variableMap));
        }
        if (auto* b = dynamic_cast<const Binary*>(&exp)) {
            return std::make_unique<Binary>(
                b->op,
                resolveExp(*b->left, variableMap),
                resolveExp(*b->right, variableMap));
        }
        if (auto* a = dynamic_cast<const Assignment*>(&exp)) {
            if (dynamic_cast<const Var*>(a->lhs.get()) == nullptr) {
                throw std::runtime_error("Invalid lvalue!");
            }
            auto lhs = resolveExp(*a->lhs, variableMap);
            auto rhs = resolveExp(*a->rhs, variableMap);
            return std::make_unique<Assignment>(std::move(lhs), std::move(rhs));
        }
        throw std::runtime_error("Resolver error: unsupported expression");
    }
}

std::unique_ptr<Program> Resolver::resolve(const Program& program) {
    std::unordered_map<std::string, std::string> variableMap;
    std::vector<std::unique_ptr<BlockItem>> body;
    body.reserve(program.function->body.size());
    for (const auto& item : program.function->body) {
        body.push_back(resolveBlockItem(*item, variableMap));
    }
    auto function = std::make_unique<Function>(program.function->name, std::move(body));
    return std::make_unique<Program>(std::move(function));
}
