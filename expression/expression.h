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
    NO_EXPRESSION,
    INVALID_SYNTAX,
    CYCLED_NODE,
    COMMON_HEIR,
    INVALID_EXPRESSION_FORMAT,
    UNKNOWN_OPERATION,
    NO_DIFF_VARIABLE,

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

#define DEF_OP(name, ...)      name,

enum class Operators
{
    #include "operations.h"

    UNK
};

#undef DEF_OP

#define DEF_OP(name, symb, ...)     \
    static const char* name = symb; \

#include "operations.h"

#undef DEF_OP

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
static const size_t MAX_VARIABLE_LEN  = 100;
static const int    NO_VARIABLE       = -1;

variable_t* AllocVariablesArray(error_t* error);
void        DestructVariablesArray(variable_t* variables);

int         SaveVariable(variable_t* vars, const char* new_var);
int         FindVariableAmongSaved(variable_t* vars, const char* new_var);
void        CopyVariablesArray(const variable_t* vars, variable_t* dest, error_t* error);

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
static const NodeType PZN_TYPE  = NodeType::POISON;

union NodeValue
{
    double      val;
    Operators   opt;
    int         var;
};
static const NodeValue ZERO_VALUE = {.val = 0};

struct Node
{
    Node* parent;

    NodeType  type;
    NodeValue value;

    Node* left;
    Node* right;
};

Node* MakeNode(const NodeType type, const NodeValue value,
               Node* left, Node* right, Node* parent);
void  NodeDtor(Node* node);
void  DestructNodes(Node* root);

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

void ConnectNodesWithParents(Node* node);

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

// ======================================================================
// OTHERS
// ======================================================================

bool FindVarInTree(Node* node, const int id);

#endif


