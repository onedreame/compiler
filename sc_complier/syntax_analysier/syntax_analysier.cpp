//
// Created by yk on 17-8-19.
//
#include <assert.h>
#include "syntax_analysier.h"
#include "../token_analysier/global_var.h"
#include "../token_analysier/TokenParser.h"
#include "../token_analysier/TokenCode.h"
#include "../syntax_analysier/syn_const.h"
#include "../syntax_analysier/Symbol.h"
#include "Stack.h"

#define ALIGN_SET 0X100
int syntax_state=SNTX_NUL;
int syntax_level=0;
/*
 * 工程中的get_token采用处理完本函数以后获得下个函数的
 * token，方便，不用做无用功的调试
 */
void print_tab(int n)
{
    for(int i=0;i<n;++i)
        printf("\t");
}

/*
 * if sp,print(" "),then color_print;
 * if lf_ht,print("\n"),print_tab(syntax_level)
 * if nul,then directly color_print()
 * if delay,then we just break,
 */
Symbol *mt;
void syntax_indent()
{
    switch (syntax_state)
    {
        case SNTX_NUL:
            color_token(LEX_NORMAL);
            break;
        case SNTX_SP:
            printf(" ");
            color_token(LEX_NORMAL);
            break;
        case SNTX_LF_HT:
            if(token==TK_END)
                syntax_level--;
//            assert(mt->c==-1);
            printf("\n");
//            assert(mt->c==-1);
            print_tab(syntax_level);
            color_token(LEX_NORMAL);
            break;
        case SNTX_DELAY:
            break;
    }
    syntax_state=SNTX_NUL;
}

void translate_unit()
{
    while(token!=TK_EOF)
    {
        external_declaration(SC_GLOBAL);
    }
}

void external_declaration(int l)
{
    Type btype,type;
    int n;

    if(!type_specifier(btype))
    {
        expect("<类型区分符>");
    }

    if(btype.t==T_STRUCT&&token==TK_SEMICOLON)
    {
        get_token();
        return;
    }

    while (true)
    {
        type=btype;
        int align=1;
        int fc;
        declarator(&align,&n,type,fc);

        if(token==TK_BEGIN){
            if(l==SC_LOCAL)
                error("不支持函数嵌套定义");
            if((type.t&T_BTYPE)!=T_FUNC)
                expect("<函数定义>");
//            syntax_state=SNTX_LF_HT;
            syntax_level++;

            Symbol *s=sym_search(n);
            if(s)
            {
                if((type.t&T_BTYPE)!=T_FUNC)
                    error("'%s'重定义",get_tkstr(n));
                s->type=type;
            }else
                s=func_sym_push(n,&type);

            s->r=SC_GLOBAL|SC_SYM;
            get_token();
            funcbody(s);
            break;
        }
        else{
            if((type.t&T_BTYPE)==T_FUNC)
            {
                if(sym_search(n)==NULL)
                    sym_push(n,&type,SC_SYM|SC_GLOBAL,0);
            }
            else{
                int r=0;
                if(!(type.t&T_ARRAY))
                    r|=SC_LVAL;
                //置为1代表保存在栈中且是左值
                r|=1;
                bool has_init=(token==TK_ASSIGN);
                if(has_init)
                {
                    get_token();
                    initializer(&type);
                    skip(TK_SEMICOLON);
                    break;
                }
                int addr;
                var_sym_put(&type,r,n,addr);
            }
//            if(token==TK_ASSIGN)
//            {
//                get_token();
//                initializer();
//                skip(TK_SEMICOLON);
//                break;
//            }
            if(token==TK_COMMA)
                get_token();
            else if(token==TK_SEMICOLON){
                skip(TK_SEMICOLON);
                break;
            }
        }
    }
//    skip(TK_SEMICOLON);
}

bool type_specifier(Type &type)
{
    Type type1;
    int t=0;
    bool isfind=false;
    switch (token)
    {
        case KW_INT:
            syntax_state=SNTX_SP;
            t=T_INT;
            get_token();
            isfind= true;
            break;
        case KW_CHAR:
            syntax_state=SNTX_SP;
            t=T_CHAR;
            get_token();
            isfind= true;
            break;
        case KW_SHORT:
            syntax_state=SNTX_SP;
            t=T_SHORT;
            get_token();
            isfind= true;
            break;
        case KW_VOID:
            syntax_state=SNTX_SP;
            t=T_VOID;
            get_token();
            isfind= true;
            break;
        case KW_STRUCT:
            syntax_state=SNTX_SP;
            get_token();
            struct_specifier(type1);
            type.ref=type1.ref;
            t=T_STRUCT;
            isfind= true;
            break;
        default:
            break;
    }
    type.t=t;
    return isfind;
}

