#ifndef DEF_OP
#define DEF_OP(...) ;
#endif

DEF_OP(ADD, "+", 1, 2, {number_1 + number_2},
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

DEF_OP(SUB, "-", 1, 2, {number_1 - number_2},
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

DEF_OP(DIV, "/", 2, 2, {number_1 / number_2},
{
    fprintf(fp, "\\frac{");
    NodesInfixPrintLatex(fp, expr, node->left);
    fprintf(fp, "}{");
    NodesInfixPrintLatex(fp, expr, node->right);
    fprintf(fp, "}");
    return;
})

DEF_OP(MUL, "*", 2, 2, {number_1 * number_2},
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

DEF_OP(DEG, "^", 2, 2, {pow(number_1, number_2)},
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

DEF_OP(LN, "ln", 2, 1, {log(number_2)},
{
    fprintf(fp, "\\ln(");
    NodesInfixPrintLatex(fp, expr, node->right);
    fprintf(fp, ")");
})

DEF_OP(EXP, "exp", 2, 1, {exp(number_2)},
{
    fprintf(fp, "\\exp(");
    NodesInfixPrintLatex(fp, expr, node->right);
    fprintf(fp, ")");
})

DEF_OP(SIN, "sin", 2, 1, {sin(number_2)},
{
    fprintf(fp, "\\sin(");
    NodesInfixPrintLatex(fp, expr, node->right);
    fprintf(fp, ")");
})

DEF_OP(COS, "cos", 2, 1, {cos(number_2)},
{
    fprintf(fp, "\\cos(");
    NodesInfixPrintLatex(fp, expr, node->right);
    fprintf(fp, ")");
})

DEF_OP(COT, "ctg", 2, 1, {1/tan(number_2)},
{
    fprintf(fp, "\\cot(");
    NodesInfixPrintLatex(fp, expr, node->right);
    fprintf(fp, ")");
})

DEF_OP(TAN, "tg", 2, 1, {tan(number_2)},
{
    fprintf(fp, "\\tan(");
    NodesInfixPrintLatex(fp, expr, node->right);
    fprintf(fp, ")");
})

DEF_OP(ARCSIN, "arcsin", 2, 1, {asin(number_2)},
{
    fprintf(fp, "\\arcsin(");
    NodesInfixPrintLatex(fp, expr, node->right);
    fprintf(fp, ")");
})

DEF_OP(ARCCOS, "arccos", 2, 1, {acos(number_2)},
{
    fprintf(fp, "\\arccos(");
    NodesInfixPrintLatex(fp, expr, node->right);
    fprintf(fp, ")");
})

DEF_OP(ARCCOT, "arcctg", 2, 1, {M_PI/2 - atan(number_2)},
{
    fprintf(fp, "\\arccot(");
    NodesInfixPrintLatex(fp, expr, node->right);
    fprintf(fp, ")");
})

DEF_OP(ARCTAN, "arctg", 2, 1, {atan(number_2)},
{
    fprintf(fp, "\\arctan(");
    NodesInfixPrintLatex(fp, expr, node->right);
    fprintf(fp, ")");
})

