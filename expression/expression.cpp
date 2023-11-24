#include <assert.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

#include "expression.h"
#include "graphs.h"
#include "common/input_and_output.h"
#include "common/file_read.h"

static void              DestructNodes(Node* root);
static ExpressionErrors  VerifyNodes(const Node* node, error_t* error);

static const double POISON = 0xDEC0;

// ======================================================================
// EXPRESSION INPUT
// ======================================================================

static const char* NIL = "nil";

static Node*       ReadExpressionAsTree(expr_t* expr, Storage* info, Node* current_node, error_t* error);
static Node*       NodesPrefixRead(expr_t* expr, Storage* info, Node* current_node, error_t* error);
static void        ReadNodeData(expr_t* expr, Storage* info, NodeType* type, NodeValue* value,  error_t* error);

static inline void DeleteClosingBracketFromWord(Storage* info, char* read);
static char        CheckOpeningBracketInInput(Storage* info);

static Node*       ExpressionReadNewNode(expr_t* expr, Storage* info, Node* parent_node, error_t* error);
static Node*       PrefixReadNewNode(expr_t* expr, Storage* info, Node* parent_node, error_t* error);
static bool        TryReadNumber(Storage* info, NodeType* type, NodeValue* value);

// ======================================================================
// EXPRESSION OUTPUT
// ======================================================================

static void        TextExpressionDump(FILE* fp, const expr_t* expr);
static void        NodesInfixPrint(FILE* fp, const expr_t* expr, const Node* node);
static void        NodesInfixPrintLatex(FILE* fp, const expr_t* expr, const Node* node);
static void        PrintNodeData(FILE* fp, const expr_t* expr, const Node* node);
static void        PrintNodeDataType(FILE* fp, const NodeType type);

static bool        CheckBracketsNeededInEquation(const Node* node);

// ======================================================================
// GRAPH BUILDING
// ======================================================================

static void DrawTreeGraph(const expr_t* expr);

static inline void DrawNodes(FILE* dotf, const expr_t* expr, const Node* node, const int rank);

// ======================================================================
// EXPRESSION OPERATORS
// ======================================================================

static Operators   DefineOperator(const char* word);
static void        PrintOperator(FILE* fp, const Operators sign);

static int         GetOperationPriority(const Operators sign);
static double      OperationWithTwoNumbers(const double number_1, const double number_2,
                                           const Operators operation, error_t* error);

// ======================================================================
// EXPRESSION VARIABLES
// ======================================================================

static void InitVariablesArray(variable_t* variables, size_t size);
static void DestructVariablesArray(variable_t* variables, size_t size);

static int SaveVariable(variable_t* vars, const char* new_var);
static int FindVariableAmongSaved(variable_t* vars, const char* new_var);

static const int NO_VARIABLE = -1;

//------------------------------------------------------------------

static double OperationWithTwoNumbers(const double number_1, const double number_2,
                                      const Operators operation, error_t* error)
{
    switch (operation)
    {
        case (Operators::ADD):
            return number_1 + number_2;
        case (Operators::MUL):
            return number_1 * number_2;
        case (Operators::DIV):
            return number_1 / number_2;
        case (Operators::SUB):
            return number_1 - number_2;
        default:
            error->code = (int) ExpressionErrors::UNKNOWN_OPERATION;
            return POISON;
    }
}

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

static void InitVariablesArray(variable_t* variables, size_t size)
{
    assert(variables);
    assert(size <= MAX_VARIABLES_AMT);

    for (size_t i = 0; i < size; i++)
    {
        variables[i].isfree        = true;
        variables[i].variable_name = nullptr;
        variables[i].value         = 0;
    }
}

//------------------------------------------------------------------

static void DestructVariablesArray(variable_t* variables, size_t size)
{
    assert(variables);
    assert(size <= MAX_VARIABLES_AMT);

    for (size_t i = 0; i < size; i++)
    {
        free(variables[i].variable_name);
        variables[i].isfree = true;
        variables[i].value  = 0;
    }
}

//-----------------------------------------------------------------------------------------------------

