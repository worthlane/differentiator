#ifndef __CALCULATION_H_
#define __CALCULATION_H_

#include "expression/expression.h"

double CalculateExpression(expr_t* expr, Node* node, error_t* error);

void SimplifyExpression(expr_t* expr, error_t* error);

expr_t* DifferentiateExpression(expr_t* expr, const char* var, error_t* error);

#endif
