#ifndef COMPILER_LOWERING_H
#define COMPILER_LOWERING_H

#include <memory>
#include "ast.h"
#include "ir.h"

class Lowering {
public:
    // Lowers the high-level AST Program to the IR Program.
    static std::unique_ptr<IRProgram> toIR(const Program& program);
};

#endif // COMPILER_LOWERING_H
