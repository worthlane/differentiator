#ifndef __EXPR_OUTPUT_H_
#define __EXPR_OUTPUT_H_

#include "expression.h"

static const int    MAX_OUTPUT_TREE_DEPTH = 6;
static const char   INIT_SUBTREE_NAME     = 'A';
static const char   MAX_SUBTREES_AMT      = 'Z' - 'A';
static const int    MAX_LINES_ON_PAGE     = 25;

struct SubtreeNames
{
    const Node* subtrees[MAX_SUBTREES_AMT];
    int         free_spot;
    int         order;
};

// ======================================================================
// EXPRESSION OPERATORS
// ======================================================================

enum class LatexOperationTypes
{
    INFIX,
    PREFIX,
    POSTFIX
};

// ======================================================================
// EXPRESSION TREE NODES
// ======================================================================

int   NodeDump(FILE* fp, const void* dumping_node, const char* func, const char* file, const int line);

#ifdef DUMP_NODE
#undef DUMP_NODE
#endif
#define DUMP_NODE(node)     do                                                              \
                            {                                                               \
                                LogDump(NodeDump, (node), __func__, __FILE__, __LINE__);    \
                            } while(0)

void  PrintNodeData(FILE* fp, const expr_t* expr, const Node* node,
                    SubtreeNames* names = nullptr);

// ======================================================================
// EXPRESSION STRUCT
// ======================================================================

void    PrintExpression(FILE* fp, const expr_t* expr);

void    PrintInfixExpression(FILE* fp, const expr_t* expr);
void    PrintTaylorLatex(FILE* fp, const expr_t* expr, const size_t order, const int val);
void    ExpressionInfixRead(LinesStorage* info, expr_t* expr, error_t* error);
void    ExpressionPrefixRead(LinesStorage* info, expr_t* expr, error_t* error);
int     ExpressionDump(FILE* fp, const void* nodes, const char* func, const char* file, const int line);

#ifdef DUMP_EXPRESSION
#undef DUMP_EXPRESSION
#endif
#define DUMP_EXPRESSION(expr)   do                                                                 \
                                {                                                               \
                                    LogDump(ExpressionDump, (expr), __func__, __FILE__, __LINE__);    \
                                } while(0)

// ======================================================================
// GRAPHICS
// ======================================================================

void DrawExprGraphic(FILE* fp, const expr_t* expr);
void DrawTwoExprGraphics(FILE* fp, const expr_t* expr_1, const expr_t* expr_2);       // TODO несколько в одной



#endif
