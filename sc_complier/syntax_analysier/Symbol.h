//
// Created by yk on 17-8-28.
//

#ifndef SC_COMPLIER_SYMBOL_H
#define SC_COMPLIER_SYMBOL_H

#include "Stack.h"
struct Symbol;

/*
 * 该结构体用来表示编译过程中遇到的多种类型
 * 如多维数组，函数，指针等等
 * int t;                  //数据类型
 * struct Symbol *ref;     //引用符号
 */
typedef struct Type
{
    int t;                  //数据类型
    struct Symbol *ref;     //引用符号
}Type;

/*
 * int v;                  符号的单词编码,在单词表中的下标
 * int r;                  符号关联的寄存器
 * int c;                  符号关联值,更确切的说是偏移值,即偏移处的值
 * Type type;              符号数据类型
 * struct Symbol *next;    关联的其他符号
 * struct Symbol *prev_tok;指向前一定义的同名符号
 */
struct Symbol{
    int v;
    int r;
    int c;
    Type type;
    struct Symbol *next;
    struct Symbol *prev_tok;
};

Symbol *sym_direct_push(Stack *stack,int v,Type *type,int c);

Symbol *sym_push(int v,Type *type,int r,int c);

Symbol *func_sym_push(int v,Type *type);

Symbol *sym_search(int v);

Symbol *struct_search(int v);

Symbol *var_sym_put(Type *type,int r,int v,int addr);

Symbol *sec_sym_put(char *sec,int c);

void sym_pop(Stack * ptop,Symbol *b);
#endif //SC_COMPLIER_SYMBOL_H
