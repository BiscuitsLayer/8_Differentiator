#include <cstdio>
#include <cassert>
#include <cstdlib>
#include <cstring>
#include <cctype>
#include <cmath>
#include <typeinfo>

//TODO Вычислялка для вещественных чисел
//TODO Отправка почты

const int STR_LEN = 100;
const int VARS_SIZE = 3;
const int CONSTS_SIZE = 3;
typedef double num_t;
#define NUM_T_FORMAT "%lg"
typedef size_t type_t;

enum Types {
    TYPE_OP = 0,
    TYPE_NUM = 1,
    TYPE_VAR = 2,
    TYPE_CONST = 3
};

enum Operations {
#define OP_CMD(name_u, id, name_l) OP_##name_u = id,
#include "diff_parts/diff.commands"
#undef OP_CMD
};

struct Variable_t {
    char name[STR_LEN]{};
    num_t value = 0;
};

typedef Variable_t Constant_t;

typedef struct Node {
    Node *parent = nullptr;
    num_t data = 0;
    size_t level = 0;
    type_t type = 1;
    Node *right = nullptr;
    Node *left = nullptr;
} Node;

char *s = nullptr;

Node *NodeInit (Node *parent, num_t data, type_t type, Node *left = nullptr, Node *right = nullptr);
void GetTreeImage (FILE *writefile, Node *node, Variable_t *vars, Constant_t *consts);
void PrintNode (FILE *writefile, Node *node, Variable_t *vars, Constant_t *consts);
char *MakeNodeLabel (Node *node, Variable_t *vars, Constant_t *consts);
Node *GetG (const char *str, Variable_t *vars, Constant_t *consts, size_t *vars_cur_size, size_t *consts_cur_size);
Node *GetN (Variable_t *vars, Constant_t *consts, size_t *vars_cur_size, size_t *consts_cur_size);
Node *GetE (Variable_t *vars, Constant_t *consts, size_t *vars_cur_size, size_t *consts_cur_size);
Node *GetT (Variable_t *vars, Constant_t *consts, size_t *vars_cur_size, size_t *consts_cur_size);
Node *GetF (Variable_t *vars, Constant_t *consts, size_t *vars_cur_size, size_t *consts_cur_size);
Node *GetP (Variable_t *vars, Constant_t *consts, size_t *vars_cur_size, size_t *consts_cur_size);
bool GetOperation (char *str, Node **node, Variable_t *vars, Constant_t *consts, size_t *vars_cur_size, size_t *consts_cur_size);
Node *GetExpression (FILE *readfile, Node *parent, Variable_t *vars, Constant_t *consts, size_t *vars_cur_size, size_t *consts_cur_size);
Node &Copy (Node *node);
Node &Diff (Node *node);
Node *Optimizer (Node *node, Variable_t *vars, size_t *vars_cur_size);
num_t GetTreeValue (Node *node, Variable_t *vars, Constant_t *consts);
void GetTreeImage (FILE *writefile, Node *node, Variable_t *vars, Constant_t *consts);
void PrintNode (FILE *writefile, Node *node, Variable_t *vars, Constant_t *consts);
char *MakeNodeLabel (Node *node, Variable_t *vars, Constant_t *consts);
size_t VarSearch (Variable_t *vars, size_t *vars_cur_size, char *name);
bool TreeContainsVar (Node *node, Variable_t *vars, size_t *vars_cur_size);
int ConstSearch (Variable_t *consts, size_t *consts_cur_size, char *name);
void TreeOffsetCorrecter (Node *node);
void FreeNode (Node *node);

namespace opt {
    Node *MulZero (Node *node, bool *flag, Variable_t *vars = nullptr, size_t *vars_cur_size = nullptr);
    Node *SumZero (Node *node, bool *flag, Variable_t *vars = nullptr, size_t *vars_cur_size = nullptr);
    Node *DivZero (Node *node, bool *flag, Variable_t *vars = nullptr, size_t *vars_cur_size = nullptr);
    Node *PowZero (Node *node, bool *flag, Variable_t *vars = nullptr, size_t *vars_cur_size = nullptr);
    Node *MulUnit (Node *node, bool *flag, Variable_t *vars = nullptr, size_t *vars_cur_size = nullptr);
    Node *DivUnit (Node *node, bool *flag, Variable_t *vars = nullptr, size_t *vars_cur_size = nullptr);
    Node *PowUnit (Node *node, bool *flag, Variable_t *vars = nullptr, size_t *vars_cur_size = nullptr);
    Node *NumSum (Node *node, bool *flag, Variable_t *vars = nullptr, size_t *vars_cur_size = nullptr);
    Node *SimplePower (Node *node, bool *flag, Variable_t *vars, size_t *vars_cur_size);
}

namespace LaTeX {
    void GetTreeTex (Node *node1, Node *node2, const char *msg1, const char *msg2, FILE *writefile, Variable_t *vars, Constant_t *consts);
    void PrintNodeTex (Node *node, FILE *writefile, Variable_t *vars, Constant_t *consts);
}

Node *GetG (const char *str, Variable_t *vars, Constant_t *consts, size_t *vars_cur_size, size_t *consts_cur_size) {
    s = (char *) str;
    Node *node = GetE (vars, consts, vars_cur_size, consts_cur_size);
    assert (*s++ == '\0');
    return node;
}

