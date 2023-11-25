#ifndef DEF_OP
#define DEF_OP(...) ;
#endif

DEF_OP(ADD, "+", 1, {number_1 + number_2},
{
    bool need_brackets_on_the_left  = CheckBracketsNeededInEquation(node->left);
    bool need_brackets_on_the_right = CheckBracketsNeededInEquation(node->right);

    if (need_brackets_on_the_left) fprintf(fp, "(");
    NodesInfixPrintLatex(fp, expr, node->left);
    if (need_brackets_on_the_left) fprintf(fp, ")");

    PrintNodeData(fp, expr, node);

    if (need_brackets_on_the_right) fprintf(fp, "(");
    NodesInfixPrintLatex(fp, expr, node->right);
    if (need_brackets_on_the_right) fprintf(fp, ")");
})

DEF_OP(SUB, "-", 1, {number_1 - number_2},
{
    bool need_brackets_on_the_left  = CheckBracketsNeededInEquation(node->left);
    bool need_brackets_on_the_right = CheckBracketsNeededInEquation(node->right);

    if (need_brackets_on_the_left) fprintf(fp, "(");
    NodesInfixPrintLatex(fp, expr, node->left);
    if (need_brackets_on_the_left) fprintf(fp, ")");

    PrintNodeData(fp, expr, node);

    if (need_brackets_on_the_right) fprintf(fp, "(");
    NodesInfixPrintLatex(fp, expr, node->right);
    if (need_brackets_on_the_right) fprintf(fp, ")");
})

DEF_OP(DIV, "/", 2, {number_1 / number_2},
{
    fprintf(fp, "\\frac{");
    NodesInfixPrintLatex(fp, expr, node->left);
    fprintf(fp, "}{");
    NodesInfixPrintLatex(fp, expr, node->right);
    fprintf(fp, "}");
    return;
})

DEF_OP(MUL, "*", 2, {number_1 * number_2},
{
    bool need_brackets_on_the_left  = CheckBracketsNeededInEquation(node->left);
    bool need_brackets_on_the_right = CheckBracketsNeededInEquation(node->right);

    if (need_brackets_on_the_left) fprintf(fp, "(");
    NodesInfixPrintLatex(fp, expr, node->left);
    if (need_brackets_on_the_left) fprintf(fp, ")");

    fprintf(fp, "\\cdot");

    if (need_brackets_on_the_right) fprintf(fp, "(");
    NodesInfixPrintLatex(fp, expr, node->right);
    if (need_brackets_on_the_right) fprintf(fp, ")");
})

DEF_OP(DEG, "^", 2, {pow(number_1, number_2)},
{
    bool need_brackets_on_the_left  = CheckBracketsNeededInEquation(node->left);
    bool need_brackets_on_the_right = CheckBracketsNeededInEquation(node->right);

    if (need_brackets_on_the_left) fprintf(fp, "(");
    NodesInfixPrintLatex(fp, expr, node->left);
    if (need_brackets_on_the_left) fprintf(fp, ")");

    PrintNodeData(fp, expr, node);

    if (need_brackets_on_the_right) fprintf(fp, "(");
    NodesInfixPrintLatex(fp, expr, node->right);
    if (need_brackets_on_the_right) fprintf(fp, ")");
})
