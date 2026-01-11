#include "codegen.h"
#include <sstream>

#if defined(__APPLE__)
    #define IS_MAC 1
#else
    #define IS_MAC 0
#endif

std::string CodeGenerator::generate(const Program& program) {
    std::stringstream ss;
    const Function& func = *program.function;

    std::string funcName = func.name;
    if (IS_MAC) {
        funcName = "_" + funcName;
    }

    ss << "    .text\n";
    ss << "    .globl " << funcName << "\n";
    ss << funcName << ":\n";

    ss << "    pushq %rbp\n";
    ss << "    movq %rsp, %rbp\n";
    ss << "    subq $0, %rsp\n";

    for (const auto& instr : func.instructions) {
        if (auto* mov = dynamic_cast<const Mov*>(instr.get())) {
            ss << "    movl " << formatOperand(mov->src.get())
               << ", " << formatOperand(mov->dst.get()) << "\n";
        } else if (dynamic_cast<const Ret*>(instr.get())) {
            ss << "    movq %rbp, %rsp\n";
            ss << "    popq %rbp\n";
            ss << "    ret\n";
        }
    }

    if (!IS_MAC) {
        ss << ".section .note.GNU-stack,\"\",@progbits\n";
    }

    return ss.str();
}

std::string CodeGenerator::formatOperand(const Operand* op) {
    if (auto* imm = dynamic_cast<const Imm*>(op)) {
        return "$" + std::to_string(imm->value);
    } else if (dynamic_cast<const Register*>(op)) {
        return "%eax";
    }
    return "UnknownOperand";
}
