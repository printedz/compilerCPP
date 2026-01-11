#ifndef COMPILER_IR_H
#define COMPILER_IR_H

#include <string>
#include <vector>
#include <memory>
#include <utility>

/*
Assembly AST Grammar:

program = Program(function_definition)
function_definition = Function(identifier name, instruction* instructions)
instruction = Mov(operand src, operand dst)
            | Unary(unary_operator, operand)
            | AllocateStack(int)
            | Ret
unary_operator = Neg | Not
operand = Imm(int) | Reg(reg) | Pseudo(identifier) | Stack(int)
reg = AX | R10
*/

enum class IRUnaryOperator {
    Neg,
    Not
};

enum class IRRegister {
    AX,
    R10
};

struct IROperand {
    virtual ~IROperand() = default;
};

struct IRImm : public IROperand {
    int value;
    explicit IRImm(int v) : value(v) {}
};

struct IRReg : public IROperand {
    IRRegister reg;
    explicit IRReg(IRRegister r) : reg(r) {}
};

struct IRPseudo : public IROperand {
    std::string name;
    explicit IRPseudo(std::string n) : name(std::move(n)) {}
};

struct IRStack : public IROperand {
    int offset;
    explicit IRStack(int o) : offset(o) {}
};

struct IRInstruction {
    virtual ~IRInstruction() = default;
};

struct IRMov : public IRInstruction {
    std::unique_ptr<IROperand> src;
    std::unique_ptr<IROperand> dst;
    IRMov(std::unique_ptr<IROperand> s, std::unique_ptr<IROperand> d)
        : src(std::move(s)), dst(std::move(d)) {}
};

struct IRUnary : public IRInstruction {
    IRUnaryOperator op;
    std::unique_ptr<IROperand> operand;
    IRUnary(IRUnaryOperator o, std::unique_ptr<IROperand> e)
        : op(o), operand(std::move(e)) {}
};

struct IRAllocateStack : public IRInstruction {
    int amount;
    explicit IRAllocateStack(int a) : amount(a) {}
};

struct IRRet : public IRInstruction {
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
