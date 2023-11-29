#ifndef DEF_OP
#define DEF_OP(...) ;
#endif

DEF_OP(ADD, "+", 1, 2, {number_1 + number_2}, "+", LatexOperationTypes::INFIX, "+", false, false,
{
    return _ADD(d(node->left), d(node->right));
})

//------------------------------------------------------------------

DEF_OP(SUB, "-", 1, 2, {number_1 - number_2}, "-", LatexOperationTypes::INFIX, "-", false, false,
{
    return _SUB(d(node->left), d(node->right));
})

//------------------------------------------------------------------

DEF_OP(DIV, "/", 2, 2, {number_1 / number_2}, "/", LatexOperationTypes::PREFIX, "\\frac", true, true,
{
    return _DIV(_SUB(_MUL(d(node->left), CPY(node->right)), _MUL(CPY(node->left), d(node->right))),
                _DEG(CPY(node->right), NUM(2)));
})

//------------------------------------------------------------------

DEF_OP(MUL, "*", 2, 2, {number_1 * number_2}, "*", LatexOperationTypes::INFIX, "\\cdot", false, false,
{
    return _ADD(_MUL(d(node->left), CPY(node->right)), _MUL(CPY(node->left), d(node->right)));;
})

//------------------------------------------------------------------

DEF_OP(DEG, "^", 2, 2, {pow(number_1, number_2)}, "**", LatexOperationTypes::INFIX, "^", true, true,
{
    bool has_var_in_base = FindVarInTree(node->left, id);
    bool has_var_in_deg  = FindVarInTree(node->right, id);

    if (has_var_in_base && has_var_in_deg)
    {
        return _MUL(_ADD(_MUL(d(node->right), _LN(CPY(node->left))),
                         _MUL(CPY(node->right), _DIV(d(node->left), CPY(node->left)))),
                    CPY(node));
    }
    else if (has_var_in_base)
    {
        return _MUL(d(node->left), _MUL(CPY(node->right), _DEG(node->left, _SUB(CPY(node->right), NUM(1)))));
    }
    else if (has_var_in_deg)
    {
        return _MUL(_LN(node->left), CPY(node->right));
    }
    else
        return NUM(0);
})

//==================================================================
//==================================================================

DEF_OP(LN, "ln", 2, 1, {log(number_2)}, "log", LatexOperationTypes::PREFIX, "\\ln", true, false,
{
    return _MUL(d(node->right), _DIV(NUM(1), CPY(node->right)));
})

//------------------------------------------------------------------

DEF_OP(EXP, "exp", 2, 1, {exp(number_2)}, "exp", LatexOperationTypes::PREFIX, "\\exp", true, false,
{
    return _MUL(d(node->right), CPY(node));
})

//==================================================================
//==================================================================

DEF_OP(SIN, "sin", 2, 1, {sin(number_2)}, "sin", LatexOperationTypes::PREFIX, "\\sin", true, false,
{
    return _MUL(d(node->right), _COS(CPY(node->right)));
})

//------------------------------------------------------------------

DEF_OP(COS, "cos", 2, 1, {cos(number_2)}, "cos", LatexOperationTypes::PREFIX, "\\cos", true, false,
{
    return _MUL(NUM(-1), _MUL(d(node->right), _SIN(CPY(node->right))));
})

//------------------------------------------------------------------

DEF_OP(COT, "ctg", 2, 1, {1/tan(number_2)}, "1/tan", LatexOperationTypes::PREFIX, "\\cot", true, false,
{
    return _MUL(NUM(-1), _MUL(d(node->right), _DIV(NUM(1), _DEG(_SIN(node->right), NUM(2)))));
})

//------------------------------------------------------------------

DEF_OP(TAN, "tg", 2, 1, {tan(number_2)}, "tan", LatexOperationTypes::PREFIX, "\\tan", true, false,
{
    return _MUL(d(node->right), _DIV(NUM(1), _DEG(_COS(node->right), NUM(2))));
})

//==================================================================
//==================================================================

DEF_OP(ARCSIN, "arcsin", 2, 1, {asin(number_2)}, "asin", LatexOperationTypes::PREFIX, "\\arcsin", true, false,
{
    return _DEG(_SUB(NUM(1), _DEG(CPY(node->right), NUM(2))), NUM(-0.5));
})

//------------------------------------------------------------------

DEF_OP(ARCCOS, "arccos", 2, 1, {acos(number_2)}, "acos", LatexOperationTypes::PREFIX, "\\arccos", true, false,
{
    return _MUL(NUM(-1), _DEG(_SUB(NUM(1), _DEG(CPY(node->right), NUM(2))), NUM(-0.5)));
})

//------------------------------------------------------------------

DEF_OP(ARCCOT, "arcctg", 2, 1, {M_PI/2 - atan(number_2)}, "1.57 - atan", LatexOperationTypes::PREFIX, "\\arccot", true, false,
{
    return _DIV(NUM(-1), _ADD(NUM(1), _DEG(CPY(node->right), NUM(2))));
})

//------------------------------------------------------------------

DEF_OP(ARCTAN, "arctg", 2, 1, {atan(number_2)}, "atan", LatexOperationTypes::PREFIX, "\\arctan", true, false,
{
    return _DIV(NUM(1), _ADD(NUM(1), _DEG(CPY(node->right), NUM(2))));
})

