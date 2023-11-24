#ifndef __EXPRESSION_H_
#define __EXPRESSION_H_

#include <stdio.h>

#include "common/errors.h"
#include "common/file_read.h"

// ======================================================================
// ERRORS
// ======================================================================

enum class ExpressionErrors
{
    NONE = 0,

    ALLOCATE_MEMORY,
    EMPTY_TREE,
    INVALID_SYNTAX,
    CYCLED_NODE,
    COMMON_HEIR,
    UNKNOWN_INPUT,
    WRONG_EQUATION,
    UNKNOWN_OPERATION,

    UNKNOWN
};
int PrintExpressionError(FILE* fp, const void* err, const char* func, const char* file, const int line);

#ifdef EXIT_IF_EXPRESSION_ERROR
#undef EXIT_IF_EXPRESSION_ERROR
#endif
#define EXIT_IF_EXPRESSION_ERROR(error)     do                                                            \
                                            {                                                           \
                                                if ((error)->code != (int) ExpressionErrors::NONE)      \
                                                {                                                       \
                                                    return LogDump(PrintExpressionError, error, __func__,     \
                                                                    __FILE__, __LINE__);                \
                                                }                                                       \
                                            } while(0)
#ifdef RETURN_IF_EXPRESSION_ERROR
#undef RETURN_IF_EXPRESSION_ERROR
#endif
#define RETURN_IF_EXPRESSION_ERROR(error)   do                                                            \
                                            {                                                           \
                                                if ((error) != ExpressionErrors::NONE)                  \
                                                {                                                       \
                                                    return error;                                       \
                                                }                                                       \
                                            } while(0)

// ======================================================================
// EXPRESSION OPERATORS
// ======================================================================

enum class Operators
{
    ADD,
    SUB,
    MUL,
    DIV,

    UNK
};

static const char* ADD = "+";
static const char* SUB = "-";
static const char* DIV = "/";
static const char* MUL = "*";

// ======================================================================
// EXPRESSION VARIABLES
// ======================================================================

struct VariableInfo
{
    char*  variable_name;
    bool   isfree;
    double value;
};

typedef struct VariableInfo variable_t;

static const size_t MAX_VARIABLES_AMT = 50;
static const size_t MAX_VARIABLE_LEN = 100;

// ======================================================================
// EXPRESSION TREE NODES
// ======================================================================

enum class NodeType
{
    VARIABLE,
    OPERATOR,
    NUMBER,

    POISON,
};
static const NodeType INIT_TYPE  = NodeType::POISON;

union NodeValue
{
    double      val;
    Operators   opt;
    int         var;
};
static const NodeValue INIT_VALUE = {0};

struct Node
{
    Node* parent;

    NodeType  type;
    NodeValue value;

    Node* left;
    Node* right;
};

Node* NodeCtor(const NodeType type, const NodeValue value,
               Node* left, Node* right, Node* parent, error_t* error);
void  NodeDtor(Node* node);
int   NodeDump(FILE* fp, const void* dumping_node, const char* func, const char* file, const int line);

#ifdef DUMP_NODE
#undef DUMP_NODE
#endif
#define DUMP_NODE(node)     do                                                              \
                            {                                                               \
                                LogDump(NodeDump, (node), __func__, __FILE__, __LINE__);    \
                            } while(0)

ExpressionErrors NodeVerify(const Node* node, error_t* error);

#ifdef CHECK_NODE
#undef CHECK_NODE
#endif
#define CHECK_NODE(node, error)     do                                                              \
                                    {                                                               \
                                        DiffErrors node_err_ = NodeVerify(node, error);             \
                                        if (node_err_ != ExpressionErrors::NONE)                          \
                                            return node_err_;                                       \
                                    } while(0)

// ======================================================================
// EXPRESSION STRUCT
// ======================================================================

struct Expression
{
    Node* root;

    variable_t* vars;
};
typedef struct Expression expr_t;

ExpressionErrors    ExpressionCtor(expr_t* expr, error_t* error);
expr_t*             MakeExpression(error_t* error);
void                ExpressionDtor(expr_t* expr);
void                PrintExpressionTree(FILE* fp, const expr_t* expr);
void                PrintExpressionTreeLatex(FILE* fp, const expr_t* expr);
void                ExpressionInfixRead(Storage* info, expr_t* expr, error_t* error);
void                ExpressionPrefixRead(Storage* info, expr_t* expr, error_t* error);
int                 ExpressionDump(FILE* fp, const void* nodes, const char* func, const char* file, const int line);

#ifdef DUMP_EXPRESSION
#undef DUMP_EXPRESSION
#endif
#define DUMP_EXPRESSION(expr)   do                                                                 \
                                {                                                               \
                                    LogDump(ExpressionDump, (expr), __func__, __FILE__, __LINE__);    \
                                } while(0)

ExpressionErrors ExpressionVerify(const expr_t* expr, error_t* error);

#ifdef CHECK_EXPRESSION
#undef CHECK_EXPRESSION
#endif
#define CHECK_EXPRESSION(expr, error)       do                                                              \
                                            {                                                               \
                                                DiffErrors tree_err_ = ExpressionVerify(expr, error);             \
                                                if (tree_err_ != ExpressionErrors::NONE)                    \
                                                    return tree_err_;                                       \
                                            } while(0)

double CalculateExpression(expr_t* expr, Node* node, error_t* error);

#endif
