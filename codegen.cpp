#include "codegen.h"
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
static int loadValToEAX(const IRVal& val, std::string& outAsm, const std::unordered_map<std::string, int>& varOffsets) {
    if (auto* c = dynamic_cast<const IRConstant*>(&val)) {
        outAsm += "    movl $" + std::to_string(c->value) + ", %eax\n";
        return 0;
    }
    if (auto* v = dynamic_cast<const IRVar*>(&val)) {
        auto it = varOffsets.find(v->name);
        if (it == varOffsets.end()) {
            // Undefined variable; default to 0
            outAsm += "    movl $0, %eax\n";
            return 0;
        }
        int offset = it->second;
        outAsm += "    movl " + std::to_string(offset) + "(%rbp), %eax\n";
        return 0;
    }
    // Unknown value type
    outAsm += "    movl $0, %eax\n";
    return 0;
}

static void storeEAXToVar(const IRVar& var, std::string& outAsm, const std::unordered_map<std::string, int>& varOffsets) {
    auto it = varOffsets.find(var.name);
    if (it == varOffsets.end()) {
        // No storage allocated; ignore
        return;
    }
    int offset = it->second;
    outAsm += "    movl %eax, " + std::to_string(offset) + "(%rbp)\n";
}

std::string CodeGenerator::genFunctionIR(const IRFunction& func) {
    std::stringstream ss;
    std::string funcName = mangleFuncName(func.name);

    // First pass: collect variables from IRVar occurrences to allocate stack slots.
    // We assign each unique variable name a 4-byte slot at negative offsets from %rbp.
    std::unordered_map<std::string, int> varOffsets; // name -> offset
    int nextOffset = -4; // start at -4(%rbp), grow negatively
    auto ensureVar = [&](const IRVal* val) {
        if (auto* v = dynamic_cast<const IRVar*>(val)) {
            if (!varOffsets.count(v->name)) {
                varOffsets[v->name] = nextOffset;
                nextOffset -= 4;
            }
        }
    };

    for (const auto& instPtr : func.body) {
        if (auto* u = dynamic_cast<const IRUnary*>(instPtr.get())) {
            ensureVar(u->src.get());
            ensureVar(u->dst.get());
        } else if (auto* r = dynamic_cast<const IRReturn*>(instPtr.get())) {
            ensureVar(r->value.get());
        }
    }

    int frameSize = -nextOffset - 4; // round up to multiple of 16 later
    if (frameSize < 0) frameSize = 0;

    // Align stack frame size to 16 bytes for System V ABI where needed
    if (frameSize % 16 != 0) {
        frameSize += (16 - (frameSize % 16));
    }

    ss << "    .text\n";
    ss << "    .globl " << funcName << "\n";
    ss << funcName << ":\n";

    // Prologue
    ss << "    pushq %rbp\n";
    ss << "    movq %rsp, %rbp\n";
    if (frameSize > 0) {
        ss << "    subq $" << frameSize << ", %rsp\n";
    }

    // Body
    for (const auto& instPtr : func.body) {
        if (auto* u = dynamic_cast<const IRUnary*>(instPtr.get())) {
            // Load src to %eax
            std::string asmChunk;
            loadValToEAX(*u->src, asmChunk, varOffsets);
            // Apply op
            switch (u->op) {
                case IRUnaryOperator::Complement:
                    asmChunk += "    notl %eax\n";
                    break;
                case IRUnaryOperator::Negate:
                    asmChunk += "    negl %eax\n";
                    break;
            }
            // Store to dst (must be a variable)
            if (auto* dstVar = dynamic_cast<const IRVar*>(u->dst.get())) {
                storeEAXToVar(*dstVar, asmChunk, varOffsets);
            }
            ss << asmChunk;
        } else if (auto* r = dynamic_cast<const IRReturn*>(instPtr.get())) {
            std::string asmChunk;
            loadValToEAX(*r->value, asmChunk, varOffsets);
            ss << asmChunk;
            // Epilogue and return
            if (frameSize > 0) {
                ss << "    addq $" << frameSize << ", %rsp\n";
            }
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
    if (frameSize > 0) {
        ss << "    addq $" << frameSize << ", %rsp\n";
    }
    ss << "    movq %rbp, %rsp\n";
    ss << "    popq %rbp\n";
    ss << "    ret\n";
#if !IS_MAC
    ss << ".section .note.GNU-stack,\"\",@progbits\n";
#endif

    return ss.str();
}

