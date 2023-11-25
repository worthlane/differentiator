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

    expr_t  tree  = {};
    error_t error = {};
    ExpressionCtor(&tree, &error);

    const char* data_file = GetInputFileName(argc, argv, &error);
    EXIT_IF_ERROR(&error);

    FILE* fp = OpenInputFile(data_file, &error);
    EXIT_IF_ERROR(&error);

    Storage info = {};
    CreateTextStorage(&info, &error, data_file);

    ExpressionInfixRead(&info, &tree, &error);
    EXIT_IF_EXPRESSION_ERROR(&error);

    PrintExpressionTree(stdout, &tree);

    printf("%lf\n", CalculateExpression(&tree, tree.root, &error));

    PrintExpressionTreeLatex(stdout, &tree);

    DUMP_EXPRESSION(&tree);

    SimplifyExpression(&tree, &error);
    PrintExpressionTreeLatex(stdout, &tree);

    DUMP_EXPRESSION(&tree);

    fclose(fp);
}
