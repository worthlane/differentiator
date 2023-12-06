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

#ifdef L
#undef L
#endif
#define L(node)   node->left

#ifdef R
#undef R
#endif
#define R(node)   node->right

#ifdef TYPE
#undef TYPE
#endif
#define TYPE(node)   node->type

#ifdef VAL
#undef VAL
#endif
#define VAL(node)   node->value.val

#ifdef VAR
#undef VAR
#endif
#define VAR(node)   node->value.var

#ifdef OPT
#undef OPT
#endif
#define OPT(node)   node->value.opt

#ifdef _NUM
#undef _NUM
#endif
#define _NUM(num)  MakeNode(NodeType::NUMBER, {.val = num}, nullptr, nullptr, nullptr)

#ifdef _VAR
#undef _VAR
#endif
#define _VAR(id)   MakeNode(NodeType::VARIABLE, {.var = id}, nullptr, nullptr, nullptr)

#ifdef _OPT
#undef _OPT
#endif
#define _OPT(op)  MakeNode(NodeType::OPERATOR, {.opt = op}, nullptr, nullptr, nullptr)

#define DEF_OP(name, symb, priority, arg_amt, ...)                                                                          \
                    static inline Node* _##name(Node* left = nullptr, Node* right = nullptr)                                          \
                    {                                                                                                       \
                        if (arg_amt == 1)                                                                                   \
                        {                                                                                                   \
                            right = left;                                                                                   \
                            left  = nullptr;                                                                                \
                        }                                                                                                   \
                                                                                                                            \
                        return MakeNode(NodeType::OPERATOR, {.opt = Operators::name}, left, right, nullptr);                \
                    }

#include "operations.h"

#undef DEF_OP
