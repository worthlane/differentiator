#include "expr_input.h"
#include "expression.h"
#include "operations.h"
#include "dsl.h"
#include "common/file_read.h"

static const int MAX_STRING_LEN = 1000;

static const char* s = NULL;
static size_t      p = 0;

static inline void syn_error();

#define syn_assert(stat) assert(stat);

static Node* GetG(Tokens* token);
static Node* GetN(Tokens* token);
static Node* GetE(Tokens* token);
static Node* GetT(Tokens* token);
static Node* GetP(Tokens* token);

static Node* GetNumber(LinesStorage* info);
static void  TokenizeInput(LinesStorage* info, Node* tokens[]);

// ==============================================================

static inline void syn_error()
{
    printf("SYNTAX ERROR\n");

    abort();
}

/*static inline void syn_assert(bool statement)
{
    if (statement)
        return;

    syn_error();
}*/

//-----------------------------------------------------------------------------------------------------

void GetExpression(LinesStorage* info, expr_t* expr, error_t* error)
{
    assert(expr);
    assert(info);

    Tokens tokens = {.ptr = 0};

    TokenizeInput(info, tokens.buf);

    Node* root = GetG(&tokens);

    expr->root = root;
}

// -----------------------------------------------------------------------------------------------------

static void TokenizeInput(LinesStorage* info, Node* tokens[])
{
    assert(info);
    assert(tokens);

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
            case ' ':
            case '\n':
            case '\t':
            {
                SkipBufSpaces(info);
                break;
            }
            case '0':
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
            case '8':
            case '9':
            {
                Bufungetc(info);
                Node* num = GetNumber(info);
                if (num != nullptr)
                    tokens[i++] = num;
                break;
            }
            default:
                printf("$$$\n");
        }
    }
}

// -------------------------------------------------------------

static Node* GetNumber(LinesStorage* info)
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
        return nullptr;
    else
        return _NUM(num);
}

// ==============================================================

static Node* GetG(Tokens* tokens)
{
    assert(tokens);

    Node* val = GetE(tokens);
    syn_assert(TYPE(tokens->buf[tokens->ptr]) == NodeType::OPERATOR &&
                OPT(tokens->buf[tokens->ptr]) == Operators::END);

    return val;
}

// -------------------------------------------------------------

static Node* GetN(Tokens* tokens)
{

    Node* num = tokens->buf[tokens->ptr];
    tokens->ptr++;

    syn_assert(TYPE(num) == NodeType::NUMBER);

    return num;
}

// -------------------------------------------------------------

static Node* GetE(Tokens* tokens)
{

    Node* val = GetT(tokens);

    while   (TYPE(tokens->buf[tokens->ptr]) == NodeType::OPERATOR &&
             (OPT(tokens->buf[tokens->ptr]) == Operators::ADD ||
              OPT(tokens->buf[tokens->ptr]) == Operators::SUB))
    {

        Node* op = tokens->buf[tokens->ptr];
        tokens->ptr++;
        Node* val2 = GetT(tokens);

        val = ConnectNodes(op, val, val2);

        /*switch (OPT(op))
        {
            case Operators::ADD:
                val = _ADD(val, val2);
                break;

            case Operators::SUB:
                val = _SUB(val, val2);
                break;

            default:
                break;
        }*/
    }
    return val;
}

// -------------------------------------------------------------

static Node* GetT(Tokens* tokens)
{
    Node* val = GetP(tokens);

    while (TYPE(tokens->buf[tokens->ptr]) == NodeType::OPERATOR &&
             (OPT(tokens->buf[tokens->ptr]) == Operators::DIV ||
              OPT(tokens->buf[tokens->ptr]) == Operators::MUL))
    {

        Node* op = tokens->buf[tokens->ptr];
        tokens->ptr++;
        Node* val2 = GetP(tokens);

        val = ConnectNodes(op, val, val2);
    }
    return val;
}

// -------------------------------------------------------------

static Node* GetP(Tokens* tokens)
{
    if (TYPE(tokens->buf[tokens->ptr]) == NodeType::OPERATOR &&
         OPT(tokens->buf[tokens->ptr]) == Operators::OPENING_BRACKET)
    {
        // NodeDtor(tokens->buf[tokens->ptr]);
        // Node* val = _NUM(0);
        tokens->ptr++;
        Node* val = GetE(tokens);

        syn_assert(TYPE(tokens->buf[tokens->ptr]) == NodeType::OPERATOR &&
                    OPT(tokens->buf[tokens->ptr]) == Operators::CLOSING_BRACKET);
        // NodeDtor(tokens->buf[tokens->ptr]);
        tokens->ptr++;

        return val;
    }

    return GetN(tokens);
}

// -------------------------------------------------------------

