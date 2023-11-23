#include <assert.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

#include "tree.h"
#include "graphs.h"
#include "common/input_and_output.h"
#include "common/file_read.h"

static void        DestructNodes(Node* root);
static DiffErrors  VerifyNodes(const Node* node, error_t* error);

static Node*       NodesInfixRead(tree_t* tree, Storage* info, Node* current_node, error_t* error);
static Node*       NodesPrefixRead(tree_t* tree, Storage* info, Node* current_node, error_t* error);
static void        ReadNodeData(tree_t* tree, Storage* info, DataType* type, DataValue* value,  error_t* error);

static inline void DeleteClosingBracketFromWord(Storage* info, char* read);
static char        CheckOpeningBracketInInput(Storage* info);

static Node*       InfixReadNewNode(tree_t* tree, Storage* info, Node* parent_node, error_t* error);
static Node*       PrefixReadNewNode(tree_t* tree, Storage* info, Node* parent_node, error_t* error);
static bool        TryReadNumber(Storage* info, DataType* type, DataValue* value);

static void        TextTreeDump(FILE* fp, const tree_t* tree);
static void        NodesInfixPrint(FILE* fp, const tree_t* tree, const Node* node);
static void        NodesInfixPrintLatex(FILE* fp, const tree_t* tree, const Node* node);
static void        PrintNodeData(FILE* fp, const tree_t* tree, const Node* node);
static void        PrintNodeDataType(FILE* fp, const DataType type);
static Operators   DefineOperator(const char* word);
static void        PrintOperator(FILE* fp, const Operators sign);

static bool        CheckBracketsNeededInEquation(const Node* node);
static int         GetOperationPriority(const Operators sign);
static double      OperationWithTwoNumbers(const double number_1, const double number_2,
                                           const Operators operation, error_t* error);

static void        PrintPrankPhrase(FILE* fp);

// ======== GRAPHS =========

static void DrawTreeGraph(const tree_t* tree);

static inline void DrawNodes(FILE* dotf, const tree_t* tree, const Node* node, const int rank);

// =========================

static const char* NIL = "_";

// ======== OPERATORS ======

static const char* ADD = "+";
static const char* SUB = "-";
static const char* DIV = "/";
static const char* MUL = "*";

// =========================

// ::::::::::::: VARIABLES FUNCS :::::::::::::

static void InitVariablesArray(variable_t* variables, size_t size);
static void DestructVariablesArray(variable_t* variables, size_t size);

static int SaveVariable(variable_t* vars, const char* new_var);
static int FindVariableAmongSaved(variable_t* vars, const char* new_var);

static const int NO_VARIABLE = -1;

// ::::::::::::::::::::::::::::::::::::::::

static const double PZN = 0xDEC0;

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
            error->code = (int) DiffErrors::UNKNOWN_OPERATION;
            return PZN;
    }
}

//------------------------------------------------------------------

