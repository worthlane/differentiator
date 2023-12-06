#include <stdio.h>
#include <string.h>

#include "expr_input.h"
#include "expression.h"
#include "operations.h"
#include "dsl.h"
#include "common/file_read.h"
#include "common/errors.h"

static const int MAX_STRING_LEN = 1000;

#ifdef  SYN_ASSERT
#undef  SYN_ASSERT
#endif
#define SYN_ASSERT(stat)                                                                            \
        if (!(stat))                                                                                \
        {                                                                                           \
            error->code = (int) ExpressionErrors::INVALID_SYNTAX;                                   \
            LOG_START(__func__, __FILE__, __LINE__);                                                \
            PrintLog("SYNTAX ASSERT\"" #stat "\"<br>\n"                                             \
                     "IN FUNCTION %s FROM FILE \"%s\"(%d)<br>\n", __func__, __FILE__, __LINE__);    \
            LOG_END();                                                                              \
            return nullptr;                                                                         \
        }                                                                                           \


static Node* GetG(Tokens* token, error_t* error);
static Node* GetN(Tokens* token, error_t* error);
static Node* GetE(Tokens* token, error_t* error);
static Node* GetT(Tokens* token, error_t* error);
static Node* GetP(Tokens* token, error_t* error);
static Node* GetS(Tokens* tokens, error_t* error);
static Node* GetDeg(Tokens* tokens, error_t* error);

static Operators DefineOperator(const char* word);

static Node* GetNumber(LinesStorage* info, error_t* error);
static Node* GetWord(LinesStorage* info, expr_t* expr, error_t* error);
static void  TokenizeInput(LinesStorage* info, Node* tokens[], expr_t* expr, error_t* error);

//-----------------------------------------------------------------------------------------------------

void GetExpression(LinesStorage* info, expr_t* expr, error_t* error)
{
    assert(expr);
    assert(info);

    Tokens tokens = {.ptr = 0};

    TokenizeInput(info, tokens.buf, expr, error);
    BREAK_IF_ERROR(error);

    Node* root = GetG(&tokens, error);
    BREAK_IF_ERROR(error);

    expr->root = root;
}

// -----------------------------------------------------------------------------------------------------

static void TokenizeInput(LinesStorage* info, Node* tokens[], expr_t* expr, error_t* error)
{
    assert(info);
    assert(tokens);
    assert(error);

    int i = 0;

    while (info->ptr <= info->text_len)
    {
        int ch = Bufgetc(info);

        switch (ch)
        {
            case '+':
            {
                tokens[i++] = _ADD();
                break;
            }
            case '-':
            {
                tokens[i++] = _SUB();
                break;
            }
            case '/':
            {
                tokens[i++] = _DIV();
                break;
            }
            case '*':
            {
                tokens[i++] = _MUL();
                break;
            }
            case '^':
            {
                tokens[i++] = _DEG();
                break;
            }
            case '(':
            {
                tokens[i++] = MakeNode(NodeType::OPERATOR, {.opt = Operators::OPENING_BRACKET});
                break;
            }
            case ')':
            {
                tokens[i++] = MakeNode(NodeType::OPERATOR, {.opt = Operators::CLOSING_BRACKET});
                break;
            }
            case '\0':
            {
                tokens[i++] = MakeNode(NodeType::OPERATOR, {.opt = Operators::END});
                break;
            }
            case '\n':
            case ' ':
            case '\t':
            {
                SkipBufSpaces(info);
                break;
            }
            case '0': case '1': case '2': case '3': case '4': case '5': case '6': case '7': case '8': case '9':
            {
                Bufungetc(info);
                Node* num = GetNumber(info, error);
                if (num != nullptr)
                    tokens[i++] = num;
                break;
            }
            case 'a': case 'b': case 'c': case 'd': case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
            case 'k': case 'l': case 'm': case 'n': case 'o': case 'p': case 'q': case 'r': case 's': case 't':
            case 'u': case 'v': case 'w': case 'x': case 'y': case 'z':
            case 'A': case 'B': case 'C': case 'D': case 'E': case 'F': case 'G': case 'H': case 'I': case 'J':
            case 'K': case 'L': case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R': case 'S': case 'T':
            case 'U': case 'V': case 'W': case 'X': case 'Y': case 'Z': case '_':
            {
                Bufungetc(info);
                Node* word = GetWord(info, expr, error);
                if (word != nullptr)
                    tokens[i++] = word;
                break;
            }
            default:
                error->code = (int) ExpressionErrors::INVALID_SYNTAX;
        }

        BREAK_IF_ERROR(error);
    }
}

