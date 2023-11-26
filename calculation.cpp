#include <math.h>
#include <string.h>

#include "calculation.h"

#include "operations_dsl.h"


// ======================================================================
// CALCULATIONS
// ======================================================================

static const double EPSILON  = 1e-9;

static double OperationWithTwoNumbers(const double number_1, const double number_2,
                                      const Operators operation, error_t* error);

static bool AreEqual(const double a, const double b);

static void RecalculateExpressionRightSubtree(expr_t* expr, Node* node, error_t* error);
static void RecalculateExpressionLeftSubtree(expr_t* expr, Node* node, error_t* error);
static void RecalculateExpressionSubtrees(expr_t* expr, Node* node, error_t* error);

// ======================================================================
// SIMPLIFYING
// ======================================================================

static void SimplifyExpressionConstants(expr_t* expr, Node* node, int* transform_amt, error_t* error);
static void SimplifyExpressionNeutrals(expr_t* expr, Node* node, int* transform_amt, error_t* error);

static void ReconnectNodesKidWithParent(expr_t* expr, Node* cur_node, Node* kid);

static void RemoveNeutralADD(expr_t* expr, Node* node, int* transform_cnt, error_t* error);
static void RemoveNeutralSUB(expr_t* expr, Node* node, int* transform_cnt, error_t* error);
static void RemoveNeutralDIV(expr_t* expr, Node* node, int* transform_cnt, error_t* error);
static void RemoveNeutralMUL(expr_t* expr, Node* node, int* transform_cnt, error_t* error);
static void RemoveNeutralDEG(expr_t* expr, Node* node, int* transform_cnt, error_t* error);

// ======================================================================
// DIFFERENTIATING
// ======================================================================

static Node* Copy(Node* node, error_t* error);

static Node* Differentiate(Node* node, const int id, error_t* error);

static inline Node* DifferentiateADD(Node* node, const int id, error_t* error);
static inline Node* DifferentiateSUB(Node* node, const int id, error_t* error);
static inline Node* DifferentiateMUL(Node* node, const int id, error_t* error);
static inline Node* DifferentiateDIV(Node* node, const int id, error_t* error);
static inline Node* DifferentiateDEG(Node* node, const int id, error_t* error);
static inline Node* DifferentiateEXP(Node* node, const int id, error_t* error);
static inline Node* DifferentiateLN(Node* node, const int id, error_t* error);
static inline Node* DifferentiateSIN(Node* node, const int id, error_t* error);
static inline Node* DifferentiateCOS(Node* node, const int id, error_t* error);
static inline Node* DifferentiateTAN(Node* node, const int id, error_t* error);
static inline Node* DifferentiateCOT(Node* node, const int id, error_t* error);
static inline Node* DifferentiateARCSIN(Node* node, const int id, error_t* error);
static inline Node* DifferentiateARCCOS(Node* node, const int id, error_t* error);
static inline Node* DifferentiateARCTAN(Node* node, const int id, error_t* error);
static inline Node* DifferentiateARCCOT(Node* node, const int id, error_t* error);

//------------------------------------------------------------------

static bool AreEqual(const double a, const double b)
{
    assert(isfinite(a));
    assert(isfinite(b));

    double diff = a - b;

    return (fabs(diff) < EPSILON) ? true : false;
}

//------------------------------------------------------------------

#define DEF_OP(name, symb, priority, action, ...)   \
            case (Operators::name):                 \
                return action;                      \

