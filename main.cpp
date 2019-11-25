#include <cstdio>
#include <cassert>
#include <cstdlib>
#include <cstring>
#include <cctype>

//TODO проверить обработку вещественных чисел
//TODO Разобраться с nil в sin и cos
//TODO Дописать tg и ctg и тд

const int STR_LEN = 100;
const int VARS_SIZE = 3;
const int CONSTS_SIZE = 0;
typedef int num_t;
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
Node *Optimize (Node *node);
void GetTreeImage (FILE *writefile, Node *node, Variable_t *vars, Constant_t *consts);
void PrintNode (FILE *writefile, Node *node, Variable_t *vars, Constant_t *consts);
char *MakeNodeLabel (Node *node, Variable_t *vars, Constant_t *consts);
size_t VarSearch (Variable_t *vars, size_t *vars_cur_size, char *name);
int ConstSearch (Variable_t *consts, size_t *consts_cur_size, char *name);
void TreeOffsetCorrecter (Node *node);
void FreeNode (Node *node);

namespace opt {
    Node *MulZero (Node *node, bool *flag);
    Node *SumZero (Node *node, bool *flag);
    Node *DivZero (Node *node, bool *flag);
    Node *MulUnit (Node *node, bool *flag);
    Node *DivUnit (Node *node, bool *flag);
    Node *NumSum (Node *node, bool *flag);
    Node *SimpleFrac (Node *node, bool *flag);
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
    Node *node = NodeInit (nullptr, OP_POW, TYPE_OP);
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
    if (strncmp (s, "sin", 3) == 0) {
        s += 3;
        node = NodeInit (nullptr, OP_SIN, TYPE_OP);
        assert (*s == '(');
        ++s;
        node->left = GetE (vars, consts, vars_cur_size, consts_cur_size);
        assert (*s == ')');
        ++s;
    } else if (strncmp (s, "cos", 3) == 0) {
        s += 3;
        node = NodeInit (nullptr, OP_COS, TYPE_OP);
        assert (*s == '(');
        ++s;
        node->left = GetE (vars, consts, vars_cur_size, consts_cur_size);
        assert (*s == ')');
        ++s;
    } else if (strncmp (s, "ln", 2) == 0) {
        s += 2;
        node = NodeInit (nullptr, OP_LN, TYPE_OP);
        assert (*s == '(');
        ++s;
        node->left = GetE (vars, consts, vars_cur_size, consts_cur_size);
        assert (*s == ')');
        ++s;
    } else
        node = GetS (vars, consts, vars_cur_size, consts_cur_size);
    //node = GetP (); //так всё работало
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
    if (isalpha (*s)) {
        char str [STR_LEN];
        size_t idx = 0;
        do {
            str [idx++] = *s;
            ++s;
        } while (isalpha (*s));
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
        do {
            node->data = node->data * 10 + (*s - '0');
            ++s;
        } while (isdigit(*s));
    }
    return node;
}

