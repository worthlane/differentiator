#ifdef d
#undef d
#endif
#define d(node) Differentiate(node, id, error)

#ifdef CPY
#undef CPY
#endif
#define CPY(node) Copy(node, error)

#ifdef NUM
#undef NUM
#endif
#define NUM(num) MakeNode(NodeType::NUMBER, {.val = num}, nullptr, nullptr, nullptr, error)

#ifdef _ADD
#undef _ADD
#endif
#define _ADD(left, right) MakeNode(NodeType::OPERATOR, {.opt = Operators::ADD}, left, right, nullptr, error)

#ifdef _SUB
#undef _SUB
#endif
#define _SUB(left, right) MakeNode(NodeType::OPERATOR, {.opt = Operators::SUB}, left, right, nullptr, error)

#ifdef _MUL
#undef _MUL
#endif
#define _MUL(left, right) MakeNode(NodeType::OPERATOR, {.opt = Operators::MUL}, left, right, nullptr, error)

#ifdef _DIV
#undef _DIC
#endif
#define _DIV(left, right) MakeNode(NodeType::OPERATOR, {.opt = Operators::DIV}, left, right, nullptr, error)

#ifdef _DEG
#undef _DEG
#endif
#define _DEG(left, right) MakeNode(NodeType::OPERATOR, {.opt = Operators::DEG}, left, right, nullptr, error)

#ifdef _LN
#undef _LN
#endif
#define _LN(right) MakeNode(NodeType::OPERATOR, {.opt = Operators::LN}, nullptr, right, nullptr, error)

#ifdef _SIN
#undef _SIN
#endif
#define _SIN(right) MakeNode(NodeType::OPERATOR, {.opt = Operators::SIN}, nullptr, right, nullptr, error)

#ifdef _COS
#undef _COS
#endif
#define _COS(right) MakeNode(NodeType::OPERATOR, {.opt = Operators::COS}, nullptr, right, nullptr, error)
