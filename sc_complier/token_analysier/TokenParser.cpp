//
// Created by yk on 17-7-26.
//

#include <assert.h>
#include "TKTable.h"
#include "DynString.h"
#include "Exception_Handler.h"
#include "../syntax_analysier/syntax_analysier.h"

/*
 * 从源文件读取一个字符
 */
char getch()
{
    ch=getc(fin);
}

/*
 * 跳过空白字符，忽略空格，Tab和回车
 */
void skip_white_space()
{
    while (ch==' '||ch=='\t'||ch=='\r'||ch=='\n')
    {
//        if (ch=='\r')
//        {
//            getch();
//            if (ch!='\n')
//                return;
//            line_num++;
//        }
        if (ch=='\n')
            line_num++;
        printf("%c",ch);
        getch();
    }
}

/*
 * 解析注释，一行一行的处理
 */
void parse_comment()
{
    getch();
    do{
        do{
            if (ch=='*'||ch=='\n'||ch==EOF)
                break;
            else getch();
        }while (1);
        if (ch=='\n')
            line_num++,getch();
        else if (ch=='*')
        {
            getch();
            if (ch=='/')
            {
                getch();
                return;
            } else{
                error("未发现/：%d",line_num);
            }
        } else{
            error("一直到文件末位都未看到配对的注释结束符");
            return;
        }
    }while (1);
}

/*
 * 预处理，忽略空白字符及注释
 * 只有空白和注释需要特殊处理
 */
void preprocess()
{
    while(1)
    {
        if (ch==' '||ch=='\t'||ch=='\r'||ch=='\n')
            skip_white_space();
        else if (ch=='/')
        {
            getch();
            if(ch=='*')
            {
                parse_comment();
            } else{
                ungetc(ch,fin);
                ch='/';
                break;
            }
        } else break;
    }
}

/*
 * 判断c是否为字母（a-z，A-Z）或下划线（_)
 */
bool is_nodigit(char c)
{
    return c>='a'&&c<='z'||c>='A'&&c<='Z'||c=='_';
}

/*
 * 判断c是否为数字
 */
bool is_digit(char c)
{
    return c>='0'&&c<='9';
}

/*
 * 解析标识符
 */
void parse_identifier()
{
    dynstring_reset(&tkstr);
    dynstring_chcat(&tkstr,ch);
    getch();
    while (is_nodigit(ch)||is_digit(ch))
    {
        dynstring_chcat(&tkstr,ch);
        getch();
    }
    dynstring_chcat(&tkstr,'\0');
}

/*
 * 解析整型常量,这里通过两个字符数组，一个用字符数组形式保存，一个用来
 * 处理成相应的变量
 */
void parse_num()
{
    dynstring_reset(&tkstr);
    dynstring_reset(&sourcestr);
    do{
        dynstring_chcat(&tkstr,ch);
        dynstring_chcat(&sourcestr,ch);
        getch();
    }while (is_digit(ch));
    if(ch=='.')
    {
        do{
            dynstring_chcat(&tkstr,ch);
            dynstring_chcat(&sourcestr,ch);
            getch();
        }while (is_digit(ch));
    }
    dynstring_chcat(&tkstr,'\0');
    dynstring_chcat(&sourcestr,'\0');
    tkvalue=atoi(tkstr.data);
}

/*
 * 解析字符串常量，字符常量
 * sep:  字符常量界符标识为单引号（’）
 *       字符串常量界符标识为双引号(")
 *  这里是个不错的处理，不用判断‘或“，只要判断结尾是否是sep就行
 */
void parse_string(char sep)
{
    dynstring_reset(&tkstr);
    dynstring_reset(&sourcestr);
    dynstring_chcat(&sourcestr,ch);
    getch();
    char c;
    while (ch!=sep)
    {
        if (ch=='\\')     //处理转义字符
        {
            dynstring_chcat(&sourcestr,ch);
            getch();
            switch (ch)
            {
                case '0':
                    c='\0';
                    break;
                case 'a':
                    c='\a';
                    break;
                case 'b':
                    c='\b';
                    break;
                case 't':
                    c='\t';
                    break;
                case 'n':
                    c='\n';
                    break;
                case 'v':
                    c='\v';
                    break;
                case 'f':
                    c='\f';
                    break;
                case 'r':
                    c='\r';
                    break;
                case '\"':
                    c='\"';
                    break;
                case '\'':
                    c='\'';
                    break;
                case '\\':
                    c='\\';
                    break;
                default:
                    c=ch;
                    if (c>='!'&&c<'-')
                        warning("非法转义字符:\'\\%c\'",c);
                    else
                        warning("非法转义字符:\'\\0x%x\'",c);
                    break;
            }
            dynstring_chcat(&tkstr,c);
            dynstring_chcat(&sourcestr,ch);
            getch();
        } else{
            dynstring_chcat(&tkstr,ch);
            dynstring_chcat(&sourcestr,ch);
            getch();
        }

    }
    dynstring_chcat(&tkstr,'\0');
    dynstring_chcat(&sourcestr,sep);
    dynstring_chcat(&sourcestr,'\0');
    getch();
}

/*
 * 读取源文件，生成token
 */
