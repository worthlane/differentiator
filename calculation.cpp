#include <math.h>
#include <string.h>

#include "calculation.h"
#include "expression/visual.h"
#include "expression/expr_input_and_output.h"
#include "common/input_and_output.h"

#ifdef PRINT_EXPR
#undef PRINT_EXPR
#endif
#define PRINT_EXPR(fp, expr)                              \
        if (fp != nullptr)                                      \
        {                                                   \
            PrintExpressionTreeLatex(fp, expr);                 \
        }

#ifdef PRINT_PRANK
#undef PRINT_PRANK
#endif
#define PRINT_PRANK(fp)                              \
        if (fp != nullptr)                                      \
        {                                                   \
            PrintPrankPhrase(fp);                           \
        }

#ifdef PRINT
#undef PRINT
#endif
#define PRINT(fp, string)                                       \
        if (fp != nullptr)                                      \
        {                                                   \
            fprintf(fp, string);                                 \
        }

// ======================================================================
// DSL
// ======================================================================

#ifdef d
#undef d
#endif
#define d(node)   Differentiate(node, id, error)

#ifdef CPY
#undef CPY
#endif
#define CPY(node) Copy(node)

#ifdef NUM
#undef NUM
#endif
#define NUM(num)  MakeNode(NodeType::NUMBER, {.val = num}, nullptr, nullptr, nullptr)

#ifdef VAR
#undef VAR
#endif
#define VAR(id)   MakeNode(NodeType::VARIABLE, {.var = id}, nullptr, nullptr, nullptr)

#define DEF_OP(name, symb, priority, arg_amt, ...)                                                                          \
                    static inline Node* _##name(Node* left, Node* right = nullptr)                                          \
                    {                                                                                                       \
                        if (arg_amt == 1)                                                                                   \
                        {                                                                                                   \
                            right = left;                                                                                   \
                            left  = nullptr;                                                                                \
                        }                                                                                                   \
                                                                                                                            \
                        Node* new_node = MakeNode(NodeType::OPERATOR, {.opt = Operators::name}, left, right, nullptr);      \
                                                                                                                            \
                        if (right != nullptr)                                                                               \
                            right->parent = new_node;                                                                       \
                                                                                                                            \
                        if (left != nullptr)                                                                                \
                            left->parent = new_node;                                                                        \
                                                                                                                            \
                        return new_node;                                                                                    \
                    }

#include "operations.h"

#undef DEF_OP

// ======================================================================
// CALCULATIONS
// ======================================================================

static const double EPSILON  = 1e-9;

static double CalculateExpressionSubtree(const expr_t* expr, Node* root, error_t* error);

static double OperatorAction(const double number_1, const double number_2,
                                      const Operators operation, error_t* error);

static bool AreEqual(const double a, const double b);

static void UniteExpressionSubtree(expr_t* expr, Node* node, error_t* error);

// ======================================================================
// SIMPLIFYING
// ======================================================================

static void SimplifyExpressionConstants(expr_t* expr, Node* node, int* transform_amt, error_t* error);
static void SimplifyExpressionNeutrals(expr_t* expr, Node* node, int* transform_amt, error_t* error, FILE* fp = nullptr);

static Node* ReplaceNodeWithItsKid(expr_t* expr, Node* cur_node, NodeKid kid_side);

static void RemoveNeutralADD(expr_t* expr, Node* node, int* transform_cnt, error_t* error, FILE* fp);
static void RemoveNeutralSUB(expr_t* expr, Node* node, int* transform_cnt, error_t* error, FILE* fp);
static void RemoveNeutralDIV(expr_t* expr, Node* node, int* transform_cnt, error_t* error, FILE* fp);
static void RemoveNeutralMUL(expr_t* expr, Node* node, int* transform_cnt, error_t* error, FILE* fp);
static void RemoveNeutralDEG(expr_t* expr, Node* node, int* transform_cnt, error_t* error, FILE* fp);

// ======================================================================
// DIFFERENTIATING
// ======================================================================

static Node* Copy(Node* node);

static Node*   Differentiate(Node* node, const int id, error_t* error);
static expr_t* DifferentiateExpression(expr_t* expr, const int var_id, error_t* error, FILE* fp);

static expr_t* MakeExpressionWithSameVar(expr_t* expr, const char* var, int* id, error_t* error);
static expr_t* MakeExpressionWithSameVar(expr_t* expr, error_t* error);

static inline int Factorial(const int n);

