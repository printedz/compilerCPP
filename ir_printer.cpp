#include "ir_printer.h"

#include <iostream>
#include <sstream>

static const char* toString(IRUnaryOperator op) {
    switch (op) {
        case IRUnaryOperator::Neg: return "neg";
        case IRUnaryOperator::Not: return "not";
        case IRUnaryOperator::LogicalNot: return "lnot";
    }
    return "?";
}

static const char* toString(IRBinaryOperator op) {
    switch (op) {
        case IRBinaryOperator::Add: return "add";
        case IRBinaryOperator::Sub: return "sub";
        case IRBinaryOperator::Mul: return "mul";
        case IRBinaryOperator::Div: return "div";
        case IRBinaryOperator::Mod: return "mod";
        case IRBinaryOperator::Eq: return "eq";
        case IRBinaryOperator::Ne: return "ne";
        case IRBinaryOperator::Lt: return "lt";
        case IRBinaryOperator::Le: return "le";
        case IRBinaryOperator::Gt: return "gt";
        case IRBinaryOperator::Ge: return "ge";
    }
    return "?";
}

static const char* toString(IRRegister reg) {
    switch (reg) {
        case IRRegister::AX: return "%eax";
        case IRRegister::R10: return "%r10d";
    }
    return "%?";
}

std::string IRPrinter::print(const IRProgram& program) {
    std::ostringstream oss;
    IRPrinter printer(oss);
    printer.emit(program);
    return oss.str();
}

void IRPrinter::emit(const IRProgram& program) const {
    if (program.function) {
        emit(*program.function);
    } else {
        out << "<empty program>\n";
    }
}

void IRPrinter::emit(const IRFunction& fn) const {
    out << "func " << fn.name << "() {\n";
    for (const auto& inst : fn.body) {
        emit(*inst);
    }
    out << "}\n";
}

void IRPrinter::emit(const IRInstruction& inst) const {
    if (auto m = dynamic_cast<const IRMov*>(&inst)) {
        emit(*m);
        return;
    }
    if (auto u = dynamic_cast<const IRUnary*>(&inst)) {
        emit(*u);
        return;
    }
    if (auto b = dynamic_cast<const IRBinary*>(&inst)) {
        emit(*b);
        return;
    }
    if (auto j = dynamic_cast<const IRJumpIfZero*>(&inst)) {
        emit(*j);
        return;
    }
    if (auto j = dynamic_cast<const IRJumpIfNotZero*>(&inst)) {
        emit(*j);
        return;
    }
    if (auto j = dynamic_cast<const IRJump*>(&inst)) {
        emit(*j);
        return;
    }
    if (auto l = dynamic_cast<const IRLabel*>(&inst)) {
        emit(*l);
        return;
    }
    if (auto a = dynamic_cast<const IRAllocateStack*>(&inst)) {
        emit(*a);
        return;
    }
    if (auto r = dynamic_cast<const IRRet*>(&inst)) {
        emit(*r);
        return;
    }
    out << "  <unknown inst>\n";
}

void IRPrinter::emit(const IRMov& m) const {
    out << "  mov ";
    emit(*m.src);
    out << ", ";
    emit(*m.dst);
    out << "\n";
}

void IRPrinter::emit(const IRUnary& u) const {
    out << "  " << toString(u.op) << " ";
    emit(*u.operand);
    out << "\n";
}

void IRPrinter::emit(const IRBinary& b) const {
    out << "  " << toString(b.op) << " ";
    emit(*b.src);
    out << ", ";
    emit(*b.dst);
    out << "\n";
}

void IRPrinter::emit(const IRJumpIfZero& j) const {
    out << "  jz ";
    emit(*j.cond);
    out << ", " << j.target << "\n";
}

void IRPrinter::emit(const IRJumpIfNotZero& j) const {
    out << "  jnz ";
    emit(*j.cond);
    out << ", " << j.target << "\n";
}

void IRPrinter::emit(const IRJump& j) const {
    out << "  jmp " << j.target << "\n";
}

void IRPrinter::emit(const IRLabel& l) const {
    out << "  label " << l.name << "\n";
}

void IRPrinter::emit(const IRAllocateStack& a) const {
    out << "  allocate_stack " << a.amount << "\n";
}

void IRPrinter::emit(const IRRet& /*r*/) const {
    out << "  ret";
    out << "\n";
}

void IRPrinter::emit(const IROperand& v) const {
    if (auto c = dynamic_cast<const IRImm*>(&v)) {
        emit(*c);
        return;
    }
    if (auto r = dynamic_cast<const IRReg*>(&v)) {
        emit(*r);
        return;
    }
    if (auto p = dynamic_cast<const IRPseudo*>(&v)) {
        emit(*p);
        return;
    }
    if (auto s = dynamic_cast<const IRStack*>(&v)) {
        emit(*s);
        return;
    }
    out << "?";
}

void IRPrinter::emit(const IRImm& c) const {
    out << "$" << c.value;
}

void IRPrinter::emit(const IRReg& r) const {
    out << toString(r.reg);
}

void IRPrinter::emit(const IRPseudo& v) const {
    out << v.name;
}

void IRPrinter::emit(const IRStack& v) const {
    out << v.offset << "(%rbp)";
}
