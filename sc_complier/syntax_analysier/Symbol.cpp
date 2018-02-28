//
// Created by yk on 17-8-28.
//
#include <cstddef>
#include "Stack.h"
#include "../token_analysier/TkWordDef.h"
#include "../token_analysier/DynArray.h"
#include "syn_const.h"
#include "../token_analysier/Exception_Handler.h"
#include "../token_analysier/TKTable.h"

/*
 * 该结构体用来表示编译过程中遇到的多种类型
 * 如多维数组，函数，指针等等
 */
//typedef struct Type
//{
//    int t;                  //数据类型
//    struct Symbol *ref;     //引用符号
//}Type;

Type char_pointer_type;  //字符串指针
Type int_type;           //int类型
Type default_func_type;  //缺省函数类型


/*
 * int v;                  符号的单词编码
 * int r;                  符号关联的寄存器
 * int c;                  符号关联值
 * Type type;              符号数据类型
 * struct Symbol *next;    关联的其他符号
 * struct Symbol *prev_tok;指向前一定义的同名符号
 */
//struct Symbol{
//    int v;
//    int r;
//    int c;
//    Type type;
//    struct Symbol *next;
//    struct Symbol *prev_tok;
//};

/*
 * 功能：  符号放入符号栈中
 * v：    符号编号
 * type： 符号数据类型
 * c：    符号关联值
 */
Symbol *sym_direct_push(Stack *stack,int v,Type *type,int c)
{
    Symbol s,*p;
    s.v=v;
    s.type.t=type->t;
    s.type.ref=type->ref;
    s.c=c;
    s.next=NULL;
    p=(Symbol*)stack_push(stack,&s, sizeof(Symbol));
    return p;
}

/*
 * 将符号放到符号表，动态判断是放入全局符号表还算局部符号表
 * v：    符号编号,根据其最高4位的不同值來判断符号是结构体，匿名，成员变量还算参数
 * type： 符号数据类型
 * r：    符号存储类型
 * c：    符号关联值,更确切的说是偏移值
 */
Symbol *sym_push(int v,Type *type,int r,int c)
{
    Symbol *ps, **pps;
    TkWord *ts;
    Stack *s;
    if(!stack_is_empty(&local_sym_stack))
        s=&local_sym_stack;
    else s=&global_sym_stack;
    ps=sym_direct_push(s,v,type,c);
    ps->r=r;

    //不记录结构体成员及匿名符号（匿名符号是常量一类的东西）
    //比如f（2,4）,这里面2和4就不需要保存，因为这个ANOM很大，
    //所以就保证了其他的标识符都能存储符号表中
    if ((v&SC_STRUCT)||v<SC_ANOM)
    {
        //更新单词sym_struct或sym_identifier字段
        ts=(TkWord*)tktable.data[(v&~SC_STRUCT)];
        if(v&SC_STRUCT)
            pps=&ts->sys_struct;
        else pps=&ts->sym_identifier;
        ps->prev_tok=*pps;
        *pps=ps;
    }
    return ps;
}

/*
 * 功能：   将函数符号放入全局符号中
 *         这里作者的多重指针用的很666啊
 * v：     符号编号
 * type：  符号数据类型
 */
Symbol *func_sym_push(int v,Type *type)
{
    Symbol *s,**ps;
    s=sym_direct_push(&global_sym_stack,v,type,0);
    ps=&((TkWord*)tktable.data[v])->sym_identifier;
    //同名符号，函数符号放到最后->->...s
    while (*ps!=NULL)
        ps=&(*ps)->prev_tok;
    s->prev_tok=NULL;
    *ps=s;
    return s;
}

/*
 * 功能： 查找结构定义
 * v：   符号编号,从0开始，以符号总数计
 */
Symbol *sym_search(int v)
{
    if(v>=tktable.count)
        return NULL;
    return ((TkWord*)tktable.data[v])->sym_identifier;
}

/*
 * 功能：查找结构定义
 * v： 符号编号
 */
Symbol *struct_search(int v)
{
    if(v>=tktable.count)
        return NULL;
    return ((TkWord*)tktable.data[v])->sys_struct;
}

/*
 * 根据存储类型r确定放入全局栈还是局部栈
 * r：    符号存储类型
 * addr:  关联值
 * v：    符号编号
 */
Symbol *var_sym_put(Type *type,int r,int v,int addr)
{
    Symbol *sym=NULL;
    if((r&SC_VALMASK)==SC_LOCAL)
        sym=sym_push(v,type,r,addr);
    else if(v&&(r&SC_VALMASK)==SC_GLOBAL)
    {
        sym=sym_search(v);
        if(sym)
            error("%s 重定义\n",((TkWord*)tktable.data[v])->spelling);
        else
           sym=sym_push(v,type,r|SC_SYM,0);
    }
    return sym;
}

/*
 * 将节名称放入全局符号表
 * sec:  节名称
 * c:    符号关联值
 */
Symbol *sec_sym_put(char *sec,int c)
{
    TkWord *tp=tkword_insert(sec);
    token=tp->tkcode;
    Type type;
    type.t=T_INT;
    Symbol *s=sym_push(token,&type,SC_GLOBAL,c);
    return s;
}

/*
 * 弹出栈中符号直到栈顶符号为'b'
 * ptop： 符号栈栈顶
 * b：    符号指针
 */
void sym_pop(Stack * ptop,Symbol *b)
{
    Symbol *s,**ps;
    TkWord *ts;
    int v;

    s=(Symbol*)stack_get_top(ptop);
    while(s!=b)
    {
        v=s->v;
        //更新单词表中sym_struct_,sym_identifier
        if ((v&SC_STRUCT)||v<SC_ANOM)
        {
            ts=(TkWord*)tktable.data[v&~SC_STRUCT];
            if(v&SC_STRUCT)
                ps=&ts->sys_struct;
            else ps=&ts->sym_identifier;
            *ps=s->prev_tok;
        }
        stack_pop(ptop);
        s=(Symbol*)stack_get_top(ptop);
    }
}