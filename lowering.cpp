#include <memory>
#include <vector>
#include <string>
#include <functional>
#include <unordered_set>
#include <stdexcept> // For std::runtime_error
#include "ast.h"     // For BinaryOperator and AST definitions
#include "ir.h"      // For IRBinaryOperator and IR definitions
#include "lowering.h"
// Helper to generate fresh temporary variable names: t0, t1, ...
namespace {
    static std::string freshTempName() {
        static int counter = 0;
        return "t" + std::to_string(counter++);
    }
    static std::string freshLabelName() {
        static int counter = 0;
        return "L" + std::to_string(counter++);
    }
    // Lower an Exp to an assembly operand, appending instructions as needed.
    static std::unique_ptr<IROperand> emitTacky(
        const Exp& e,
        std::vector<std::unique_ptr<IRInstruction>>& instructions,
        std::unordered_set<std::string>& pseudos) {
        if (auto c = dynamic_cast<const Constant*>(&e)) {
            return std::make_unique<IRImm>(c->value);
        }
        if (auto u = dynamic_cast<const Unary*>(&e)) {
            auto srcVal = emitTacky(*u->expr, instructions, pseudos);
            std::string tmpName = freshTempName();
            pseudos.insert(tmpName);
            auto dstVar = std::make_unique<IRPseudo>(tmpName);
            auto dstVarRef = std::make_unique<IRPseudo>(tmpName);
            instructions.push_back(std::make_unique<IRMov>(std::move(srcVal), std::move(dstVar)));
            IRUnaryOperator op;
            switch (u->op) {
            case UnaryOperator::Complement: op = IRUnaryOperator::Not; break;
            case UnaryOperator::Negate: op = IRUnaryOperator::Neg; break;
            case UnaryOperator::LogicalNot: op = IRUnaryOperator::LogicalNot; break;
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
                if (b->op == BinaryOperator::And) {
                    instructions.push_back(std::make_unique<IRJumpIfZero>(std::move(leftVal), shortLabel));
                } else {
                    instructions.push_back(std::make_unique<IRJumpIfNotZero>(std::move(leftVal), shortLabel));
                }

                auto rightVal = emitTacky(*b->right, instructions, pseudos);
                if (b->op == BinaryOperator::And) {
                    instructions.push_back(std::make_unique<IRJumpIfZero>(std::move(rightVal), shortLabel));
                    instructions.push_back(std::make_unique<IRMov>(std::make_unique<IRImm>(1), std::move(resultVar)));
                } else {
                    instructions.push_back(std::make_unique<IRJumpIfNotZero>(std::move(rightVal), shortLabel));
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
            auto dstVar = std::make_unique<IRPseudo>(tmpName);
            auto dstVarRef = std::make_unique<IRPseudo>(tmpName);
            instructions.push_back(std::make_unique<IRMov>(std::move(leftVal), std::move(dstVar)));
            IRBinaryOperator op;
            switch (b->op) {
            case BinaryOperator::Add: op = IRBinaryOperator::Add; break;
            case BinaryOperator::Sub: op = IRBinaryOperator::Sub; break;
            case BinaryOperator::Mul: op = IRBinaryOperator::Mul; break;
            case BinaryOperator::Div: op = IRBinaryOperator::Div; break;
            case BinaryOperator::Mod: op = IRBinaryOperator::Mod; break;
            case BinaryOperator::Equal: op = IRBinaryOperator::Eq; break;
            case BinaryOperator::NotEqual: op = IRBinaryOperator::Ne; break;
            case BinaryOperator::LessThan: op = IRBinaryOperator::Lt; break;
            case BinaryOperator::LessOrEqual: op = IRBinaryOperator::Le; break;
            case BinaryOperator::GreaterThan: op = IRBinaryOperator::Gt; break;
            case BinaryOperator::GreaterOrEqual: op = IRBinaryOperator::Ge; break;
            default: throw std::runtime_error("Unsupported binary operator");
            }
            instructions.push_back(std::make_unique<IRBinary>(op, std::move(rightVal), std::move(dstVarRef)));
            return std::make_unique<IRPseudo>(tmpName);
        }
        return std::make_unique<IRImm>(0);
    }
}
std::unique_ptr<IRProgram> Lowering::toIR(const Program& program) {
    const Function& func = *program.function;
    std::vector<std::unique_ptr<IRInstruction>> body;
    std::unordered_set<std::string> pseudos;
    if (auto ret = dynamic_cast<const Return*>(func.body.get())) {
        auto retVal = emitTacky(*ret->expr, body, pseudos);
        body.push_back(std::make_unique<IRMov>(std::move(retVal), std::make_unique<IRReg>(IRRegister::AX)));
        body.push_back(std::make_unique<IRRet>());
    }
    else {
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
