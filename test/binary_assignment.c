int main(void)
{
    int i = 0;
    i += 1;             // 1
    i *= 2 + 2 - 2;     // 2
    i -= 1;             // 1
    i *= 3;             // 3
    i *= 1 + 4 * 3 + 2; // 45;

    return i == 45;
}
