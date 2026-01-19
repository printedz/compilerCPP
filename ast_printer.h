#ifndef COMPILER_ASTPRINTER_H
#define COMPILER_ASTPRINTER_H

#include <string>
#include "ast.h"

class ASTPrinter {
public:
    static std::string print(const Program& program);
    static std::string print(const Function& function, int indent = 0);
    static std::string print(const BlockItem& item, int indent = 0);
    static std::string print(const Statement& statement, int indent = 0);
    static std::string print(const Exp& exp, int indent = 0);
};

#endif //COMPILER_ASTPRINTER_H