Node *GetE (Variable_t *vars, Constant_t *consts, size_t *vars_cur_size, size_t *consts_cur_size) {
    Node *node = NodeInit (nullptr, 0, TYPE_OP);
    node->left = GetT (vars, consts, vars_cur_size, consts_cur_size);
    while (*s == '+' || *s == '-') {
        char op = *s;
        ++s;
        if (op == '+')
            node->data = OP_SUM;
        else
            node->data = OP_SUB;
        node->right = GetT (vars, consts, vars_cur_size, consts_cur_size);
        if (*s == '+' || *s == '-') {
            Node *new_node = NodeInit(nullptr, 0, TYPE_OP, node);
            node = new_node;
        }
    }
    if (node->right)
        return node;
    else
        return node->left;
}

Node *GetS (Variable_t *vars, Constant_t *consts, size_t *vars_cur_size, size_t *consts_cur_size) {
    Node *node = NodeInit (nullptr, OP_POW1, TYPE_OP);
    node->left = GetP (vars, consts, vars_cur_size, consts_cur_size);
    if (*s == '^') {
        ++s;
        node->right = GetP (vars, consts, vars_cur_size, consts_cur_size);
    }
    if (node->right)
        return node;
    else
        return node->left;
}

Node *GetT (Variable_t *vars, Constant_t *consts, size_t *vars_cur_size, size_t *consts_cur_size) {
    Node *node = NodeInit (nullptr, 0, TYPE_OP);
    node->left = GetF (vars, consts, vars_cur_size, consts_cur_size);
    while (*s == '*' || *s == '/') {
        char op = *s;
        ++s;
        if (op == '*')
            node->data = OP_MUL;
        else
            node->data = OP_DIV;
        node->right = GetF (vars, consts, vars_cur_size, consts_cur_size);
        if (*s == '*' || *s == '/') {
            Node *new_node = NodeInit(nullptr, 0, TYPE_OP, node);
            node = new_node;
        }
    }
    if (node->right)
        return node;
    else
        return node->left;
}

Node *GetF (Variable_t *vars, Constant_t *consts, size_t *vars_cur_size, size_t *consts_cur_size) {
    Node *node = nullptr;
#define OP_CMD(name_u, name_len, name_l) else if (strncmp (s, #name_l, name_len) == 0) {    \
    s += name_len;                                                                          \
    node = NodeInit(nullptr, OP_##name_u, TYPE_OP);                                         \
    assert (*s == '(');                                                                     \
    ++s;                                                                                    \
    node->left = GetE(vars, consts, vars_cur_size, consts_cur_size);                        \
    assert (*s == ')');                                                                     \
    ++s;                                                                                    \
}
    if (false) {}

    OP_CMD(SIN, 3, sin)
    OP_CMD(COS, 3, cos)
    OP_CMD(TG, 2, tg)
    OP_CMD(CTG, 3, ctg)
    OP_CMD(SH, 2, sh)
    OP_CMD(CH, 2, ch)
    OP_CMD(TH, 2, th)
    OP_CMD(CTH, 3, cth)
    OP_CMD(LN, 2, ln)
    OP_CMD(ARCSIN, 6, arcsin)
    OP_CMD(ARCCOS, 6, arccos)
    OP_CMD(ARCTG, 5, arctg)
    OP_CMD(ARCCTG, 6, arcctg)
    OP_CMD(ARCSH, 5, arcsh)
    OP_CMD(ARCCH, 5, arcch)
    OP_CMD(ARCTH, 5, arcth)
    OP_CMD(ARCCTH, 6, arccth)

#undef OP_CMD
    else
        node = GetS (vars, consts, vars_cur_size, consts_cur_size);
    return node;
}

Node *GetP (Variable_t *vars, Constant_t *consts, size_t *vars_cur_size, size_t *consts_cur_size) {
    Node *node = nullptr;
    if (*s == '(') {
        ++s;
        node = GetE (vars, consts, vars_cur_size, consts_cur_size);
        assert (*s == ')');
        ++s;
    } else
        node = GetN (vars, consts, vars_cur_size, consts_cur_size);
    return node;
}

Node *GetN (Variable_t *vars, Constant_t *consts, size_t *vars_cur_size, size_t *consts_cur_size) {
    Node *node = NodeInit (nullptr, 0, TYPE_NUM);
    bool negative = false; //Проверка на отрицательность числа
    if (*s == '-') {
        negative = true;
        ++s;
    }
    if (isalpha (*s)) {
        char str [STR_LEN];
        size_t idx = 0;
        do {
            str [idx++] = *s;
            ++s;
        } while (isalnum (*s));
        str [idx] = '\0';
        int temp = ConstSearch (consts, consts_cur_size, str);
        if (temp != -1) {
            node->type = TYPE_CONST;
            node->data = temp;
        } else {
            node->type = TYPE_VAR;
            node->data = VarSearch(vars, vars_cur_size, str);
        }
    } else {
        if (typeid(num_t) == typeid(double)) { //Если число с плавающей запятой
            bool point = false;
            size_t offset = 0; //Если пошла часть числа после запятой, то запоминаем сдвиг
            do {
                if (*s == '.') {
                    point = true;
                    ++s;
                } else {
                    node->data = node->data * 10 + (*s - '0');
                    ++s;
                    if (point)
                        ++offset; //Если точка уже была, увеличиваем сдвиг
                }
            } while (isdigit(*s) || (!point && *s == '.'));
            node->data /= pow (10, offset);
        } else {
            do {
                node->data = node->data * 10 + (*s - '0');
                ++s;
            } while (isdigit(*s));
        }
    }
#define NUM(value) NodeInit (nullptr, value, TYPE_NUM)
    return (negative ? NodeInit (nullptr, OP_MUL, TYPE_OP, NUM (-1), node) : node);
#undef NUM
}

