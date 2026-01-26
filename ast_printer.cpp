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
    ss << ind << "  body=\n" << print(*function.body, indent + 1) << "\n";
    ss << ind << ")";
    return ss.str();
}
// Print a block
std::string ASTPrinter::print(const Block& block, int indent) {
    std::ostringstream ss;
    std::string ind = indentStr(indent);
    ss << ind << "Block(\n";
    ss << ind << "  items=[\n";
    for (size_t i = 0; i < block.items.size(); ++i) {
        ss << print(*block.items[i], indent + 2);
        if (i + 1 < block.items.size()) {
            ss << ",";
        }
        ss << "\n";
    }
    ss << ind << "  ]\n";
    ss << ind << ")";
    return ss.str();
}
// Print a block item
std::string ASTPrinter::print(const BlockItem& item, int indent) {
    if (auto* decl = dynamic_cast<const Declaration*>(&item)) {
        std::ostringstream ss;
        std::string ind = indentStr(indent);
        ss << ind << "Declaration(\n";
        ss << ind << "  name=\"" << decl->name << "\"";
        if (decl->init) {
            ss << ",\n" << ind << "  init=\n";
            ss << print(*decl->init, indent + 2) << "\n" << ind << ")";
        } else {
            ss << "\n" << ind << ")";
        }
        return ss.str();
    }
    if (auto* td = dynamic_cast<const Typedef*>(&item)) {
        std::ostringstream ss;
        std::string ind = indentStr(indent);
        ss << ind << "Typedef(\n";
        ss << ind << "  name=\"" << td->name << "\",\n";
        ss << ind << "  baseType=\"" << td->baseType << "\"\n";
        ss << ind << ")";
        return ss.str();
    }
    if (auto* stmt = dynamic_cast<const Statement*>(&item)) {
        return print(*stmt, indent);
    }
    return indentStr(indent) + "<UnknownBlockItem>";
}
// Print a statement
std::string ASTPrinter::print(const Statement& statement, int indent) {
    if (auto* ret = dynamic_cast<const Return*>(&statement)) {
        std::ostringstream ss;
        std::string ind = indentStr(indent);
        ss << ind << "Return(\n" << print(*ret->expr, indent + 1) << "\n" << ind << ")";
        return ss.str();
    }
    if (auto* exprStmt = dynamic_cast<const ExpressionStatement*>(&statement)) {
        std::ostringstream ss;
        std::string ind = indentStr(indent);
        ss << ind << "ExpressionStatement(\n";
        ss << print(*exprStmt->expr, indent + 1) << "\n" << ind << ")";
        return ss.str();
    }
    if (auto* ifStmt = dynamic_cast<const IfStatement*>(&statement)) {
        std::ostringstream ss;
        std::string ind = indentStr(indent);
        ss << ind << "IfStatement(\n";
        ss << ind << "  condition=\n" << print(*ifStmt->condition, indent + 2) << ",\n";
        ss << ind << "  then=\n" << print(*ifStmt->thenStmt, indent + 2);
        if (ifStmt->elseStmt) {
            ss << ",\n" << ind << "  else=\n" << print(*ifStmt->elseStmt, indent + 2);
        }
        ss << "\n" << ind << ")";
        return ss.str();
    }
    if (dynamic_cast<const EmptyStatement*>(&statement)) {
        return indentStr(indent) + "EmptyStatement()";
    }
    if (dynamic_cast<const BreakStatement*>(&statement)) {
        return indentStr(indent) + "Break()";
    }
    if (dynamic_cast<const ContinueStatement*>(&statement)) {
        return indentStr(indent) + "Continue()";
    }
    if (auto* whileStmt = dynamic_cast<const WhileStatement*>(&statement)) {
        std::ostringstream ss;
        std::string ind = indentStr(indent);
        ss << ind << "While(\n";
        ss << ind << "  condition=\n" << print(*whileStmt->condition, indent + 2) << ",\n";
        ss << ind << "  body=\n" << print(*whileStmt->body, indent + 2) << "\n";
        ss << ind << ")";
        return ss.str();
    }
    if (auto* doWhile = dynamic_cast<const DoWhileStatement*>(&statement)) {
        std::ostringstream ss;
        std::string ind = indentStr(indent);
        ss << ind << "DoWhile(\n";
        ss << ind << "  body=\n" << print(*doWhile->body, indent + 2) << ",\n";
        ss << ind << "  condition=\n" << print(*doWhile->condition, indent + 2) << "\n";
        ss << ind << ")";
        return ss.str();
    }
    if (auto* forStmt = dynamic_cast<const ForStatement*>(&statement)) {
        std::ostringstream ss;
        std::string ind = indentStr(indent);
        ss << ind << "For(\n";
        ss << ind << "  init=";
        if (auto* d = dynamic_cast<const InitDecl*>(forStmt->init.get())) {
            ss << "\n" << print(*d->decl, indent + 2);
        } else if (auto* e = dynamic_cast<const InitExp*>(forStmt->init.get())) {
            if (e->expr) {
                ss << "\n" << print(*e->expr, indent + 2);
            } else {
                ss << "null";
            }
        }
        ss << ",\n";
        ss << ind << "  condition=";
        if (forStmt->condition) {
            ss << "\n" << print(*forStmt->condition, indent + 2);
        } else {
            ss << "null";
        }
        ss << ",\n";
        ss << ind << "  post=";
        if (forStmt->post) {
            ss << "\n" << print(*forStmt->post, indent + 2);
        } else {
            ss << "null";
        }
        ss << ",\n";
        ss << ind << "  body=\n" << print(*forStmt->body, indent + 2) << "\n";
        ss << ind << ")";
        return ss.str();
    }
    if (auto* compound = dynamic_cast<const CompoundStatement*>(&statement)) {
        std::ostringstream ss;
        std::string ind = indentStr(indent);
        ss << ind << "Compound(\n";
        ss << print(*compound->block, indent + 1) << "\n";
        ss << ind << ")";
        return ss.str();
    }
    return indentStr(indent) + "<UnknownStatement>";
}
// Print an expression
std::string ASTPrinter::print(const Exp& exp, int indent) {
    if (auto* c = dynamic_cast<const Constant*>(&exp)) {
        return indentStr(indent) + "Constant(" + std::to_string(c->value) + ")";
    }
    if (auto* v = dynamic_cast<const Var*>(&exp)) {
        return indentStr(indent) + "Var(\"" + v->name + "\")";
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
    if (auto* a = dynamic_cast<const Assignment*>(&exp)) {
        std::ostringstream ss;
        std::string ind = indentStr(indent);
        ss << ind << "Assignment(\n";
        ss << print(*a->lhs, indent + 1) << ",\n";
        ss << print(*a->rhs, indent + 1) << "\n";
        ss << ind << ")";
        return ss.str();
    }
    if (auto* c = dynamic_cast<const Conditional*>(&exp)) {
        std::ostringstream ss;
        std::string ind = indentStr(indent);
        ss << ind << "Conditional(\n";
        ss << print(*c->condition, indent + 1) << ",\n";
        ss << print(*c->thenExpr, indent + 1) << ",\n";
        ss << print(*c->elseExpr, indent + 1) << "\n";
        ss << ind << ")";
        return ss.str();
    }
    return indentStr(indent) + "<UnknownExp>";
}