Node* NodeCtor(const NodeType type, const NodeValue value,
               Node* left, Node* right, Node* parent, error_t* error)
{
    assert(error);

    Node* node = (Node*) calloc(1, sizeof(Node));
    if (node == nullptr)
    {
        error->code = (int) ExpressionErrors::ALLOCATE_MEMORY;
        error->data = "NODE";
        return nullptr;
    }

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

static void PrintNodeData(FILE* fp, const expr_t* expr, const Node* node)
{
    assert(node);

    switch(node->type)
    {
        case (NodeType::NUMBER):
            fprintf(fp, " %g ", node->value.val);
            break;
        case (NodeType::VARIABLE):
            fprintf(fp, " %s ", expr->vars[node->value.var].variable_name);
            break;
        case (NodeType::OPERATOR):
            PrintOperator(fp, node->value.opt);
            break;
        default:
            fprintf(fp, " undefined ");
    }
}

//-----------------------------------------------------------------------------------------------------

static void PrintOperator(FILE* fp, const Operators sign)
{
    switch (sign)
    {
        case (Operators::ADD):
            fprintf(fp, ADD);
            break;
        case (Operators::SUB):
            fprintf(fp, SUB);
            break;
        case (Operators::MUL):
            fprintf(fp, MUL);
            break;
        case (Operators::DIV):
            fprintf(fp, DIV);
            break;
        default:
            fprintf(fp, " undefined_operator ");
    }
}

//-----------------------------------------------------------------------------------------------------

static void PrintNodeDataType(FILE* fp, const NodeType type)
{
    switch (type)
    {
        case (NodeType::NUMBER):
            fprintf(fp, "number");
            break;
        case (NodeType::OPERATOR):
            fprintf(fp, "operator");
            break;
        case (NodeType::VARIABLE):
            fprintf(fp, "variable");
            break;
        default:
            fprintf(fp, "unknown_type");
    }
}

//-----------------------------------------------------------------------------------------------------

ExpressionErrors ExpressionCtor(expr_t* expr, error_t* error)
{
    Node* root = NodeCtor(INIT_TYPE, INIT_VALUE, nullptr, nullptr, nullptr, error);
    RETURN_IF_EXPRESSION_ERROR((ExpressionErrors) error->code);

    variable_t* vars = (variable_t*) calloc(MAX_VARIABLES_AMT, sizeof(variable_t));
    if (vars == nullptr)
    {
        error->code = (int) ExpressionErrors::ALLOCATE_MEMORY;
        error->data = "VARIABLES ARRAY";
        return ExpressionErrors::ALLOCATE_MEMORY;
    }
    InitVariablesArray(vars, MAX_VARIABLES_AMT);

    expr->vars = vars;
    expr->root = root;

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

    DestructVariablesArray(expr->vars, MAX_VARIABLES_AMT);
    free(expr->vars);
    expr->root = nullptr;
}

//-----------------------------------------------------------------------------------------------------

static void DestructNodes(Node* root)
{
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

        case (ExpressionErrors::EMPTY_TREE):
            fprintf(fp, "EXPRESSION TREE IS EMPTY<br>\n");
            LOG_END();
            return (int) error->code;

        case (ExpressionErrors::INVALID_SYNTAX):
            fprintf(fp, "UNKNOWN INPUT<br>\n");
            LOG_END();
            return (int) error->code;

        case (ExpressionErrors::CYCLED_NODE):
            fprintf(fp, "NODE ID IT'S OWN PREDECESSOR<br>\n");
            DUMP_NODE(error->data);
            LOG_END();
            return (int) error->code;

        case (ExpressionErrors::COMMON_HEIR):
            fprintf(fp, "NODE'S HEIRS ARE SAME<br>\n");
            DUMP_NODE(error->data);
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

void PrintExpressionTree(FILE* fp, const expr_t* expr)
{
    assert(expr);

    NodesInfixPrint(fp, expr, expr->root);
    fprintf(fp, "\n");
}

//-----------------------------------------------------------------------------------------------------

void PrintExpressionTreeLatex(FILE* fp, const expr_t* expr)
{
    assert(expr);
    PrintPrankPhrase(fp);
    fprintf(fp, "$");
    NodesInfixPrintLatex(fp, expr, expr->root);
    fprintf(fp, "$\n");
}

//-----------------------------------------------------------------------------------------------------

static void NodesInfixPrint(FILE* fp, const expr_t* expr, const Node* node)
{
    if (!node) { return; }

    if (node->left == nullptr || node->right == nullptr)
    {
        PrintNodeData(fp, expr, node);
        return;
    }

    bool need_brackets_on_the_left  = CheckBracketsNeededInEquation(node->left);
    bool need_brackets_on_the_right = CheckBracketsNeededInEquation(node->right);

    if (need_brackets_on_the_left) fprintf(fp, "(");
    NodesInfixPrint(fp, expr, node->left);
    if (need_brackets_on_the_left) fprintf(fp, ")");

    PrintNodeData(fp, expr, node);

    if (need_brackets_on_the_right) fprintf(fp, "(");
    NodesInfixPrint(fp, expr, node->right);
    if (need_brackets_on_the_right) fprintf(fp, ")");

}

//-----------------------------------------------------------------------------------------------------

static void NodesInfixPrintLatex(FILE* fp, const expr_t* expr, const Node* node)
{
    if (!node) { return; }

    if (node->left == nullptr || node->right == nullptr)
    {
        PrintNodeData(fp, expr, node);
        return;
    }

    if (node->type == NodeType::OPERATOR && node->value.opt == Operators::DIV)
    {
        fprintf(fp, "\\frac{");
        NodesInfixPrintLatex(fp, expr, node->left);
        fprintf(fp, "}{");
        NodesInfixPrintLatex(fp, expr, node->right);
        fprintf(fp, "}");
        return;
    }

    bool need_brackets_on_the_left  = CheckBracketsNeededInEquation(node->left);
    bool need_brackets_on_the_right = CheckBracketsNeededInEquation(node->right);

    if (need_brackets_on_the_left) fprintf(fp, "(");
    NodesInfixPrintLatex(fp, expr, node->left);
    if (need_brackets_on_the_left) fprintf(fp, ")");

    PrintNodeData(fp, expr, node);

    if (need_brackets_on_the_right) fprintf(fp, "(");
    NodesInfixPrintLatex(fp, expr, node->right);
    if (need_brackets_on_the_right) fprintf(fp, ")");

}

//-----------------------------------------------------------------------------------------------------

static bool CheckBracketsNeededInEquation(const Node* node)
{
    assert(node);

    if (node->type != NodeType::OPERATOR)
        return false;

    int kid_priority    = GetOperationPriority(node->left->value.opt);
    int parent_priority = GetOperationPriority(node->parent->value.opt);

    if (kid_priority < parent_priority)
        return true;

    return false;
}

//-----------------------------------------------------------------------------------------------------

static int GetOperationPriority(const Operators sign)
{
    switch (sign)
    {
        case (Operators::ADD):
            return 1;
        case (Operators::SUB):
            return 1;
        case (Operators::MUL):
            return 2;
        case (Operators::DIV):
            return 2;
        default:
            return 0;
    }
}

//-----------------------------------------------------------------------------------------------------

void ExpressionInfixRead(Storage* info, expr_t* expr, error_t* error)
{
    assert(expr);
    assert(info);

    SkipBufSpaces(info);
    int ch = Bufgetc(info);

    Node* root = nullptr;

    if (ch == EOF)
    {
        error->code = (int) ExpressionErrors::EMPTY_TREE;
        return;
    }
    else
    {
        Bufungetc(info);
        root = ReadExpressionAsTree(expr, info, root, error);
    }

    expr->root = root;
}

//-----------------------------------------------------------------------------------------------------

void ExpressionPrefixRead(Storage* info, expr_t* expr, error_t* error)
{
    assert(expr);
    assert(info);

    SkipBufSpaces(info);
    int ch = Bufgetc(info);

    Node* root = nullptr;

    if (ch == EOF)
    {
        error->code = (int) ExpressionErrors::EMPTY_TREE;
        return;
    }
    else
    {
        Bufungetc(info);
        root = NodesPrefixRead(expr, info, root, error);
    }

    expr->root = root;
}

//-----------------------------------------------------------------------------------------------------

static char CheckOpeningBracketInInput(Storage* info)
{
    SkipBufSpaces(info);
    char opening_bracket_check = Bufgetc(info);

    return opening_bracket_check;
}

//-----------------------------------------------------------------------------------------------------

static Node* ReadExpressionAsTree(expr_t* expr, Storage* info, Node* current_node, error_t* error)
{
    assert(error);
    assert(expr);
    assert(info);

    char opening_bracket_check = CheckOpeningBracketInInput(info);

    if (opening_bracket_check == '(')
    {
        Node* new_node = ExpressionReadNewNode(expr, info, current_node, error);

        char closing_bracket_check = Bufgetc(info);
        if (closing_bracket_check != ')')
        {
            error->code = (int) ExpressionErrors::INVALID_SYNTAX;
            return nullptr;
        }

        return new_node;
    }
    else
    {
        Bufungetc(info);

        return nullptr;
    }
}

//-----------------------------------------------------------------------------------------------------

static Node* NodesPrefixRead(expr_t* expr, Storage* info, Node* current_node, error_t* error)
{
    assert(error);
    assert(expr);
    assert(info);

    char opening_bracket_check = CheckOpeningBracketInInput(info);

    if (opening_bracket_check == '(')
    {
        Node* new_node = PrefixReadNewNode(expr, info, current_node, error);

        char closing_bracket_check = Bufgetc(info);
        if (closing_bracket_check != ')')
        {
            error->code = (int) ExpressionErrors::INVALID_SYNTAX;
            return nullptr;
        }

        return new_node;
    }
    else
    {
        Bufungetc(info);

        char read[MAX_STRING_LEN] = "";
        BufScanfWord(info, read);

        DeleteClosingBracketFromWord(info, read);

        if (strncmp(read, NIL, MAX_STRING_LEN))
            error->code = (int) ExpressionErrors::INVALID_SYNTAX;
    }

    return nullptr;
}

//-----------------------------------------------------------------------------------------------------

static Node* ExpressionReadNewNode(expr_t* expr, Storage* info, Node* parent_node, error_t* error)
{
    assert(expr);
    assert(info);
    assert(error);

    Node* node = NodeCtor(INIT_TYPE, INIT_VALUE, nullptr, nullptr, nullptr, error);

    NodeType type = INIT_TYPE;
    NodeValue val = INIT_VALUE;

    Node* left = ReadExpressionAsTree(expr, info, node, error);

    ReadNodeData(expr, info, &type, &val, error);
    if (error->code != (int) ExpressionErrors::NONE)
        return nullptr;

    Node* right = ReadExpressionAsTree(expr, info, node, error);

    node->parent = parent_node;
    node->type   = type;
    node->value  = val;
    node->left   = left;
    node->right  = right;

    SkipBufSpaces(info);

    return node;
}

//-----------------------------------------------------------------------------------------------------

static Node* PrefixReadNewNode(expr_t* expr, Storage* info, Node* parent_node, error_t* error)
{
    assert(expr);
    assert(info);
    assert(error);

    Node* node = NodeCtor(INIT_TYPE, INIT_VALUE, nullptr, nullptr, nullptr, error);

    NodeType type = INIT_TYPE;
    NodeValue val = INIT_VALUE;

    ReadNodeData(expr, info, &type, &val, error);
    if (error->code != (int) ExpressionErrors::NONE)
        return nullptr;

    Node* left  = NodesPrefixRead(expr, info, node, error);
    Node* right = NodesPrefixRead(expr, info, node, error);

    node->parent = parent_node;
    node->type   = type;
    node->value  = val;
    node->left   = left;
    node->right  = right;

    SkipBufSpaces(info);

    return node;
}

//-----------------------------------------------------------------------------------------------------

static void ReadNodeData(expr_t* expr, Storage* info, NodeType* type, NodeValue* value,  error_t* error)
{
    assert(error);
    assert(type);
    assert(value);
    assert(expr);

    if (TryReadNumber(info, type, value))
        return;

    char word[MAX_STRING_LEN] = "";
    BufScanfWord(info, word);

    Operators sign = DefineOperator(word);

    if (sign != Operators::UNK)
    {
        *type            = NodeType::OPERATOR;
        value->opt = sign;
        return;
    }

    int id = FindVariableAmongSaved(expr->vars, word);

    if (id == NO_VARIABLE)
        id = SaveVariable(expr->vars, word);

    if (id == NO_VARIABLE)
    {
        *type = NodeType::POISON;
        error->code = (int) ExpressionErrors::UNKNOWN_INPUT;
    }
    else
    {
        *type           = NodeType::VARIABLE;
        value->var = id;
    }
}

//-----------------------------------------------------------------------------------------------------

static int FindVariableAmongSaved(variable_t* vars, const char* new_var)
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

static int SaveVariable(variable_t* vars, const char* new_var)
{
    assert(new_var);

    int id = NO_VARIABLE;

    for (int i = 0; i < MAX_VARIABLES_AMT; i++)
    {
        if (vars[i].isfree)
        {
            id = i;
            vars[i].variable_name = strndup(new_var, MAX_VARIABLE_LEN);
            return id;
        }
    }

    return id;
}

//-----------------------------------------------------------------------------------------------------

static bool TryReadNumber(Storage* info, NodeType* type, NodeValue* value)
{
    double number = 0;
    int is_number = BufScanfDouble(info, &number);

    if (is_number)
    {
        *type      = NodeType::NUMBER;
        value->val = number;
        return true;
    }

    return false;
}

//-----------------------------------------------------------------------------------------------------

static Operators DefineOperator(const char* word)
{
    assert(word);

    if (!strncmp(word, ADD, MAX_STRING_LEN))
        return Operators::ADD;
    else if (!strncmp(word, SUB, MAX_STRING_LEN))
        return Operators::SUB;
    else if (!strncmp(word, DIV, MAX_STRING_LEN))
        return Operators::DIV;
    else if (!strncmp(word, MUL, MAX_STRING_LEN))
        return Operators::MUL;

    return Operators::UNK;
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

static inline void DeleteClosingBracketFromWord(Storage* info, char* read)
{
    assert(read);

    size_t bracket_pos = strlen(NIL);
    if (read[bracket_pos] == ')')
    {
        read[bracket_pos] = '\0';
        Bufungetc(info);
    }
}

//-----------------------------------------------------------------------------------------------------

int NodeDump(FILE* fp, const void* dumping_node, const char* func, const char* file, const int line)
{
    assert(dumping_node);

    LOG_START_DUMP(func, file, line);

    const Node* node = (const Node*) dumping_node;

    fprintf(fp, "NODE [%p]<br>\n"
                "LEFT > [%p]<br>\n"
                "RIGHT > [%p]<br>\n");

    LOG_END();

    return (int) ExpressionErrors::NONE;
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

//-----------------------------------------------------------------------------------------------------

int ExpressionDump(FILE* fp, const void* nodes, const char* func, const char* file, const int line)
{
    assert(nodes);

    LOG_START_DUMP(func, file, line);

    const expr_t* expr = (const expr_t*) nodes;

    TextExpressionDump(fp, expr);
    DrawTreeGraph(expr);

    LOG_END();

    return (int) ExpressionErrors::NONE;
}

//-----------------------------------------------------------------------------------------------------

static void TextExpressionDump(FILE* fp, const expr_t* expr)
{
    assert(expr);

    fprintf(fp, "<pre>");

    fprintf(fp, "<b>DUMPING EXPRESSION</b>\n");

    PrintExpressionTree(fp, expr);

    fprintf(fp, "</pre>");
}

//-----------------------------------------------------------------------------------------------------

static void DrawTreeGraph(const expr_t* expr)
{
    assert(expr);

    FILE* dotf = fopen(DOT_FILE, "w");

    StartGraph(dotf);
    DrawNodes(dotf, expr, expr->root, 1);
    EndGraph(dotf);

    fclose(dotf);

    MakeImgFromDot(DOT_FILE);
}

//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::;::::::::::::::::::::::::::

static inline void DrawNodes(FILE* dotf, const expr_t* expr, const Node* node, const int rank)
{
    if (!node) return;

    fprintf(dotf, "%lld [shape=Mrecord, style=filled, fillcolor=\"lightblue\", color = darkblue, rank = %d, label=\" "
                  "{ node: %p | parent: %p | { type: " , node, rank, node, node->parent);

    PrintNodeDataType(dotf, node->type);

    fprintf(dotf, " | data: ");

    PrintNodeData(dotf, expr, node);

    fprintf(dotf, "} | { left: %p| right: %p } }\"]\n", node->left, node->right);

    DrawNodes(dotf, expr, node->left, rank + 1);
    DrawNodes(dotf, expr, node->right, rank + 1);

    if (node->parent != nullptr)
        fprintf(dotf, "%lld->%lld [color = blue]\n", node, node->parent);

    if (node->left != nullptr)
        fprintf(dotf, "%lld->%lld [color = black, fontcolor = black]\n", node, node->left);

    if (node->right != nullptr)
        fprintf(dotf, "%lld->%lld [color = black, fontcolor = black]\n", node, node->right);
}