int main () {
    //FILE *readfile = fopen ("../input.txt","rb");
    Variable_t vars [VARS_SIZE];
    Constant_t consts [CONSTS_SIZE];
    size_t vars_cur_size = 0, consts_cur_size = 0;

    //КОСТЫЛЬ
    strcpy (consts[0].name, "a");
    consts[0].value = 100;
    strcpy (consts[1].name, "b");
    consts[1].value = 1000;
    consts_cur_size = 2;
    strcpy (vars[0].name, "x");
    vars[0].value = 1;
    vars_cur_size = 1;
    //КОСТЫЛЬ

    //Node *root = GetExpression (readfile, nullptr, vars, consts, &vars_cur_size, &consts_cur_size);
    Node *root = GetG ("15.65213/x^2", vars, consts, &vars_cur_size, &consts_cur_size);
    root = Optimizer (root, vars, &vars_cur_size);

    Node *f1_root = &Diff(root);
    f1_root = Optimizer (f1_root, vars, &vars_cur_size);

    FILE *dot_writefile = fopen ("../temp.dot", "w");
    fprintf (dot_writefile,"digraph G {\nfontsize = 50\n");
    GetTreeImage (dot_writefile, root, vars, consts);
    GetTreeImage (dot_writefile, f1_root, vars, consts);
    fprintf (dot_writefile, "}");
    fclose (dot_writefile);

    system ("dot -Tpng /home/biscuitslayer/CLionProjects/8_Differentiator/temp.dot -o/home/biscuitslayer/CLionProjects/8_Differentiator/temp.png");

    system ("rm /home/biscuitslayer/CLionProjects/8_Differentiator/LaTeX/*");

    FILE *tex_writefile = fopen ("../temp.tex", "w");
    LaTeX::GetTreeTex (root, f1_root, "О боже какая формула", "Я хочу от тебя производную", tex_writefile, vars, consts);
    fclose (tex_writefile);

    system ("pdflatex --output-directory=/home/biscuitslayer/CLionProjects/8_Differentiator/LaTeX /home/biscuitslayer/CLionProjects/8_Differentiator/temp.tex");
    system ("xdg-open /home/biscuitslayer/CLionProjects/8_Differentiator/LaTeX/temp.pdf");
    printf ("Root value: %d\n", GetTreeValue (root, vars, consts));
    printf ("f1_Root value: %d\n", GetTreeValue (f1_root, vars, consts));

    FreeNode (f1_root);
    FreeNode (root);
}

Node *NodeInit (Node *parent, num_t data, type_t type, Node *left, Node *right) {
    Node *node = (Node *) calloc (1, sizeof (Node));
    if (parent) {
        node->parent = parent;
        node->level = parent->level + 1;
    }
    node->left = left;
    node->right = right;
    node->data = data;
    node->type = type;
    return node;
}

Node *GetExpression (FILE *readfile, Node *parent, Variable_t *vars, Constant_t *consts, size_t *vars_cur_size, size_t *consts_cur_size) {
    Node *node = NodeInit (parent, 0, TYPE_OP);
    char operation [STR_LEN], temp [STR_LEN];
    char br = '0';

    if (fscanf (readfile, "%c", &br) > 0 && br == '(') {
        if (node->level == 0) {
            node = GetExpression(readfile, node, vars, consts, vars_cur_size, consts_cur_size);
            node->parent = nullptr;
            return node;
        } else
            node->left = GetExpression(readfile, node, vars, consts, vars_cur_size, consts_cur_size);
    } else {
        sprintf (operation, "%c", br);
    }
    if (fscanf(readfile, "%[^()]", temp) > 0) {
        if (br != '(')
            sprintf(operation, "%c%s", br, temp);
        else
            sprintf(operation, "%s", temp);
    }
    if (!GetOperation (operation, &node, vars, consts, vars_cur_size, consts_cur_size))
        return nullptr;
    if (fscanf (readfile, "%c", &br) > 0 && br == '(') {
        node->right = GetExpression(readfile, node, vars, consts, vars_cur_size, consts_cur_size);
    }
    fscanf(readfile, ")");
    return node;
}

bool GetOperation (char *str, Node **node, Variable_t *vars, Constant_t *consts, size_t *vars_cur_size, size_t *consts_cur_size) {
    if (strcmp (str, "nil") == 0)
        return false;
    if (isdigit (str[0]) || str[0] == '-') {
        (*node)->type = TYPE_NUM;
        sscanf(str, "%d", &(*node)->data);
    }
    else {
        (*node)->type = TYPE_OP;
#define OP_CMD(name_u, id, name_l) else if (strcmp (str, name_l) == 0) (*node)->data = OP_##name_u;
        if (false) {}
#include "diff_parts/diff.commands"
#undef OP_CMD
    }
    if ((*node)->type == TYPE_OP && (*node)->data == 0) {
        int temp = 0;
        temp = ConstSearch (consts, consts_cur_size, str);
        if (temp != -1) {
            (*node)->type = TYPE_CONST;
            (*node)->data = temp;
        } else {
            (*node)->type = TYPE_VAR;
            (*node)->data = VarSearch(vars, vars_cur_size, str);
        }
    }
    return true;
}