static void    CalculateLinearParams(expr_t* expr, const int var_id, double* tang, double* b,
                                     error_t* error, FILE* fp = nullptr);


//------------------------------------------------------------------

static bool AreEqual(const double a, const double b)
{
    assert(isfinite(a));
    assert(isfinite(b));

    double diff = a - b;

    return (fabs(diff) < EPSILON) ? true : false;
}

//------------------------------------------------------------------

#define DEF_OP(name, symb, priority, arg_amt, action, ...)      \
            case (Operators::name):                             \
                return action;                                  \

static double OperatorAction(const double number_1, const double number_2,
                                      const Operators operation, error_t* error)
{
    switch (operation)
    {
        #include "operations.h"
        default:
            error->code = (int) ExpressionErrors::UNKNOWN_OPERATION;
            return POISON;
    }
}

#undef DEF_OP

//------------------------------------------------------------------

static double CalculateExpressionSubtree(const expr_t* expr, Node* node, error_t* error)
{
    assert(expr);
    assert(error);

    if (!node) return 0;

    if (node->left == nullptr && node->right == nullptr)
    {
        if (node->type == NodeType::NUMBER)             return node->value.val;
        else if (node->type == NodeType::VARIABLE)      return expr->vars[node->value.var].value;
        else
        {
            error->code = (int) ExpressionErrors::INVALID_EXPRESSION_FORMAT;
            return 0;
        }
    }

    double left_result  = CalculateExpressionSubtree(expr, node->left, error);
    double right_result = CalculateExpressionSubtree(expr, node->right, error);

    if (node->type != NodeType::OPERATOR)
    {
        error->code = (int) ExpressionErrors::INVALID_EXPRESSION_FORMAT;
        return 0;
    }

    double result = OperatorAction(left_result, right_result, node->value.opt, error);

    if (error->code == (int) ExpressionErrors::NONE)
        return result;
    else
        return POISON;

}

//------------------------------------------------------------------

double CalculateExpression(const expr_t* expr, error_t* error)
{
    assert(error);
    assert(expr);

    return CalculateExpressionSubtree(expr, expr->root, error);
}

//------------------------------------------------------------------

static void SimplifyExpressionConstants(expr_t* expr, Node* node, int* transform_cnt, error_t* error)
{
    assert(expr);
    assert(error);
    assert(transform_cnt);

    if (!node)
        return;
    if (node->left == nullptr && node->right == nullptr)
        return;

    SimplifyExpressionConstants(expr, node->left, transform_cnt, error);
    if (error->code != (int) ExpressionErrors::NONE)
        return;

    SimplifyExpressionConstants(expr, node->right, transform_cnt, error);
    if (error->code != (int) ExpressionErrors::NONE)
        return;

    if (node->left == nullptr)
    {
        if (node->right->type == NodeType::NUMBER)
        {
            UniteExpressionSubtree(expr, node, error);
            *transform_cnt++;
        }

        return;
    }

    if (node->right == nullptr)
    {
        if (node->left->type == NodeType::NUMBER)
        {
            UniteExpressionSubtree(expr, node, error);
            *transform_cnt++;
        }

        return;
    }

    if (node->left->type == NodeType::NUMBER && node->right->type == NodeType::NUMBER)
    {
        UniteExpressionSubtree(expr, node, error);
        *transform_cnt++;
        return;
    }
}

//------------------------------------------------------------------

static void UniteExpressionSubtree(expr_t* expr, Node* node, error_t* error)
{
    assert(expr);
    assert(error);

    double num = CalculateExpressionSubtree(expr, node, error);
    if (error->code != (int) ExpressionErrors::NONE)
        return;

    DestructNodes(node->left);
    DestructNodes(node->right);

    node->left      = nullptr;
    node->right     = nullptr;
    node->type      = NodeType::NUMBER;
    node->value.val = num;
}

//------------------------------------------------------------------

