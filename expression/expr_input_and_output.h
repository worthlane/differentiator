#ifndef __EXPR_INPUT_AND_OUTPUT_H_
#define __EXPR_INPUT_AND_OUTPUT_H_

#include "expression.h"

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

void  PrintNodeData(FILE* fp, const expr_t* expr, const Node* node);

// ======================================================================
// EXPRESSION STRUCT
// ======================================================================

void    PrintExpressionTree(FILE* fp, const expr_t* expr);
void    PrintExpressionTreeLatex(FILE* fp, const expr_t* expr);
void    ExpressionInfixRead(Storage* info, expr_t* expr, error_t* error);
void    ExpressionPrefixRead(Storage* info, expr_t* expr, error_t* error);
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

void DrawExprGraphic(const expr_t* expr);



#endif