#define OPERATOR_OVERLOAD(SYM, OP) Node &operator SYM (Node &left, Node &right) { \
    Node *node = nullptr;                                                         \
    if (left.parent == nullptr) {                                                 \
        node = NodeInit(nullptr, OP, TYPE_OP);                                    \
        left.parent = node;                                                       \
        right.parent = node;                                                      \
    }                                                                             \
    else                                                                          \
        node = NodeInit (left.parent->parent, OP, TYPE_OP);                       \
    node->level = left.level - 1;                                                 \
    node->left = &left;                                                           \
    node->right = &right;                                                         \
    return *node;                                                                 \
}

OPERATOR_OVERLOAD (+, OP_SUM)
OPERATOR_OVERLOAD (-, OP_SUB)
OPERATOR_OVERLOAD (*, OP_MUL)
OPERATOR_OVERLOAD (/, OP_DIV)

#undef OPERATOR_OVERLOAD

Node &Copy (Node *node) {
    Node *new_node = NodeInit (node->parent, node->data, node->type);
    if (node->left)
        new_node->left = &Copy (node->left);
    if (node->right)
        new_node->right = &Copy (node->right);
    new_node->level = node->level;
    return *new_node;
}

Node &Diff (Node *node) {
#define c(node) Copy (node)
#define d(node) Diff (node)
#define L node->left
#define R node->right
#define NUM(value) *NodeInit (node->parent, value, TYPE_NUM)
#define OP(name) NodeInit(node, OP_##name, TYPE_OP, &c(L))
#define HALF NodeInit(node, OP_DIV, TYPE_OP, &NUM(1), &NUM(2))
#define SQR NodeInit(node, OP_POW2, TYPE_OP, &c(L), &NUM(2))
    switch (node->type) {
        case TYPE_NUM:
            return NUM(0);
        case TYPE_VAR:
            return NUM(1);
        case TYPE_CONST:
            return NUM(0);
        case TYPE_OP:
            switch ((int)node->data) {
                case OP_SUM: return (d(L) + d(R));
                case OP_SUB: return (d(L) - d(R));
                case OP_MUL: return (d(L) * c(R) + d(R) * c(L));
                case OP_DIV: return ((d(L) * c(R) - d(R) * c(L)) / (c(R) * c(R)));
                case OP_SIN: return (*OP(COS) * d(L));
                case OP_COS: return ((NUM(-1) * *OP(SIN)) * d(L));
                case OP_TG: return ((NUM(1) / (*NodeInit(node, OP_POW2, TYPE_OP, OP(COS), &NUM(2)))) * d(L));
                case OP_CTG: return ((NUM(-1) / (*NodeInit(node, OP_POW2, TYPE_OP, OP(SIN), &NUM(2)))) * d(L));
                case OP_SH: return (*OP(CH) * d(L));
                case OP_CH: return (*OP(SH) * d(L));
                case OP_TH: return ((NUM(1) / (*NodeInit(node, OP_POW2, TYPE_OP, OP(CH), &NUM(2)))) * d(L));
                case OP_CTH: return ((NUM(-1) / (*NodeInit(node, OP_POW2, TYPE_OP, OP(SH), &NUM(2)))) * d(L));
                case OP_POW1: return (c(node) * d(NodeInit(node, OP_MUL, TYPE_OP, &c(R), OP(LN))));
                case OP_POW2: return ((c(R) * *NodeInit(node, OP_POW2, TYPE_OP, &c(L), NodeInit (node->right, OP_SUB, TYPE_OP, &c(R), &NUM (1)))) * d(L));
                case OP_LN: return ((NUM(1) / c(L)) * d(L));
                case OP_ARCSIN: return ((NUM(1) / *NodeInit(node, OP_POW2, TYPE_OP, NodeInit (node, OP_SUB, TYPE_OP, &NUM(1), SQR), HALF)) * d(L));
                case OP_ARCCOS: return ((NUM(-1) / *NodeInit(node, OP_POW2, TYPE_OP, NodeInit (node, OP_SUB, TYPE_OP, &NUM(1), SQR), HALF)) * d(L));
                case OP_ARCTG: return ((NUM(1) / *NodeInit (node, OP_SUM, TYPE_OP, &NUM(1), SQR)) * d(L));
                case OP_ARCCTG: return ((NUM(-1) / *NodeInit (node, OP_SUM, TYPE_OP, &NUM(1), SQR)) * d(L));
                case OP_ARCSH: return ((NUM(1) / *NodeInit(node, OP_POW2, TYPE_OP, NodeInit (node, OP_SUM, TYPE_OP, SQR, &NUM(1)), HALF)) * d(L));
                case OP_ARCCH: return ((NUM(1) / *NodeInit(node, OP_POW2, TYPE_OP, NodeInit (node, OP_SUB, TYPE_OP, SQR, &NUM(1)), HALF)) * d(L));
                case OP_ARCTH: return ((NUM(1) / *NodeInit (node, OP_SUB, TYPE_OP, &NUM(1), SQR)) * d(L));
                case OP_ARCCTH: return ((NUM(1) / *NodeInit (node, OP_SUB, TYPE_OP, &NUM(1), SQR)) * d(L));
            }
    }
    return NUM(0);
#undef c
#undef d
#undef L
#undef R
#undef NUM
#undef OP
#undef HALF
#undef SQR
}

