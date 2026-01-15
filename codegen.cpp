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
        case IRRegister::DX: return "%edx";
        case IRRegister::R10: return "%r10d";
        case IRRegister::R11: return "%r11d";
    }
    return "%eax";
}

static const char* regToAsm8(IRRegister reg) {
    switch (reg) {
        case IRRegister::AX: return "%al";
        case IRRegister::DX: return "%dl";
        case IRRegister::R10: return "%r10b";
        case IRRegister::R11: return "%r11b";
    }
    return "%al";
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

static std::string formatLabel(const std::string& label) {
    if (label.rfind(".L", 0) == 0) {
        return label;
    }
    return ".L" + label;
}

static bool isMemoryOperand(const IROperand& op) {
    return dynamic_cast<const IRPseudo*>(&op) != nullptr
        || dynamic_cast<const IRStack*>(&op) != nullptr;
}

static bool isImmediateOperand(const IROperand& op) {
    return dynamic_cast<const IRImm*>(&op) != nullptr;
}

static const char* condToSuffix(IRCondCode cond) {
    switch (cond) {
        case IRCondCode::E: return "e";
        case IRCondCode::NE: return "ne";
        case IRCondCode::G: return "g";
        case IRCondCode::GE: return "ge";
        case IRCondCode::L: return "l";
        case IRCondCode::LE: return "le";
    }
    return "e";
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
        } else if (auto* c = dynamic_cast<const IRCmp*>(instPtr.get())) {
            ensurePseudo(c->src.get());
            ensurePseudo(c->dst.get());
        } else if (auto* d = dynamic_cast<const IRIdiv*>(instPtr.get())) {
            ensurePseudo(d->divisor.get());
        } else if (auto* s = dynamic_cast<const IRSetCC*>(instPtr.get())) {
            ensurePseudo(s->dst.get());
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
            const char* op = "addl";
            switch (b->op) {
                case IRBinaryOperator::Add: op = "addl"; break;
                case IRBinaryOperator::Sub: op = "subl"; break;
                case IRBinaryOperator::Mul: op = "imull"; break;
            }
            std::string src = formatOperand(*b->src, pseudoOffsets);
            std::string dst = formatOperand(*b->dst, pseudoOffsets);
            if (b->op == IRBinaryOperator::Mul) {
                if (isMemoryOperand(*b->dst)) {
                    // Always load destination from memory into temp, multiply, store back
                    ss << "    movl " << dst << ", %r10d\n";
                    if (isImmediateOperand(*b->src) || isMemoryOperand(*b->src)) {
                        // Ensure src is in a register if it's imm or memory
                        ss << "    movl " << src << ", %eax\n";
                        ss << "    imull %eax, %r10d\n";
                    } else {
                        // src is already a register
                        ss << "    imull " << src << ", %r10d\n";
                    }
                    ss << "    movl %r10d, " << dst << "\n";
                } else {
                    // dst is a register; we can use imull src, dst directly
                    if (isMemoryOperand(*b->src) && isMemoryOperand(*b->dst)) {
                        // unreachable due to dst not memory, but keep structure consistent
                        ss << "    movl " << src << ", %r10d\n";
                        ss << "    imull %r10d, " << dst << "\n";
                    } else if (isImmediateOperand(*b->src) || isMemoryOperand(*b->src)) {
                        // Some assemblers accept imull imm, reg; GAS does. Use it directly.
                        ss << "    imull " << src << ", " << dst << "\n";
                    } else {
                        // src is a register; direct form
                        ss << "    imull " << src << ", " << dst << "\n";
                    }
                }
            } else if (isMemoryOperand(*b->src) && isMemoryOperand(*b->dst)) {
                ss << "    movl " << src << ", %r10d\n";
                ss << "    " << op << " %r10d, " << dst << "\n";
            } else {
                ss << "    " << op << " " << src << ", " << dst << "\n";
            }
        } else if (auto* c = dynamic_cast<const IRCmp*>(instPtr.get())) {
            std::string src = formatOperand(*c->src, pseudoOffsets);
            std::string dst = formatOperand(*c->dst, pseudoOffsets);
            if (isImmediateOperand(*c->dst)) {
                ss << "    movl " << dst << ", %r11d\n";
                ss << "    cmpl " << src << ", %r11d\n";
            } else if (isMemoryOperand(*c->src) && isMemoryOperand(*c->dst)) {
                ss << "    movl " << src << ", %r10d\n";
                ss << "    cmpl %r10d, " << dst << "\n";
            } else {
                ss << "    cmpl " << src << ", " << dst << "\n";
            }
        } else if (auto* d = dynamic_cast<const IRIdiv*>(instPtr.get())) {
            std::string divisor = formatOperand(*d->divisor, pseudoOffsets);
            if (isImmediateOperand(*d->divisor)) {
                ss << "    movl " << divisor << ", %r10d\n";
                ss << "    idivl %r10d\n";
            } else {
                ss << "    idivl " << divisor << "\n";
            }
        } else if (dynamic_cast<const IRCdq*>(instPtr.get())) {
            ss << "    cdq\n";
        } else if (auto* j = dynamic_cast<const IRJump*>(instPtr.get())) {
            ss << "    jmp " << formatLabel(j->target) << "\n";
        } else if (auto* j = dynamic_cast<const IRJumpCC*>(instPtr.get())) {
            ss << "    j" << condToSuffix(j->cond) << " " << formatLabel(j->target) << "\n";
        } else if (auto* s = dynamic_cast<const IRSetCC*>(instPtr.get())) {
            if (auto* reg = dynamic_cast<const IRReg*>(s->dst.get())) {
                ss << "    set" << condToSuffix(s->cond) << " " << regToAsm8(reg->reg) << "\n";
            } else {
                std::string dst = formatOperand(*s->dst, pseudoOffsets);
                ss << "    set" << condToSuffix(s->cond) << " " << dst << "\n";
            }
        } else if (auto* l = dynamic_cast<const IRLabel*>(instPtr.get())) {
            ss << formatLabel(l->name) << ":\n";
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
#if defined(__linux__)
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
#if defined(__linux__)
    ss << ".section .note.GNU-stack,\"\",@progbits\n";
#endif

    return ss.str();
}
