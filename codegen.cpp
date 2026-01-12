#include "codegen.h"
#include "lowering.h"
#include <sstream>
#include <unordered_map>

#if defined(__APPLE__)
    #define IS_MAC 1
#else
    #define IS_MAC 0
#endif

static std::string mangleFuncName(const std::string& name) {
    if (IS_MAC) return "_" + name; else return name;
}

std::string CodeGenerator::generate(const IRProgram& program) {
    return genFunctionIR(*program.function);
}

std::string CodeGenerator::generate(const Program& program) {
    auto ir = Lowering::toIR(program);
    return generate(*ir);
}

static const char* regToAsm32(IRRegister reg) {
    switch (reg) {
        case IRRegister::AX: return "%eax";
        case IRRegister::R10: return "%r10d";
    }
    return "%eax";
}

static std::string formatOperand(
    const IROperand& op,
    const std::unordered_map<std::string, int>& pseudoOffsets) {
    if (auto* imm = dynamic_cast<const IRImm*>(&op)) {
        return "$" + std::to_string(imm->value);
    }
    if (auto* reg = dynamic_cast<const IRReg*>(&op)) {
        return regToAsm32(reg->reg);
    }
    if (auto* pseudo = dynamic_cast<const IRPseudo*>(&op)) {
        auto it = pseudoOffsets.find(pseudo->name);
        int offset = (it == pseudoOffsets.end()) ? 0 : it->second;
        return std::to_string(offset) + "(%rbp)";
    }
    if (auto* stack = dynamic_cast<const IRStack*>(&op)) {
        return std::to_string(stack->offset) + "(%rbp)";
    }
    return "0(%rbp)";
}

static bool isMemoryOperand(const IROperand& op) {
    return dynamic_cast<const IRPseudo*>(&op) != nullptr
        || dynamic_cast<const IRStack*>(&op) != nullptr;
}

static bool isImmediateOperand(const IROperand& op) {
    return dynamic_cast<const IRImm*>(&op) != nullptr;
}

static bool isRegAX(const IROperand& op) {
    auto* reg = dynamic_cast<const IRReg*>(&op);
    return reg && reg->reg == IRRegister::AX;
}