void struct_specifier(Type &type)
{
    if(token<TK_IDENT)
        expect("缺少结构体名");
    int v=token;
    syntax_state=SNTX_DELAY;
    int old_line=line_num;
    get_token();

    Symbol *s=struct_search(v);
    Type type1;
    //若当前的结构体还没加入栈中，那么就加入，加入以后下面不用处理
    if(!s)
    {
        type1.t=KW_STRUCT;
        //s->c设置为-1,代表结构体尚未定义
        s=sym_push(v|SC_STRUCT,&type1,0,-1);
        s->r=0;
    }
    type.t=T_STRUCT;
    type.ref=s;
    if(token==TK_BEGIN)
        if(old_line==line_num)  //如果{和标识符在同一行
            syntax_state=SNTX_LF_HT;
        else syntax_state=SNTX_NUL;
    else if(token==TK_CLOSEPA)
        syntax_state=SNTX_NUL;
    else if(token==TK_SEMICOLON) syntax_state=SNTX_SP;
//    printf("before:%d,mt:%d\n",type.ref->c,mt.ref->c);
    syntax_indent();
//    type.ref->c=old_line;
//    printf("after:%d,mt:%d,old:%d\n",type.ref->c,mt.ref->c,old_line);

    if(token==TK_BEGIN)
    {
//        syntax_state=SNTX_LF_HT;
        struct_declaration_list(&type);
    }
}

void struct_declaration_list(const Type *type)
{
//    从这里进来以后没有甚至state，
    syntax_level++;  //这里我原本放到了上面的strutc_list,不过放在这里更好
//    作者在这里家了一个状态，但是这种状态只适用于{和第一行声明
//    在同一行的情形，要不然会多出一行。
//    syntax_state=SNTX_LF_HT;
    Symbol *s=type->ref;
    get_token();
//    if(line_num==ori){
//        syntax_state=SNTX_LF_HT;
//        syntax_indent();
//    }

    int offset=0;
    int maxalign=1;
//    Symbol *s=type->ref;
    if(s->c!=-1)        //防止结构体的重复定义，这里传递的type可能指向其他已定义的ref
        error("结构体已定义");
    Symbol **ps=&s->next;
    while(token!=TK_END)
    {
        struct_declaration(&maxalign,&offset,&ps);
    }
    syntax_level--;
    s->c=cal_align(offset,maxalign);
    //这里说是结构体对齐,不过是以最大单元对齐
    s->r=maxalign;
    skip(TK_END);
}

int cal_align(int offset,int align)
{
    return (offset+align-1)&(~(align-1));
}

/*
 * 这里主要的处理是每次获得一个标识符，根据其是否对齐来生成
 * 该标识符的偏移值，同时把所有的成员变量都置为0.
 * 这里对于结构体的成员变量不保存在栈中，通过设置最高位实现，
 * 而是用符号结构体的next指针来保存结构体所有的变量
 */
void struct_declaration(int *maxalign,int *offset,Symbol ***ps)
{
    Type btype,type1;
    if(!type_specifier(btype))
    {
        expect("结构体中类型标识符");
    }

    //每次都要对type1重新赋值，这样可以让多个变量保持同一个定义
    type1=btype;
    int fc;int force_align, align;
    int v=0;
    declarator(&force_align,&v,type1,fc);
    //这里的align首先会给出默认值,如果有强制对齐，那么再修改
    int size=type_size(type1,align);
    //这里的对齐有什么意义么？没看出来有什么意义
    //意义是如果设置了强制对齐,那么必然设置第9位
    //后面的declarator处理以后只能以1,2,4字节对齐
    //这样处理以后，每次这里必定会处理，从而使得每次align
    //必定会被赋予正确的值。
    if(force_align&ALIGN_SET)
        align=force_align&~ALIGN_SET;
    *offset=cal_align(*offset,align);

    //SC_MEMBER的值保证了成员变量不会保存到栈中
    Symbol *ss=sym_push(v|SC_MEMBER,&type1,0,*offset);
    *offset+=size;
    if (align>*maxalign) {
        *maxalign=align;
    }

    **ps=ss;*ps=&ss->next;
    while(token==TK_COMMA)
    {
        get_token();
        v=0;
        type1=btype;
        declarator(&force_align,&v,type1,fc);
        size=type_size(type1,align);
        if(force_align&ALIGN_SET)
            align=force_align&~ALIGN_SET;
        if(*maxalign<align)
            *maxalign=align;
        *offset=cal_align(*offset,align);
        ss=sym_push(v|SC_MEMBER,&type1,0,*offset);
        *offset+=size;
        **ps=ss;
        *ps=&ss->next;
    }
    skip(TK_SEMICOLON);
//    struct_declarator_list(maxalign,offset,ps,btype);
}