Node *Optimizer (Node *node, Variable_t *vars, size_t *vars_cur_size) {
    bool flag = true; //Показывает, были ли изменения
    while (flag) {
        flag = false;
#define CHECK(func) node->parent = nullptr; node = opt::func (node, &flag, vars, vars_cur_size);
        CHECK (MulZero)
        CHECK (SumZero)
        CHECK (DivZero)
        CHECK (PowZero)
        CHECK (MulUnit)
        CHECK (DivUnit)
        CHECK (PowUnit)
        CHECK (NumSum)
        CHECK (SimplePower)
#undef CHECK
    }
    return node;
}

num_t GetTreeValue (Node *node, Variable_t *vars, Constant_t *consts) {
    if (node->type == TYPE_NUM)
        return node->data;
    else if (node->type == TYPE_VAR)
        return vars[(int)node->data].value;
    else if (node->type == TYPE_CONST)
        return consts[(int)node->data].value;
    else {
#define CALCULATE(SYM, NAME) else if (node->data == NAME && node->left && node->right)            \
        return GetTreeValue (node->left, vars, consts) SYM GetTreeValue (node->right, vars, consts);
        if (false) {}
        CALCULATE (+, OP_SUM)
        CALCULATE (-, OP_SUB)
        CALCULATE (*, OP_MUL)
        CALCULATE (/, OP_DIV)
#undef CALCULATE
        else if (node->data == OP_SIN)
            return sin((double)GetTreeValue(node->left, vars, consts));
        else if (node->data == OP_COS)
            return cos((double)GetTreeValue(node->left, vars, consts));
        else if (node->data == OP_TG)
            return tan((double)GetTreeValue(node->left, vars, consts));
        else if (node->data == OP_CTG)
            return 1.0 / tan((double)GetTreeValue(node->left, vars, consts));
        else if (node->data == OP_SH)
            return sinh((double)GetTreeValue(node->left, vars, consts));
        else if (node->data == OP_CH)
            return cosh((double)GetTreeValue(node->left, vars, consts));
        else if (node->data == OP_TH)
            return tanh((double)GetTreeValue(node->left, vars, consts));
        else if (node->data == OP_CTH)
            return 1.0 / tanh((double)GetTreeValue(node->left, vars, consts));
        else if (node->data == OP_POW1 || node->data == OP_POW2)
            return pow((double)GetTreeValue(node->left, vars, consts), (double)GetTreeValue(node->right, vars, consts));
        else if (node->data == OP_LN)
            return log((double)GetTreeValue(node->left, vars, consts));
    }
}

void GetTreeImage (FILE *writefile, Node *node, Variable_t *vars, Constant_t *consts) {
    PrintNode (writefile, node, vars, consts);
}

void PrintNode (FILE *writefile, Node *node, Variable_t *vars, Constant_t *consts) {
    if (node == nullptr)
        return;
    char *str = MakeNodeLabel(node, vars, consts);
    if (node->left) {
        char *str1 = MakeNodeLabel (node->left, vars, consts);
        fprintf (writefile, "%zu[label = \"%s\"]\n%zu[label = \"%s\"]\n", node, str, node->left, str1);
        fprintf (writefile, "%zu -> %zu\n", node, node->left);
        PrintNode (writefile, node->left, vars, consts);
        free (str1);
    }
    if (node->right) {
        char *str2 = MakeNodeLabel (node->right, vars, consts);
        fprintf (writefile, "%zu[label = \"%s\"]\n%zu[label = \"%s\"]\n", node, str, node->right, str2);
        fprintf (writefile, "%zu -> %zu\n", node, node->right);
        PrintNode (writefile, node->right, vars, consts);
        free (str2);
    }
    if (!node->left && !node->right) {
        fprintf (writefile, "%zu[label = \"%s\"]\n", node, str);
    }
    free (str);
}

char *MakeNodeLabel (Node *node, Variable_t *vars, Constant_t *consts) {
    char *str = (char *) calloc (STR_LEN, sizeof (char));
    if (node->type == TYPE_NUM) {
        sprintf (str, NUM_T_FORMAT, node->data);
    } else if (node->type == TYPE_VAR) {
        sprintf (str, "%s", vars[(int)node->data].name);
    } else if (node->type == TYPE_CONST) {
        sprintf (str, "%s", consts[(int)node->data].name);
    } else if (node->type == TYPE_OP) {
#define OP_CMD(name_u, id, name_l) else if (node->data == OP_##name_u) sprintf (str, name_l);
        if (false) {}
#include "diff_parts/diff.commands"
#undef OP_CMD
    }
    return str;
}

size_t VarSearch (Variable_t *vars, size_t *vars_cur_size, char *name) {
    for (size_t i = 0; i < *vars_cur_size; ++i)
        if (strcmp (vars[i].name, name) == 0)
            return i;
    strcpy (vars[*vars_cur_size].name, name);
    return (*vars_cur_size)++;
}

bool TreeContainsVar (Node *node, Variable_t *vars, size_t *vars_cur_size) {
    bool flag = false;
    if (node->left)
        flag += TreeContainsVar (node->left, vars, vars_cur_size);
    if (node->right)
        flag += TreeContainsVar (node->right, vars, vars_cur_size);
    if (node->type == TYPE_VAR)
        flag = true;
    return flag;
}