int main () {
    FILE *readfile = fopen ("../input.txt","rb");
    Variable_t vars [VARS_SIZE];
    Constant_t consts [CONSTS_SIZE];
    size_t vars_cur_size = 0, consts_cur_size = 0;

    //КОСТЫЛЬ
    strcpy (consts[0].name, "a");
    strcpy (consts[1].name, "b");
    consts_cur_size = 2;
    strcpy (vars[0].name, "x");
    vars_cur_size = 1;
    //КОСТЫЛЬ

    //Node *root = GetExpression (readfile, nullptr, vars, consts, &vars_cur_size, &consts_cur_size);
    Node *root = GetG ("(sin(cos(ln(x^(x^(sin(x)))))))^2", vars, consts, &vars_cur_size, &consts_cur_size);
    Node *f1_root = &Diff(root);

    root = Optimize (root);
    f1_root = Optimize (f1_root);

    FILE *dot_writefile = fopen ("../temp.dot", "w");
    fprintf (dot_writefile,"digraph G {\nfontsize = 50\n");
    GetTreeImage (dot_writefile, root, vars, consts);
    GetTreeImage (dot_writefile, f1_root, vars, consts);
    fprintf (dot_writefile, "}");
    fclose (dot_writefile);

    system ("dot -Tpng /home/biscuitslayer/CLionProjects/8_Differentiator/temp.dot -o/home/biscuitslayer/CLionProjects/8_Differentiator/temp.png");

    FILE *tex_writefile = fopen ("../LaTeX/temp.tex", "w");
    LaTeX::GetTreeTex (root, f1_root, "О боже какая формула", "Я хочу от тебя производную", tex_writefile, vars, consts);
    fclose (tex_writefile);
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

Node &operator + (Node &left, Node &right) {
    Node *node = nullptr;
    if (left.parent == nullptr) {
        node = NodeInit(nullptr, OP_SUM, TYPE_OP);
        left.parent = node;
        right.parent = node;
    }
    else
        node = NodeInit (left.parent->parent, OP_SUM, TYPE_OP);
    node->level = left.level - 1;
    node->left = &left;
    node->right = &right;
    return *node;
}

Node &operator - (Node &left, Node &right) {
    Node *node = nullptr;
    if (left.parent == nullptr) {
        node = NodeInit(nullptr, OP_SUB, TYPE_OP);
        left.parent = node;
        right.parent = node;
    }
    else
        node = NodeInit (left.parent->parent, OP_SUB, TYPE_OP);
    node->level = left.level - 1;
    node->left = &left;
    node->right = &right;
    return *node;
}

Node &operator * (Node &left, Node &right) {
    Node *node = nullptr;
    if (left.parent == nullptr) {
        node = NodeInit(nullptr, OP_MUL, TYPE_OP);
        left.parent = node;
        right.parent = node;
    }
    else
        node = NodeInit (left.parent->parent, OP_MUL, TYPE_OP);
    node->level = left.level - 1;
    node->left = &left;
    node->right = &right;
    return *node;
}

Node &operator / (Node &left, Node &right) {
    Node *node = nullptr;
    if (left.parent == nullptr) {
        node = NodeInit(nullptr, OP_DIV, TYPE_OP);
        left.parent = node;
        right.parent = node;
    } else
        node = NodeInit(left.parent->parent, OP_DIV, TYPE_OP);
    node->level = left.level - 1;
    node->left = &left;
    node->right = &right;
    return *node;
}

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
#define NUM(value) *NodeInit (node->parent, value, TYPE_NUM)
    switch (node->type) {
        case TYPE_NUM:
            return NUM(0);
        case TYPE_VAR:
            return NUM(1);
        case TYPE_CONST:
            return NUM(0);
        case TYPE_OP:
            switch (node->data) {
                case OP_SUM: return (Diff (node->left) + Diff (node->right));
                case OP_SUB: return (Diff (node->left) - Diff (node->right));
                case OP_MUL: return (Diff (node->left) * Copy (node->right) + Diff (node->right) * Copy (node->left));
                case OP_DIV: return ((Diff (node->left) * Copy (node->right) - Diff (node->right) * Copy (node->left)) / (Copy(node->right) * Copy(node->right)));
                case OP_SIN: return (*NodeInit(node, OP_COS, TYPE_OP, &Copy (node->left)) * Diff (node->left));
                case OP_COS: return ((*NodeInit(node, -1, TYPE_NUM) * *NodeInit(node, OP_SIN, TYPE_OP, &Copy (node->left))) * Diff (node->left));
                case OP_POW: return (Copy (node) * Diff (NodeInit(node, OP_MUL, TYPE_OP, &Copy (node->right), NodeInit(node, OP_LN, TYPE_OP, &Copy (node->left)))));
                case OP_LN: return ((NUM(1) / Copy (node->left)) * Diff (node->left));
            }
    }
#undef NUM
    return *NodeInit(node->parent, 0, TYPE_NUM);
}

Node *Optimize (Node *node) {
    bool flag = true; //Показывает, были ли изменения
    while (flag) {
        flag = false;
#define CHECK(func) node->parent = nullptr; node = opt::func (node, &flag);
        CHECK (MulZero)
        CHECK (SumZero)
        CHECK (DivZero)
        CHECK (MulUnit)
        CHECK (DivUnit)
        CHECK (NumSum)
        CHECK (SimpleFrac)
#undef CHECK
    }
    return node;
}

