//
// Created by yk on 17-7-26.
//

#include <cstring>
#include "TkWordDef.h"
#include "Exception_Handler.h"
#include "DynArray.h"
#include "TokenCode.h"
#include "global_var.h"
#include "TKTable.h"

#define MAXKEY 1024

TkWord *tk_hashtable[MAXKEY];   //单词哈希表

/*
 * 计算单词的哈希码，这种方法在UNIX中广泛采用
 */
int elf_hash(char *key) {
    int h = 0, g;

    if (key) {
        while (*key) {
            h = (h << 4) + *key++;
            g = h & 0xf0000000;
            if (g)
                h ^= g >> 24;
            h &= ~g;
        }
        return h % MAXKEY;
    }
    printf("字符串为空，无法哈希...\n");
    return -1;
}

/*
 * int keyno=elf_hash(tp->spelling);
    dynarray_add(&tktable,tp);
    tp->next=tk_hashtable[keyno];
    tk_hashtable[keyno]=tp;
    return tp;
    这个函数和tkword_insert不同的是他不检查是否已经存在，直接插入
    而且免去了insert过程中构造单词节点的步骤
 */
TkWord *tkword_direct_insert(TkWord *tp) {
    int keyno = elf_hash(tp->spelling);
    dynarray_add(&tktable, tp);
    tp->next = tk_hashtable[keyno];
    tk_hashtable[keyno] = tp;
    return tp;
}

/*
 *根据keyno到哈希表中寻找相应的单词节点
 */
TkWord *tkword_find(char *p, int keyno) {
    TkWord *tp = NULL, *tp1;
    for (tp1 = tk_hashtable[keyno]; tp1; tp1 = tp1->next) {
        if (!strcmp(p, tp1->spelling)) {
            token = tp1->tkcode;
            tp = tp1;
        }
    }
    return tp;
}

/*
 * 每一步操作都要同步处理单词表和哈希表，别忘了
 * 这企鹅对于每一个TkWord，把spelling的内容都置于最后面，即
 * spelling指向sizeof（TkWord）后
 */
TkWord *tkword_insert(char *p) {
    int keyno = elf_hash(p);
    TkWord *tp = tkword_find(p, keyno);
    char *s;
    if (tp == NULL) {
        int length = strlen(p);
        tp = (TkWord *) mallocz(sizeof(TkWord) + length + 1);
        tp->next = tk_hashtable[keyno];
        tk_hashtable[keyno] = tp;
        dynarray_add(&tktable, tp);
        tp->tkcode = tktable.count - 1;
        s = (char *) tp + sizeof(TkWord);
        tp->spelling = s;
        for (char *end = p + length; p < end;) {
            *s++ = *p++;
        }
        *s = (char) '\0';
    }
    return tp;
}

/*
 * 这个函数基本类似于malloc，只不过malloc以后memset为0
 */
void *mallocz(int size) {
    void *ptr = malloc(size);
    if (!ptr && size)
        error("内存分配失败");
    memset(ptr, 0, size);
    return ptr;
}

/*
 * 词法分析初始化,构建好相应的关键字单词表和哈希表
 */
void init_lex() {
    TkWord *tp;
    static TkWord keywords[] = {
            {TK_PLUS,      NULL, "+",           NULL, NULL},
            {TK_MINUS,     NULL, "-",           NULL, NULL},
            {TK_STAR,      NULL, "*",           NULL, NULL},
            {TK_DIVIDE,    NULL, "/",           NULL, NULL},
            {TK_MOD,       NULL, "%",           NULL, NULL},
            {TK_EQ,        NULL, "==",          NULL, NULL},
            {TK_NEQ,       NULL, "!=",          NULL, NULL},
            {TK_LT,        NULL, "<",           NULL, NULL},
            {TK_LEQ,       NULL, "<=",          NULL, NULL},
            {TK_GT,        NULL, ">",           NULL, NULL},
            {TK_GEQ,       NULL, ">=",          NULL, NULL},
            {TK_ASSIGN,    NULL, "=",           NULL, NULL},
            {TK_POINTSTO,  NULL, "->",          NULL, NULL},
            {TK_DOT,       NULL, ".",           NULL, NULL},
            {TK_AND,       NULL, "&",           NULL, NULL},
            {TK_OPENPA,    NULL, "(",           NULL, NULL},
            {TK_CLOSEPA,   NULL, ")",           NULL, NULL},
            {TK_OPENBR,    NULL, "[",           NULL, NULL},
            {TK_CLOSEBR,   NULL, "]",           NULL, NULL},
            {TK_BEGIN,     NULL, "{",           NULL, NULL},
            {TK_END,       NULL, "}",           NULL, NULL},
            {TK_SEMICOLON, NULL, ";",           NULL, NULL},
            {TK_COMMA,     NULL, ",",           NULL, NULL},
            {TK_ELLIPSIS,  NULL, "...",         NULL, NULL},
            {TK_EOF,       NULL, "End_Of_File", NULL, NULL},
            {TK_CINT,      NULL, "整型常量",        NULL, NULL},
            {TK_CCHAR,     NULL, "字符常量",        NULL, NULL},
            {TK_CSTR,      NULL, "字符串常量",       NULL, NULL},
            {KW_CHAR,      NULL, "char",        NULL, NULL},
            {KW_SHORT,     NULL, "short",       NULL, NULL},
            {KW_INT,       NULL, "int",         NULL, NULL},
            {KW_VOID,      NULL, "void",        NULL, NULL},
            {KW_STRUCT,    NULL, "struct",      NULL, NULL},
            {KW_IF,        NULL, "if",          NULL, NULL},
            {KW_ELSE,      NULL, "else",        NULL, NULL},
            {KW_FOR,       NULL, "for",         NULL, NULL},
            {KW_CONTINUE,  NULL, "continue",    NULL, NULL},
            {KW_BREAK,     NULL, "break",       NULL, NULL},
            {KW_RETURN,    NULL, "return",      NULL, NULL},
            {KW_SIZEOF,    NULL, "sizeof",      NULL, NULL},
            {KW_ALIGN,     NULL, "__align",     NULL, NULL},
            {KW_CDECL,     NULL, "__cdecl",     NULL, NULL},
            {KW_STDCALL,   NULL, "__stdcall",   NULL, NULL},
            {0,            NULL, NULL,          NULL, NULL},
    };
    dynarray_init(&tktable,8);
    //因为最后一个元素的spelling为null，这样就巧妙的结束了
    for (tp=&keywords[0];tp&&tp->spelling;tp++) {
        tkword_direct_insert(tp);
    }
}