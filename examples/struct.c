struct v2
{
    int x;
    int y;
    int sum;
};

int main(void)
{
    struct v2 pos;

    pos.x = 51;
    pos.y = 49;
    pos.sum = pos.x + pos.y;
    
    return (pos.sum);
}