//void struct_declarator_list(int *maxalign,int *offset,Symbol ***ps,Type &type)
//{
//    declarator(maxalign,offset,type);
//    while(token==TK_COMMA)
//    {
//        get_token();
//        declarator();
//    }
//    skip(TK_SEMICOLON);
//}

int function_calling_convention(int &fc)
{
    fc=KW_STDCALL;
    if(token==KW_STDCALL||token==KW_CDECL)
    {
        fc=token;
        get_token();
    }
    return fc;
}

void struct_member_alignment(int *align)
{
    int v=1;
    if(token==KW_ALIGN)
    {
        get_token();
        skip(TK_OPENPA);
        if(token!=TK_CINT)
        {
            error("对齐需要声明整型");
        }
        v=tkvalue;
        if(v!=1&&v!=2&&v!=4)
            v=1;
        v|=ALIGN_SET;
        *align=v;
        get_token();
        skip(TK_CLOSEPA);
    }
    else *align=1;
}

void mk_pointer(Type &type)
{
    type.t=T_PTR;
}

/*
 * align:    该变量是否声明了强制对齐
 * v：       该声明符在单词表中的下标
 * type：    该声明符的类型，如果是函数就算返回类型
 * fc：      如果是函数就代表函数的声明方式
 */
void declarator(int *align,int *v,Type &type,int &fc)
{
    //标记该declator是个指针，这里还未处理
    while(token==TK_STAR)
    {
        mk_pointer(type);
        get_token();
    }
    function_calling_convention(fc);
    if(align)
    struct_member_alignment(align);
    direct_declarator(type,v,fc);
}

void direct_declarator(Type &type,int *v, int &fc)
{
    if(token<TK_IDENT)
//        printf("当前符号为："),
//        color_token(LEX_NORMAL),
        expect("标识符");
    *v=token;
    get_token();
    direct_declarator_postfix(type,fc);
}

void direct_declarator_postfix(Type &type,int fc)
{
    int n;
    Symbol *s;
    if(token==TK_OPENPA)
        parameter_type_list(type,fc);
    else if(token==TK_OPENBR){
        get_token();
        n=-1;
        if(token==TK_CINT)
        {
            n=tkvalue;
            get_token();
        }
        skip(TK_CLOSEBR);
        //这里最后才保存第一维的数，应该是因为首先访问
        //最后的那一维，行优先访问
        direct_declarator_postfix(type,fc);
        //然后这里，type指向的是declarator的type，根据这些后缀
        //修改该declarotor的类型，
        s=sym_push(SC_ANOM,&type,0,n);
        type.t|=T_ARRAY|T_PTR;
        type.ref=s;
    }

}

/*
 * 用函数名的符号指针去链接所有他的符号
 */
void parameter_type_list(Type &type,int fc)
{
    int n;
    Symbol **last,*s, *first;
    int ffc;
    Type pt;

    first=NULL;
    last=&first;
    get_token();
    while (token!=TK_CLOSEPA)
    {
        if(!type_specifier(pt))
            expect("类型标识符");
        declarator(NULL,&n,pt,ffc);
        s=sym_push(n|SC_PAPAMS,&pt,0,0);
        *last=s;
        last=&s->next;
        if(token==TK_CLOSEPA)
            break;
        skip(TK_COMMA);
        if(token==TK_ELLIPSIS)
        {
            get_token();
            break;
        }
    }
    syntax_state=SNTX_DELAY;
    int ori=line_num;
    skip(TK_CLOSEPA);
    if(token==TK_BEGIN)
        if (line_num==ori)
            syntax_state=SNTX_LF_HT;
        else syntax_state=SNTX_NUL;
    else if(token==TK_SEMICOLON)
        syntax_state=SNTX_NUL;
    else error("语法错误");
    syntax_indent();

    //此处将函数返回类型存储，然后指向参数，最后将type设为函数引用
    //引用的相关信息存储在ref中
    //注意：这里解析出的type并没有添加到declarator中，还需要进一步处理
    s=sym_push(SC_ANOM,&type,fc,0);
    //这里又指向了首指针，形成了回环链表
    s->next=first;
    type.t=T_FUNC;
    type.ref=s;
}

