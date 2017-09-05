//
// Created by yk on 17-7-24.
//

#ifndef SC_COMPLIER_DYNSTRING_H
#define SC_COMPLIER_DYNSTRING_H

#include <stdlib.h>
#include <stddef.h>
#include "DynStringDef.h"


extern DynString sourcestr;    //单词源码字符串
extern DynString tkstr;        //单词字符串
/*
 * 初始化动态数组，设置相应的变量
 * if(pstr!=NULL)
    {
        char *data=(char*)malloc(sizeof(char)*initsize)
        pstr->data=data;
        pstr->count=0;
        pstr->capacity=initsize;
    }
 */
extern void dynstring_init(DynString *pstr,int initsize);

/*
 * if(pstr!=NULL)
    {
        if(pstr->data)
            free(pstr->data);
        pstr->count=0;
        pstr->capacity=0;
    }
 */
extern void dynstring_free(DynString* pstr);

/*
 * dynstring_free(pstr);
   dynstring_init(pstr,8);
 */
extern void dynstring_reset(DynString *pstr);

/*
 * int capacity=pstr->capacity;
    while (capacity<new_size)
        capacity*=2;

    char *data=realloc(pstr->data,capacity);
    if(!data)
        error("内存分配失败");
    pstr->capacity=capacity;
    pstr->data=data;
 */
extern void dynstring_realloc(DynString *pstr,int new_size);

/*
 * int count=pstr->count+1;
    if(count>pstr->capacity)
        dynstring_realloc(pstr,count);
    (pstr->data)[count-1]=ch;
    //原作者是这么写的
    //((char*)pstr->data)[count-1]=ch
    pstr->count=count;
 */
extern void dynstring_chcat(DynString*pstr, int ch);
#endif //SC_COMPLIER_DYNSTRING_H
