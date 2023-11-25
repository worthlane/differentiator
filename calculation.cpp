#include <math.h>

#include "calculation.h"

static int global_transformarion_counter = 0;

static double OperationWithTwoNumbers(const double number_1, const double number_2,
                                      const Operators operation, error_t* error);

static void SimplifyExpressionConstants(expr_t* expr, Node* node, int* transform_amt, error_t* error);
static void SimplifyExpressionNeutrals(expr_t* expr, Node* node, int* transform_amt, error_t* error);

static void RecalculateExpressionRightSubtree(expr_t* expr, Node* node, error_t* error);
static void RecalculateExpressionLeftSubtree(expr_t* expr, Node* node, error_t* error);
static void RecalculateExpressionSubtrees(expr_t* expr, Node* node, error_t* error);

static void ReconnectNodesKidWithParent(expr_t* expr, Node* cur_node, Node* kid);

static bool AreEqual(const double a, const double b);

static const double EPSILON  = 1e-9;


static void RemoveNeutralADD(expr_t* expr, Node* node, int* transform_cnt, error_t* error);
static void RemoveNeutralSUB(expr_t* expr, Node* node, int* transform_cnt, error_t* error);
static void RemoveNeutralDIV(expr_t* expr, Node* node, int* transform_cnt, error_t* error);
static void RemoveNeutralMUL(expr_t* expr, Node* node, int* transform_cnt, error_t* error);
static void RemoveNeutralDEG(expr_t* expr, Node* node, int* transform_cnt, error_t* error);

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
            error->code = (int) ExpressionErrors::WRONG_EQUATION;
            return 0;
        }
    }

    double left_result  = CalculateExpression(expr, node->left, error);
    double right_result = CalculateExpression(expr, node->right, error);

    if (node->type != NodeType::OPERATOR)
    {
        error->code = (int) ExpressionErrors::WRONG_EQUATION;
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
        error->code = (int) ExpressionErrors::WRONG_EQUATION;
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
        error->code = (int) ExpressionErrors::WRONG_EQUATION;
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
        error->code = (int) ExpressionErrors::WRONG_EQUATION;
        return;
    }

    if (node->right->type == NodeType::NUMBER && AreEqual(node->right->value.val, 0))
    {
        (*transform_cnt)++;

        DestructNodes(node->right);
        ReconnectNodesKidWithParent(expr, node, node->left);
        return;
    }

    if ((node->right->type      == NodeType::NUMBER   &&
         node->left->type       == NodeType::NUMBER)  &&
         node->right->value.var == node->left->value.var);
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
        error->code = (int) ExpressionErrors::WRONG_EQUATION;
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
        error->code = (int) ExpressionErrors::WRONG_EQUATION;
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
        error->code = (int) ExpressionErrors::WRONG_EQUATION;
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