int ConstSearch (Variable_t *consts, size_t *consts_cur_size, char *name) {
    for (size_t i = 0; i < *consts_cur_size; ++i)
        if (strcmp (consts[i].name, name) == 0)
            return i;
    return -1;
}

void TreeOffsetCorrecter (Node *node) {
    if (!node->parent)
        node->level = 1;
    if (node->left) {
        node->left->parent = node;
        node->left->level = node->level + 1;
        TreeOffsetCorrecter (node->left);
    }
    if (node->right) {
        node->right->parent = node;
        node->right->level = node->level + 1;
        TreeOffsetCorrecter (node->right);
    }
}

void FreeNode (Node *node) {
    if (!node)
        return;
    if (node->left)
        FreeNode (node->left);
    if (node->right)
        FreeNode (node->right);
    free (node);
}

Node *opt::MulZero (Node *node, bool *flag, Variable_t *vars, size_t *vars_cur_size) {
    if (!node->parent)
        TreeOffsetCorrecter (node);
    if (node->left)
        opt::MulZero (node->left, flag);
    if (node->right)
        opt::MulZero (node->right, flag);
    if (node->type == TYPE_OP && node->data == OP_MUL) {
        if ((node->left->type == TYPE_NUM && node->left->data == 0) || (node->right->type == TYPE_NUM && node->right->data == 0)) {
            Node *new_node = NodeInit (node->parent, 0, TYPE_NUM, nullptr, nullptr);
            FreeNode (node->left);
            FreeNode (node->right);
            if (node->parent)
                (node->parent->left == node ? node->parent->left : node->parent->right) = new_node;
            return (*flag = true), new_node;
        }
    }
    return node;
}

Node *opt::SumZero (Node *node, bool *flag, Variable_t *vars, size_t *vars_cur_size) {
    if (!node->parent)
        TreeOffsetCorrecter (node);
    if (node->left)
        opt::SumZero (node->left, flag);
    if (node->right)
        opt::SumZero (node->right, flag);
    if (node->type == TYPE_OP && node->data == OP_SUM) {
        if (node->left->type == TYPE_NUM && node->left->data == 0) {
            FreeNode (node->left);
            if (node->parent)
                (node->parent->left == node ? node->parent->left : node->parent->right) = node->right;
            return (*flag = true), node->right;
        }
        else if (node->right->type == TYPE_NUM && node->right->data == 0) {
            FreeNode (node->right);
            if (node->parent)
                (node->parent->left == node ? node->parent->left : node->parent->right) = node->left;
            return (*flag = true), node->left;
        }
    }
    else if (node->type == TYPE_OP && node->data == OP_SUB) { //Обработка вычитания нуля и из нуля
        if (node->left->type == TYPE_NUM && node->left->data == 0) { //Замена на -1
            Node *new_node = NodeInit (node->parent, OP_MUL, TYPE_OP, node->left, node->right);
            if (node->parent)
                (node->parent->left == node ? node->parent->left : node->parent->right) = new_node;
            new_node->left = NodeInit (node, -1, TYPE_NUM, nullptr, nullptr);
            return (*flag = true), new_node;
        }
        else if (node->right->type == TYPE_NUM && node->right->data == 0) { //Сдвиг
            FreeNode (node->right);
            if (node->parent)
                (node->parent->left == node ? node->parent->left : node->parent->right) = node->left;
            return (*flag = true), node->left;
        }
    }
    return node;
}

Node *opt::DivZero (Node *node, bool *flag, Variable_t *vars, size_t *vars_cur_size) {
    if (!node->parent)
        TreeOffsetCorrecter (node);
    if (node->left)
        opt::DivZero (node->left, flag);
    if (node->right)
        opt::DivZero (node->right, flag);
    if (node->type == TYPE_OP && node->data == OP_DIV) {
        if (node->left->type == TYPE_NUM && node->left->data == 0) {
            FreeNode(node->right);
            if (node->parent)
                (node->parent->left == node ? node->parent->left : node->parent->right) = node->left;
            return (*flag = true), node->left;
        }
    }
    return node;
}

Node *opt::PowZero (Node *node, bool *flag, Variable_t *vars, size_t *vars_cur_size) {
    if (!node->parent)
        TreeOffsetCorrecter (node);
    if (node->left)
        opt::PowZero (node->left, flag);
    if (node->right)
        opt::PowZero (node->right, flag);
    if (node->type == TYPE_OP && (node->data == OP_POW1 || node->data == OP_POW2)) {
        if (node->right->type == TYPE_NUM && node->right->data == 0) {
            Node *new_node = NodeInit (node->parent, 1, TYPE_NUM, nullptr, nullptr);
            FreeNode(node->left);
            FreeNode(node->right);
            if (node->parent)
                (node->parent->left == node ? node->parent->left : node->parent->right) = new_node;
            return (*flag = true), new_node;
        }
    }
    return node;
}

Node *opt::MulUnit (Node *node, bool *flag, Variable_t *vars, size_t *vars_cur_size) {
    if (!node->parent)
        TreeOffsetCorrecter (node);
    if (node->left)
        opt::MulUnit (node->left, flag);
    if (node->right)
        opt::MulUnit (node->right, flag);
    if (node->type == TYPE_OP && node->data == OP_MUL) {
        if (node->left->type == TYPE_NUM && node->left->data == 1) {
            FreeNode (node->left);
            if (node->parent)
                (node->parent->left == node ? node->parent->left : node->parent->right) = node->right;
            return (*flag = true), node->right;
        }
        else if (node->right->type == TYPE_NUM && node->right->data == 1) {
            FreeNode (node->right);
            if (node->parent)
                (node->parent->left == node ? node->parent->left : node->parent->right) = node->left;
            return (*flag = true), node->left;
        }
    }
    return node;
}

