#include <memory>
#include <vector>
#include <string>
#include <functional>
#include <unordered_set>
#include "lowering.h"

// Helper to generate fresh temporary variable names: t0, t1, ...
namespace {
    static std::string freshTempName() {
        static int counter = 0;
        return "t" + std::to_string(counter++);
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
            // src = emit(inner)
            auto srcVal = emitTacky(*u->expr, instructions, pseudos);
            // dst_name = make_temporary(); dst = Pseudo(dst_name)
            std::string tmpName = freshTempName();
            pseudos.insert(tmpName);
            auto dstVar = std::make_unique<IRPseudo>(tmpName);
            auto dstVarRef = std::make_unique<IRPseudo>(tmpName);
            // mov src, dst
            instructions.push_back(std::make_unique<IRMov>(std::move(srcVal), std::move(dstVar)));
            // unary op in-place on dst
            IRUnaryOperator op;
            switch (u->op) {
                case UnaryOperator::Complement: op = IRUnaryOperator::Not; break;
                case UnaryOperator::Negate: op = IRUnaryOperator::Neg; break;
                case UnaryOperator::LogicalNot: op = IRUnaryOperator::LogicalNot; break;
            }
            instructions.push_back(std::make_unique<IRUnary>(op, std::move(dstVarRef)));
            // return dst (fresh operand)
            return std::make_unique<IRPseudo>(tmpName);
        }
        if (auto b = dynamic_cast<const Binary*>(&e)) {
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
            }
            instructions.push_back(std::make_unique<IRBinary>(op, std::move(rightVal), std::move(dstVarRef)));
            return std::make_unique<IRPseudo>(tmpName);
        }
        // Fallback: constant 0 for unsupported expressions
        return std::make_unique<IRImm>(0);
    }
}

std::unique_ptr<IRProgram> Lowering::toIR(const Program& program) {
    // Assume exactly one function per high-level Program, as per existing AST.
    const Function& func = *program.function;

    std::vector<std::unique_ptr<IRInstruction>> body;
    std::unordered_set<std::string> pseudos;

    // Lower the Return statement body. The existing grammar guarantees a single Return.
    if (auto ret = dynamic_cast<const Return*>(func.body.get())) {
        // Lower expression to IR, emitting instructions into body
        auto retVal = emitTacky(*ret->expr, body, pseudos);
        body.push_back(std::make_unique<IRMov>(std::move(retVal), std::make_unique<IRReg>(IRRegister::AX)));
        body.push_back(std::make_unique<IRRet>());
    } else {
        // If no return, default return 0
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
