#include "ir_printer.h"

#include <iostream>
#include <sstream>

static const char* toString(IRUnaryOperator op) {
    switch (op) {
        case IRUnaryOperator::Neg: return "neg";
        case IRUnaryOperator::Not: return "not";
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