Node *opt::DivUnit (Node *node, bool *flag, Variable_t *vars, size_t *vars_cur_size) {
    if (!node->parent)
        TreeOffsetCorrecter (node);
    if (node->left)
        opt::DivUnit (node->left, flag);
    if (node->right)
        opt::DivUnit (node->right, flag);
    if (node->type == TYPE_OP && node->data == OP_DIV) {
        if (node->right->type == TYPE_NUM && node->right->data == 1) {
            FreeNode(node->right);
            if (node->parent)
                (node->parent->left == node ? node->parent->left : node->parent->right) = node->left;
            return (*flag = true), node->left;
        }
    }
    return node;
}

Node *opt::PowUnit (Node *node, bool *flag, Variable_t *vars, size_t *vars_cur_size) {
    if (!node->parent)
        TreeOffsetCorrecter (node);
    if (node->left)
        opt::PowUnit (node->left, flag);
    if (node->right)
        opt::PowUnit (node->right, flag);
    if (node->type == TYPE_OP && (node->data == OP_POW1 || node->data == OP_POW2)) {
        if (node->right->type == TYPE_NUM && node->right->data == 1) {
            FreeNode(node->right);
            if (node->parent)
                (node->parent->left == node ? node->parent->left : node->parent->right) = node->left;
            return (*flag = true), node->left;
        }
    }
    return node;
}

Node *opt::NumSum (Node *node, bool *flag, Variable_t *vars, size_t *vars_cur_size) {
    if (!node->parent)
        TreeOffsetCorrecter (node);
    if (node->left)
        opt::NumSum (node->left, flag);
    if (node->right)
        opt::NumSum (node->right, flag);
    if (false) {}
#define CALCULATE(SYM, NAME) else if (node->type == TYPE_OP && node->data == NAME) {                                           \
        if (node->left->type == TYPE_NUM && node->right->type == TYPE_NUM) {                                               \
            Node *new_node = NodeInit (node->parent, node->left->data SYM node->right->data, TYPE_NUM, nullptr, nullptr);  \
            FreeNode (node->left);                                                                                         \
            FreeNode (node->right);                                                                                        \
            if (node->parent)                                                                                              \
                (node->parent->left == node ? node->parent->left : node->parent->right) = new_node;                        \
            return (*flag = true), new_node;                                                                               \
        }                                                                                                                  \
    }
    CALCULATE(+, OP_SUM)
    CALCULATE(-, OP_SUB)
    CALCULATE(*, OP_MUL)
    return node;
#undef CALCULATE
}

Node *opt::SimplePower (Node *node, bool *flag, Variable_t *vars, size_t *vars_cur_size) {
    if (!node->parent)
        TreeOffsetCorrecter (node);
    if (node->left)
        opt::SimplePower (node->left, flag, vars, vars_cur_size);
    if (node->right)
        opt::SimplePower (node->right, flag, vars, vars_cur_size);
    if (node->type == TYPE_OP && node->data == OP_POW1) {
        if (!TreeContainsVar (node->right, vars, vars_cur_size))
            node->data = OP_POW2;
    }
    return node;
}

void LaTeX::GetTreeTex (Node *node1, Node *node2, const char *msg1, const char *msg2, FILE *writefile, Variable_t *vars, Constant_t *consts) {
    fprintf (writefile, "\\documentclass[a4paper, 12pt, oneside]{scrartcl}\n"
                        "\\usepackage[T2A]{fontenc}\n"
                        "\\usepackage[utf8x]{inputenc}\n"
                        "\\usepackage[english, russian]{babel}\n"
                        "\\usepackage{amsmath}\n"
                        "\\usepackage{amsfonts}\n"
                        "\\usepackage{amssymb}\n"
                        "\n"
                        "\\begin{document}\n");
    /*
    fprintf (writefile, "%s \\\\ \n\\[", msg1);
    LaTeX::PrintNodeTex (node1, writefile, vars, consts);
    fprintf (writefile, "\\] \\\\ \n%s \\\\ \n\\[", msg2);
    LaTeX::PrintNodeTex (node2, writefile, vars, consts);
    fprintf (writefile, "\\] \\\\ \n\\end{document}");
     */
    //КОСТЫЛЬ: ВРЕМЕННЫЙ КОСМЕТИЧЕСКИЙ РЕМОНТ
    fprintf (writefile, "%s \\\\ \\\\ \n$", msg1);
    LaTeX::PrintNodeTex (node1, writefile, vars, consts);
    fprintf (writefile, "$ \\\\ \\\\ \n%s \\\\ \\\\ \n$", msg2);
    LaTeX::PrintNodeTex (node2, writefile, vars, consts);
    fprintf (writefile, "$ \\\\ \\\\ \n\\end{document}");
    //КОСТЫЛЬ: ВРЕМЕННЫЙ КОСМЕТИЧЕСКИЙ РЕМОНТ
}

