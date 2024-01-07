int return_99(void);

int main(void)
{
    int w = return_99() + 5 * 2;
    int z = return_99() * 5 + 2;
    int x = 1 + 2 * 3;
    int y = 1 * 2 + 3;

    return z == 497 && w == 102 && x == 7 && y == 5;
}

int return_99(void)
{
    return 99;
}
