#ifndef COMPILER_IRPRINTER_H
#define COMPILER_IRPRINTER_H

#include <string>
#include "ir.h"

class IRPrinter {
public:
    static std::string print(const IRProgram& program);
    static std::string print(const IRFunction& function, int indent = 0);
    static std::string print(const IRInstruction& instr, int indent = 0);
    static std::string print(const IRVal& val, int indent = 0);
};

#endif // COMPILER_IRPRINTER_H
