#ifndef COMPILER_ASTPRINTER_H
#define COMPILER_ASTPRINTER_H

#include <string>
#include "ast.h"

class ASTPrinter {
public:
    static std::string print(const Program& program);
    static std::string print(const Function& function, int indent = 0);
    static std::string print(const Instruction& instruction, int indent = 0);
    static std::string printOperand(const Operand& operand);
};

#endif //COMPILER_ASTPRINTER_H
