//
// Created by yk on 17-7-26.
//
#include <stdlib.h>
#include <stddef.h>
#include "Exception_Handler.h"
#include "DynStringDef.h"
#include "DynString.h"

DynString sourcestr;    //单词源码字符串
DynString tkstr;        //单词字符串
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
void dynstring_init(DynString *pstr,int initsize)
{
    if(pstr!=NULL)
    {
        char *data=(char*)malloc(sizeof(char)*initsize);
        pstr->data=data;
        pstr->count=0;
        pstr->capacity=initsize;
    }
}

/*
 * if(pstr!=NULL)
    {
        if(pstr->data)
            free(pstr->data);
        pstr->count=0;
        pstr->capacity=0;
    }
 */
void dynstring_free(DynString* pstr)
{
    if(pstr!=NULL)
    {
        if(pstr->data)
            free(pstr->data);
        pstr->count=0;
        pstr->capacity=0;
    }
}

/*
 * dynstring_free(pstr);
   dynstring_init(pstr,8);
 */
void dynstring_reset(DynString *pstr)
{
    dynstring_free(pstr);
    dynstring_init(pstr,8);
}

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
void dynstring_realloc(DynString *pstr,int new_size)
{
    int capacity=pstr->capacity;
    while (capacity<new_size)
        capacity*=2;

    char *data=(char *)realloc(pstr->data,capacity);
    if(!data)
        error("内存分配失败");
    pstr->capacity=capacity;
    pstr->data=data;
}

/*
 * int count=pstr->count+1;
    if(count>pstr->capacity)
        dynstring_realloc(pstr,count);
    (pstr->data)[count-1]=ch;
    //原作者是这么写的
    //((char*)pstr->data)[count-1]=ch
    pstr->count=count;
 */
void dynstring_chcat(DynString*pstr, int ch)
{
    int count=pstr->count+1;
    if(count>pstr->capacity)
        dynstring_realloc(pstr,count);
    (pstr->data)[count-1]=ch;
    //原作者是这么写的
    //((char*)pstr->data)[count-1]=ch
    pstr->count=count;
}