std::string CodeGenerator::genFunctionIR(const IRFunction& func) {
    std::stringstream ss;
    std::string funcName = mangleFuncName(func.name);

    // First pass: collect pseudos to allocate stack slots.
    // We assign each unique pseudo name a 4-byte slot at negative offsets from %rbp.
    std::unordered_map<std::string, int> pseudoOffsets; // name -> offset
    int nextOffset = -4; // start at -4(%rbp), grow negatively
    auto ensurePseudo = [&](const IROperand* op) {
        if (auto* p = dynamic_cast<const IRPseudo*>(op)) {
            if (!pseudoOffsets.count(p->name)) {
                pseudoOffsets[p->name] = nextOffset;
                nextOffset -= 4;
            }
        }
    };

    bool hasAllocate = false;
    for (const auto& instPtr : func.body) {
        if (auto* m = dynamic_cast<const IRMov*>(instPtr.get())) {
            ensurePseudo(m->src.get());
            ensurePseudo(m->dst.get());
        } else if (auto* u = dynamic_cast<const IRUnary*>(instPtr.get())) {
            ensurePseudo(u->operand.get());
        } else if (auto* b = dynamic_cast<const IRBinary*>(instPtr.get())) {
            ensurePseudo(b->src.get());
            ensurePseudo(b->dst.get());
        } else if (dynamic_cast<const IRAllocateStack*>(instPtr.get())) {
            hasAllocate = true;
        }
    }

    int frameSize = -nextOffset - 4;
    if (frameSize < 0) frameSize = 0;
    if (frameSize % 16 != 0) {
        frameSize += (16 - (frameSize % 16));
    }

    ss << "    .text\n";
    ss << "    .globl " << funcName << "\n";
    ss << funcName << ":\n";

    // Prologue
    ss << "    pushq %rbp\n";
    ss << "    movq %rsp, %rbp\n";
    if (!hasAllocate && frameSize > 0) {
        ss << "    subq $" << frameSize << ", %rsp\n";
    }

    // Body
    for (const auto& instPtr : func.body) {
        if (auto* m = dynamic_cast<const IRMov*>(instPtr.get())) {
            std::string src = formatOperand(*m->src, pseudoOffsets);
            std::string dst = formatOperand(*m->dst, pseudoOffsets);
            if (isMemoryOperand(*m->src) && isMemoryOperand(*m->dst)) {
                ss << "    movl " << src << ", %r10d\n";
                ss << "    movl %r10d, " << dst << "\n";
            } else {
                ss << "    movl " << src << ", " << dst << "\n";
            }
        } else if (auto* u = dynamic_cast<const IRUnary*>(instPtr.get())) {
            const char* op = (u->op == IRUnaryOperator::Neg) ? "negl" : "notl";
            ss << "    " << op << " " << formatOperand(*u->operand, pseudoOffsets) << "\n";
        } else if (auto* b = dynamic_cast<const IRBinary*>(instPtr.get())) {
            if (b->op == IRBinaryOperator::Div || b->op == IRBinaryOperator::Mod) {
                std::string dst = formatOperand(*b->dst, pseudoOffsets);
                std::string src = formatOperand(*b->src, pseudoOffsets);
                ss << "    movl " << dst << ", %eax\n";
                ss << "    cdq\n";
                if (isImmediateOperand(*b->src)) {
                    ss << "    movl " << src << ", %r10d\n";
                    ss << "    idivl %r10d\n";
                } else {
                    ss << "    idivl " << src << "\n";
                }
                if (b->op == IRBinaryOperator::Div) {
                    if (!isRegAX(*b->dst)) {
                        ss << "    movl %eax, " << dst << "\n";
                    }
                } else {
                    ss << "    movl %edx, " << dst << "\n";
                }
            } else {
                const char* op = "addl";
                switch (b->op) {
                    case IRBinaryOperator::Add: op = "addl"; break;
                    case IRBinaryOperator::Sub: op = "subl"; break;
                    case IRBinaryOperator::Mul: op = "imull"; break;
                    case IRBinaryOperator::Div: break;
                    case IRBinaryOperator::Mod: break;
                }
                std::string src = formatOperand(*b->src, pseudoOffsets);
                std::string dst = formatOperand(*b->dst, pseudoOffsets);
                if (b->op == IRBinaryOperator::Mul &&
                    isImmediateOperand(*b->src) && isMemoryOperand(*b->dst)) {
                    // imull imm, mem is invalid in AT&T syntax; use a temp reg.
                    ss << "    movl " << dst << ", %r10d\n";
                    ss << "    imull " << src << ", %r10d\n";
                    ss << "    movl %r10d, " << dst << "\n";
                } else if (isMemoryOperand(*b->src) && isMemoryOperand(*b->dst)) {
                    ss << "    movl " << src << ", %r10d\n";
                    ss << "    " << op << " %r10d, " << dst << "\n";
                } else {
                    ss << "    " << op << " " << src << ", " << dst << "\n";
                }
            }
        } else if (auto* a = dynamic_cast<const IRAllocateStack*>(instPtr.get())) {
            if (a->amount > 0) {
                int amount = a->amount;
                if (frameSize > amount) {
                    amount = frameSize;
                }
                ss << "    subq $" << amount << ", %rsp\n";
            }
        } else if (dynamic_cast<const IRRet*>(instPtr.get())) {
            ss << "    movq %rbp, %rsp\n";
            ss << "    popq %rbp\n";
            ss << "    ret\n";
#if !IS_MAC
            ss << ".section .note.GNU-stack,\"\",@progbits\n";
#endif
            return ss.str();
        }
    }

    // If no explicit return, default to 0
    ss << "    movl $0, %eax\n";
    ss << "    movq %rbp, %rsp\n";
    ss << "    popq %rbp\n";
    ss << "    ret\n";
#if !IS_MAC
    ss << ".section .note.GNU-stack,\"\",@progbits\n";
#endif

    return ss.str();
}