static double OperationWithTwoNumbers(const double number_1, const double number_2,
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

double CalculateExpression(expr_t* expr, Node* node, error_t* error)
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

    double left_result  = CalculateExpression(expr, node->left, error);
    double right_result = CalculateExpression(expr, node->right, error);

    if (node->type != NodeType::OPERATOR)
    {
        error->code = (int) ExpressionErrors::INVALID_EXPRESSION_FORMAT;
        return 0;
    }

    double result = OperationWithTwoNumbers(left_result, right_result, node->value.opt, error);

    if (error->code == (int) ExpressionErrors::NONE)
        return result;
    else
        return POISON;

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
    if (error->code != (int) ExpressionErrors::NONE) return;
    SimplifyExpressionConstants(expr, node->right, transform_cnt, error);
    if (error->code != (int) ExpressionErrors::NONE) return;

    if (node->left == nullptr)
    {
        if (node->right->type == NodeType::NUMBER)
        {
            RecalculateExpressionRightSubtree(expr, node, error);
            *transform_cnt++;
        }

        return;
    }

    if (node->right == nullptr)
    {
        if (node->left->type == NodeType::NUMBER)
        {
            RecalculateExpressionLeftSubtree(expr, node, error);
            *transform_cnt++;
        }

        return;
    }

    if (node->left->type == NodeType::NUMBER && node->right->type == NodeType::NUMBER)
    {
        RecalculateExpressionSubtrees(expr, node, error);
        *transform_cnt++;
        return;
    }
}

//------------------------------------------------------------------

static void RecalculateExpressionRightSubtree(expr_t* expr, Node* node, error_t* error)
{
    assert(expr);
    assert(error);

    double num = CalculateExpression(expr, node, error);
    if (error->code != (int) ExpressionErrors::NONE)
        return;

    DestructNodes(node->right);

    node->right     = nullptr;
    node->type      = NodeType::NUMBER;
    node->value.val = num;
}

//------------------------------------------------------------------

static void RecalculateExpressionLeftSubtree(expr_t* expr, Node* node, error_t* error)
{
    assert(expr);
    assert(error);

    double num = CalculateExpression(expr, node, error);
    if (error->code != (int) ExpressionErrors::NONE)
        return;

    DestructNodes(node->left);

    node->left      = nullptr;
    node->type      = NodeType::NUMBER;
    node->value.val = num;
}

//------------------------------------------------------------------

static void RecalculateExpressionSubtrees(expr_t* expr, Node* node, error_t* error)
{
    assert(expr);
    assert(error);

    double num = CalculateExpression(expr, node, error);
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

static void SimplifyExpressionNeutrals(expr_t* expr, Node* node, int* transform_cnt, error_t* error)
{
    assert(expr);
    assert(error);
    assert(transform_cnt);

    if (!node)
        return;
    if (node->left == nullptr && node->right == nullptr)
        return;

    SimplifyExpressionNeutrals(expr, node->left, transform_cnt, error);
    if (error->code != (int) ExpressionErrors::NONE) return;
    SimplifyExpressionNeutrals(expr, node->right, transform_cnt, error);
    if (error->code != (int) ExpressionErrors::NONE) return;

    if (node->type != NodeType::OPERATOR)
    {
        error->code = (int) ExpressionErrors::INVALID_EXPRESSION_FORMAT;
        return;
    }

    switch (node->value.opt)
    {
        case (Operators::ADD):
            RemoveNeutralADD(expr, node, transform_cnt, error);
            break;
        case (Operators::SUB):
            RemoveNeutralSUB(expr, node, transform_cnt, error);
            break;
        case (Operators::MUL):
            RemoveNeutralMUL(expr, node, transform_cnt, error);
            break;
        case (Operators::DIV):
            RemoveNeutralDIV(expr, node, transform_cnt, error);
            break;
        case (Operators::DEG):
            RemoveNeutralDEG(expr, node, transform_cnt, error);
            break;
        default:
            break;
    }
}

//------------------------------------------------------------------

static void ReconnectNodesKidWithParent(expr_t* expr, Node* cur_node, Node* kid)
{
    assert(expr);
    assert(cur_node);
    assert(kid);
    assert(cur_node->left == kid || cur_node->right == kid);

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
}

//------------------------------------------------------------------

static void RemoveNeutralADD(expr_t* expr, Node* node, int* transform_cnt, error_t* error)
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
        ReconnectNodesKidWithParent(expr, node, node->right);
        return;
    }

    if (node->right->type == NodeType::NUMBER && AreEqual(node->right->value.val, 0))
    {
        (*transform_cnt)++;

        DestructNodes(node->right);
        ReconnectNodesKidWithParent(expr, node, node->left);
    }
}

