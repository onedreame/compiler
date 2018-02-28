//
// Created by yk on 17-7-25.
//

#ifndef SC_COMPLIER_TKTABLE_H
#define SC_COMPLIER_TKTABLE_H

#include <stdio.h>
#include <string.h>
#include "TokenCode.h"
#include "global_var.h"
#include "DynArray.h"
#include "TkWordDef.h"

#define MAXKEY 1024

extern TkWord *tk_hashtable[MAXKEY];   //单词哈希表
/*
 * 计算单词的哈希码，这种方法在UNIX中广泛采用
 */
extern int elf_hash(char *key) ;

/*
 * int keyno=elf_hash(tp->spelling);
    dynarray_add(&tktable,tp);
    tp->next=tk_hashtable[keyno];
    tk_hashtable[keyno]=tp;
    return tp;
    这个函数和tkword_insert不同的是他不检查是否已经存在，直接插入
    而且免去了insert过程中构造单词节点的步骤
 */
extern TkWord *tkword_direct_insert(TkWord *tp);

/*
 *根据keyno到哈希表中寻找相应的单词节点
 */
extern TkWord *tkword_find(char *p, int keyno);

/*
 * 每一步操作都要同步处理单词表和哈希表，别忘了
 * 这企鹅对于每一个TkWord，把spelling的内容都置于最后面，即
 * spelling指向sizeof（TkWord）后
 */
extern TkWord *tkword_insert(char *p) ;

/*
 * 这个函数基本类似于malloc，只不过malloc以后memset为0
 */
extern void *mallocz(int size);

/*
 * 词法分析初始化,构建好相应的关键字单词表和哈希表
 */
extern void init_lex();


#endif //SC_COMPLIER_TKTABLE_H
