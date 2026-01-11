#ifndef COMPILER_IRPRINTER_H
#define COMPILER_IRPRINTER_H

#include <ostream>
#include <string>
#include "ir.h"

struct IRPrinter {
    // Returns a string representation of the IR program, similar to ASTPrinter::print
    static std::string print(const IRProgram& program);

private:
    std::ostream& out;
    explicit IRPrinter(std::ostream& o) : out(o) {}

    // Internal emitters used by the static print
    void emit(const IRProgram& program) const;
    void emit(const IRFunction& fn) const;
    void emit(const IRInstruction& inst) const;
    void emit(const IRMov& m) const;
    void emit(const IRUnary& u) const;
    void emit(const IRAllocateStack& a) const;
    void emit(const IRRet& r) const;
    void emit(const IROperand& v) const;
    void emit(const IRImm& c) const;
    void emit(const IRReg& r) const;
    void emit(const IRPseudo& v) const;
    void emit(const IRStack& v) const;
};

#endif // COMPILER_IRPRINTER_H
