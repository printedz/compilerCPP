#ifndef COMPILER_RESOLVER_H
#define COMPILER_RESOLVER_H

#include <memory>
#include "ast.h"

class Resolver {
public:
    static std::unique_ptr<Program> resolve(const Program& program);
};

#endif // COMPILER_RESOLVER_H
