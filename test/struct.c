struct v2
{
    int x;
    int y;
    int sum;
};

int v2_sum(int added, struct v2 vector)
{
    return added + vector.x + vector.y;
}

int main(void)
{
    struct v2 pos;

    pos.x = 51;
    pos.y = 49;
    pos.sum = v2_sum(2, pos);
    
    return (pos.sum) == 102;
}
