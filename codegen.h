#ifndef COMPILER_CODEGENERATOR_H
#define COMPILER_CODEGENERATOR_H

#include <string>
#include "ast.h"

class CodeGenerator {
public:
    static std::string generate(const Program& program);

private:
    static std::string formatOperand(const Operand* op);
};

#endif //COMPILER_CODEGENERATOR_H
