// We try to include struct v3
#include "header.h"

int main(void)
{
    struct v3 v;

    v.x = 32;
    v.y = 7;
    v.z = 3;

    struct v3 *vp;
    if (1) {
	vp = &v;
    }
    
    return v3_sum(vp) + v3_product(v) == 714;
}

int v3_product(struct v3 vec)
{
    return vec.x * vec.y * vec.z;
}

int v3_sum(struct v3 *vec)
{
    return vec->x + vec->y + vec->z;
}
