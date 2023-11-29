#include <stdio.h>
#include <time.h>

#include "common/logs.h"
#include "common/errors.h"
#include "common/input_and_output.h"
#include "common/file_read.h"
#include "expression/expression.h"
#include "expression/expr_input_and_output.h"
#include "calculation.h"

int main(const int argc, const char* argv[])
{
    OpenLogFile(argv[0]);

    expr_t  expr  = {};
    error_t error = {};
    ExpressionCtor(&expr, &error);

    const char* data_file = GetInputFileName(argc, argv, &error);
    EXIT_IF_ERROR(&error);

    FILE* outstream = fopen("text.txt", "w");

    FILE* fp = OpenInputFile(data_file, &error);
    EXIT_IF_ERROR(&error);

    Storage info = {};
    CreateTextStorage(&info, &error, data_file);

    ExpressionInfixRead(&info, &expr, &error);
    EXIT_IF_EXPRESSION_ERROR(&error);

    PrintExpressionTree(stdout, &expr);

    printf("%lf\n", CalculateExpression(&expr, expr.root, &error));

    DUMP_EXPRESSION(&expr);

    SimplifyExpression(&expr, &error, outstream);
    PrintExpressionTreeLatex(stdout, &expr);

    /*expr_t* d_expr = DifferentiateExpression(&expr, "x", &error, outstream);
    EXIT_IF_EXPRESSION_ERROR(&error);*/

    expr_t* d_expr = TaylorSeries(&expr, 3, "x", 0, &error, outstream);
    EXIT_IF_EXPRESSION_ERROR(&error);

    expr_t* difference = SubExpressions(&expr, d_expr, &error);

    DrawExprGraphic(difference);

    DUMP_EXPRESSION(d_expr);

    fclose(fp);
}
