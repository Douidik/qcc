struct structure
{
    int x;
    int y;
    int z;
    char c;
};

int structure_sum(struct structure *s)
{
    int struct_x = (*s).x;
    int struct_y = (*s).y;
    int struct_z = (*s).z;
    return (struct_x + struct_y + struct_z);
}

int main(void)
{
    struct structure s;
    s.x = 1;
    s.y = 2;
    s.z = 3;

    return structure_sum(&s);
}
