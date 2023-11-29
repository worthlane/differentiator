#include <string.h>

#include "expr_input_and_output.h"
#include "common/input_and_output.h"
#include "visual.h"

// ======================================================================
// EXPRESSION INPUT
// ======================================================================

static const char* NIL = "nil";

static Node*       ReadExpressionTree(expr_t* expr, Storage* info, Node* current_node, error_t* error);
static Node*       NodesPrefixRead(expr_t* expr, Storage* info, Node* current_node, error_t* error);
static void        ReadNodeData(expr_t* expr, Storage* info, NodeType* type, NodeValue* value,  error_t* error);

static inline void DeleteClosingBracketFromWord(Storage* info, char* read);
static char        CheckOpeningBracketInInput(Storage* info);

static Node*       ExpressionReadNewInfixNode(expr_t* expr, Storage* info, Node* parent_node, error_t* error);
static Node*       PrefixReadNewNode(expr_t* expr, Storage* info, Node* parent_node, error_t* error);
static bool        TryReadNumber(Storage* info, NodeType* type, NodeValue* value);

// ======================================================================
// EXPRESSION OUTPUT
// ======================================================================

static void        TextExpressionDump(FILE* fp, const expr_t* expr);
static void        NodesInfixPrint(FILE* fp, const expr_t* expr, const Node* node);
static void        NodesInfixPrintLatex(FILE* fp, const expr_t* expr, const Node* node);
static void        PrintNodeDataType(FILE* fp, const NodeType type);

static bool        CheckBracketsNeededInEquation(const Node* node);
static void        PutOpeningBracket(FILE* fp, bool need_bracket, bool figure_bracket);
static void        PutClosingBracket(FILE* fp, bool need_bracket, bool figure_bracket);

static void        LatexPrintTwoArgumentsOperation(FILE* fp, const expr_t* expr, const Node* node,
                                                   const char* opt, LatexOperationTypes type,
                                                   bool need_brackets, bool figure_brackets);
static void        LatexPrintOneArgumentOperation(FILE* fp, const expr_t* expr, const Node* node,
                                                  const char* opt, LatexOperationTypes type,
                                                  bool need_brackets, bool figure_brackets);

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

//-----------------------------------------------------------------------------------------------------

void PrintNodeData(FILE* fp, const expr_t* expr, const Node* node)
{
    assert(node);

    switch(node->type)
    {
        case (NodeType::NUMBER):
            fprintf(fp, "%g", node->value.val);
            break;
        case (NodeType::VARIABLE):
            fprintf(fp, "%s", expr->vars[node->value.var].variable_name);
            break;
        case (NodeType::OPERATOR):
            PrintOperator(fp, node->value.opt);
            break;
        default:
            fprintf(fp, " undefined ");
    }
}

//-----------------------------------------------------------------------------------------------------

#define DEF_OP(name, symb, ...) \
        case (Operators::name): \
            fprintf(fp, " ");   \
            fprintf(fp, name);  \
            fprintf(fp, " ");   \
            break;

