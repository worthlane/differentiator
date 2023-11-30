#include <assert.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

#include "expression.h"
#include "visual.h"
#include "common/input_and_output.h"
#include "common/file_read.h"
#include "common/colorlib.h"

static ExpressionErrors  VerifyNodes(const Node* node, error_t* error);

// ======================================================================
// EXPRESSION VARIABLES
// ======================================================================

static void InitVariablesArray(variable_t* variables, const size_t size);

//-----------------------------------------------------------------------------------------------------

variable_t* MakeVariablesArray(error_t* error, const size_t size)
{
    variable_t* vars = (variable_t*) calloc(size, sizeof(variable_t));
    if (vars == nullptr)
    {
        error->code = (int) ExpressionErrors::ALLOCATE_MEMORY;
        error->data = "VARIABLES ARRAY";
        return nullptr;
    }
    InitVariablesArray(vars, size);

    return vars;
}


//------------------------------------------------------------------

static void InitVariablesArray(variable_t* variables, const size_t size)
{
    assert(variables);

    for (size_t i = 0; i < size; i++)
    {
        variables[i].isfree        = true;
        variables[i].variable_name = "";
        variables[i].value         = 0;
    }
}

//------------------------------------------------------------------

void DestructVariablesArray(variable_t* variables, const size_t size)
{
    assert(variables);

    for (size_t i = 0; i < size; i++)
    {
        if (variables[i].isfree == false)
            free(variables[i].variable_name);

        variables[i].isfree = true;
        variables[i].value  = 0;
    }
}

//------------------------------------------------------------------

void CopyVariablesArray(const variable_t* vars, variable_t* dest, error_t* error)
{
    assert(vars);
    assert(dest);
    assert(error);

    for (size_t i = 0; i < MAX_VARIABLES_AMT; i++)
    {
        char* name = strndup(vars[i].variable_name, MAX_VARIABLE_LEN);
        if (!name)
        {
            error->code = (int) ExpressionErrors::ALLOCATE_MEMORY;
            error->data = "COPYING VARS";
            return;
        }

        dest[i].variable_name = vars[i].variable_name;
        dest[i].isfree        = vars[i].isfree;
        dest[i].value         = vars[i].value;
    }
}

//-----------------------------------------------------------------------------------------------------

bool IsVarInTree(Node* node, const int id)
{
    if (!node)
        return false;

    if (node->type == NodeType::VARIABLE && node->value.var == id)
        return true;

    bool var_in_left_subtree  = IsVarInTree(node->left, id);
    bool var_in_right_subtree = IsVarInTree(node->right, id);

    return (var_in_left_subtree || var_in_right_subtree);
}

//-----------------------------------------------------------------------------------------------------

int FindVariableAmongSaved(variable_t* vars, const char* new_var)
{
    assert(new_var);

    int id = NO_VARIABLE;

    for (int i = 0; i < MAX_VARIABLES_AMT; i++)
    {
        if (!vars[i].isfree)
        {
            if (!strncmp(new_var, vars[i].variable_name, MAX_VARIABLE_LEN))
            {
                id = i;
                return id;
            }
        }
    }

    return id;
}

//-----------------------------------------------------------------------------------------------------

int SaveVariable(variable_t* vars, const char* new_var)
{
    assert(new_var);

    for (int i = 0; i < MAX_VARIABLES_AMT; i++)
    {
        if (vars[i].isfree)
        {
            char* name = strndup(new_var, MAX_VARIABLE_LEN);
            if (!name)  return NO_VARIABLE;

            vars[i].variable_name = name;
            vars[i].isfree        = false;
            return i;
        }
    }

    return NO_VARIABLE;
}

//-----------------------------------------------------------------------------------------------------

Node* MakeNode(const NodeType type, const NodeValue value,
               Node* left, Node* right, Node* parent)
{
    Node* node = (Node*) calloc(1, sizeof(Node));
    if (node == nullptr)
        return nullptr;

    node->type   = type;
    node->value  = value;
    node->parent = parent;
    node->left   = left;
    node->right  = right;

    return node;
}

//-----------------------------------------------------------------------------------------------------

void NodeDtor(Node* node)
{
    assert(node);

    free(node);
}

//-----------------------------------------------------------------------------------------------------

void LinkNodesWithParents(Node* node)
{
    if (!node) return;

    LinkNodesWithParents(node->left);
    LinkNodesWithParents(node->right);

    if (node->left != nullptr)
        node->left->parent = node;

    if (node->right != nullptr)
        node->right->parent = node;

}

//-----------------------------------------------------------------------------------------------------

ExpressionErrors ExpressionCtor(expr_t* expr, error_t* error)
{
    Node* root = MakeNode(PZN_TYPE, ZERO_VALUE, nullptr, nullptr, nullptr);

    variable_t* vars = MakeVariablesArray(error, MAX_VARIABLES_AMT);
    RETURN_IF_EXPRESSION_ERROR((ExpressionErrors) error->code);

    expr->vars         = vars;
    expr->root         = root;
    expr->max_vars_amt = MAX_VARIABLES_AMT;

    return ExpressionErrors::NONE;
}

// :::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

