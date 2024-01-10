struct Point
{
    int x, y;
};

int main(void)
{
    int xs[3];
    xs[0] = 2;
    xs[1] = 4;
    xs[2] = 11;
    
    struct Point points[4];
    points[0].x = 1;
    points[0].y = 2;
    points[1].x = 3;
    points[1].y = 4;
    points[2].x = 5;
    points[2].y = 6;
    points[3].x = 7;
    points[3].y = 8;

    struct Point *ps[4];
    ps[0] = &points[0];
    ps[1] = &points[1];
    ps[2] = &points[2];
    ps[3] = &points[3];

    return
	xs[0] == 2 &&
	xs[1] == 4 &&
	xs[2] == 8 &&
	ps[0]->x==1 &&
	ps[0]->y==2 &&
	ps[1]->x==3 &&
	ps[1]->y==4 &&
	ps[2]->x==5 &&
	ps[2]->y==6 &&
	ps[3]->x==7 &&
	ps[3]->y==8;
}