//-----------------------------------------------------------------------------------------------------

static Node* GetWord(LinesStorage* info, expr_t* expr, error_t* error)
{
    assert(expr);
    assert(info);
    assert(error);

    char buffer[MAX_STRING_LEN] = "";
    int i = 0;

    int ch = Bufgetc(info);

    while (('a' <= ch && ch <= 'z') || ('A' <= ch && ch <= 'Z') || (ch == '_') && i < MAX_STRING_LEN)
    {
        buffer[i++] = ch;
        ch = Bufgetc(info);
    }
    buffer[i] = '\0';

    Bufungetc(info);

    Operators op = DefineOperator(buffer);
    if (op != Operators::UNKNOWN)
        return _OPT(op);

    int id = FindVariableAmongSaved(expr->vars, buffer);

    if (id == NO_VARIABLE)
        id = SaveVariable(expr->vars, buffer);

    if (id == NO_VARIABLE)
    {
        error->code = (int) ExpressionErrors::INVALID_SYNTAX;
        return nullptr;
    }
    else
        return _VAR(id);
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

    /* else */ return Operators::UNKNOWN;
}

#undef DEF_OP

// -------------------------------------------------------------

static Node* GetNumber(LinesStorage* info, error_t* error)
{
    assert(info);

    char buffer[MAX_STRING_LEN] = "";
    int  i = 0;

    int ch = Bufgetc(info);

    while ('0' <= ch && ch <= '9' && i < MAX_STRING_LEN)
    {
        buffer[i++] = ch;
        ch = Bufgetc(info);
    }

    if (ch == '.')
    {
        buffer[i++] = ch;
        ch = Bufgetc(info);

        while ('0' <= ch && ch <= '9' && i < MAX_STRING_LEN)
        {
            buffer[i++] = ch;
            ch = Bufgetc(info);
        }
    }

    Bufungetc(info);

    char*  end = 0;
    double num = strtod(buffer, &end);

    if (num == 0 || buffer[0] == '\0')
    {
        error->code = (int) ExpressionErrors::INVALID_SYNTAX;
        return nullptr;
    }
    else
        return _NUM(num);
}

// ==============================================================

static Node* GetG(Tokens* tokens, error_t* error)
{
    assert(tokens);
    assert(error);

    Node* val = GetE(tokens, error);
    if (error->code != (int) ExpressionErrors::NONE) return nullptr;
    SYN_ASSERT(TYPE(tokens->buf[tokens->ptr]) == NodeType::OPERATOR &&
                OPT(tokens->buf[tokens->ptr]) == Operators::END);

    return val;
}

// -------------------------------------------------------------

static Node* GetN(Tokens* tokens, error_t* error)
{
    assert(tokens);
    assert(error);

    Node* val = nullptr;

    if  (TYPE(tokens->buf[tokens->ptr]) == NodeType::OPERATOR &&
         (OPT(tokens->buf[tokens->ptr]) == Operators::ADD ||
          OPT(tokens->buf[tokens->ptr]) == Operators::SUB))
    {
        Node* op = tokens->buf[tokens->ptr];
        tokens->ptr++;

        Node* num = tokens->buf[tokens->ptr];
        tokens->ptr++;

        SYN_ASSERT(TYPE(num) == NodeType::NUMBER || TYPE(num) == NodeType::VARIABLE);

        val = ConnectNodes(op, _NUM(0), num);
    }
    else
    {
        Node* num = tokens->buf[tokens->ptr];
        tokens->ptr++;

        SYN_ASSERT(TYPE(num) == NodeType::NUMBER || TYPE(num) == NodeType::VARIABLE);

        val = num;
    }

    return val;
}

