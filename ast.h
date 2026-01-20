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
    IF_KEYWORD,
    ELSE_KEYWORD,
    TYPEDEF_KEYWORD,
    IDENTIFIER,
    CONSTANT,
    TILDE,      // '~' complement
    BANG,       // '!' logical not
    DECREMENT,  // '--' (not used by grammar, but kept for lexer completeness)
    HYPHEN,     // '-' negate/subtract
    PLUS,       // '+'
    STAR,       // '*'
    SLASH,      // '/'
    PERCENT,    // '%'
    EXCLAMATION,
    DOUBLEAND,
    DOUBLEBAR,
    TWOEQUAL,
    EQUAL,
    NOTEQUAL,
    LESSTHAN,
    GREATERTHAN,
    LESSEQUALTHAN,
    GREATEREQUALTHAN,
    OPEN_PAREN,
    CLOSE_PAREN,
    OPEN_BRACE,
    CLOSE_BRACE,
    QUESTION,
    COLON,
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
//          | Expression(exp)
//          | If(exp condition, statement then, statement? else)
//          | Null
// exp = Constant(int) | Unary(unary_operator, exp) | Binary(binary_operator, exp, exp)
//    | Assignment(exp, exp)
//    | Conditional(exp condition, exp, exp)
// unary_operator = Complement | Negate | Not
// binary_operator = Add | Subtract | Multiply | Divide | Remainder | And | Or
//                | Equal | NotEqual | LessThan | LessOrEqual
//                | GreaterThan | GreaterOrEqual
// ========================

// Forward decls
struct Statement;
struct Exp;

// Unary operator kinds
enum class UnaryOperator {
    Complement, // '~'
    Negate,     // '-'
    Not,        // '!'
    LogicalNot = Not // backward-compatible alias
};

// Binary operator kinds
enum class BinaryOperator {
    Add,        // '+'
    Subtract,   // '-'
    Sub = Subtract, // alias for older name
    Multiply,   // '*'
    Mul = Multiply, // alias
    Divide,     // '/'
    Div = Divide, // alias
    Remainder,  // '%'
    Mod = Remainder, // alias
    And,        // '&&'
    Or,         // '||'
    Equal,      // '=='
    NotEqual,   // '!='
    LessThan,   // '<'
    LessOrEqual,// '<='
    GreaterThan,// '>'
    GreaterOrEqual // '>='
};

// Expressions
struct Exp {
    virtual ~Exp() = default;
};

struct Constant : public Exp {
    int value;
    explicit Constant(int v) : value(v) {}
};

struct Var : public Exp {
    std::string name;
    explicit Var(std::string n) : name(std::move(n)) {}
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

struct Assignment : public Exp {
    std::unique_ptr<Exp> lhs;
    std::unique_ptr<Exp> rhs;
    Assignment(std::unique_ptr<Exp> l, std::unique_ptr<Exp> r)
        : lhs(std::move(l)), rhs(std::move(r)) {}
};

struct Conditional : public Exp {
    std::unique_ptr<Exp> condition;
    std::unique_ptr<Exp> thenExpr;
    std::unique_ptr<Exp> elseExpr;
    Conditional(std::unique_ptr<Exp> c,
                std::unique_ptr<Exp> t,
                std::unique_ptr<Exp> e)
        : condition(std::move(c)),
          thenExpr(std::move(t)),
          elseExpr(std::move(e)) {}
};

// Block items
struct BlockItem {
    virtual ~BlockItem() = default;
};

// Statements
struct Statement : public BlockItem {
    ~Statement() override = default;
};

struct Return : public Statement {
    std::unique_ptr<Exp> expr;
    explicit Return(std::unique_ptr<Exp> e) : expr(std::move(e)) {}
};

struct ExpressionStatement : public Statement {
    std::unique_ptr<Exp> expr;
    explicit ExpressionStatement(std::unique_ptr<Exp> e) : expr(std::move(e)) {}
};

struct IfStatement : public Statement {
    std::unique_ptr<Exp> condition;
    std::unique_ptr<Statement> thenStmt;
    std::unique_ptr<Statement> elseStmt; // nullptr if no else clause
    IfStatement(std::unique_ptr<Exp> c,
                std::unique_ptr<Statement> t,
                std::unique_ptr<Statement> e)
        : condition(std::move(c)),
          thenStmt(std::move(t)),
          elseStmt(std::move(e)) {}
};

struct EmptyStatement : public Statement {
};

struct Declaration : public BlockItem {
    std::string name;
    std::unique_ptr<Exp> init; // nullptr when no initializer is present
    Declaration(std::string n, std::unique_ptr<Exp> i)
        : name(std::move(n)), init(std::move(i)) {}
};

struct Typedef : public BlockItem {
    std::string name;
    std::string baseType;
    Typedef(std::string n, std::string t)
        : name(std::move(n)), baseType(std::move(t)) {}
};

// Function and Program
struct Function {
    std::string name;
    std::vector<std::unique_ptr<BlockItem>> body;
    Function(std::string n, std::vector<std::unique_ptr<BlockItem>> b)
        : name(std::move(n)), body(std::move(b)) {}
};

struct Program {
    std::unique_ptr<Function> function;
    explicit Program(std::unique_ptr<Function> f) : function(std::move(f)) {}
};

#endif //COMPILER_AST_H
