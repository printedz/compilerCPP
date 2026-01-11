#include "ir_printer.h"

#include <iostream>
#include <sstream>

static const char* toString(IRUnaryOperator op) {
    switch (op) {
        case IRUnaryOperator::Negate: return "-";
        case IRUnaryOperator::Complement: return "~";
    }
    return "?";
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
    if (auto u = dynamic_cast<const IRUnary*>(&inst)) {
        emit(*u);
        return;
    }
    if (auto r = dynamic_cast<const IRReturn*>(&inst)) {
        emit(*r);
        return;
    }
    out << "  <unknown inst>\n";
}

void IRPrinter::emit(const IRUnary& u) const {
    out << "  ";
    // dst = op src
    emit(*u.dst);
    out << " = " << toString(u.op) << " ";
    emit(*u.src);
    out << "\n";
}

void IRPrinter::emit(const IRReturn& r) const {
    out << "  return ";
    emit(*r.value);
    out << "\n";
}

void IRPrinter::emit(const IRVal& v) const {
    if (auto c = dynamic_cast<const IRConstant*>(&v)) {
        emit(*c);
        return;
    }
    if (auto var = dynamic_cast<const IRVar*>(&v)) {
        emit(*var);
        return;
    }
    out << "?";
}
void IRPrinter::emit(const IRConstant& c) const {
    out << c.value;
}

void IRPrinter::emit(const IRVar& v) const {
    out << v.name;
}