void funcbody(Symbol *s)
{
    /*作者说这里放一匿名符号在局部符号表中？？？*/
    sym_direct_push(&local_sym_stack,SC_ANOM,&int_type,0);
    compound_statement(NULL,NULL);
    sym_pop(&local_sym_stack,NULL);
}

bool  is_type_specifier()
{
    if(token==KW_INT||token==KW_CHAR||token==KW_SHORT||
       token==KW_STRUCT)
        return true;
    return false;
}

/*
 * 功能：      解析复合语句,首先获得当前栈顶元素,处理完成以后一直pop直到获得该栈顶元素
 * bsym：     break跳转语句
 * csym：     continue跳转语句
 */
void compound_statement(int *bsym,int *csym)
{
//    skip(TK_BEGIN);
    Symbol *s=(Symbol*)stack_get_top(&local_sym_stack);
    while (token!=TK_END&&token!=TK_EOF)
    {
        while(is_type_specifier())
        {
            external_declaration(SC_LOCAL);
        }
        if(token==TK_END||token==TK_EOF) break;
        statement();
    }
    if (token==TK_END)
    {
        syntax_level--;
        sym_pop(&local_sym_stack,s);
        skip(TK_END);
    }
}

/*
 * 如果type是数组，那么直接get_token，不然才赋值语句
 * 那么应该是数组不能初始化么？
 */
void initializer(Type *type)
{
    if(type->t&T_ARRAY)
        get_token();
    else assignment_expression();
}

void assignment_expression()
{
    equality_expression();
    while (token==TK_ASSIGN)
    {
        get_token();
        assignment_expression();
    }
}

void equality_expression()
{
    relational_expression();
    while(token==TK_EQ||token==TK_NEQ)
    {
        get_token();
        relational_expression();
    }
}

void relational_expression()
{
    additive_expression();
    switch (token)
    {
        case TK_LT:case TK_LEQ:
        case TK_GT:case TK_GEQ:
            get_token();
            additive_expression();
            break;
        default:
            break;
    }
}

void additive_expression()
{
    multiplicative_expression();
    while (true)
    {
        bool is= false;
        switch (token)
        {
            case TK_PLUS:
            case TK_MINUS:
                get_token();
                multiplicative_expression();
                is= true;
                break;
            default:
                break;
        }
        if (is== false) break;
    }
}

void multiplicative_expression()
{
    unary_expression();
    while (true)
    {
        bool is= false;
        switch (token)
        {
            case TK_STAR:
            case TK_DIVIDE:
            case TK_MOD:
                get_token();
                unary_expression();
                is= true;
                break;
            default:
                break;
        }
        if (is== false) break;
    }
}

void unary_expression()
{
    switch (token)
    {
        case TK_AND:
        case TK_STAR:
        case TK_PLUS:
        case TK_MINUS:
            get_token();
            unary_expression();
            break;
        case KW_SIZEOF:
            get_token();
            sizeof_expression();
            break;
        default:
            postfix_expression();
            break;
    }
}

void postfix_expression()
{
    primary_expression();
    if(token==TK_OPENBR){
        while(token==TK_OPENBR)
        {
            get_token();
            expression();
            skip(TK_CLOSEBR);
        }
    }else if(token==TK_OPENPA){
        get_token();
        argument_expression_list();
    }else if(token==TK_DOT||token==TK_POINTSTO) {
        while (token == TK_DOT || token == TK_POINTSTO) {
            get_token();
            if (token < TK_IDENT)
                expect("成员变量名");
            get_token();
        }
    }
//    } else
//        error("后缀表达岁不支持的类型");
}

/*
 * 功能：         返回类型长度
 * type：        数据类型指针
 * align：       对齐值
 */
