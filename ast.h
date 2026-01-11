#ifndef COMPILER_AST_H
#define COMPILER_AST_H

#include <string>
#include <vector>
#include <memory>

enum class TokenType {
    INT_KEYWORD,
    VOID_KEYWORD,
    RETURN_KEYWORD,
    IDENTIFIER,
    CONSTANT,
    TILDE,
    DECREMENT,
    HYPHEN,
    OPEN_PAREN,
    CLOSE_PAREN,
    OPEN_BRACE,
    CLOSE_BRACE,
    SEMICOLON
};

struct Token {
    TokenType type;
    std::string value;
};

// AST Nodes
struct Operand {
    virtual ~Operand() = default;
};

struct Imm : public Operand {
    int value;
    explicit Imm(int val) : value(val) {}
};

struct Register : public Operand {
};

struct Instruction {
    virtual ~Instruction() = default;
};

struct Mov : public Instruction {
    std::unique_ptr<Operand> src;
    std::unique_ptr<Operand> dst;
    Mov(std::unique_ptr<Operand> s, std::unique_ptr<Operand> d) : src(std::move(s)), dst(std::move(d)) {}
};

struct Ret : public Instruction {
};

struct Function {
    std::string name;
    std::vector<std::unique_ptr<Instruction>> instructions;
    Function(std::string n, std::vector<std::unique_ptr<Instruction>> instrs) : name(std::move(n)), instructions(std::move(instrs)) {}
};

struct Program {
    std::unique_ptr<Function> function;
    explicit Program(std::unique_ptr<Function> f) : function(std::move(f)) {}
};

#endif //COMPILER_AST_H