void GetTreeImage (FILE *writefile, Node *node, Variable_t *vars, Constant_t *consts) {
    //fprintf (writefile,"digraph G {\nfontsize = 50\n");
    PrintNode (writefile, node, vars, consts);
    //fprintf (writefile, "}");
    //system ("dot -Tpng /home/biscuitslayer/CLionProjects/8_Differentiator/temp.dot"
    //        "-o/home/biscuitslayer/CLionProjects/8_Differentiator/temp.png");
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
        sprintf (str, "%d", node->data);
    } else if (node->type == TYPE_VAR) {
        sprintf (str, "%s", vars[node->data].name);
    } else if (node->type == TYPE_CONST) {
        sprintf (str, "%s", consts[node->data].name);
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

Node *opt::MulZero (Node *node, bool *flag) {
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

Node *opt::SumZero (Node *node, bool *flag) {
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
        /*if (node->left->type == TYPE_NUM && node->left->data == 0) { //Замена на -1
            Node *new_node = NodeInit (node->parent, OP_MUL, TYPE_OP, node->left, node->right);
            if (node->parent)
                (node->parent->left == node ? node->parent->left : node->parent->right) = new_node;
            node->left = NodeInit (node, -1, TYPE_NUM, nullptr, nullptr);
            return (*flag = true), node;
        }
        else */if (node->right->type == TYPE_NUM && node->right->data == 0) { //Сдвиг
            FreeNode (node->right);
            if (node->parent)
                (node->parent->left == node ? node->parent->left : node->parent->right) = node->left;
            return (*flag = true), node->left;
        }
    }
    return node;
}

Node *opt::DivZero (Node *node, bool *flag) {
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

Node *opt::MulUnit (Node *node, bool *flag) {
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

Node *opt::DivUnit (Node *node, bool *flag) {
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

Node *opt::NumSum (Node *node, bool *flag) {
    if (!node->parent)
        TreeOffsetCorrecter (node);
    if (node->left)
        opt::NumSum (node->left, flag);
    if (node->right)
        opt::NumSum (node->right, flag);
    if (node->type == TYPE_OP && node->data == OP_SUM) {
        if (node->left->type == TYPE_NUM && node->right->type == TYPE_NUM) {
            Node *new_node = NodeInit (node->parent, node->left->data + node->right->data, TYPE_NUM, nullptr, nullptr);
            FreeNode (node->left);
            FreeNode (node->right);
            if (node->parent)
                (node->parent->left == node ? node->parent->left : node->parent->right) = new_node;
            return (*flag = true), new_node;
        }
    }
    else if (node->type == TYPE_OP && node->data == OP_SUB) {
        if (node->left->type == TYPE_NUM && node->right->type == TYPE_NUM) {
            Node *new_node = NodeInit (node->parent, node->left->data - node->right->data, TYPE_NUM, nullptr, nullptr);
            FreeNode (node->left);
            FreeNode (node->right);
            if (node->parent)
                (node->parent->left == node ? node->parent->left : node->parent->right) = new_node;
            return (*flag = true), new_node;
        }
    }
    return node;
}

Node *opt::SimpleFrac (Node *node, bool *flag) {
    if (!node->parent)
        TreeOffsetCorrecter (node);
    if (node->left)
        opt::SimpleFrac (node->left, flag);
    if (node->right)
        opt::SimpleFrac (node->right, flag);
    if (node->type == TYPE_OP && node->data == OP_DIV && node->left->data == 1 && node->right->type == TYPE_VAR) {
        node->left = node->right;
        node->data = OP_POW;
#define NUM(value) NodeInit (node->right, value, TYPE_NUM)
        node->right = NodeInit (node, OP_SUB, TYPE_OP, NUM(0), NUM(1));
#undef NUM
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
            fprintf(writefile, "%d", node->data);
            break;
        }
        case TYPE_VAR: {
            fprintf(writefile, "%s", vars[node->data].name);
            break;
        }
        case TYPE_CONST: {
            fprintf(writefile, "%s", consts[node->data].name);
            break;
        }
        case TYPE_OP:
            switch (node->data) {
                case OP_SUM:
                case OP_SUB: {
                    if (node->left) {
                        LaTeX::PrintNodeTex(node->left, writefile, vars, consts);
                    }
                    fprintf(writefile, "%c", node->data);
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
                    fprintf(writefile, "%c", node->data);
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
                case OP_LN: {
                    fprintf(writefile, "%s", (node->data == OP_LN ? "ln" : (node->data == OP_SIN ? "sin" : "cos")));
                    if (node->left) {
                        fprintf(writefile, "(");
                        LaTeX::PrintNodeTex(node->left, writefile, vars, consts);
                        fprintf(writefile, ")");
                    }
                    break;
                }
                case OP_POW: {
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