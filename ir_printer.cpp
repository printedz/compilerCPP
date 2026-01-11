#include <sstream>
#include "ir_printer.h"

namespace {
static std::string indentStr(int indent) { return std::string(indent * 2, ' '); }
}

std::string IRPrinter::print(const IRProgram& program) {
    std::ostringstream os;
    os << "Program(\n"
       << indentStr(1) << print(*program.function, 1) << "\n"
       << ")";
    return os.str();
}

std::string IRPrinter::print(const IRFunction& func, int indent) {
    std::ostringstream os;
    std::string ind = indentStr(indent);
    os << "Function(\n";
    os << ind << "  name=\"" << func.name << "\",\n";
    os << ind << "  body=[\n";
    for (size_t i = 0; i < func.body.size(); ++i) {
        os << indentStr(indent + 2) << print(*func.body[i], indent + 2);
        if (i + 1 < func.body.size()) {
            os << ",";
        }
        os << "\n";
    }
    os << ind << "  ]\n";
    os << ind << ")";
    return os.str();
}

std::string IRPrinter::print(const IRInstruction& instr, int /*indent*/) {
    std::ostringstream os;

    if (auto ret = dynamic_cast<const IRReturn*>(&instr)) {
        os << "Return(" << print(*ret->value, 0) << ")";
    } else if (auto unary = dynamic_cast<const IRUnary*>(&instr)) {
        const char* opName = (unary->op == IRUnaryOperator::Complement) ? "Complement" : "Negate";
        os << "Unary(" << opName
           << ", src=" << print(*unary->src, 0)
           << ", dst=" << print(*unary->dst, 0)
           << ")";
    } else {
        os << "<UnknownIRInstruction>";
    }

    return os.str();
}

std::string IRPrinter::print(const IRVal& val, int /*indent*/) {
    if (auto c = dynamic_cast<const IRConstant*>(&val)) {
        return "Constant(" + std::to_string(c->value) + ")";
    } else if (auto v = dynamic_cast<const IRVar*>(&val)) {
        return "Var(" + v->name + ")";
    } else {
        return "<UnknownIRVal>";
    }
}
