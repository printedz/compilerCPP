#ifndef COMPILER_CODEGENERATOR_H
#define COMPILER_CODEGENERATOR_H

#include <string>
#include "ast.h"
#include "ir.h"

class CodeGenerator {
public:
    static std::string generate(const Program& program);
    static std::string generate(const IRProgram& program);

private:
    static std::string genFunctionIR(const IRFunction& func);
};

#endif //COMPILER_CODEGENERATOR_H
