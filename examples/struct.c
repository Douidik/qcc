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
    
    return (pos.sum);
}

/* struct number */
/* { */
/*     int n; */
/* }; */

/* int number_double(struct number num) { */
/*     return num.n + num.n; */
/* } */

/* int main(void) { */
/*     struct number num; */
/*     num.n = 4; */
/*     return number_double(num); */
/* } */