double CalculateTree(tree_t* tree, Node* node, error_t* error)
{
    assert(tree);
    assert(error);
    assert(node);

    if (node->left == nullptr || node->right == nullptr)
    {
        if (node->type == DataType::NUMBER)             return node->value.value;
        else if (node->type == DataType::VARIABLE)      return tree->vars[node->value.variable].value;
        else
        {
            error->code = (int) DiffErrors::WRONG_EQUATION;
            return 0;
        }
    }

    double left_result  = CalculateTree(tree, node->left, error);
    double right_result = CalculateTree(tree, node->right, error);

    if (node->type != DataType::OPERATOR)
    {
        error->code = (int) DiffErrors::WRONG_EQUATION;
        return 0;
    }

    double result = OperationWithTwoNumbers(left_result, right_result, node->value.operation, error);

    if (error->code == (int) DiffErrors::NONE)
        return result;
    else
        return PZN;

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

Node* NodeCtor(const DataType type, const DataValue value,
               Node* left, Node* right, Node* parent, error_t* error)
{
    assert(error);

    Node* node = (Node*) calloc(1, sizeof(Node));
    if (node == nullptr)
    {
        error->code = (int) DiffErrors::ALLOCATE_MEMORY;
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

static void PrintNodeData(FILE* fp, const tree_t* tree, const Node* node)
{
    assert(node);

    switch(node->type)
    {
        case (DataType::NUMBER):
            fprintf(fp, " %g ", node->value.value);
            break;
        case (DataType::VARIABLE):
            fprintf(fp, " %s ", tree->vars[node->value.variable].variable_name);
            break;
        case (DataType::OPERATOR):
            PrintOperator(fp, node->value.operation);
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

static void PrintNodeDataType(FILE* fp, const DataType type)
{
    switch (type)
    {
        case (DataType::NUMBER):
            fprintf(fp, "number");
            break;
        case (DataType::OPERATOR):
            fprintf(fp, "operator");
            break;
        case (DataType::VARIABLE):
            fprintf(fp, "variable");
            break;
        default:
            fprintf(fp, "unknown_type");
    }
}

//-----------------------------------------------------------------------------------------------------

DiffErrors TreeCtor(tree_t* tree, error_t* error)
{
    Node* root = NodeCtor(DEFAULT_TYPE, DEFAULT_VALUE, nullptr, nullptr, nullptr, error);
    RETURN_IF_TREE_ERROR((DiffErrors) error->code);

    variable_t* vars = (variable_t*) calloc(MAX_VARIABLES_AMT, sizeof(variable_t));
    if (vars == nullptr)
    {
        error->code = (int) DiffErrors::ALLOCATE_MEMORY;
        error->data = "VARIABLES ARRAY";
        return DiffErrors::ALLOCATE_MEMORY;
    }
    InitVariablesArray(vars, MAX_VARIABLES_AMT);

    tree->vars = vars;
    tree->root = root;

    return DiffErrors::NONE;
}

//-----------------------------------------------------------------------------------------------------

void TreeDtor(tree_t* tree)
{
    DestructNodes(tree->root);

    DestructVariablesArray(tree->vars, MAX_VARIABLES_AMT);
    free(tree->vars);
    tree->root = nullptr;
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

int PrintTreeError(FILE* fp, const void* err, const char* func, const char* file, const int line)
{
    assert(err);

    LOG_START(func, file, line);

    const struct ErrorInfo* error = (const struct ErrorInfo*) err;

    switch ((DiffErrors) error->code)
    {
        case (DiffErrors::NONE):
            LOG_END();
            return (int) error->code;

        case (DiffErrors::ALLOCATE_MEMORY):
            fprintf(fp, "CAN NOT ALLOCATE MEMORY FOR %s<br>\n", (const char*) error->data);
            LOG_END();
            return (int) error->code;

        case (DiffErrors::EMPTY_TREE):
            fprintf(fp, "TREE IS EMPTY<br>\n");
            LOG_END();
            return (int) error->code;

        case (DiffErrors::INVALID_SYNTAX):
            fprintf(fp, "UNKNOWN INPUT<br>\n");
            LOG_END();
            return (int) error->code;

        case (DiffErrors::CYCLED_NODE):
            fprintf(fp, "NODE ID IT'S OWN PREDECESSOR<br>\n");
            DUMP_NODE(error->data);
            LOG_END();
            return (int) error->code;

        case (DiffErrors::COMMON_HEIR):
            fprintf(fp, "NODE'S HEIRS ARE SAME<br>\n");
            DUMP_NODE(error->data);
            LOG_END();
            return (int) error->code;

        case (DiffErrors::UNKNOWN):
        // fall through
        default:
            fprintf(fp, "UNKNOWN ERROR WITH TREE<br>\n");
            LOG_END();
            return (int) DiffErrors::UNKNOWN;
    }
}

//-----------------------------------------------------------------------------------------------------

void TreePrintEquation(FILE* fp, const tree_t* tree)
{
    assert(tree);

    NodesInfixPrint(fp, tree, tree->root);
    fprintf(fp, "\n");
}

//-----------------------------------------------------------------------------------------------------

void TreePrintEquationLatex(FILE* fp, const tree_t* tree)
{
    assert(tree);
    PrintPrankPhrase(fp);
    fprintf(fp, "$");
    NodesInfixPrintLatex(fp, tree, tree->root);
    fprintf(fp, "$\n");
}

//-----------------------------------------------------------------------------------------------------

static void PrintPrankPhrase(FILE* fp)
{
    static const int   PHRASE_AMT = 5;
    static const char* PHRASES[] = {"Очевидно, что",
                                    "Несложно показать, что",
                                    "При виде формулы становится ясно, что",
                                    "Нет такого?",
                                    "Согл?"};

    srand(time(NULL));

    int nmb = rand() % 5;

    fprintf(fp, "%s ", PHRASES[nmb]);
}

//-----------------------------------------------------------------------------------------------------

static void NodesInfixPrint(FILE* fp, const tree_t* tree, const Node* node)
{
    if (!node) { return; }

    if (node->left == nullptr || node->right == nullptr)
    {
        PrintNodeData(fp, tree, node);
        return;
    }

    bool need_brackets_on_the_left  = CheckBracketsNeededInEquation(node->left);
    bool need_brackets_on_the_right = CheckBracketsNeededInEquation(node->right);

    if (need_brackets_on_the_left) fprintf(fp, "(");
    NodesInfixPrint(fp, tree, node->left);
    if (need_brackets_on_the_left) fprintf(fp, ")");

    PrintNodeData(fp, tree, node);

    if (need_brackets_on_the_right) fprintf(fp, "(");
    NodesInfixPrint(fp, tree, node->right);
    if (need_brackets_on_the_right) fprintf(fp, ")");

}

//-----------------------------------------------------------------------------------------------------

static void NodesInfixPrintLatex(FILE* fp, const tree_t* tree, const Node* node)
{
    if (!node) { return; }

    if (node->left == nullptr || node->right == nullptr)
    {
        PrintNodeData(fp, tree, node);
        return;
    }

    if (node->type == DataType::OPERATOR && node->value.operation == Operators::DIV)
    {
        fprintf(fp, "\\frac{");
        NodesInfixPrintLatex(fp, tree, node->left);
        fprintf(fp, "}{");
        NodesInfixPrintLatex(fp, tree, node->right);
        fprintf(fp, "}");
        return;
    }

    bool need_brackets_on_the_left  = CheckBracketsNeededInEquation(node->left);
    bool need_brackets_on_the_right = CheckBracketsNeededInEquation(node->right);

    if (need_brackets_on_the_left) fprintf(fp, "(");
    NodesInfixPrintLatex(fp, tree, node->left);
    if (need_brackets_on_the_left) fprintf(fp, ")");

    PrintNodeData(fp, tree, node);

    if (need_brackets_on_the_right) fprintf(fp, "(");
    NodesInfixPrintLatex(fp, tree, node->right);
    if (need_brackets_on_the_right) fprintf(fp, ")");

}

//-----------------------------------------------------------------------------------------------------

static bool CheckBracketsNeededInEquation(const Node* node)
{
    assert(node);

    if (node->type != DataType::OPERATOR)
        return false;

    int kid_priority    = GetOperationPriority(node->left->value.operation);
    int parent_priority = GetOperationPriority(node->parent->value.operation);

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

void TreeInfixRead(Storage* info, tree_t* tree, error_t* error)
{
    assert(tree);
    assert(info);

    SkipBufSpaces(info);
    int ch = Bufgetc(info);

    Node* root = nullptr;

    if (ch == EOF)
    {
        error->code = (int) DiffErrors::EMPTY_TREE;
        return;
    }
    else
    {
        Bufungetc(info);
        root = NodesInfixRead(tree, info, root, error);
    }

    tree->root = root;
}

//-----------------------------------------------------------------------------------------------------

void TreePrefixRead(Storage* info, tree_t* tree, error_t* error)
{
    assert(tree);
    assert(info);

    SkipBufSpaces(info);
    int ch = Bufgetc(info);

    Node* root = nullptr;

    if (ch == EOF)
    {
        error->code = (int) DiffErrors::EMPTY_TREE;
        return;
    }
    else
    {
        Bufungetc(info);
        root = NodesPrefixRead(tree, info, root, error);
    }

    tree->root = root;
}

//-----------------------------------------------------------------------------------------------------

static char CheckOpeningBracketInInput(Storage* info)
{
    SkipBufSpaces(info);
    char opening_bracket_check = Bufgetc(info);

    return opening_bracket_check;
}

//-----------------------------------------------------------------------------------------------------

static Node* NodesInfixRead(tree_t* tree, Storage* info, Node* current_node, error_t* error)
{
    assert(error);
    assert(tree);
    assert(info);

    char opening_bracket_check = CheckOpeningBracketInInput(info);

    if (opening_bracket_check == '(')
    {
        Node* new_node = InfixReadNewNode(tree, info, current_node, error);

        char closing_bracket_check = Bufgetc(info);
        if (closing_bracket_check != ')')
        {
            error->code = (int) DiffErrors::INVALID_SYNTAX;
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
            error->code = (int) DiffErrors::INVALID_SYNTAX;

        return nullptr;
    }
}

//-----------------------------------------------------------------------------------------------------

static Node* NodesPrefixRead(tree_t* tree, Storage* info, Node* current_node, error_t* error)
{
    assert(error);
    assert(tree);
    assert(info);

    char opening_bracket_check = CheckOpeningBracketInInput(info);

    if (opening_bracket_check == '(')
    {
        Node* new_node = PrefixReadNewNode(tree, info, current_node, error);

        char closing_bracket_check = Bufgetc(info);
        if (closing_bracket_check != ')')
        {
            error->code = (int) DiffErrors::INVALID_SYNTAX;
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
            error->code = (int) DiffErrors::INVALID_SYNTAX;
    }

    return nullptr;
}

//-----------------------------------------------------------------------------------------------------

static Node* InfixReadNewNode(tree_t* tree, Storage* info, Node* parent_node, error_t* error)
{
    assert(tree);
    assert(info);
    assert(error);

    Node* node = NodeCtor(DEFAULT_TYPE, DEFAULT_VALUE, nullptr, nullptr, nullptr, error);

    DataType type = DEFAULT_TYPE;
    DataValue val = DEFAULT_VALUE;

    Node* left = NodesInfixRead(tree, info, node, error);

    ReadNodeData(tree, info, &type, &val, error);
    if (error->code != (int) DiffErrors::NONE)
        return nullptr;

    Node* right = NodesInfixRead(tree, info, node, error);

    node->parent = parent_node;
    node->type   = type;
    node->value  = val;
    node->left   = left;
    node->right  = right;

    SkipBufSpaces(info);

    return node;
}

//-----------------------------------------------------------------------------------------------------

static Node* PrefixReadNewNode(tree_t* tree, Storage* info, Node* parent_node, error_t* error)
{
    assert(tree);
    assert(info);
    assert(error);

    Node* node = NodeCtor(DEFAULT_TYPE, DEFAULT_VALUE, nullptr, nullptr, nullptr, error);

    DataType type = DEFAULT_TYPE;
    DataValue val = DEFAULT_VALUE;

    ReadNodeData(tree, info, &type, &val, error);
    if (error->code != (int) DiffErrors::NONE)
        return nullptr;

    Node* left  = NodesPrefixRead(tree, info, node, error);
    Node* right = NodesPrefixRead(tree, info, node, error);

    node->parent = parent_node;
    node->type   = type;
    node->value  = val;
    node->left   = left;
    node->right  = right;

    SkipBufSpaces(info);

    return node;
}

//-----------------------------------------------------------------------------------------------------

static void ReadNodeData(tree_t* tree, Storage* info, DataType* type, DataValue* value,  error_t* error)
{
    assert(error);
    assert(type);
    assert(value);
    assert(tree);

    if (TryReadNumber(info, type, value))
        return;

    char word[MAX_STRING_LEN] = "";
    BufScanfWord(info, word);

    Operators sign = DefineOperator(word);

    if (sign != Operators::UNK)
    {
        *type            = DataType::OPERATOR;
        value->operation = sign;
        return;
    }

    int id = FindVariableAmongSaved(tree->vars, word);

    if (id == NO_VARIABLE)
        id = SaveVariable(tree->vars, word);

    if (id == NO_VARIABLE)
    {
        *type = DataType::PZN;
        error->code = (int) DiffErrors::UNKNOWN_INPUT;
    }
    else
    {
        *type           = DataType::VARIABLE;
        value->variable = id;
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

static bool TryReadNumber(Storage* info, DataType* type, DataValue* value)
{
    double number = 0;
    int is_number = BufScanfDouble(info, &number);

    if (is_number)
    {
        *type        = DataType::NUMBER;
        value->value = number;
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

DiffErrors NodeVerify(const Node* node, error_t* error)
{
    assert(node);
    assert(error);

    if (node == node->left || node == node->right)
    {
        error->code = (int) DiffErrors::CYCLED_NODE;
        error->data = node;
        return DiffErrors::CYCLED_NODE;
    }
    if (node->left == node->right)
    {
        error->code = (int) DiffErrors::COMMON_HEIR;
        error->data = node;
        return DiffErrors::COMMON_HEIR;
    }

    return DiffErrors::NONE;
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

    return (int) DiffErrors::NONE;
}

//-----------------------------------------------------------------------------------------------------

DiffErrors TreeVerify(const tree_t* tree, error_t* error)
{
    assert(tree);
    assert(error);

    VerifyNodes(tree->root, error);

    return (DiffErrors) error->code;
}

//-----------------------------------------------------------------------------------------------------

static DiffErrors VerifyNodes(const Node* node, error_t* error)
{
    assert(node);
    assert(error);

    if (node->left != nullptr)
    {
        NodeVerify(node->left, error);
        RETURN_IF_TREE_ERROR((DiffErrors) error->code);
    }

    if (node->right != nullptr)
    {
        NodeVerify(node->right, error);
        RETURN_IF_TREE_ERROR((DiffErrors) error->code);
    }

    NodeVerify(node, error);

    return (DiffErrors) error->code;
}

//-----------------------------------------------------------------------------------------------------

int TreeDump(FILE* fp, const void* nodes, const char* func, const char* file, const int line)
{
    assert(nodes);

    LOG_START_DUMP(func, file, line);

    const tree_t* tree = (const tree_t*) nodes;

    TextTreeDump(fp, tree);
    DrawTreeGraph(tree);

    LOG_END();

    return (int) DiffErrors::NONE;
}

//-----------------------------------------------------------------------------------------------------

static void TextTreeDump(FILE* fp, const tree_t* tree)
{
    assert(tree);

    fprintf(fp, "<pre>");

    fprintf(fp, "<b>DUMPING TREE</b>\n");

    TreePrintEquation(fp, tree);

    fprintf(fp, "</pre>");
}

//-----------------------------------------------------------------------------------------------------

static void DrawTreeGraph(const tree_t* tree)
{
    assert(tree);

    FILE* dotf = fopen(DOT_FILE, "w");

    StartGraph(dotf);
    DrawNodes(dotf, tree, tree->root, 1);
    EndGraph(dotf);

    fclose(dotf);

    MakeImgFromDot(DOT_FILE);
}

//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::;::::::::::::::::::::::::::

static inline void DrawNodes(FILE* dotf, const tree_t* tree, const Node* node, const int rank)
{
    if (!node) return;

    fprintf(dotf, "%lld [shape=Mrecord, style=filled, fillcolor=\"lightblue\", color = darkblue, rank = %d, label=\" "
                  "{ node: %p | parent: %p | { type: " , node, rank, node, node->parent);

    PrintNodeDataType(dotf, node->type);

    fprintf(dotf, " | data: ");

    PrintNodeData(dotf, tree, node);

    fprintf(dotf, "} | { left: %p| right: %p } }\"]\n", node->left, node->right);

    DrawNodes(dotf, tree, node->left, rank + 1);
    DrawNodes(dotf, tree, node->right, rank + 1);

    if (node->parent != nullptr)
        fprintf(dotf, "%lld->%lld [color = blue]\n", node, node->parent);

    if (node->left != nullptr)
        fprintf(dotf, "%lld->%lld [color = black, fontcolor = black]\n", node, node->left);

    if (node->right != nullptr)
        fprintf(dotf, "%lld->%lld [color = black, fontcolor = black]\n", node, node->right);
}
