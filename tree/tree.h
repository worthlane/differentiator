#ifndef __TREE_H_
#define __TREE_H_

#include <stdio.h>

#include "common/errors.h"
#include "common/file_read.h"

enum class DataType
{
    VARIABLE,
    OPERATOR,
    NUMBER,

    PZN,
};

enum class Operators
{
    ADD,
    SUB,
    MUL,
    DIV,

    UNK
};

union DataValue
{
    double      value;
    Operators   operation;
    int         variable;
};

static const DataType  DEFAULT_TYPE  = DataType::NUMBER;
static const DataValue DEFAULT_VALUE = {0};

struct Node
{
    Node* parent;

    DataType  type;
    DataValue value;

    Node* left;
    Node* right;
};

struct VariableInfo
{
    char*  variable_name;
    bool   isfree;
    double value;
};

typedef struct VariableInfo variable_t;

static const size_t MAX_VARIABLES_AMT = 50;
static const size_t MAX_VARIABLE_LEN = 100;

struct Tree
{
    Node* root;

    variable_t* vars;
};
typedef struct Tree tree_t;

enum class DiffErrors
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
int PrintTreeError(FILE* fp, const void* err, const char* func, const char* file, const int line);

#ifdef EXIT_IF_TREE_ERROR
#undef EXIT_IF_TREE_ERROR
#endif
#define EXIT_IF_TREE_ERROR(error)         do                                                            \
                                            {                                                           \
                                                if ((error)->code != (int) DiffErrors::NONE)            \
                                                {                                                       \
                                                    return LogDump(PrintTreeError, error, __func__,     \
                                                                    __FILE__, __LINE__);                \
                                                }                                                       \
                                            } while(0)
#ifdef RETURN_IF_TREE_ERROR
#undef RETURN_IF_TREE_ERROR
#endif
#define RETURN_IF_TREE_ERROR(error)       do                                                            \
                                            {                                                           \
                                                if ((error) != DiffErrors::NONE)                        \
                                                {                                                       \
                                                    return error;                                       \
                                                }                                                       \
                                            } while(0)

Node* NodeCtor(const DataType type, const DataValue value,
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

DiffErrors NodeVerify(const Node* node, error_t* error);

#ifdef CHECK_NODE
#undef CHECK_NODE
#endif
#define CHECK_NODE(node, error)     do                                                              \
                                    {                                                               \
                                        DiffErrors node_err_ = NodeVerify(node, error);             \
                                        if (node_err_ != DiffErrors::NONE)                          \
                                            return node_err_;                                       \
                                    } while(0)

DiffErrors TreeCtor(tree_t* tree, error_t* error);
void       TreeDtor(tree_t* tree);
void       TreePrintEquation(FILE* fp, const tree_t* tree);
void       TreeInfixRead(Storage* info, tree_t* tree, error_t* error);
void       TreePrefixRead(Storage* info, tree_t* tree, error_t* error);
int        TreeDump(FILE* fp, const void* nodes, const char* func, const char* file, const int line);

#ifdef DUMP_TREE
#undef DUMP_TREE
#endif
#define DUMP_TREE(tree)  do                                                                 \
                            {                                                               \
                                LogDump(TreeDump, (tree), __func__, __FILE__, __LINE__);    \
                            } while(0)

DiffErrors TreeVerify(const tree_t* tree, error_t* error);

#ifdef CHECK_TREE
#undef CHECK_TREE
#endif
#define CHECK_TREE(tree, error)     do                                                              \
                                    {                                                               \
                                        DiffErrors tree_err_ = TreeVerify(tree, error);             \
                                        if (tree_err_ != DiffErrors::NONE)                          \
                                            return tree_err_;                                       \
                                    } while(0)

double CalculateTree(tree_t* tree, Node* node, error_t* error);

#endif