static void SimplifyExpressionNeutrals(expr_t* expr, Node* node, int* transform_cnt, error_t* error, FILE* fp)
{
    assert(expr);
    assert(error);
    assert(transform_cnt);

    if (!node)
        return;
    if (node->left == nullptr && node->right == nullptr)
        return;

    SimplifyExpressionNeutrals(expr, node->left, transform_cnt, error, fp);
    if (error->code != (int) ExpressionErrors::NONE)
        return;
    SimplifyExpressionNeutrals(expr, node->right, transform_cnt, error, fp);
    if (error->code != (int) ExpressionErrors::NONE)
        return;

    if (node->type != NodeType::OPERATOR)
    {
        error->code = (int) ExpressionErrors::INVALID_EXPRESSION_FORMAT;
        return;
    }

    switch (node->value.opt)
    {
        case (Operators::ADD):
            RemoveNeutralADD(expr, node, transform_cnt, error, fp);
            break;
        case (Operators::SUB):
            RemoveNeutralSUB(expr, node, transform_cnt, error, fp);
            break;
        case (Operators::MUL):
            RemoveNeutralMUL(expr, node, transform_cnt, error, fp);
            break;
        case (Operators::DIV):
            RemoveNeutralDIV(expr, node, transform_cnt, error, fp);
            break;
        case (Operators::DEG):
            RemoveNeutralDEG(expr, node, transform_cnt, error, fp);
            break;
        default:
            break;
    }
}

//------------------------------------------------------------------

static Node* ReplaceNodeWithItsKid(expr_t* expr, Node* cur_node, NodeKid kid_side)
{
    assert(expr);
    assert(cur_node);

    Node* kid = nullptr;

    if (kid_side == NodeKid::LEFT)
        kid = cur_node->left;
    else
        kid = cur_node->right;

    assert(kid);

    if (cur_node->parent != nullptr)
        assert(cur_node->parent->left == cur_node || cur_node->parent->right == cur_node);

    kid->parent = cur_node->parent;

    if (cur_node->parent != nullptr)
    {
        if (cur_node->parent->left  == cur_node)
            cur_node->parent->left  = kid;
        else
            cur_node->parent->right = kid;
    }
    else
    {
        expr->root = kid;
    }

    cur_node->right = nullptr;
    cur_node->left  = nullptr;

    NodeDtor(cur_node);

    return kid->parent;
}

//------------------------------------------------------------------

static void RemoveNeutralADD(expr_t* expr, Node* node, int* transform_cnt, error_t* error, FILE* fp)
{
    assert(expr);
    assert(error);
    assert(node);
    assert(transform_cnt);

    if (node->left == nullptr || node->right == nullptr)
    {
        error->code = (int) ExpressionErrors::INVALID_EXPRESSION_FORMAT;
        return;
    }

    if (node->left->type == NodeType::NUMBER && AreEqual(node->left->value.val, 0))
    {
        (*transform_cnt)++;

        DestructNodes(node->left);
        ReplaceNodeWithItsKid(expr, node, NodeKid::RIGHT);

        return;
    }

    if (node->right->type == NodeType::NUMBER && AreEqual(node->right->value.val, 0))
    {
        (*transform_cnt)++;

        DestructNodes(node->right);
        ReplaceNodeWithItsKid(expr, node, NodeKid::LEFT);
    }
}

//------------------------------------------------------------------

static void RemoveNeutralSUB(expr_t* expr, Node* node, int* transform_cnt, error_t* error, FILE* fp)
{
    assert(expr);
    assert(error);
    assert(node);
    assert(transform_cnt);

    if (node->left == nullptr || node->right == nullptr)
    {
        error->code = (int) ExpressionErrors::INVALID_EXPRESSION_FORMAT;
        return;
    }

    if (node->right->type == NodeType::NUMBER && AreEqual(node->right->value.val, 0))
    {
        (*transform_cnt)++;

        DestructNodes(node->right);
        ReplaceNodeWithItsKid(expr, node, NodeKid::LEFT);

        return;
    }


    if ((node->right->type      == NodeType::VARIABLE   &&
         node->left->type       == NodeType::VARIABLE)  &&
         node->right->value.var == node->left->value.var)
    {
        (*transform_cnt)++;

        DestructNodes(node->right);
        DestructNodes(node->left);

        node->type      = NodeType::NUMBER;
        node->value.val = 0;
        node->left      = nullptr;
        node->right     = nullptr;
    }
}

//------------------------------------------------------------------

static void RemoveNeutralDIV(expr_t* expr, Node* node, int* transform_cnt, error_t* error, FILE* fp)
{
    assert(expr);
    assert(error);
    assert(node);
    assert(transform_cnt);

    if (node->left == nullptr || node->right == nullptr)
    {
        error->code = (int) ExpressionErrors::INVALID_EXPRESSION_FORMAT;
        return;
    }

    if (node->right->type == NodeType::NUMBER && AreEqual(node->right->value.val, 1))
    {
        (*transform_cnt)++;

        DestructNodes(node->right);
        ReplaceNodeWithItsKid(expr, node, NodeKid::LEFT);

        return;
    }
}

