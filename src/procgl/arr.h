/*  Dynamic array macro API
    Macro                           Evaluates to...
    ARR_PUSH, ARR_INSERT,
    ARR_SWAPINSERT, ARR_CONCAT  ->  ARR_SUCCESS or ARR_FAIL
    ARR_TRUNCATE, ARR_SPLICE,
    ARR_SWAPSPLICE              ->  New length of array
    ARR_POP                     ->  Popped element
    ARR_FOREACH...              ->  For loop header
    ARR_INIT, ARR_DEINIT,
    ARR_SORT                    ->  (Statements)  */

#ifndef ARR_INCLUDED
#define ARR_INCLUDED

#include <stdlib.h>
#include <string.h>

#define ARR_T(...) struct { __VA_ARGS__ *data; size_t cap; size_t len; }
typedef ARR_T(int)              arr_int_t;
typedef ARR_T(unsigned)         arr_uint_t;
typedef ARR_T(double)           arr_double_t;
typedef ARR_T(char)             arr_char_t;
typedef ARR_T(unsigned char)    arr_uchar_t;
typedef ARR_T(char*)            arr_str_t;

#define ARR_FAIL    0
#define ARR_SUCCEED 1

/*  Bookkeeping */
#define ARR_INIT(arr) do { (arr).len = 0; (arr).cap = 0; (arr).data = NULL; } \
                      while(0)

#define ARR_DEINIT(arr) do { free((arr).data); ARR_INIT(arr); } while(0)

#define ARR_RESERVE(arr, count) \
    ( ((arr).cap > (count)) ? ARR_SUCCEED \
        : ((arr).data = realloc((arr).data, (count) * sizeof(*(arr).data)), \
           (arr).data ? ((arr).cap = (count), ARR_SUCCEED) : ARR_FAIL) )  

#define ARR_RESERVE_CLEAR(arr, count) \
    ( ((arr).cap > (count)) ? ARR_SUCCEED \
        : ((arr).data = realloc((arr).data, (count) * sizeof(*(arr).data)), \
        (!(arr).data) ? ARR_FAIL : \
            ((arr).cap = (count), ARR_TRUNCATE_CLEAR((arr), (arr).len), \
            ARR_SUCCEED)) )

/*  Adding elements */
#define ARR_PUSH(arr, val) \
    ( !ARR_RESERVE((arr), (arr).len + 1) ? ARR_FAIL \
        : ((arr).data[(arr).len++] = (val), ARR_SUCCEED) )

#define ARR_INSERT(arr, idx, val) \
    ( !ARR_RESERVE((arr), (arr).len + 1) ? ARR_FAIL \
        : (memmove((arr).data + (idx) + 1, \
                   (arr).data + (idx), ((arr).len++) - (idx)), \
            ((arr).data[idx] = (val)), ARR_SUCCEED) )

#define ARR_SWAPINSERT(arr, idx, val) \
    ( !ARR_RESERVE((arr), (arr).len + 1) ? ARR_FAIL\
        : ((arr).data[(arr).len++] = (arr).data[idx], (arr).data[idx] = (val), \
            ARR_SUCCEED) )

#define ARR_CONCAT(arr1, arr2) \
    ( (arr1).data == (arr2).data, \
      !ARR_RESERVE((arr1), (arr1).len + (arr2).len) ? ARR_FAIL \
        : (memcpy((arr1).data + (arr1).len, \
                  (arr2).data, \
                  (arr2).len * sizeof(*(arr2).data)), \
            (arr1).len += (arr2).len, ARR_SUCCEED) )

/*  Removing elements   */
#define ARR_POP(arr) \
    ( (arr).data[--((arr).len)] )

#define ARR_TRUNCATE(arr, count) \
    ( ((arr).len > (count)) ? (arr).len = (count) : (arr).len )

#define ARR_TRUNCATE_CLEAR(arr, count) \
    ( ((arr).cap > (count)) \
        ? (memset((arr).data + (count), 0, sizeof(*(arr).data) * ((arr).cap - (count))), \
            (arr).len = (count)) \
        : (arr).len )

#define ARR_SPLICE(arr, idx, count) \
    ( memmove((arr).data + (idx),   (arr).data + (idx) + (count),\
             ((arr).len - (idx) - (count)) * sizeof(*(arr).data)), \
        (arr).len -= (count) )

#define ARR_SWAPSPLICE(arr, idx, count) \
    ( memmove((arr).data + (idx), (arr).data + ((arr).len - (count)), \
              (count) * sizeof(*(arr).data)), \
        (arr).len -= (count) ) 

/*  Traversing elements */
#define ARR_SORT(arr, fn) \
    qsort((arr).data, (arr).len, sizeof(*(arr).data), fn)

#define ARR_FOREACH(arr, iter, idx) \
    for((idx) = 0; \
        ((idx) < (arr).len) && ((iter) = (arr).data[idx], 1); \
        ++(idx))

#define ARR_FOREACH_PTR(arr, iter, idx) \
    for((idx) = 0; \
        ((idx) < (arr).len) && ((iter) = &((arr).data[idx]), 1); \
        ++(idx))

#define ARR_FOREACH_REV(arr, iter, idx) \
    for((idx) = (arr).len - 1; \
        ((idx) >= 0) && ((iter) = (arr).data[idx], 1); \
        --(idx))

#define ARR_FOREACH_PTR_REV(arr, iter, idx) \
    for((idx) = (arr).len - 1; \
        ((idx) >= 0) && ((iter) = &((arr).data[idx]), 1); \
        --(idx))
#define ARR_FOREACH_REV_PTR ARR_FOREACH_PTR_REV

#endif
