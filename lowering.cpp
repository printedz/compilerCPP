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
}

std::unique_ptr<IRProgram> Lowering::toIR(const Program& program) {
    // Assume exactly one function per high-level Program, as per existing AST.
    const Function& func = *program.function;

    std::vector<std::unique_ptr<IRInstruction>> body;

    // Lower the Return statement body. The existing grammar guarantees a single Return.
    if (auto ret = dynamic_cast<const Return*>(func.body.get())) {
        // Lower the expression to a value in a temp variable if needed.
        // We produce a sequence where unary operations store into temporaries.
        // Finally, we emit IRReturn of the final value.

        // Recursive lambda to lower Exp to IRVal, emitting instructions as needed.
        std::function<std::unique_ptr<IRVal>(const Exp&)> lowerExp = [&](const Exp& e) -> std::unique_ptr<IRVal> {
            if (auto c = dynamic_cast<const Constant*>(&e)) {
                return std::make_unique<IRConstant>(c->value);
            }
            if (auto u = dynamic_cast<const Unary*>(&e)) {
                // Lower inner expression to a source value
                auto srcVal = lowerExp(*u->expr);
                // Create a destination temp variable
                std::string tmpName = freshTempName();
                auto dstVar = std::make_unique<IRVar>(tmpName);
                auto dstVarRef = std::make_unique<IRVar>(tmpName);
                // Map operator
                IRUnaryOperator op = (u->op == UnaryOperator::Complement) ? IRUnaryOperator::Complement : IRUnaryOperator::Negate;
                body.push_back(std::make_unique<IRUnary>(op, std::move(srcVal), std::move(dstVar)));
                return dstVarRef;
            }
            // Fallback: constant 0
            return std::make_unique<IRConstant>(0);
        };

        auto retVal = lowerExp(*ret->expr);
        body.push_back(std::make_unique<IRReturn>(std::move(retVal)));
    } else {
        // If no return, default return 0
        body.push_back(std::make_unique<IRReturn>(std::make_unique<IRConstant>(0)));
    }

    auto irFunc = std::make_unique<IRFunction>(func.name, std::move(body));
    return std::make_unique<IRProgram>(std::move(irFunc));
}

