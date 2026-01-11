#include "codegen.h"
#include <sstream>

#if defined(__APPLE__)
    #define IS_MAC 1
#else
    #define IS_MAC 0
#endif

static std::string mangleFuncName(const std::string& name) {
    if (IS_MAC) return "_" + name; else return name;
}

std::string CodeGenerator::generate(const Program& program) {
    return genFunction(*program.function);
}

std::string CodeGenerator::genFunction(const Function& func) {
    std::stringstream ss;
    std::string funcName = mangleFuncName(func.name);

    ss << "    .text\n";
    ss << "    .globl " << funcName << "\n";
    ss << funcName << ":\n";

    // Prologue
    ss << "    pushq %rbp\n";
    ss << "    movq %rsp, %rbp\n";

    // Evaluate return expression into %eax
    std::string exprAsm;
    // func.body must be Return
    if (auto* ret = dynamic_cast<const Return*>(func.body.get())) {
        evalExpToEAX(*ret->expr, exprAsm);
        ss << exprAsm;
    } else {
        // Should not happen with current grammar; default to 0
        ss << "    movl $0, %eax\n";
    }

    // Epilogue and return
    ss << "    movq %rbp, %rsp\n";
    ss << "    popq %rbp\n";
    ss << "    ret\n";

#if !IS_MAC
    ss << ".section .note.GNU-stack,\"\",@progbits\n";
#endif

    return ss.str();
}

int CodeGenerator::evalExpToEAX(const Exp& exp, std::string& outAsm) {
    if (auto* c = dynamic_cast<const Constant*>(&exp)) {
        outAsm += "    movl $" + std::to_string(c->value) + ", %eax\n";
        return 0;
    }
    if (auto* u = dynamic_cast<const Unary*>(&exp)) {
        // Evaluate inner to %eax first
        evalExpToEAX(*u->expr, outAsm);
        switch (u->op) {
            case UnaryOperator::Complement:
                outAsm += "    notl %eax\n"; // bitwise not
                break;
            case UnaryOperator::Negate:
                outAsm += "    negl %eax\n"; // arithmetic negation
                break;
        }
        return 0;
    }
    // Fallback: set 0
    outAsm += "    movl $0, %eax\n";
    return 0;
}
