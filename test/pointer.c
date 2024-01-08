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

    return dereferenced == 132 && sp->v == 2 && sp->w == 4;
}
