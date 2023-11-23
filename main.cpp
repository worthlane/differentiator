#include <stdio.h>

#include "common/logs.h"
#include "common/errors.h"
#include "common/input_and_output.h"
#include "common/file_read.h"
#include "tree/tree.h"

int main(const int argc, const char* argv[])
{
    OpenLogFile(argv[0]);

    tree_t  tree  = {};
    error_t error = {};
    TreeCtor(&tree, &error);

    const char* data_file = GetInputFileName(argc, argv, &error);
    EXIT_IF_ERROR(&error);

    FILE* fp = OpenInputFile(data_file, &error);
    EXIT_IF_ERROR(&error);

    Storage info = {};
    CreateTextStorage(&info, &error, data_file);

    TreeInfixRead(&info, &tree, &error);
    EXIT_IF_TREE_ERROR(&error);

    TreePrintEquation(stdout, &tree);

    printf("%lf\n", CalculateTree(&tree, tree.root, &error));

    TreePrintEquationLatex(stdout, &tree);

    DUMP_TREE(&tree);

    fclose(fp);
}
