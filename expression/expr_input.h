#ifndef _EXPR_INPUT_H_
#define _EXPR_INPUT_H_

#include "expression.h"

static const int MAX_TOKENS_AMT = 1000;

struct Tokens
{
    Node* buf[MAX_TOKENS_AMT];
    int   ptr;
};

void GetExpression(LinesStorage* info, expr_t* expr, error_t* error);

#endif