//------------------------------------------------------------------

static void RemoveNeutralMUL(expr_t* expr, Node* node, int* transform_cnt, error_t* error, FILE* fp)
{
    assert(expr);
    assert(error);
    assert(node);
    assert(transform_cnt);

    if (node->left == nullptr || node->right == nullptr)
    {
        error->code = (int) ExpressionErrors::INVALID_EXPRESSION_FORMAT;
        return;
    }

    if (node->left->type == NodeType::NUMBER)
    {
        if (AreEqual(node->left->value.val, 1))
        {
            (*transform_cnt)++;

            DestructNodes(node->left);
            ReplaceNodeWithItsKid(expr, node, NodeKid::RIGHT);
        }
        else if (AreEqual(node->left->value.val, 0))
        {
            (*transform_cnt)++;

            DestructNodes(node->right);
            ReplaceNodeWithItsKid(expr, node, NodeKid::LEFT);
        }

        return;
    }

    if (node->right->type == NodeType::NUMBER)
    {
        if (AreEqual(node->right->value.val, 1))
        {
            (*transform_cnt)++;

            DestructNodes(node->right);
            ReplaceNodeWithItsKid(expr, node, NodeKid::LEFT);
        }
        else if (AreEqual(node->right->value.val, 0))
        {
            (*transform_cnt)++;

            DestructNodes(node->left);
            ReplaceNodeWithItsKid(expr, node, NodeKid::RIGHT);
        }
    }
}

//------------------------------------------------------------------

static void RemoveNeutralDEG(expr_t* expr, Node* node, int* transform_cnt, error_t* error, FILE* fp)
{
    assert(expr);
    assert(error);
    assert(node);
    assert(transform_cnt);

    if (node->left == nullptr || node->right == nullptr)
    {
        error->code = (int) ExpressionErrors::INVALID_EXPRESSION_FORMAT;
        return;
    }

    if (node->left->type == NodeType::NUMBER)
    {
        if (AreEqual(node->left->value.val, 1))
        {
            (*transform_cnt)++;

            DestructNodes(node->right);
            DestructNodes(node->left);

            node->left      = nullptr;
            node->right     = nullptr;
            node->type      = NodeType::NUMBER;
            node->value.val = 1;
        }

        return;
    }

    if (node->right->type == NodeType::NUMBER)
    {
        if (AreEqual(node->right->value.val, 1))
        {
            (*transform_cnt)++;

            DestructNodes(node->right);
            ReplaceNodeWithItsKid(expr, node, NodeKid::LEFT);
        }
        else if (AreEqual(node->right->value.val, 0))
        {
            (*transform_cnt)++;

            DestructNodes(node->right);
            DestructNodes(node->left);

            node->left      = nullptr;
            node->right     = nullptr;
            node->type      = NodeType::NUMBER;
            node->value.val = 1;
        }
    }
}

//------------------------------------------------------------------

void SimplifyExpression(expr_t* expr, error_t* error, FILE* fp)
{
    assert(expr);
    assert(error);

    PRINT_EXPR(fp, expr);
    PRINT(fp, "Lets simplify this expression.\n");

    int cnt = 1;
    while (cnt != 0)
    {
        cnt = 0;
        SimplifyExpressionConstants(expr, expr->root, &cnt, error);
        if (error->code != (int) ExpressionErrors::NONE)
            return;

        int save_cnt = cnt;

        if (cnt != 0)
        {
            PRINT_PRANK(fp);
            PRINT_EXPR(fp, expr);
        }

        SimplifyExpressionNeutrals(expr, expr->root, &cnt, error, fp);
        if (error->code != (int) ExpressionErrors::NONE)
            return;

        if (cnt != save_cnt)
        {
            PRINT_PRANK(fp);
            PRINT_EXPR(fp, expr);
        }
    }
}

//------------------------------------------------------------------

#define DEF_OP(name, symb, priority, arg_amt, action, gnu_symb, type, tex_symb, need_brackets, figure_brackets, diff, ...)   \
        case (Operators::name):                                                                                     \
        {                                                                                                           \
            assert(node);                                                                                           \
            diff;                                                                                                   \
                                                                                                                    \
            break;                                                                                                  \
        }                                                                                                           \


