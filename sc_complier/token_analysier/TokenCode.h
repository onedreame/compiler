//
// Created by yk on 17-7-24.
//

#ifndef SC_COMPLIER_TOKENCODE_H
#define SC_COMPLIER_TOKENCODE_H
enum e_TokenCode{
    //运算符及分隔符
    TK_PLUS, //+加号
    TK_MINUS,   //-减号
    TK_STAR,    //*星号
    TK_DIVIDE,  // /除号
    TK_MOD,     //%取模运算符
    TK_EQ,      //==等号
    TK_NEQ,     //！=不等号
    TK_LT,      //<小于号
    TK_LEQ,     //<=小于等于号
    TK_GT,      //>大于号
    TK_GEQ,     //>=大于等于号
    TK_ASSIGN,      //=赋值运算符
    TK_POINTSTO,    //->指向结构体成员运算符
    TK_DOT,     //.结构图成员运算符
    TK_AND,     //&地址与运算符
    TK_OPENPA,  //(左圆括号
    TK_CLOSEPA, //)右圆括号
    TK_OPENBR,  //[左中括号
    TK_CLOSEBR, //]右中括号
    TK_BEGIN,   // {左大括号
    TK_END,     //}右大括号
    TK_SEMICOLON,   //；分号
    TK_COMMA,        //,逗号
    TK_ELLIPSIS,    //...省略号
    TK_EOF,     //文件结束符

    /*常量*/
    TK_CINT,    //整型常量
    TK_CCHAR,   //字符常量
    TK_CSTR,    //字符串常量

    /*关键字*/
    KW_CHAR,    //char关键字
    KW_SHORT,   //short关键字
    KW_INT,     //int关键字
    KW_VOID,    //void关键字
    KW_STRUCT,  //struct关键字
    KW_IF,      //if关键字
    KW_ELSE,    //else关键字
    KW_FOR,     //for关键字
    KW_CONTINUE,    //continue关键字
    KW_BREAK,   //break关键字
    KW_RETURN,  //return关键字
    KW_SIZEOF,  //sizeof关键字

    KW_ALIGN,   //__align关键字
    KW_CDECL,   //__cdecl关键字
    KW_STDCALL, //__stdcall关键字

    /*标识符*/
    TK_IDENT,

};

/*词法状态枚举定义*/
enum e_LexState
{
    LEX_NORMAL,
    LEX_SEP
};

/*
 * 错误层级，这里只定义了warning和error两种
 */
enum e_ErrorLevel
{
    LEVEL_WARNING,
    LEVEL_ERROR,
};

/*
 * 发生错误的阶段，分为编译阶段和链接阶段
 */
enum e_WorkStage
{
    STAGE_COMPILE,
    STAGE_LINK,
};
#endif //SC_COMPLIER_TOKENCODE_H
