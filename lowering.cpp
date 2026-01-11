#include <memory>
#include <vector>
#include <string>
#include <functional>
#include "lowering.h"

// Helper to generate fresh temporary variable names: t0, t1, ...
namespace {
    static std::string freshTempName() {
        static int counter = 0;
        return "t" + std::to_string(counter++);
    }

    // Lower an Exp to an IRVal, appending IR instructions as needed.
    static std::unique_ptr<IRVal> emitTacky(const Exp& e, std::vector<std::unique_ptr<IRInstruction>>& instructions) {
        if (auto c = dynamic_cast<const Constant*>(&e)) {
            return std::make_unique<IRConstant>(c->value);
        }
        if (auto u = dynamic_cast<const Unary*>(&e)) {
            // src = emit_tacky(inner, instructions)
            auto srcVal = emitTacky(*u->expr, instructions);
            // dst_name = make_temporary(); dst = Var(dst_name)
            std::string tmpName = freshTempName();
            auto dstVar = std::make_unique<IRVar>(tmpName);
            auto dstVarRef = std::make_unique<IRVar>(tmpName);
            // tacky_op = convert_unop(op)
            IRUnaryOperator op = (u->op == UnaryOperator::Complement) ? IRUnaryOperator::Complement : IRUnaryOperator::Negate;
            // instructions.append(Unary(tacky_op, src, dst))
            instructions.push_back(std::make_unique<IRUnary>(op, std::move(srcVal), std::move(dstVar)));
            // return dst
            return dstVarRef;
        }
        // Fallback: constant 0 for unsupported expressions
        return std::make_unique<IRConstant>(0);
    }
}

std::unique_ptr<IRProgram> Lowering::toIR(const Program& program) {
    // Assume exactly one function per high-level Program, as per existing AST.
    const Function& func = *program.function;

    std::vector<std::unique_ptr<IRInstruction>> body;

    // Lower the Return statement body. The existing grammar guarantees a single Return.
    if (auto ret = dynamic_cast<const Return*>(func.body.get())) {
        // Lower expression to IR, emitting instructions into body
        auto retVal = emitTacky(*ret->expr, body);
        body.push_back(std::make_unique<IRReturn>(std::move(retVal)));
    } else {
        // If no return, default return 0
        body.push_back(std::make_unique<IRReturn>(std::make_unique<IRConstant>(0)));
    }

    auto irFunc = std::make_unique<IRFunction>(func.name, std::move(body));
    return std::make_unique<IRProgram>(std::move(irFunc));
}