static Node* Differentiate(Node* node, const int id, error_t* error)
{
    assert(error);

    if (!node)  return nullptr;

    if (node->type == NodeType::POISON)
    {
        error->code = (int) ExpressionErrors::INVALID_EXPRESSION_FORMAT;
        return nullptr;
    }

    if (node->type == NodeType::NUMBER ||
       (node->type == NodeType::VARIABLE && node->value.var != id))
        return NUM(0);

    if (node->type == NodeType::VARIABLE)
        return NUM(1);

    switch (node->value.opt)
    {
        #include "operations.h"

        default:
            return nullptr;
    }

    return nullptr;
}

#undef DEF_OP

//------------------------------------------------------------------

static Node* Copy(Node* node)
{
    if (!node) return nullptr;

    Node* left  = Copy(node->left);
    Node* right = Copy(node->right);

    Node* new_node = MakeNode(node->type, node->value, left, right, nullptr);

    if (right != nullptr)
        right->parent = new_node;

    if (left != nullptr)
        left->parent = new_node;

    return new_node;
}

//------------------------------------------------------------------

static expr_t* MakeExpressionWithSameVar(expr_t* expr, const char* var, int* id, error_t* error)
{
    assert(expr);
    assert(var);
    assert(id);
    assert(error);

    int var_id = FindVariableAmongSaved(expr->vars, var);

    expr_t* d_expr = MakeExpression(error);
    if (error->code != (int) ExpressionErrors::NONE)
        return nullptr;

    CopyVariablesArray(expr->vars, d_expr->vars, error);
    if (error->code != (int) ExpressionErrors::NONE)
        return nullptr;

    *id = var_id;

    return d_expr;
}

//:::::::::::::::::::::::::::::::::::::::::::::::::::::

static expr_t* MakeExpressionWithSameVar(expr_t* expr, error_t* error)
{
    assert(expr);
    assert(error);

    expr_t* d_expr = MakeExpression(error);
    if (error->code != (int) ExpressionErrors::NONE)
        return nullptr;

    CopyVariablesArray(expr->vars, d_expr->vars, error);
    if (error->code != (int) ExpressionErrors::NONE)
        return nullptr;

    return d_expr;
}

//------------------------------------------------------------------

expr_t* DifferentiateExpression(expr_t* expr, const char* var, error_t* error, FILE* fp)
{
    assert(var);
    assert(expr);
    assert(error);

    PRINT_EXPR(fp, expr);
    PRINT(fp, "LET'S DIFFERENTIATE THIS!!!\n");

    int     var_id = NO_VARIABLE;
    expr_t* d_expr = MakeExpressionWithSameVar(expr, var, &var_id, error);
    if (error->code != (int) ExpressionErrors::NONE)
        return nullptr;

    Node* root = Differentiate(expr->root, var_id, error);
    if (error->code != (int) ExpressionErrors::NONE)
        return nullptr;

    d_expr->root = root;

    PRINT_PRANK(fp);

    SimplifyExpression(d_expr, error, fp);

    return d_expr;
}

//:::::::::::::::::::::::::::::::::::::::::::::::::::::::

static expr_t* DifferentiateExpression(expr_t* expr, const int var_id, error_t* error, FILE* fp)
{
    assert(expr);
    assert(error);

    PRINT(fp, "After differentiation:\n");

    expr_t* d_expr  = MakeExpressionWithSameVar(expr, error);
    if (error->code != (int) ExpressionErrors::NONE)
        return nullptr;

    Node* root = Differentiate(expr->root, var_id, error);
    if (error->code != (int) ExpressionErrors::NONE)
        return nullptr;

    d_expr->root = root;

    PRINT_EXPR(fp, d_expr);

    SimplifyExpression(d_expr, error, fp);

    return d_expr;
}

//------------------------------------------------------------------

static inline int Factorial(const int n)
{
    if (!n) return 1;

    if (n < 0) return -1;

    int result = 1;

    for (int i = 2; i <= n; i++)
    {
        result *= i;
    }

    return result;
}

//------------------------------------------------------------------

