struct S
{
    int v, w;
};

int return_132(int *number)
{
    return *number;
}

int main(void)
{
    int main_number = 132;
    int dereferenced = return_132(&main_number);

    struct S s;
    struct S *sp = &s;
    sp->v = 2;
    sp->w = 4;

    struct S *sp_1 = sp++;
    struct S *sp_2 = sp + 1;
    struct S *sp_3 = --sp + 3;

    return dereferenced == 132 && sp->v == 2 && sp->w == 4 && sp_1 == sp + 1 && sp_2 == sp + 2 &&
           sp_3 == sp + 3;
}
