#ifndef COMPILER_AST_H
#define COMPILER_AST_H

#include <string>
#include <vector>
#include <memory>

// Tokens (keep existing kinds used by the lexer)
enum class TokenType {
    INT_KEYWORD,
    VOID_KEYWORD,
    RETURN_KEYWORD,
    IDENTIFIER,
    CONSTANT,
    TILDE,      // '~' complement
    DECREMENT,  // '--' (not used by grammar, but kept for lexer completeness)
    HYPHEN,     // '-' negate/subtract
    PLUS,       // '+'
    STAR,       // '*'
    SLASH,      // '/'
    PERCENT,    // '%'
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

// ========================
// High-level AST for grammar
// program = Program(function_definition)
// function_definition = Function(identifier name, statement body)
// statement = Return(exp)
// exp = Constant(int) | Unary(unary_operator, exp) | Binary(binary_operator, exp, exp)
// unary_operator = Complement | Negate
// binary_operator = Add | Sub | Mul | Div | Mod
// ========================

// Forward decls
struct Statement;
struct Exp;

// Unary operator kinds
enum class UnaryOperator {
    Complement, // '~'
    Negate      // '-'
};

// Binary operator kinds
enum class BinaryOperator {
    Add, // '+'
    Sub, // '-'
    Mul, // '*'
    Div, // '/'
    Mod  // '%'
};

// Expressions
struct Exp {
    virtual ~Exp() = default;
};

struct Constant : public Exp {
    int value;
    explicit Constant(int v) : value(v) {}
};

struct Unary : public Exp {
    UnaryOperator op;
    std::unique_ptr<Exp> expr;
    Unary(UnaryOperator o, std::unique_ptr<Exp> e) : op(o), expr(std::move(e)) {}
};

struct Binary : public Exp {
    BinaryOperator op;
    std::unique_ptr<Exp> left;
    std::unique_ptr<Exp> right;
    Binary(BinaryOperator o, std::unique_ptr<Exp> l, std::unique_ptr<Exp> r)
        : op(o), left(std::move(l)), right(std::move(r)) {}
};

// Statements
struct Statement {
    virtual ~Statement() = default;
};

struct Return : public Statement {
    std::unique_ptr<Exp> expr;
    explicit Return(std::unique_ptr<Exp> e) : expr(std::move(e)) {}
};

// Function and Program
struct Function {
    std::string name;
    std::unique_ptr<Statement> body; // single statement body per grammar
    Function(std::string n, std::unique_ptr<Statement> b)
        : name(std::move(n)), body(std::move(b)) {}
};

struct Program {
    std::unique_ptr<Function> function;
    explicit Program(std::unique_ptr<Function> f) : function(std::move(f)) {}
};

#endif //COMPILER_AST_H
