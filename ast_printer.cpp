#include "ast_printer.h"
#include "ast.h" // Include the header file defining BinaryOperator
#include <sstream>
#include <stdexcept>
// Helper function to generate indentation string
static std::string indentStr(int indent) {
    return std::string(indent * 2, ' '); // Each indent level adds two spaces
}
// Print the entire program
std::string ASTPrinter::print(const Program& program) {
    std::ostringstream ss;
    ss << "Program(\n" << print(*program.function, 1) << "\n)";
    return ss.str();
}
// Print a function
std::string ASTPrinter::print(const Function& function, int indent) {
    std::ostringstream ss;
    std::string ind = indentStr(indent);
    ss << ind << "Function(\n";
    ss << ind << "  name=\"" << function.name << "\",\n";
    ss << ind << "  body=\n" << print(*function.body, indent + 2) << "\n";
    ss << ind << ")";
    return ss.str();
}
// Print a statement
std::string ASTPrinter::print(const Statement& statement, int indent) {
    if (auto* ret = dynamic_cast<const Return*>(&statement)) {
        std::ostringstream ss;
        std::string ind = indentStr(indent);
        ss << ind << "Return(\n" << print(*ret->expr, indent + 1) << "\n" << ind << ")";
        return ss.str();
    }
    return indentStr(indent) + "<UnknownStatement>";
}
// Print an expression
std::string ASTPrinter::print(const Exp& exp, int indent) {
    if (auto* c = dynamic_cast<const Constant*>(&exp)) {
        return indentStr(indent) + "Constant(" + std::to_string(c->value) + ")";
    }
    if (auto* u = dynamic_cast<const Unary*>(&exp)) {
        std::ostringstream ss;
        std::string ind = indentStr(indent);
        ss << ind << "Unary(";
        const char* opStr = nullptr;
        switch (u->op) {
        case UnaryOperator::Complement: opStr = "Complement"; break;
        case UnaryOperator::Negate: opStr = "Negate"; break;
        case UnaryOperator::LogicalNot: opStr = "LogicalNot"; break;
        default: throw std::runtime_error("Unknown Unary Operator");
        }
        ss << opStr;
        ss << ",\n" << print(*u->expr, indent + 1) << "\n" << ind << ")";
        return ss.str();
    }
    if (auto* b = dynamic_cast<const Binary*>(&exp)) {
        std::ostringstream ss;
        std::string ind = indentStr(indent);
        const char* opStr = nullptr;
        switch (b->op) {
        case BinaryOperator::Add: opStr = "Add"; break;
        case BinaryOperator::Sub: opStr = "Sub"; break;
        case BinaryOperator::Mul: opStr = "Mul"; break;
        case BinaryOperator::Div: opStr = "Div"; break; // Ensure Div is handled
        case BinaryOperator::Mod: opStr = "Mod"; break;
        case BinaryOperator::And: opStr = "And"; break;
        case BinaryOperator::Or: opStr = "Or"; break;
        case BinaryOperator::Equal: opStr = "Equal"; break;
        case BinaryOperator::NotEqual: opStr = "NotEqual"; break;
        case BinaryOperator::LessThan: opStr = "LessThan"; break;
        case BinaryOperator::LessOrEqual: opStr = "LessOrEqual"; break;
        case BinaryOperator::GreaterThan: opStr = "GreaterThan"; break;
        case BinaryOperator::GreaterOrEqual: opStr = "GreaterOrEqual"; break;
        default: throw std::runtime_error("Unknown Binary Operator");
        }
        ss << ind << "Binary(" << opStr << ",\n";
        ss << print(*b->left, indent + 1) << ",\n";
        ss << print(*b->right, indent + 1) << "\n";
        ss << ind << ")";
        return ss.str();
    }
    return indentStr(indent) + "<UnknownExp>";
}
