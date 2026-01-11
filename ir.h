#ifndef COMPILER_IR_H
#define COMPILER_IR_H

#include <string>
#include <vector>
#include <memory>
#include <utility>

/*
Grammar:

program = Program(function_definition)
function_definition = Function(identifier, instruction* body)
instruction = Return(val) | Unary(unary_operator, val src, val dst)
val = Constant(int) | Var(identifier)
unary_operator = Complement | Negate
*/

enum class IRUnaryOperator {
    Complement,
    Negate
};

struct IRVal {
    virtual ~IRVal() = default;
};

struct IRConstant : public IRVal {
    int value;
    explicit IRConstant(int v) : value(v) {}
};

struct IRVar : public IRVal {
    std::string name;
    explicit IRVar(std::string n) : name(std::move(n)) {}
};

struct IRInstruction {
    virtual ~IRInstruction() = default;
};

struct IRReturn : public IRInstruction {
    std::unique_ptr<IRVal> value;
    explicit IRReturn(std::unique_ptr<IRVal> v) : value(std::move(v)) {}
};

struct IRUnary : public IRInstruction {
    IRUnaryOperator op;
    std::unique_ptr<IRVal> src;
    std::unique_ptr<IRVal> dst;
    IRUnary(IRUnaryOperator o, std::unique_ptr<IRVal> s, std::unique_ptr<IRVal> d)
        : op(o), src(std::move(s)), dst(std::move(d)) {}
};

struct IRFunction {
    std::string name;
    std::vector<std::unique_ptr<IRInstruction>> body;
    IRFunction(std::string n, std::vector<std::unique_ptr<IRInstruction>> b)
        : name(std::move(n)), body(std::move(b)) {}
};

struct IRProgram {
    std::unique_ptr<IRFunction> function;
    explicit IRProgram(std::unique_ptr<IRFunction> f) : function(std::move(f)) {}
};

#endif // COMPILER_IR_H

