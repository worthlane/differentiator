#include <math.h>

#include "calculation.h"

static int global_transformarion_counter = 0;

static double OperationWithTwoNumbers(const double number_1, const double number_2,
                                      const Operators operation, error_t* error);

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
    assert(node);

    if (node->left == nullptr || node->right == nullptr)
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

void SimplifyExpression(expr_t* expr, Node* node, error_t* error)
{
    if (node->left == nullptr || node->right == nullptr)
        return;

    if (node->left->left == nullptr && node->left->right == nullptr)
    {
        return;
    }
}
