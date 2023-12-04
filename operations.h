#ifndef DEF_OP
#define DEF_OP(...) ;
#endif

// ======================================================================
// FORMAT
// ======================================================================

// DEF_OP(NAME, INPUT_SYMBOL, PRIORITY, ARGUMENTS_AMOUNT, ACTION, GNUPLOT_SYMBOL, TEX_OUTPUT_TYPE, TEX_SYMB,
//        NEED_LEFT_BRACKETS, LEFT_FIGURE_BRACKET, NEED_RIGHT_BRACKETS, RIGHT_FIGURE_BRACKET,
//        {
//              DIFFERENTIATED OPERATION
//        })

// ======================================================================
// OPERATIONS
// ======================================================================


DEF_OP(ADD, "+", 1, 2, (NUMBER_1 + NUMBER_2), "+", LatexOperationTypes::INFIX, "+", false, false, false, false,
{
    return _ADD(d(node->left), d(node->right));
})

//------------------------------------------------------------------

DEF_OP(SUB, "-", 1, 2, (NUMBER_1 - NUMBER_2), "-", LatexOperationTypes::INFIX, "-", false, false, false, false,
{
    return _SUB(d(node->left), d(node->right));
})

//------------------------------------------------------------------

DEF_OP(DIV, "/", 2, 2, (NUMBER_1 / NUMBER_2), "/", LatexOperationTypes::PREFIX, "\\frac", true, true, true, true,
{
    return _DIV(_SUB(_MUL(d(node->left), CPY(node->right)), _MUL(CPY(node->left), d(node->right))),
                _DEG(CPY(node->right), _NUM(2)));
})

//------------------------------------------------------------------

DEF_OP(MUL, "*", 2, 2, (NUMBER_1 * NUMBER_2), "*", LatexOperationTypes::INFIX, "\\cdot", false, false, false, false,
{
    return _ADD(_MUL(d(node->left), CPY(node->right)), _MUL(CPY(node->left), d(node->right)));;
})

//------------------------------------------------------------------

DEF_OP(DEG, "^", 2, 2, (pow(NUMBER_1, NUMBER_2)), "**", LatexOperationTypes::INFIX, "^", false, false, true, true,
{
    bool has_var_in_base = IsVarInTree(node->left, id);
    bool has_var_in_deg  = IsVarInTree(node->right, id);

    if (has_var_in_base && has_var_in_deg)
    {
        return _MUL(_ADD(_MUL(d(node->right), _LN(CPY(node->left))),
                         _MUL(CPY(node->right), _DIV(d(node->left), CPY(node->left)))),
                    CPY(node));
    }
    else if (has_var_in_base)
    {
        return _MUL(d(node->left), _MUL(CPY(node->right), _DEG(CPY(node->left), _SUB(CPY(node->right), _NUM(1)))));
    }
    else if (has_var_in_deg)
    {
        return _MUL(_LN(CPY(node->left)), CPY(node));
    }
    else
        return _NUM(0);
})

//==================================================================
//==================================================================

DEF_OP(LN, "ln", 2, 1, (log(NUMBER_2)), "log", LatexOperationTypes::PREFIX, "\\ln", false, false, true, false,
{
    return _MUL(d(node->right), _DIV(_NUM(1), CPY(node->right)));
})

//------------------------------------------------------------------

DEF_OP(EXP, "exp", 2, 1, (exp(NUMBER_2)), "exp", LatexOperationTypes::PREFIX, "\\exp", false, false, true, false,
{
    return _MUL(d(node->right), CPY(node));
})

//==================================================================
//==================================================================

DEF_OP(SIN, "sin", 2, 1, (sin(NUMBER_2)), "sin", LatexOperationTypes::PREFIX, "\\sin", false, false, true, false,
{
    return _MUL(d(node->right), _COS(CPY(node->right)));
})

//------------------------------------------------------------------

DEF_OP(COS, "cos", 2, 1, (cos(NUMBER_2)), "cos", LatexOperationTypes::PREFIX, "\\cos", false, false, true, false,
{
    return _MUL(_NUM(-1), _MUL(d(node->right), _SIN(CPY(node->right))));
})

//------------------------------------------------------------------

DEF_OP(COT, "ctg", 2, 1, (1/tan(NUMBER_2)), "1/tan", LatexOperationTypes::PREFIX, "\\cot", false, false, true, false,
{
    return _MUL(_NUM(-1), _MUL(d(node->right), _DIV(_NUM(1), _DEG(_SIN(CPY(node->right)), _NUM(2)))));
})

//------------------------------------------------------------------

DEF_OP(TAN, "tg", 2, 1, (tan(NUMBER_2)), "tan", LatexOperationTypes::PREFIX, "\\tan", false, false, true, false,
{
    return _MUL(d(node->right), _DIV(_NUM(1), _DEG(_COS(CPY(node->right)), _NUM(2))));
})

//==================================================================
//==================================================================

DEF_OP(ARCSIN, "arcsin", 2, 1, (asin(NUMBER_2)), "asin", LatexOperationTypes::PREFIX, "\\arcsin", false, false, true, false,
{
    return _DEG(_SUB(_NUM(1), _DEG(CPY(node->right), _NUM(2))), _NUM(-0.5));
})

//------------------------------------------------------------------

DEF_OP(ARCCOS, "arccos", 2, 1, (acos(NUMBER_2)), "acos", LatexOperationTypes::PREFIX, "\\arccos", false, false, true, false,
{
    return _MUL(_NUM(-1), _DEG(_SUB(_NUM(1), _DEG(CPY(node->right), _NUM(2))), _NUM(-0.5)));
})

//------------------------------------------------------------------

DEF_OP(ARCCOT, "arcctg", 2, 1, (M_PI/2 - atan(NUMBER_2)), "pi/2 - atan", LatexOperationTypes::PREFIX, "\\arccot", false, false, true, false,
{
    return _DIV(_NUM(-1), _ADD(_NUM(1), _DEG(CPY(node->right), _NUM(2))));
})

//------------------------------------------------------------------

DEF_OP(ARCTAN, "arctg", 2, 1, (atan(NUMBER_2)), "atan", LatexOperationTypes::PREFIX, "\\arctan", false, false, true, false,
{
    return _DIV(_NUM(1), _ADD(_NUM(1), _DEG(CPY(node->right), _NUM(2))));
})

