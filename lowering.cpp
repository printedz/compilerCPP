#include <memory>
#include <vector>
#include <string>
#include <functional>
#include <unordered_set>
#include <stdexcept> // For std::runtime_error
#include "ast.h"     // For BinaryOperator and AST definitions
#include "ir.h"      // For IRBinaryOperator and IR definitions
#include "lowering.h"
// Helper to generate fresh temporary variable names: tmp.0, tmp.1, ...
namespace {
    static std::string freshTempName() {
        static int counter = 0;
        return "tmp." + std::to_string(counter++);
    }
    static std::string freshLabelName() {
        static int counter = 0;
        return "L" + std::to_string(counter++);
    }
    static std::unique_ptr<IROperand> ensureCmpDst(
        std::unique_ptr<IROperand> operand,
        std::vector<std::unique_ptr<IRInstruction>>& instructions,
        std::unordered_set<std::string>& pseudos) {
        if (dynamic_cast<IRImm*>(operand.get()) != nullptr) {
            std::string tmpName = freshTempName();
            pseudos.insert(tmpName);
            auto dstVar = std::make_unique<IRPseudo>(tmpName);
            auto dstVarRef = std::make_unique<IRPseudo>(tmpName);
            instructions.push_back(std::make_unique<IRMov>(std::move(operand), std::move(dstVar)));
            return dstVarRef;
        }
        return operand;
    }
    // Lower an Exp to an assembly operand, appending instructions as needed.
    static std::unique_ptr<IROperand> emitTacky(
        const Exp& e,
        std::vector<std::unique_ptr<IRInstruction>>& instructions,
        std::unordered_set<std::string>& pseudos) {
        if (auto c = dynamic_cast<const Constant*>(&e)) {
            return std::make_unique<IRImm>(c->value);
        }
        if (auto v = dynamic_cast<const Var*>(&e)) {
            pseudos.insert(v->name);
            return std::make_unique<IRPseudo>(v->name);
        }
        if (auto a = dynamic_cast<const Assignment*>(&e)) {
            auto lhsVar = dynamic_cast<const Var*>(a->lhs.get());
            if (lhsVar == nullptr) {
                throw std::runtime_error("Lowering error: assignment to non-variable");
            }
            auto rhsVal = emitTacky(*a->rhs, instructions, pseudos);
            pseudos.insert(lhsVar->name);
            instructions.push_back(std::make_unique<IRMov>(
                std::move(rhsVal),
                std::make_unique<IRPseudo>(lhsVar->name)));
            return std::make_unique<IRPseudo>(lhsVar->name);
        }
        if (auto u = dynamic_cast<const Unary*>(&e)) {
            auto srcVal = emitTacky(*u->expr, instructions, pseudos);
            if (u->op == UnaryOperator::LogicalNot) {
                if (auto imm = dynamic_cast<IRImm*>(srcVal.get())) {
                    return std::make_unique<IRImm>(imm->value == 0 ? 1 : 0);
                }
                std::string tmpName = freshTempName();
                pseudos.insert(tmpName);
                auto dstVar = std::make_unique<IRPseudo>(tmpName);
                auto cmpDst = ensureCmpDst(std::move(srcVal), instructions, pseudos);
                instructions.push_back(std::make_unique<IRCmp>(std::make_unique<IRImm>(0), std::move(cmpDst)));
                instructions.push_back(std::make_unique<IRMov>(std::make_unique<IRImm>(0), std::make_unique<IRPseudo>(tmpName)));
                instructions.push_back(std::make_unique<IRSetCC>(IRCondCode::E, std::move(dstVar)));
                return std::make_unique<IRPseudo>(tmpName);
            }
            std::string tmpName = freshTempName();
            pseudos.insert(tmpName);
            auto dstVar = std::make_unique<IRPseudo>(tmpName);
            auto dstVarRef = std::make_unique<IRPseudo>(tmpName);
            instructions.push_back(std::make_unique<IRMov>(std::move(srcVal), std::move(dstVar)));
            IRUnaryOperator op;
            switch (u->op) {
            case UnaryOperator::Complement: op = IRUnaryOperator::Not; break;
            case UnaryOperator::Negate: op = IRUnaryOperator::Neg; break;
            default: throw std::runtime_error("Unsupported unary operator");
            }
            instructions.push_back(std::make_unique<IRUnary>(op, std::move(dstVarRef)));
            return std::make_unique<IRPseudo>(tmpName);
        }
        if (auto b = dynamic_cast<const Binary*>(&e)) {
            if (b->op == BinaryOperator::And || b->op == BinaryOperator::Or) {
                std::string tmpName = freshTempName();
                pseudos.insert(tmpName);
                auto resultVar = std::make_unique<IRPseudo>(tmpName);
                auto resultVarRef = std::make_unique<IRPseudo>(tmpName);

                std::string shortLabel = freshLabelName();
                std::string endLabel = freshLabelName();

                auto leftVal = emitTacky(*b->left, instructions, pseudos);
                auto leftCmpDst = ensureCmpDst(std::move(leftVal), instructions, pseudos);
                instructions.push_back(std::make_unique<IRCmp>(std::make_unique<IRImm>(0), std::move(leftCmpDst)));
                if (b->op == BinaryOperator::And) {
                    instructions.push_back(std::make_unique<IRJumpCC>(IRCondCode::E, shortLabel));
                } else {
                    instructions.push_back(std::make_unique<IRJumpCC>(IRCondCode::NE, shortLabel));
                }

                auto rightVal = emitTacky(*b->right, instructions, pseudos);
                auto rightCmpDst = ensureCmpDst(std::move(rightVal), instructions, pseudos);
                instructions.push_back(std::make_unique<IRCmp>(std::make_unique<IRImm>(0), std::move(rightCmpDst)));
                if (b->op == BinaryOperator::And) {
                    instructions.push_back(std::make_unique<IRJumpCC>(IRCondCode::E, shortLabel));
                    instructions.push_back(std::make_unique<IRMov>(std::make_unique<IRImm>(1), std::move(resultVar)));
                } else {
                    instructions.push_back(std::make_unique<IRJumpCC>(IRCondCode::NE, shortLabel));
                    instructions.push_back(std::make_unique<IRMov>(std::make_unique<IRImm>(0), std::move(resultVar)));
                }

                instructions.push_back(std::make_unique<IRJump>(endLabel));
                instructions.push_back(std::make_unique<IRLabel>(shortLabel));
                if (b->op == BinaryOperator::And) {
                    instructions.push_back(std::make_unique<IRMov>(std::make_unique<IRImm>(0), std::move(resultVarRef)));
                } else {
                    instructions.push_back(std::make_unique<IRMov>(std::make_unique<IRImm>(1), std::move(resultVarRef)));
                }
                instructions.push_back(std::make_unique<IRLabel>(endLabel));

                return std::make_unique<IRPseudo>(tmpName);
            }

            auto leftVal = emitTacky(*b->left, instructions, pseudos);
            auto rightVal = emitTacky(*b->right, instructions, pseudos);
            std::string tmpName = freshTempName();
            pseudos.insert(tmpName);
            if (b->op == BinaryOperator::Add || b->op == BinaryOperator::Sub || b->op == BinaryOperator::Mul) {
                auto dstVar = std::make_unique<IRPseudo>(tmpName);
                auto dstVarRef = std::make_unique<IRPseudo>(tmpName);
                instructions.push_back(std::make_unique<IRMov>(std::move(leftVal), std::move(dstVar)));
                IRBinaryOperator op;
                switch (b->op) {
                case BinaryOperator::Add: op = IRBinaryOperator::Add; break;
                case BinaryOperator::Sub: op = IRBinaryOperator::Sub; break;
                case BinaryOperator::Mul: op = IRBinaryOperator::Mul; break;
                default: throw std::runtime_error("Unsupported binary operator");
                }
                instructions.push_back(std::make_unique<IRBinary>(op, std::move(rightVal), std::move(dstVarRef)));
                return std::make_unique<IRPseudo>(tmpName);
            }
            if (b->op == BinaryOperator::Div || b->op == BinaryOperator::Mod) {
                auto resultVar = std::make_unique<IRPseudo>(tmpName);
                instructions.push_back(std::make_unique<IRMov>(std::move(leftVal), std::make_unique<IRReg>(IRRegister::AX)));
                instructions.push_back(std::make_unique<IRCdq>());
                instructions.push_back(std::make_unique<IRIdiv>(std::move(rightVal)));
                if (b->op == BinaryOperator::Div) {
                    instructions.push_back(std::make_unique<IRMov>(std::make_unique<IRReg>(IRRegister::AX), std::move(resultVar)));
                } else {
                    instructions.push_back(std::make_unique<IRMov>(std::make_unique<IRReg>(IRRegister::DX), std::move(resultVar)));
                }
                return std::make_unique<IRPseudo>(tmpName);
            }
            if (b->op == BinaryOperator::Equal || b->op == BinaryOperator::NotEqual ||
                b->op == BinaryOperator::LessThan || b->op == BinaryOperator::LessOrEqual ||
                b->op == BinaryOperator::GreaterThan || b->op == BinaryOperator::GreaterOrEqual) {
                IRCondCode cond;
                switch (b->op) {
                case BinaryOperator::Equal: cond = IRCondCode::E; break;
                case BinaryOperator::NotEqual: cond = IRCondCode::NE; break;
                case BinaryOperator::LessThan: cond = IRCondCode::L; break;
                case BinaryOperator::LessOrEqual: cond = IRCondCode::LE; break;
                case BinaryOperator::GreaterThan: cond = IRCondCode::G; break;
                case BinaryOperator::GreaterOrEqual: cond = IRCondCode::GE; break;
                default: throw std::runtime_error("Unsupported binary operator");
                }
                auto resultVar = std::make_unique<IRPseudo>(tmpName);
                auto cmpDst = ensureCmpDst(std::move(leftVal), instructions, pseudos);
                instructions.push_back(std::make_unique<IRCmp>(std::move(rightVal), std::move(cmpDst)));
                instructions.push_back(std::make_unique<IRMov>(std::make_unique<IRImm>(0), std::make_unique<IRPseudo>(tmpName)));
                instructions.push_back(std::make_unique<IRSetCC>(cond, std::move(resultVar)));
                return std::make_unique<IRPseudo>(tmpName);
            }
            throw std::runtime_error("Unsupported binary operator");
        }
        if (auto c = dynamic_cast<const Conditional*>(&e)) {
            std::string tmpName = freshTempName();
            pseudos.insert(tmpName);

            std::string elseLabel = freshLabelName();
            std::string endLabel = freshLabelName();

            auto condVal = emitTacky(*c->condition, instructions, pseudos);
            auto cmpDst = ensureCmpDst(std::move(condVal), instructions, pseudos);
            instructions.push_back(std::make_unique<IRCmp>(std::make_unique<IRImm>(0), std::move(cmpDst)));
            instructions.push_back(std::make_unique<IRJumpCC>(IRCondCode::E, elseLabel));

            auto thenVal = emitTacky(*c->thenExpr, instructions, pseudos);
            instructions.push_back(std::make_unique<IRMov>(
                std::move(thenVal),
                std::make_unique<IRPseudo>(tmpName)));
            instructions.push_back(std::make_unique<IRJump>(endLabel));

            instructions.push_back(std::make_unique<IRLabel>(elseLabel));
            auto elseVal = emitTacky(*c->elseExpr, instructions, pseudos);
            instructions.push_back(std::make_unique<IRMov>(
                std::move(elseVal),
                std::make_unique<IRPseudo>(tmpName)));
            instructions.push_back(std::make_unique<IRLabel>(endLabel));

            return std::make_unique<IRPseudo>(tmpName);
        }
        return std::make_unique<IRImm>(0);
    }

