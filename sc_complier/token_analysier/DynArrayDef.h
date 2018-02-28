//
// Created by yk on 17-7-26.
//

#ifndef SC_COMPLIER_DYNARRAYDEF_H
#define SC_COMPLIER_DYNARRAYDEF_H
typedef struct DynArray
{
    int count;  //元素数目
    int capacity;   //缓冲区长度
    void **data;    //指针数组
} DynArray;
#endif //SC_COMPLIER_DYNARRAYDEF_H