ExpressionErrors ExpressionCtor(expr_t* expr, const size_t size, error_t* error)
{
    Node* root = MakeNode(PZN_TYPE, ZERO_VALUE, nullptr, nullptr, nullptr);

    variable_t* vars = MakeVariablesArray(error, size);
    RETURN_IF_EXPRESSION_ERROR((ExpressionErrors) error->code);

    expr->vars         = vars;
    expr->root         = root;
    expr->max_vars_amt = size;

    return ExpressionErrors::NONE;
}

//-----------------------------------------------------------------------------------------------------

expr_t* MakeExpression(error_t* error)
{
    assert(error);

    expr_t* expr = (expr_t*) calloc(1, sizeof(expr_t));
    if (expr == nullptr)
    {
        error->code = (int) ExpressionErrors::ALLOCATE_MEMORY;
        error->data = "EXPRESSION STRUCT";
        return nullptr;
    }

    ExpressionCtor(expr, error);
    if (error->code != (int) ExpressionErrors::NONE)
        return nullptr;

    return expr;
}


//-----------------------------------------------------------------------------------------------------

void ExpressionDtor(expr_t* expr)
{
    DestructNodes(expr->root);

    DestructVariablesArray(expr->vars, expr->max_vars_amt);
    free(expr->vars);
    expr->max_vars_amt = 0;
    expr->root         = nullptr;
}

//-----------------------------------------------------------------------------------------------------

void DestructNodes(Node* root)
{
    if (!root) return;

    if (root->left != nullptr)
        DestructNodes(root->left);

    if (root->right != nullptr)
        DestructNodes(root->right);

    NodeDtor(root);
}

//-----------------------------------------------------------------------------------------------------

int PrintExpressionError(FILE* fp, const void* err, const char* func, const char* file, const int line)
{
    assert(err);

    LOG_START(func, file, line);

    const struct ErrorInfo* error = (const struct ErrorInfo*) err;

    switch ((ExpressionErrors) error->code)
    {
        case (ExpressionErrors::NONE):
            LOG_END();
            return (int) error->code;

        case (ExpressionErrors::ALLOCATE_MEMORY):
            fprintf(fp, "CAN NOT ALLOCATE MEMORY FOR %s<br>\n", (const char*) error->data);
            LOG_END();
            return (int) error->code;

        case (ExpressionErrors::NO_EXPRESSION):
            fprintf(fp, "EXPRESSION TREE IS EMPTY<br>\n");
            LOG_END();
            return (int) error->code;

        case (ExpressionErrors::INVALID_SYNTAX):
            fprintf(fp, "UNKNOWN INPUT<br>\n");
            LOG_END();
            return (int) error->code;

        case (ExpressionErrors::CYCLED_NODE):
            fprintf(fp, "NODE ID IT'S OWN PREDECESSOR<br>\n");
            LOG_END();
            return (int) error->code;

        case (ExpressionErrors::COMMON_HEIR):
            fprintf(fp, "NODE'S HEIRS ARE SAME<br>\n");
            LOG_END();
            return (int) error->code;

        case (ExpressionErrors::INVALID_EXPRESSION_FORMAT):
            fprintf(fp, "EXPRESSION FORMAT IS WRONG<br>\n");
            LOG_END();
            return (int) error->code;

        case (ExpressionErrors::UNKNOWN_OPERATION):
            fprintf(fp, "UNKNOWN OPERATION<br>\n");
            LOG_END();
            return (int) error->code;

        case (ExpressionErrors::NO_DIFF_VARIABLE):
            fprintf(fp, "DID NOT FOUND \"%s\" IN EXPRESSION<br>\n", (const char*) error->data);
            LOG_END();
            return (int) error->code;

        case (ExpressionErrors::UNKNOWN):
        // fall through
        default:
            fprintf(fp, "UNKNOWN ERROR WITH EXPRESSION<br>\n");
            LOG_END();
            return (int) ExpressionErrors::UNKNOWN;
    }
}

//-----------------------------------------------------------------------------------------------------

ExpressionErrors NodeVerify(const Node* node, error_t* error)
{
    assert(node);
    assert(error);

    if (node == node->left || node == node->right)
    {
        error->code = (int) ExpressionErrors::CYCLED_NODE;
        error->data = node;
        return ExpressionErrors::CYCLED_NODE;
    }
    if (node->left == node->right)
    {
        error->code = (int) ExpressionErrors::COMMON_HEIR;
        error->data = node;
        return ExpressionErrors::COMMON_HEIR;
    }

    return ExpressionErrors::NONE;
}

//-----------------------------------------------------------------------------------------------------

ExpressionErrors ExpressionVerify(const expr_t* expr, error_t* error)
{
    assert(expr);
    assert(error);

    VerifyNodes(expr->root, error);

    return (ExpressionErrors) error->code;
}

//-----------------------------------------------------------------------------------------------------

static ExpressionErrors VerifyNodes(const Node* node, error_t* error)
{
    assert(node);
    assert(error);

    if (node->left != nullptr)
    {
        NodeVerify(node->left, error);
        RETURN_IF_EXPRESSION_ERROR((ExpressionErrors) error->code);
    }

    if (node->right != nullptr)
    {
        NodeVerify(node->right, error);
        RETURN_IF_EXPRESSION_ERROR((ExpressionErrors) error->code);
    }

    NodeVerify(node, error);

    return (ExpressionErrors) error->code;
}
