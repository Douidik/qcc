int return_132(int *number)
{
    return *number;
}

int main(void)
{
    int main_number = 132;
    int dereferenced = return_132(&main_number);
    return dereferenced == 132;
}