extern Symbol *mt;
void get_token()
{
    int ori=line_num;
    int ori_t=token;
//    assert(!mt||mt->c==-1);
    preprocess();
    switch (ch)
    {
        case 'a':case 'b':case 'c':case 'd':case 'e':case 'f':case 'g':
        case 'h':case 'i':case 'j':case 'k':case 'l':case 'm':case 'n':
        case 'o':case 'p':case 'q':case 'r':case 's':case 't':
        case 'u':case 'v':case 'w':case 'x':case 'y':case 'z':
        case 'A':case 'B':case 'C':case 'D':case 'E':case 'F':case 'G':
        case 'H':case 'I':case 'J':case 'K':case 'L':case 'M':case 'N':
        case 'O':case 'P':case 'Q':case 'R':case 'S':case 'T':
        case 'U':case 'V':case 'W':case 'X':case 'Y':case 'Z':
        case '_':
        {
            TkWord *tp;
            parse_identifier();
            tp=tkword_insert(tkstr.data);
            token=tp->tkcode;
            break;
        }
        case '0':case '1':case '2':case '3':
        case '4':case '5':case '6':case '7':
        case '8':case '9':
            parse_num();token=TK_CINT;
            break;
        case '+':
            getch();
            token=TK_PLUS;
            break;
        case '-':
            token=TK_MINUS;
            getch();
            if (ch=='>')
            {
                token=TK_POINTSTO;
                getch();
            }
            break;
        case '/':
            token=TK_DIVIDE;
            getch();
            break;
        case '%':
            token=TK_MOD;
            getch();
            break;
        case '=':
            token=TK_ASSIGN;
            getch();
            if (ch=='=')
            {
                token=TK_EQ;
                getch();
            }
            break;
        case '!':
            getch();
            if (ch=='=')
            {
                token=TK_NEQ;
                getch();
            } else{
                error("暂不支持'!'(非操作符)");
            }
            break;
        case '<':
            token=TK_LT;
            getch();
            if (ch=='=')
            {
                token=TK_LEQ;
                getch();
            }
            break;
        case '>':
            token=TK_GT;
            getch();
            if (ch=='=')
            {
                token=TK_GEQ;
                getch();
            }
            break;
        case '.':
            getch();
            if (ch=='.')
            {
                getch();
                if (ch=='.')
                {
                    token=TK_ELLIPSIS;
                    getch();
                } else error("省略号拼写错误");
            } else{
                token=TK_DOT;
            }
            break;
        case '&':
            token=TK_AND;
            getch();
            break;
        case ';':
            token=TK_SEMICOLON;
            getch();
            break;
        case ']':
            token=TK_CLOSEBR;
            getch();
            break;
        case '[':
            token=TK_OPENBR;
            getch();
            break;
        case '}':
            token=TK_END;
            getch();
            break;
        case '{':
            token=TK_BEGIN;
            getch();
            break;
        case '(':
            token=TK_OPENPA;
            getch();
            break;
        case ')':
            token=TK_CLOSEPA;
            getch();
            break;
        case ',':
            token=TK_COMMA;
            getch();
            break;
        case '*':
            token=TK_STAR;
            getch();
            break;
        case '\'':
            parse_string(ch);
            token=TK_CCHAR;
            tkvalue=*(char*)tkstr.data;
            break;
        case '\"':
            parse_string(ch);
            token=TK_CSTR;
            break;
        case EOF:
            token=TK_EOF;
            break;
        default:
            error("不认识的字符：\\x%02x",ch);
            getch();
            break;
    }
//    if(ori!=line_num&&syntax_state!=SNTX_LF_HT&&ori_t==TK_BEGIN)
//    if(ori_t==TK_BEGIN&&syntax_state!=SNTX_LF_HT&&line_num==ori||
//            token==TK_END&&ori==line_num&&syntax_state!=SNTX_LF_HT)
//        printf("\n");
//    if(syntax_state!=SNTX_LF_HT&&ori!=line_num)
//    {
//        if(token==TK_END)
//            print_tab(syntax_level?syntax_level-1:0);
//        else print_tab(syntax_level);
//    }
    if(syntax_state!=SNTX_LF_HT)
    {
        //如果当前为{或},且与前一个词在同一行，那么需要两次换行
        if(token==TK_END&&ori==line_num) {
            printf("\n");
        }
        //如果换行了，或者为换行但是前一个单词为{,那么也需要
        if(ori!=line_num||token==TK_END)
        {
            if(token==TK_END) {
                print_tab(syntax_level?syntax_level-1:0);
            }
            else print_tab(syntax_level);
        }
    }
    syntax_indent();
}

/*
 * 词法着色
 */
void color_token(int lex_state)
{
    char *p;
    switch (lex_state)
    {
        case LEX_NORMAL:
            p=get_tkstr(token);
            if (token>=TK_IDENT)  /*标识符为灰色*/
            {
                printf(GRAY "%s" NONE,p);
            }
            else if (token>=KW_CHAR)    //关键字为绿色
            {
                printf(GREEN "%s" NONE,p);
            }
            else if (token>=TK_CINT) //常量为黄色
            {
                printf(YELLOW "%s" NONE,p);
            }
            else    //运算符和分隔符为红色
            {
                printf(RED "%s" NONE,p);
            }
            break;
        case LEX_SEP:
            printf("%c",ch);
            break;
    }
}