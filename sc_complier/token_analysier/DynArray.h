//
// Created by yk on 17-7-26.
//

#ifndef SC_COMPLIER_DYNARRAY_H
#define SC_COMPLIER_DYNARRAY_H

#include <stdlib.h>
#include <stddef.h>
#include "DynArrayDef.h"

//DynArray tktable;
extern DynArray tktable;
/*
 * int capacity=patr->capacity;
    while (capacity<new_size)
        capacity*=2;
    void *data=realloc(patr->data,capacity);
    if (!data)
        error("内存分配失败");
    *(patr->data)=data;
    patr->capacity=capacity;
 */
extern void dynarray_realloc(DynArray* patr,int new_size);

/*
 * int count=parr->count+1;
    if (count*sizeof(void*)>parr->capacity)
        dynarray_realloc(parr,count*sizeof(void*));
    parr->data[count-1]=data;
    parr->count=count;
 */
extern void dynarray_add(DynArray*parr,void *data);

/*
 * if (!parr)
    {
        parr->data=(void**)malloc(sizeof(void*)*initsize);
        parr->count=0;
        parr->capacity=initsize;
    }
 */
extern void dynarray_init(DynArray*parr,int initsize);

/*
 *  if (parr)
    {
        for (void **p = parr->data; parr->count ; ++p,--parr->count) {
            if (*p)
                free(*p);
        }
        free(parr->data);
        parr->count=0;
        parr->data=NULL;
    }
 */
extern void dynarray_free(DynArray*parr);

/*
 * int **p=(int**)parr->data;
    for (int i = 0; i < parr->count; ++i,++p) {
        if (key==**p)
            return i;
    }
    return -1;
 */
extern int dynarray_search(DynArray* parr,int key);
#endif //SC_COMPLIER_DYNARRAY_H