void LaTeX::PrintNodeTex (Node *node, FILE *writefile, Variable_t *vars, Constant_t *consts) {
    switch (node->type) {
        case TYPE_NUM: {
            fprintf(writefile, NUM_T_FORMAT, node->data);
            break;
        }
        case TYPE_VAR: {
            fprintf(writefile, "%s", vars[(int)node->data].name);
            break;
        }
        case TYPE_CONST: {
            fprintf(writefile, "%s", consts[(int)node->data].name);
            break;
        }
        case TYPE_OP: {
            switch ((int) node->data) {
                case OP_SUM:
                case OP_SUB: {
                    if (node->left) {
                        LaTeX::PrintNodeTex(node->left, writefile, vars, consts);
                    }
                    fprintf(writefile, "%c", (int) node->data);
                    if (node->right) {
                        LaTeX::PrintNodeTex(node->right, writefile, vars, consts);
                    }
                    break;
                }
                case OP_MUL: {
                    if (node->left) {
                        if (node->left->type == TYPE_OP && (node->left->data == OP_SUM || node->left->data == OP_SUB))
                            fprintf(writefile, "(");
                        LaTeX::PrintNodeTex(node->left, writefile, vars, consts);
                        if (node->left->type == TYPE_OP && (node->left->data == OP_SUM || node->left->data == OP_SUB))
                            fprintf(writefile, ")");
                    }
                    fprintf(writefile, "%c", (int) node->data);
                    if (node->right) {
                        if (node->right->type == TYPE_OP &&
                            (node->right->data == OP_SUM || node->right->data == OP_SUB))
                            fprintf(writefile, "(");
                        LaTeX::PrintNodeTex(node->right, writefile, vars, consts);
                        if (node->right->type == TYPE_OP &&
                            (node->right->data == OP_SUM || node->right->data == OP_SUB))
                            fprintf(writefile, ")");
                    }
                    break;
                }
                case OP_DIV: {
                    fprintf(writefile, "\\frac", node->data);
                    if (node->left) {
                        fprintf(writefile, "{");
                        LaTeX::PrintNodeTex(node->left, writefile, vars, consts);
                        fprintf(writefile, "}");
                    }
                    if (node->right) {
                        fprintf(writefile, "{");
                        LaTeX::PrintNodeTex(node->right, writefile, vars, consts);
                        fprintf(writefile, "}");
                    }
                    break;
                }
                case OP_SIN:
                case OP_COS:
                case OP_TG:
                case OP_CTG:
                case OP_SH:
                case OP_CH:
                case OP_TH:
                case OP_CTH:
                case OP_LN:
                case OP_ARCSIN:
                case OP_ARCCOS:
                case OP_ARCTG:
                case OP_ARCCTG:
                case OP_ARCSH:
                case OP_ARCCH:
                case OP_ARCTH:
                case OP_ARCCTH: {
                    switch ((int) node->data) {
                        case OP_SIN: {
                            fprintf(writefile, "%s", "sin");
                            break;
                        }
                        case OP_COS: {
                            fprintf(writefile, "%s", "cos");
                            break;
                        }
                        case OP_TG: {
                            fprintf(writefile, "%s", "tg");
                            break;
                        }
                        case OP_CTG: {
                            fprintf(writefile, "%s", "ctg");
                            break;
                        }
                        case OP_SH: {
                            fprintf(writefile, "%s", "sh");
                            break;
                        }
                        case OP_CH: {
                            fprintf(writefile, "%s", "ch");
                            break;
                        }
                        case OP_TH: {
                            fprintf(writefile, "%s", "th");
                            break;
                        }
                        case OP_CTH: {
                            fprintf(writefile, "%s", "cth");
                            break;
                        }
                        case OP_LN: {
                            fprintf(writefile, "%s", "ln");
                            break;
                        }
                        case OP_ARCSIN: {
                            fprintf(writefile, "%s", "arcsin");
                            break;
                        }
                        case OP_ARCCOS: {
                            fprintf(writefile, "%s", "arccos");
                            break;
                        }
                        case OP_ARCTG: {
                            fprintf(writefile, "%s", "arctg");
                            break;
                        }
                        case OP_ARCCTG: {
                            fprintf(writefile, "%s", "arcctg");
                            break;
                        }
                        case OP_ARCSH: {
                            fprintf(writefile, "%s", "arcsh");
                            break;
                        }
                        case OP_ARCCH: {
                            fprintf(writefile, "%s", "arcch");
                            break;
                        }
                        case OP_ARCTH: {
                            fprintf(writefile, "%s", "arcth");
                            break;
                        }
                        case OP_ARCCTH: {
                            fprintf(writefile, "%s", "arccth");
                            break;
                        }
                    }
                    if (node->left) {
                        fprintf(writefile, "(");
                        LaTeX::PrintNodeTex(node->left, writefile, vars, consts);
                        fprintf(writefile, ")");
                    }
                    break;
                }
                case OP_POW1:
                case OP_POW2: {
                    if (node->left) {
                        if (!(node->left->type == TYPE_NUM || node->left->type == TYPE_VAR ||
                              node->left->type == TYPE_CONST))
                            fprintf(writefile, "(");
                        LaTeX::PrintNodeTex(node->left, writefile, vars, consts);
                        if (!(node->left->type == TYPE_NUM || node->left->type == TYPE_VAR ||
                              node->left->type == TYPE_CONST))
                            fprintf(writefile, ")");
                    }
                    fprintf(writefile, "^");
                    if (node->right) {
                        fprintf(writefile, "{");
                        LaTeX::PrintNodeTex(node->right, writefile, vars, consts);
                        fprintf(writefile, "}");
                    }
                    break;
                }
            }
        }
    }
}