// -------------------------------------------------------------

static Node* GetE(Tokens* tokens, error_t* error)
{
    assert(tokens);
    assert(error);

    Node* val = GetT(tokens, error);

    while   (TYPE(tokens->buf[tokens->ptr]) == NodeType::OPERATOR &&
             (OPT(tokens->buf[tokens->ptr]) == Operators::ADD ||
              OPT(tokens->buf[tokens->ptr]) == Operators::SUB))
    {
        Node* op = tokens->buf[tokens->ptr];
        tokens->ptr++;
        Node* val2 = GetT(tokens, error);

        val = ConnectNodes(op, val, val2);
    }
    return val;
}

// -------------------------------------------------------------

static Node* GetT(Tokens* tokens, error_t* error)
{
    assert(error);
    assert(tokens);

    Node* val = GetDeg(tokens, error);

    while (TYPE(tokens->buf[tokens->ptr]) == NodeType::OPERATOR &&
           (OPT(tokens->buf[tokens->ptr]) == Operators::DIV ||
            OPT(tokens->buf[tokens->ptr]) == Operators::MUL))
    {
        Node* op = tokens->buf[tokens->ptr];
        tokens->ptr++;
        Node* val2 = GetDeg(tokens, error);

        val = ConnectNodes(op, val, val2);
    }
    return val;
}

// -------------------------------------------------------------

static Node* GetDeg(Tokens* tokens, error_t* error)
{
    assert(error);
    assert(tokens);

    Node* val = GetS(tokens, error);

    while (TYPE(tokens->buf[tokens->ptr]) == NodeType::OPERATOR &&
            OPT(tokens->buf[tokens->ptr]) == Operators::DEG)
    {

        Node* op = tokens->buf[tokens->ptr];
        tokens->ptr++;
        Node* val2 = GetS(tokens, error);

        val = ConnectNodes(op, val, val2);
    }
    return val;
}

// -------------------------------------------------------------

static Node* GetS(Tokens* tokens, error_t* error)
{
    assert(tokens);
    assert(error);

    if (TYPE(tokens->buf[tokens->ptr]) == NodeType::OPERATOR &&
        (OPT(tokens->buf[tokens->ptr]) == Operators::SIN    || OPT(tokens->buf[tokens->ptr]) == Operators::COS    ||
         OPT(tokens->buf[tokens->ptr]) == Operators::TAN    || OPT(tokens->buf[tokens->ptr]) == Operators::COT    ||
         OPT(tokens->buf[tokens->ptr]) == Operators::ARCSIN || OPT(tokens->buf[tokens->ptr]) == Operators::ARCCOS ||
         OPT(tokens->buf[tokens->ptr]) == Operators::ARCTAN || OPT(tokens->buf[tokens->ptr]) == Operators::ARCCOT ||
         OPT(tokens->buf[tokens->ptr]) == Operators::LN     || OPT(tokens->buf[tokens->ptr]) == Operators::EXP))
    {
        Node* op = tokens->buf[tokens->ptr];
        tokens->ptr++;
        Node* val = GetP(tokens, error);

        val = ConnectNodes(op, nullptr, val);

        return val;
    }

    return GetP(tokens, error);
}

// -------------------------------------------------------------

static Node* GetP(Tokens* tokens, error_t* error)
{
    assert(tokens);
    assert(error);

    if (TYPE(tokens->buf[tokens->ptr]) == NodeType::OPERATOR &&
         OPT(tokens->buf[tokens->ptr]) == Operators::OPENING_BRACKET)
    {
        tokens->ptr++;
        Node* val = GetE(tokens, error);

        SYN_ASSERT(TYPE(tokens->buf[tokens->ptr]) == NodeType::OPERATOR &&
                    OPT(tokens->buf[tokens->ptr]) == Operators::CLOSING_BRACKET);

        tokens->ptr++;

        return val;
    }

    return GetN(tokens, error);
}

// -------------------------------------------------------------