//------------------------------------------------------------------

static void RemoveNeutralSUB(expr_t* expr, Node* node, int* transform_cnt, error_t* error)
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
        ReconnectNodesKidWithParent(expr, node, node->left);
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

static void RemoveNeutralDIV(expr_t* expr, Node* node, int* transform_cnt, error_t* error)
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
        ReconnectNodesKidWithParent(expr, node, node->left);
    }
}

//------------------------------------------------------------------

static void RemoveNeutralMUL(expr_t* expr, Node* node, int* transform_cnt, error_t* error)
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
            ReconnectNodesKidWithParent(expr, node, node->right);
        }
        else if (AreEqual(node->left->value.val, 0))
        {
            (*transform_cnt)++;

            DestructNodes(node->right);
            ReconnectNodesKidWithParent(expr, node, node->left);
        }

        return;
    }

    if (node->right->type == NodeType::NUMBER)
    {
        if (AreEqual(node->right->value.val, 1))
        {
            (*transform_cnt)++;

            DestructNodes(node->right);
            ReconnectNodesKidWithParent(expr, node, node->left);
        }
        else if (AreEqual(node->right->value.val, 0))
        {
            (*transform_cnt)++;

            DestructNodes(node->left);
            ReconnectNodesKidWithParent(expr, node, node->right);
        }
    }
}

//------------------------------------------------------------------

static void RemoveNeutralDEG(expr_t* expr, Node* node, int* transform_cnt, error_t* error)
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

            node->right->type      = NodeType::NUMBER;
            node->right->value.val = 1;

            DestructNodes(node->left);
            ReconnectNodesKidWithParent(expr, node, node->right);
        }

        return;
    }

    if (node->right->type == NodeType::NUMBER)
    {
        if (AreEqual(node->right->value.val, 1))
        {
            (*transform_cnt)++;

            DestructNodes(node->right);
            ReconnectNodesKidWithParent(expr, node, node->left);
        }
        else if (AreEqual(node->right->value.val, 0))
        {
            (*transform_cnt)++;

            node->left->type      = NodeType::NUMBER;
            node->left->value.val = 1;

            DestructNodes(node->right);
            ReconnectNodesKidWithParent(expr, node, node->left);
        }
    }
}

//------------------------------------------------------------------

void SimplifyExpression(expr_t* expr, error_t* error)
{
    int cnt = 1;
    while (cnt != 0)
    {
        cnt = 0;
        SimplifyExpressionConstants(expr, expr->root, &cnt, error);
        if (error->code != (int) ExpressionErrors::NONE)
            return;

        SimplifyExpressionNeutrals(expr, expr->root, &cnt, error);
        if (error->code != (int) ExpressionErrors::NONE)
            return;
    }
}

//------------------------------------------------------------------

#define DEF_OP(name, ...)                                   \
        case (Operators::name):                             \
            return Differentiate##name(node, id, error);    \


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

// ::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

static inline Node* DifferentiateADD(Node* node, const int id, error_t* error)
{
    assert(node);

    return _ADD(d(node->left), d(node->right));
}

// ::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

static inline Node* DifferentiateSUB(Node* node, const int id, error_t* error)
{
    assert(node);

    return _SUB(d(node->left), d(node->right));
}

// ::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

static inline Node* DifferentiateMUL(Node* node, const int id, error_t* error)
{
    assert(node);

    return _ADD(_MUL(d(node->left), CPY(node->right)), _MUL(CPY(node->left), d(node->right)));;
}

// ::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

static inline Node* DifferentiateDIV(Node* node, const int id, error_t* error)
{
    assert(node);

    return _DIV(_SUB(_MUL(d(node->left), CPY(node->right)), _MUL(CPY(node->left), d(node->right))),
                _DEG(CPY(node->right), NUM(2)));
}

// ::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

static inline Node* DifferentiateDEG(Node* node, const int id, error_t* error)
{
    assert(node);

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
        return _MUL(CPY(node->right), _DEG(node->left, _SUB(CPY(node->right), NUM(1))));
    }
    else if (has_var_in_deg)
    {
        return _MUL(_LN(node->left), CPY(node->right));
    }
    else
        return NUM(0);
}