static void PrintOperator(FILE* fp, const Operators sign)
{
    switch (sign)
    {
        #include "operations.h"

        default:
            fprintf(fp, " undefined_operator ");
    }

    #undef DEF_OP
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

    if (node->left == nullptr && node->right == nullptr)
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

#define DEF_OP(name, symb, priority, arg_amt, action, type, tex_symb, need_brackets, figure_brackets, ...)      \
    case (Operators::name):                                                                                     \
    {                                                                                                           \
        if (arg_amt == 2)                                                                                       \
            LatexPrintTwoArgumentsOperation(fp, expr, node, tex_symb, type, need_brackets, figure_brackets);    \
        if (arg_amt == 1)                                                                                       \
            LatexPrintOneArgumentOperation(fp, expr, node, tex_symb, type, need_brackets, figure_brackets);     \
        break;                                                                                                  \
    }

static void NodesInfixPrintLatex(FILE* fp, const expr_t* expr, const Node* node)
{
    if (!node) { return; }

    if (node->left == nullptr && node->right == nullptr)
    {
        PrintNodeData(fp, expr, node);
        return;
    }

    if (node->type == NodeType::OPERATOR)
    {
        switch (node->value.opt)
        {
            #include "operations.h"

            default:
                return;
        }
    }

}

#undef DEF_OP

//-----------------------------------------------------------------------------------------------------

static void LatexPrintTwoArgumentsOperation(FILE* fp, const expr_t* expr, const Node* node,
                                            const char* opt, LatexOperationTypes type,
                                            bool need_brackets, bool figure_brackets)
{
    assert(opt);
    assert(expr);
    assert(node);

    bool need_brackets_on_the_left  = (need_brackets || CheckBracketsNeededInEquation(node->left));
    bool need_brackets_on_the_right = (need_brackets || CheckBracketsNeededInEquation(node->right));

    if (type == LatexOperationTypes::PREFIX)
        fprintf(fp, "%s", opt);

    PutOpeningBracket(fp, need_brackets_on_the_left, figure_brackets);
    NodesInfixPrintLatex(fp, expr, node->left);
    PutClosingBracket(fp, need_brackets_on_the_left, figure_brackets);

    if (type == LatexOperationTypes::INFIX)
        fprintf(fp, " %s ", opt);

    PutOpeningBracket(fp, need_brackets_on_the_right, figure_brackets);
    NodesInfixPrintLatex(fp, expr, node->right);
    PutClosingBracket(fp, need_brackets_on_the_right, figure_brackets);

    if (type == LatexOperationTypes::POSTFIX)
        fprintf(fp, "%s", opt);
}

//-----------------------------------------------------------------------------------------------------

static void LatexPrintOneArgumentOperation(FILE* fp, const expr_t* expr, const Node* node,
                                            const char* opt, LatexOperationTypes type,
                                            bool need_brackets, bool figure_brackets)
{
    assert(opt);
    assert(expr);
    assert(node);

    bool need_brackets_on_the_left  = (need_brackets || CheckBracketsNeededInEquation(node->left));
    bool need_brackets_on_the_right = (need_brackets || CheckBracketsNeededInEquation(node->right));

    if (type == LatexOperationTypes::PREFIX)
    {
        fprintf(fp, "%s", opt);
        PutOpeningBracket(fp, need_brackets_on_the_right, figure_brackets);
        NodesInfixPrintLatex(fp, expr, node->right);
        PutClosingBracket(fp, need_brackets_on_the_right, figure_brackets);
    }

    if (type == LatexOperationTypes::POSTFIX)
    {
        PutOpeningBracket(fp, need_brackets_on_the_left, figure_brackets);
        NodesInfixPrintLatex(fp, expr, node->left);
        PutClosingBracket(fp, need_brackets_on_the_left, figure_brackets);
        fprintf(fp, "%s", opt);
    }
}

//-----------------------------------------------------------------------------------------------------

static void PutOpeningBracket(FILE* fp, bool need_bracket, bool figure_bracket)
{
    if (!need_bracket)  return;

    if (figure_bracket)
        fputc('{', fp);
    else
        fputc('(', fp);
}

//-----------------------------------------------------------------------------------------------------

static void PutClosingBracket(FILE* fp, bool need_bracket, bool figure_bracket)
{
    if (!need_bracket)  return;

    if (figure_bracket)
        fputc('}', fp);
    else
        fputc(')', fp);
}

//-----------------------------------------------------------------------------------------------------

static bool CheckBracketsNeededInEquation(const Node* node)
{
    if (!node) return false;

    if (node->type != NodeType::OPERATOR)
        return false;

    int kid_priority    = GetOperationPriority(node->value.opt);
    int parent_priority = GetOperationPriority(node->parent->value.opt);

    if (kid_priority <= parent_priority)
        return true;

    return false;
}

//-----------------------------------------------------------------------------------------------------

static inline void DeleteClosingBracketFromWord(Storage* info, char* read)
{
    assert(read);

    size_t bracket_pos = strlen(read) - 1;
    if (read[bracket_pos] == ')')
    {
        read[bracket_pos] = '\0';
        Bufungetc(info);
    }
}

//-----------------------------------------------------------------------------------------------------

#define DEF_OP(name, symb, priority, ...)   \
        case (Operators::name):             \
            return priority;                \

static int GetOperationPriority(const Operators sign)
{
    switch (sign)
    {
        #include "operations.h"

        default:
            return 0;
    }
}

#undef DEF_OP

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
        error->code = (int) ExpressionErrors::NO_EXPRESSION;
        return;
    }
    else
    {
        Bufungetc(info);
        root = ReadExpressionTree(expr, info, root, error);
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
        error->code = (int) ExpressionErrors::NO_EXPRESSION;
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

static Node* ReadExpressionTree(expr_t* expr, Storage* info, Node* current_node, error_t* error)
{
    assert(error);
    assert(expr);
    assert(info);

    SkipBufSpaces(info);

    char opening_bracket_check = CheckOpeningBracketInInput(info);

    if (opening_bracket_check == '(')
    {
        Node* new_node = ExpressionReadNewInfixNode(expr, info, current_node, error);
        if (error->code != (int) ExpressionErrors::NONE)
            return nullptr;

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

static Node* ExpressionReadNewInfixNode(expr_t* expr, Storage* info, Node* parent_node, error_t* error)
{
    assert(expr);
    assert(info);
    assert(error);

    Node* node = MakeNode(PZN_TYPE, ZERO_VALUE, nullptr, nullptr, nullptr);

    NodeType type = PZN_TYPE;
    NodeValue val = ZERO_VALUE;

    Node* left = ReadExpressionTree(expr, info, node, error);
    if (error->code != (int) ExpressionErrors::NONE)
        return nullptr;

    SkipBufSpaces(info);

    ReadNodeData(expr, info, &type, &val, error);
    if (error->code != (int) ExpressionErrors::NONE)
        return nullptr;

    Node* right = ReadExpressionTree(expr, info, node, error);
    if (error->code != (int) ExpressionErrors::NONE)
        return nullptr;

    SkipBufSpaces(info);

    node->parent = parent_node;
    node->type   = type;
    node->value  = val;
    node->left   = left;
    node->right  = right;

    return node;
}

//-----------------------------------------------------------------------------------------------------

static Node* PrefixReadNewNode(expr_t* expr, Storage* info, Node* parent_node, error_t* error)
{
    assert(expr);
    assert(info);
    assert(error);

    Node* node = MakeNode(PZN_TYPE, ZERO_VALUE, nullptr, nullptr, nullptr);

    NodeType type = PZN_TYPE;
    NodeValue val = ZERO_VALUE;

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

    DeleteClosingBracketFromWord(info, word);

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
        error->code = (int) ExpressionErrors::INVALID_SYNTAX;
    }
    else
    {
        *type      = NodeType::VARIABLE;
        value->var = id;
    }
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

#define DEF_OP(name, symb, ...)                         \
        if (!strncmp(word, symb, MAX_STRING_LEN))       \
            return Operators::name;                   \
        else                                            \


static Operators DefineOperator(const char* word)
{
    assert(word);

    #include "operations.h"

    /* else */ return Operators::UNK;
}

#undef DEF_OP

//-----------------------------------------------------------------------------------------------------

int NodeDump(FILE* fp, const void* dumping_node, const char* func, const char* file, const int line)
{
    assert(dumping_node);

    LOG_START_DUMP(func, file, line);

    const Node* node = (const Node*) dumping_node;

    fprintf(fp, "NODE [%p]<br>\n"
                "LEFT > [%p]<br>\n"
                "RIGHT > [%p]<br>\n"
                "TYPE > ", node, node->left, node->right);

    PrintNodeDataType(fp, node->type);

    fprintf(fp, "<br>\n"
                "VALUE > %g<br>\n", node->value);

    LOG_END();

    return (int) ExpressionErrors::NONE;
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

    FILE* dotf = fopen(TMP_DOT_FILE, "w");
    if (dotf == nullptr)
    {
        PrintLog("CAN NOT DRAW TREE GRAPH<br>\n");
        return;
    }

    StartGraph(dotf);
    DrawNodes(dotf, expr, expr->root, 1);
    EndGraph(dotf);

    fclose(dotf);

    MakeImgFromDot(TMP_DOT_FILE);
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

//------------------------------------------------------------------

void DrawExprGraphic(const expr_t* expr)
{
    assert(expr);

    FILE* gnuf = fopen(TMP_GNU_FILE, "w");
    if (gnuf == nullptr)
        PrintLog("CAN NOT DRAW GRAPHIC");

    char* img_name = GenImgName();

    StartGraphic(gnuf, img_name);

    

    EndGraphic(gnuf);
    MakeImgFromGpl(TMP_GNU_FILE, img_name);

    free(img_name);
}