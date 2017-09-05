//
// Created by yk on 17-7-26.
//
#include <cstdlib>
#include "Exception_Handler.h"
#include "DynArrayDef.h"
#include "DynArray.h"

DynArray tktable;       //单词表中放置标识符，包括变量名，函数名，结构定义名
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
void dynarray_realloc(DynArray* patr,int new_size)
{
    int capacity=patr->capacity;
    while (capacity<new_size)
        capacity*=2;
//    void *mda=*(patr->data);
//    printf("开始重新分配");
    void **data=(void**)realloc(patr->data,capacity);
//    printf("重新分配失败");
    if (!data)
        error("内存分配失败");
    patr->data=data;
    patr->capacity=capacity;
}

/*
 * int count=parr->count+1;
    if (count*sizeof(void*)>parr->capacity)
        dynarray_realloc(parr,count*sizeof(void*));
    parr->data[count-1]=data;
    parr->count=count;
 */
void dynarray_add(DynArray*parr,void *data)
{
    int count=parr->count+1;
    if (count*sizeof(void*)>parr->capacity)
        dynarray_realloc(parr,count*sizeof(void*));
    parr->data[count-1]=data;
    parr->count=count;
}

/*
 * if (!parr)
    {
        parr->data=(void**)malloc(sizeof(void*)*initsize);
        parr->count=0;
        parr->capacity=initsize;
    }
 */
void dynarray_init(DynArray*parr,int initsize)
{
    if (parr)
    {
        parr->data=(void**)malloc(sizeof(void*)*initsize);
        parr->count=0;
        parr->capacity=initsize;
    }
}

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
void dynarray_free(DynArray*parr)
{
    if (parr)
    {
        for (void **p = parr->data; parr->count ; ++p,--parr->count) {
            if (*p)
                free(*p);
        }
        free(parr->data);
        parr->count=0;
        parr->data=NULL;
    }
}

/*
 * int **p=(int**)parr->data;
    for (int i = 0; i < parr->count; ++i,++p) {
        if (key==**p)
            return i;
    }
    return -1;
 */
int dynarray_search(DynArray* parr,int key)
{
    int **p=(int**)parr->data;
    for (int i = 0; i < parr->count; ++i,++p) {
        if (key==**p)
            return i;
    }
    return -1;
}
