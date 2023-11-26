#include <stdio.h>

#include "common/logs.h"
#include "common/errors.h"
#include "common/input_and_output.h"
#include "common/file_read.h"
#include "expression/expression.h"
#include "calculation.h"

int main(const int argc, const char* argv[])
{
    OpenLogFile(argv[0]);

    expr_t  expr  = {};
    error_t error = {};
    ExpressionCtor(&expr, &error);

    const char* data_file = GetInputFileName(argc, argv, &error);
    EXIT_IF_ERROR(&error);

    FILE* fp = OpenInputFile(data_file, &error);
    EXIT_IF_ERROR(&error);

    Storage info = {};
    CreateTextStorage(&info, &error, data_file);

    ExpressionInfixRead(&info, &expr, &error);
    EXIT_IF_EXPRESSION_ERROR(&error);

    PrintExpressionTree(stdout, &expr);

    printf("%lf\n", CalculateExpression(&expr, expr.root, &error));

    PrintExpressionTreeLatex(stdout, &expr);

    DUMP_EXPRESSION(&expr);

    SimplifyExpression(&expr, &error);
    PrintExpressionTreeLatex(stdout, &expr);

    expr_t* d_expr = DifferentiateExpression(&expr, "x", &error);
    EXIT_IF_EXPRESSION_ERROR(&error);

    PrintExpressionTreeLatex(stdout, d_expr);

    DUMP_EXPRESSION(d_expr);

    fclose(fp);
}