// ::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

static inline Node* DifferentiateEXP(Node* node, const int id, error_t* error)
{
    assert(node);

    return _MUL(d(node->right), CPY(node));
}

// ::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

static inline Node* DifferentiateLN(Node* node, const int id, error_t* error)
{
    assert(node);

    return _MUL(d(node->right), _DIV(NUM(1), CPY(node->right)));
}

// ::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

static inline Node* DifferentiateSIN(Node* node, const int id, error_t* error)
{
    assert(node);

    return _MUL(d(node->right), _COS(CPY(node->right)));
}

// ::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

static inline Node* DifferentiateCOS(Node* node, const int id, error_t* error)
{
    assert(node);

    return _MUL(NUM(-1), _MUL(d(node->right), _SIN(CPY(node->right))));
}

// ::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

static inline Node* DifferentiateTAN(Node* node, const int id, error_t* error)
{
    assert(node);

    return _MUL(d(node->right), _DIV(NUM(1), _DEG(_COS(node->right), NUM(2))));
}

// ::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

static inline Node* DifferentiateCOT(Node* node, const int id, error_t* error)
{
    assert(node);

    return _MUL(NUM(-1), _MUL(d(node->right), _DIV(NUM(1), _DEG(_SIN(node->right), NUM(2)))));
}

// ::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

static inline Node* DifferentiateARCSIN(Node* node, const int id, error_t* error)
{
    assert(node);

    return _DEG(_SUB(NUM(1), _DEG(CPY(node->right), NUM(2))), NUM(-0.5));
}

// ::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

static inline Node* DifferentiateARCCOS(Node* node, const int id, error_t* error)
{
    assert(node);

    return _MUL(NUM(-1), _DEG(_SUB(NUM(1), _DEG(CPY(node->right), NUM(2))), NUM(-0.5)));
}

// ::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

static inline Node* DifferentiateARCTAN(Node* node, const int id, error_t* error)
{
    assert(node);

    return _DIV(NUM(1), _ADD(NUM(1), _DEG(CPY(node->right), NUM(2))));
}

// ::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

static inline Node* DifferentiateARCCOT(Node* node, const int id, error_t* error)
{
    assert(node);

    return _DIV(NUM(-1), _ADD(NUM(1), _DEG(CPY(node->right), NUM(2))));
}


//------------------------------------------------------------------

static Node* Copy(Node* node, error_t* error)
{
    if (!node) return nullptr;

    return MakeNode(node->type, node->value, Copy(node->left, error), Copy(node->right, error), nullptr, error);
}

//------------------------------------------------------------------

expr_t* DifferentiateExpression(expr_t* expr, const char* var, error_t* error)
{
    assert(var);
    assert(expr);
    assert(error);

    int var_id = FindVariableAmongSaved(expr->vars, var);
    if (var_id == NO_VARIABLE)
    {
        error->code = (int) ExpressionErrors::NO_DIFF_VARIABLE;
        error->data = var;
        return nullptr;
    }

    expr_t* d_expr = MakeExpression(error);
    if (error->code != (int) ExpressionErrors::NONE)
        return nullptr;

    variable_t* vars = AllocVariablesArray(error);
    if (error->code != (int) ExpressionErrors::NONE)
        return nullptr;

    void* success = memcpy(vars, expr->vars, MAX_VARIABLES_AMT * sizeof(variable_t));
    if (!success)
    {
        error->code = (int) ExpressionErrors::ALLOCATE_MEMORY;
        error->data = "VARIABLES ARRAY";
        return nullptr;
    }

    SimplifyExpression(expr, error);

    Node* root = Differentiate(expr->root, var_id, error);
    if (error->code != (int) ExpressionErrors::NONE)
        return nullptr;

    ConnectNodesWithParents(root);

    d_expr->root = root;
    d_expr->vars = vars;

    SimplifyExpression(d_expr, error);

    return d_expr;
}
