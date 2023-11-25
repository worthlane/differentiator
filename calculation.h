#ifndef __CALCULATION_H_
#define __CALCULATION_H_

#include "expression/expression.h"

double CalculateExpression(expr_t* expr, Node* node, error_t* error);

void SimplifyExpression(expr_t* expr, Node* node, error_t* error);

#endif
