static const int debruijnposition[32] = 
{
  0, 1, 28, 2, 29, 14, 24, 3, 30, 22, 20, 15, 25, 17, 4, 8, 
  31, 27, 13, 23, 21, 19, 16, 7, 26, 12, 18, 6, 11, 5, 10, 9
};

#define LEAST_SIGNIFICANT_BIT(a) \
    (debruijnposition[((uint32_t)(((a) & -(a)) * 0x077CB531U)) >> 27])

static inline unsigned int COUNT_SET_BITS(unsigned int i)
{
    /*  It isn't for us to understand how this works, but it returns
        the number of set bits in an integer    */
    i = i - ((i >> 1) & 0x55555555);
    i = (i & 0x33333333) + ((i >> 2) & 0x33333333);
    return (((i + (i >> 4)) & 0x0F0F0F0F) * 0x01010101) >> 24;
}
