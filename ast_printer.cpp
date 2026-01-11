#include "ast_printer.h"
#include <sstream>

static std::string indentStr(int indent) {
    std::string s;
    for (int i = 0; i < indent; ++i) s += "  ";
    return s;
}

std::string ASTPrinter::print(const Program& program) {
    std::stringstream ss;
    ss << "Program(\n" << print(*program.function, 1) << "\n)";
    return ss.str();
}

std::string ASTPrinter::print(const Function& function, int indent) {
    std::stringstream ss;
    std::string ind = indentStr(indent);
    ss << ind << "Function(\n";
    ss << ind << "  name=\"" << function.name << "\",\n";
    ss << ind << "  body=\n" << print(*function.body, indent + 2) << "\n";
    ss << ind << ")";
    return ss.str();
}

std::string ASTPrinter::print(const Statement& statement, int indent) {
    if (auto* ret = dynamic_cast<const Return*>(&statement)) {
        std::stringstream ss;
        std::string ind = indentStr(indent);
        ss << ind << "Return(\n" << print(*ret->expr, indent + 1) << "\n" << ind << ")";
        return ss.str();
    }
    return indentStr(indent) + "<UnknownStatement>";
}

std::string ASTPrinter::print(const Exp& exp, int indent) {
    if (auto* c = dynamic_cast<const Constant*>(&exp)) {
        return indentStr(indent) + "Constant(" + std::to_string(c->value) + ")";
    }
    if (auto* u = dynamic_cast<const Unary*>(&exp)) {
        std::stringstream ss;
        std::string ind = indentStr(indent);
        ss << ind << "Unary(";
        ss << (u->op == UnaryOperator::Complement ? "Complement" : "Negate");
        ss << ",\n" << print(*u->expr, indent + 1) << "\n" << ind << ")";
        return ss.str();
    }
    return indentStr(indent) + "<UnknownExp>";
}
