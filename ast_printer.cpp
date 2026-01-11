#include "ast_printer.h"
#include <sstream>

std::string getIndentation(int indent) {
    std::string s;
    for (int i = 0; i < indent; ++i) s += "  ";
    return s;
}

std::string ASTPrinter::print(const Program& program) {
    return "Program(\n" + print(*program.function, 1) + "\n)";
}

std::string ASTPrinter::print(const Function& function, int indent) {
    std::stringstream ss;
    std::string indentation = getIndentation(indent);
    ss << indentation << "Function(\n";
    ss << indentation << "  name=\"" << function.name << "\",\n";
    ss << indentation << "  instructions=[\n";
    for (const auto& instr : function.instructions) {
        ss << print(*instr, indent + 2) << "\n";
    }
    ss << indentation << "  ]\n";
    ss << indentation << ")";
    return ss.str();
}

std::string ASTPrinter::print(const Instruction& instruction, int indent) {
    std::string indentation = getIndentation(indent);
    if (auto* mov = dynamic_cast<const Mov*>(&instruction)) {
        return indentation + "Mov(" + printOperand(*mov->src) + ", " + printOperand(*mov->dst) + ")";
    } else if (dynamic_cast<const Ret*>(&instruction)) {
        return indentation + "Ret";
    }
    return indentation + "UnknownInstruction";
}

std::string ASTPrinter::printOperand(const Operand& operand) {
    if (auto* imm = dynamic_cast<const Imm*>(&operand)) {
        return "Imm(" + std::to_string(imm->value) + ")";
    } else if (dynamic_cast<const Register*>(&operand)) {
        return "Register";
    }
    return "UnknownOperand";
}