int type_size(Type &type,int &align)
{
    int bt=type.t&T_BTYPE;
    Symbol *s;
    switch (bt)
    {
        case T_STRUCT:
            s=type.ref;
            align=s->r;
            return s->c;

        case T_PTR:
            if(type.t&T_ARRAY)
            {
                s=type.ref;
                return type_size(s->type,align)*s->c;
            }
            else{
                align=4;
                return 4;
            }

        case T_INT:
            align=4;
            return 4;

        case T_SHORT:
            align=2;
            return 2;

        default:
            align=1;
            return 1;
    }
}

void sizeof_expression()
{
    skip(TK_OPENPA);
    Type type;
    int align;
    if(!is_type_specifier())
        expect("基本类型,int,char等");
    type_specifier(type);
    skip(TK_CLOSEPA);
    int size=type_size(type,align);
    if (size<0)
        error("sizeof计算类型尺寸失败");
}

/*
 * 初级表达式原来是字符串,整数,函数返回值等
 * default情况就算函数返回值,所以需要加括号
 */
void primary_expression()
{
//    if(token>=TK_IDENT||token==TK_CINT||token==TK_CSTR
//            ||token==TK_CCHAR)
//    {
//        get_token();
//        return ;
//    }
    int t,addr;
    Type type;
    Symbol *s;
    switch (token)
    {
        case TK_OPENPA:
            expression();
            skip(TK_CLOSEPA);
            break;
            break;

        case TK_CINT:
            get_token();
            break;

        case TK_CSTR:
            t=T_CHAR;
            type.t=t;
            mk_pointer(type);
            type.t|=T_ARRAY;
/*
 * 我感觉这里如果重复定义了一个字符串，那么会报错
 * 比如:
 * char *s="abc";
 * char *b="abc";
 * 等会需要实验一下
 * 这里还有一点,就算赋予了该局部变量一个随机地址,从而获得未初始化的值
 */
            var_sym_put(&type,SC_GLOBAL,0,addr);
            get_token();
            initializer(&type);
            break;

        case TK_CCHAR:
            get_token();
            break;

        default:
            t=token;
            get_token();
            if(t<TK_IDENT)
                expect("标识符或常量");
            s=sym_search(t);
            if(!s)
            {
                if(token!=TK_OPENPA)
                    error("'%s'未声明\n",get_tkstr(t));
                //允许函数不声明，直接引用
                s=func_sym_push(t,&default_func_type);
                s->r=SC_GLOBAL|SC_SYM;
            }
            break;
    }
//    expression();
//    skip(TK_CLOSEPA);
}

void expression()
{
    assignment_expression();
    while(token==TK_COMMA)
    {
        get_token();
        assignment_expression();
    }
}

void argument_expression_list()
{
    while(token!=TK_CLOSEPA)
    {
        assignment_expression();
        if(token==TK_CLOSEPA) break;
        skip(TK_COMMA);
//        assignment_expression();
    }
    skip(TK_CLOSEPA);
}

void expression_statement()
{
    while (token!=TK_SEMICOLON)
        expression();
    skip(TK_SEMICOLON);
}

void if_statement()
{
    skip(TK_OPENPA);
    expression();
    skip(TK_CLOSEPA);
    statement();
    if (token==KW_ELSE)
    {
        get_token();
        statement();
    }
}

void for_statement()
{
    skip(TK_OPENPA);
    if(is_type_specifier())
        get_token();
    expression_statement();
    expression_statement();
    if(token!=TK_CLOSEPA)
        expression();
    skip(TK_CLOSEPA);
    statement();
}

void continue_statement()
{
    skip(TK_SEMICOLON);
}

void break_statement()
{
    skip(TK_SEMICOLON);
}

void return_statement()
{
    if (token!=TK_SEMICOLON)
        expression();
    skip(TK_SEMICOLON);
}

void statement()
{
    switch (token) {
        case TK_BEGIN:
            syntax_state=SNTX_LF_HT;
            syntax_level++;
            skip(TK_BEGIN);
            int bs,cs;
            compound_statement(&bs,&cs);
//            skip(TK_END);
            break;
        case KW_IF:
            get_token();
            if_statement();
            break;
        case KW_RETURN:
            get_token();
            return_statement();
            break;
        case KW_BREAK:
            get_token();
            break_statement();
            break;
        case KW_CONTINUE:
            get_token();
            continue_statement();
            break;
        case KW_FOR:
            get_token();
            for_statement();
            break;
        default:
            expression_statement();
    }
}