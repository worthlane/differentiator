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
    FILE* fp = OpenInputFile(data_file, &error);
    EXIT_IF_ERROR(&error);

    const char* output_file = GetOutputFileName(argc, argv, &error);
    EXIT_IF_ERROR(&error);
    FILE* out_stream = OpenOutputFile(output_file, &error);
    EXIT_IF_ERROR(&error);

    LinesStorage info = {};
    CreateTextStorage(&info, &error, data_file);

    ExpressionInfixRead(&info, &expr, &error);
    EXIT_IF_EXPRESSION_ERROR(&error);

    printf("%lf\n", CalculateExpression(&expr, &error));

    expr_t* d_expr = DifferentiateExpression(&expr, "x", &error, out_stream);
    EXIT_IF_EXPRESSION_ERROR(&error);

    expr_t* expr_2 = DifferentiateExpression(d_expr, "x", &error, out_stream);
    EXIT_IF_EXPRESSION_ERROR(&error);

    expr_t* expr_3 = DifferentiateExpression(expr_2, "x", &error, out_stream);
    EXIT_IF_EXPRESSION_ERROR(&error);

    // SimplifyExpression(&expr, &error, out_sream);

    fclose(fp);
}