expr_t* TaylorSeries(expr_t* expr, const int n, const char* var, const double val, error_t* error, FILE* fp)
{
    assert(var);
    assert(expr);
    assert(error);

    PRINT(fp, "Lets find taylor series of:\n");
    PRINT_EXPR(fp, expr);

    int     var_id    = NO_VARIABLE;
    expr_t* new_expr = MakeExpressionWithSameVar(expr, var, &var_id, error);
    if (error->code != (int) ExpressionErrors::NONE)
        return nullptr;

    double prev_val          = expr->vars[var_id].value;
    expr->vars[var_id].value = val;

    expr_t* diff_expr     = expr;
    double  calc          = CalculateExpressionSubtree(diff_expr, diff_expr->root, error);
    Node*   taylor_series = NUM(0);

    for (int i = 0; i <= n; i++)
    {
        taylor_series = _ADD(taylor_series,
                             _MUL(_DIV(NUM(calc), NUM((double) Factorial(i))),
                                  _DEG(_SUB(VAR(var_id), NUM(expr->vars[var_id].value)),
                                       NUM((double) i))));

        PRINT(fp, "We need to differentiate this:\n");
        PRINT_EXPR(fp, diff_expr);

        diff_expr = DifferentiateExpression(diff_expr, var_id, error, fp);
        if (error->code != (int) ExpressionErrors::NONE)
            return nullptr;

        calc = CalculateExpressionSubtree(diff_expr, diff_expr->root, error);
        if (error->code != (int) ExpressionErrors::NONE)
            return nullptr;
    }

    new_expr->root = taylor_series;

    SimplifyExpression(new_expr, error, fp);

    expr->vars[var_id].value = prev_val;

    return new_expr;
}

//------------------------------------------------------------------

expr_t* GetExpressionsDifference(expr_t* expr_1, expr_t* expr_2, error_t* error, FILE* fp)
{
    assert(expr_2);
    assert(expr_1);
    assert(error);

    PRINT(fp, "Lets find out difference between:\n");
    PRINT_EXPR(fp, expr_1);
    PRINT(fp, "and\n");
    PRINT_EXPR(fp, expr_2);

    expr_t* new_expr = MakeExpression(error);
    if (error->code != (int) ExpressionErrors::NONE)
        return nullptr;

    variable_t* new_vars = MakeVariablesArray(error, expr_1->max_vars_amt);
    if (error->code != (int) ExpressionErrors::NONE)
        return nullptr;

    CopyVariablesArray(expr_1->vars, new_vars, error);
    if (error->code != (int) ExpressionErrors::NONE)
        return nullptr;

    Node* root = _SUB(CPY(expr_1->root), CPY(expr_2->root));

    new_expr->root = root;
    new_expr->vars = new_vars;

    PRINT_PRANK(fp);
    PRINT_EXPR(fp, new_expr);

    SimplifyExpression(new_expr, error, fp);

    return new_expr;
}

//------------------------------------------------------------------

expr_t* GetTangent(expr_t* expr, const char* var, const double val, error_t* error, FILE* fp)
{
    assert(var);
    assert(expr);
    assert(error);

    PRINT_EXPR(fp, expr);
    PRINT(fp, "Lets find tangent!\n");

    int         var_id   = NO_VARIABLE;
    expr_t*     new_expr = MakeExpressionWithSameVar(expr, var, &var_id, error);
    if (error->code != (int) ExpressionErrors::NONE)
        return nullptr;

    double prev_val          = expr->vars[var_id].value;
    expr->vars[var_id].value = val;

    // tangent: func_val = tang * var + b

    double tang     = POISON;
    double b        = POISON;

    CalculateLinearParams(expr, var_id, &tang, &b, error, fp);
    if (error->code != (int) ExpressionErrors::NONE)
        return nullptr;

    Node* root = _ADD(NUM(b), _MUL(VAR(var_id), NUM(tang)));

    new_expr->root = root;

    PRINT_PRANK(fp);
    PRINT_EXPR(fp, new_expr);

    expr->vars[var_id].value = prev_val;

    return new_expr;
}

//------------------------------------------------------------------

static void CalculateLinearParams(expr_t* expr, const int var_id, double* tang, double* b,
                                  error_t* error, FILE* fp)
{
    assert(tang);
    assert(b);

    PRINT(fp, "We must differentiate expression to find tangent parameters.\n")

    expr_t* d_expr   = DifferentiateExpression(expr, var_id, error, fp);
    if (error->code != (int) ExpressionErrors::NONE) return;

    PrintExpressionTree(stdout, d_expr);

    double  tan      = CalculateExpressionSubtree(d_expr, d_expr->root, error);
    if (error->code != (int) ExpressionErrors::NONE) return;

    double  func_val = CalculateExpressionSubtree(expr, expr->root, error);
    if (error->code != (int) ExpressionErrors::NONE) return;

    *b    = func_val - (tan * expr->vars[var_id].value);
    *tang = tan;

    printf("%lg %lg\n", *b, *tang);

    ExpressionDtor(d_expr);

    return;
}

