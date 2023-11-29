#ifndef __CALCULATION_H_
#define __CALCULATION_H_

#include "expression/expression.h"

double CalculateExpression(expr_t* expr, Node* node, error_t* error);

void SimplifyExpression(expr_t* expr, error_t* error, FILE* fp = nullptr);

expr_t* DifferentiateExpression(expr_t* expr, const char* var, error_t* error, FILE* fp = nullptr);

expr_t* TaylorSeries(expr_t* expr, const int n, const char* var, const double val, error_t* error, FILE* fp = nullptr);

expr_t* SubExpressions(expr_t* expr_1, expr_t* expr_2, error_t* error, FILE* fp = nullptr);


#endif