    static void emitStatement(
        const Statement& stmt,
        std::vector<std::unique_ptr<IRInstruction>>& instructions,
        std::unordered_set<std::string>& pseudos) {
        if (auto ret = dynamic_cast<const Return*>(&stmt)) {
            auto retVal = emitTacky(*ret->expr, instructions, pseudos);
            instructions.push_back(std::make_unique<IRMov>(std::move(retVal), std::make_unique<IRReg>(IRRegister::AX)));
            instructions.push_back(std::make_unique<IRRet>());
            return;
        }
        if (auto exprStmt = dynamic_cast<const ExpressionStatement*>(&stmt)) {
            (void)emitTacky(*exprStmt->expr, instructions, pseudos);
            return;
        }
        if (auto ifStmt = dynamic_cast<const IfStatement*>(&stmt)) {
            std::string elseLabel = freshLabelName();
            std::string endLabel = freshLabelName();

            auto condVal = emitTacky(*ifStmt->condition, instructions, pseudos);
            auto cmpDst = ensureCmpDst(std::move(condVal), instructions, pseudos);
            instructions.push_back(std::make_unique<IRCmp>(std::make_unique<IRImm>(0), std::move(cmpDst)));
            instructions.push_back(std::make_unique<IRJumpCC>(IRCondCode::E, elseLabel));

            emitStatement(*ifStmt->thenStmt, instructions, pseudos);
            if (ifStmt->elseStmt) {
                instructions.push_back(std::make_unique<IRJump>(endLabel));
                instructions.push_back(std::make_unique<IRLabel>(elseLabel));
                emitStatement(*ifStmt->elseStmt, instructions, pseudos);
                instructions.push_back(std::make_unique<IRLabel>(endLabel));
            } else {
                instructions.push_back(std::make_unique<IRLabel>(elseLabel));
            }
            return;
        }
        if (dynamic_cast<const EmptyStatement*>(&stmt)) {
            return;
        }
        throw std::runtime_error("Lowering error: unsupported statement");
    }
}
std::unique_ptr<IRProgram> Lowering::toIR(const Program& program) {
    const Function& func = *program.function;
    std::vector<std::unique_ptr<IRInstruction>> body;
    std::unordered_set<std::string> pseudos;
    bool sawReturn = false;
    for (const auto& item : func.body) {
        if (sawReturn) {
            break;
        }
        if (auto decl = dynamic_cast<const Declaration*>(item.get())) {
            if (decl->init) {
                auto initVal = emitTacky(*decl->init, body, pseudos);
                pseudos.insert(decl->name);
                body.push_back(std::make_unique<IRMov>(
                    std::move(initVal),
                    std::make_unique<IRPseudo>(decl->name)));
            }
            continue;
        }
        if (dynamic_cast<const Typedef*>(item.get())) {
            continue;
        }
        if (auto stmt = dynamic_cast<const Statement*>(item.get())) {
            emitStatement(*stmt, body, pseudos);
            if (dynamic_cast<const Return*>(stmt) != nullptr) {
                sawReturn = true;
            }
            continue;
        }
        throw std::runtime_error("Lowering error: unsupported block item");
    }
    if (!sawReturn) {
        body.push_back(std::make_unique<IRMov>(std::make_unique<IRImm>(0), std::make_unique<IRReg>(IRRegister::AX)));
        body.push_back(std::make_unique<IRRet>());
    }
    int stackSize = static_cast<int>(pseudos.size()) * 4;
    if (stackSize % 16 != 0) {
        stackSize += (16 - (stackSize % 16));
    }
    if (stackSize > 0) {
        body.insert(body.begin(), std::make_unique<IRAllocateStack>(stackSize));
    }
    auto irFunc = std::make_unique<IRFunction>(func.name, std::move(body));
    return std::make_unique<IRProgram>(std::move(irFunc));
}
