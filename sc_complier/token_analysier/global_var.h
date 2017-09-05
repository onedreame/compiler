//
// Created by yk on 17-7-25.
//

#ifndef SC_COMPLIER_GLOBAL_VAR_H
#define SC_COMPLIER_GLOBAL_VAR_H

#define NONE                 "\e[0m"
#define BLACK                "\e[0;30m"
#define L_BLACK              "\e[1;30m"
#define RED                  "\e[0;31m"
#define L_RED                "\e[1;31m"
#define GREEN                "\e[0;32m"
#define L_GREEN              "\e[1;32m"
#define BROWN                "\e[0;33m"
#define YELLOW               "\e[1;33m"
#define BLUE                 "\e[0;34m"
#define L_BLUE               "\e[1;34m"
#define PURPLE               "\e[0;35m"
#define L_PURPLE             "\e[1;35m"
#define CYAN                 "\e[0;36m"
#define L_CYAN               "\e[1;36m"
#define GRAY                 "\e[0;37m"
#define WHITE                "\e[1;37m"

#define BOLD                 "\e[1m"
#define UNDERLINE            "\e[4m"
#define BLINK                "\e[5m"
#define REVERSE              "\e[7m"
#define HIDE                 "\e[8m"
#define CLEAR                "\e[2J"
#define CLRLINE              "\r\e[K" //or "\e[1K\r"

extern int token;              //单词编码
extern char *filename;     //文件名
extern int line_num;       //行数
extern char ch;        //当前取到的源码字符
extern int tkvalue;
extern FILE *fin;

/*语法状态枚举值*/
enum e_SynTaxState
{
    SNTX_NUL,  //空状态，没有语法缩进动作
    SNTX_SP,   //空格
    SNTX_LF_HT, //换行并缩进，每一个声明，函数定义，语句结束都要设置此状态
    SNTX_DELAY  //延迟到取出下一单词后确定输出格式
};

extern int syntax_state;
extern int syntax_level;

/*词法状态枚举定义*/
//extern enum e_LexState;

/*
 * 错误层级，这里只定义了warning和error两种
 */
//extern enum e_ErrorLevel;

/*
 * 发生错误的阶段，分为编译阶段和链接阶段
 */
//extern enum e_WorkStage;

#endif //SC_COMPLIER_GLOBAL_VAR_H
