//
// Created by yk on 17-7-26.
//

#ifndef SC_COMPLIER_TKWORDDEF_H
#define SC_COMPLIER_TKWORDDEF_H

#include "../syntax_analysier/Symbol.h"

/*
 * int tkcode;              单词编码
 * TkWord next;             指向哈希冲突的同义词
 * char *spelling;          单词字符串
 * Symbol *sys_struct;      指向单词所表示的结构定义
 * Symbol *sym_identifier;  指向单词所表示的标识符
 */
typedef struct TkWord {
    int tkcode;
    struct TkWord *next;
    char *spelling;
    Symbol *sys_struct;
    Symbol *sym_identifier;
} TkWord;
#endif //SC_COMPLIER_TKWORDDEF_H
