//
// Created by yk on 17-7-26.
//

#include <cstdio>
#include <cstdarg>
#include "global_var.h"
#include "TokenCode.h"
#include "DynString.h"
#include "TokenParser.h"
#include "DynArray.h"
#include "TkWordDef.h"

/*
 * 异常处理函数
 *
 */
void handle_exception(int stage,int level,char *fmt,va_list ap)
{
    char buf[1024];
    vsprintf(buf,fmt,ap);
    if (stage==STAGE_COMPILE)
    {
        if (level==LEVEL_WARNING)
        {
            printf("\n%s(第%d行):编译警告:%s:\n",filename,line_num,buf);
        }
        else{
            printf("\n%s(第%d行):编译错误:%s:\n",filename,line_num,buf);
            exit(-1);
        }
    }
    else{
        printf("\n链接错误：%s!\n",buf);
        exit(-1);
    }
}

/*
 * 编译警告错误处理
 */
void warning(char *fmt,...)
{
    va_list ap;
    va_start(ap,fmt);
    handle_exception(STAGE_COMPILE,LEVEL_WARNING,fmt,ap);
    va_end(ap);
}
/*
 * 错误消息处理
 */
void error(char *msg,...)
{
    va_list ap;
    va_start(ap,msg);
    handle_exception(STAGE_COMPILE,LEVEL_ERROR,msg,ap);
    va_end(ap);
}

/*
 * 提示错误，此处缺少某个语法成分
 * msg：需要什么语法成分
 */
void expect(char *msg)
{
    error("缺少%s",msg);
}

/*
 * 取得单词v所代表的源码字符串
 * 如果是常量则返回常量字符，如果是关键字则返回关键字的拼写
 * v：单词编号
 */
char * get_tkstr(int v)
{
    if(v>tktable.count)
        return NULL;
    if (v>=TK_CINT&&v<=TK_CSTR)
        return sourcestr.data;
    else return ((TkWord*)tktable.data[v])->spelling;
}

/*
 * 跳过单词c，取下一单词，如果当前单词不是c，提示错误
 * c：要跳过的单词
 */
void skip(int c)
{
    if(token!=c)
        error("缺少'%s'",get_tkstr(c));
    get_token();
}

/*
 * 链接错误处理
 */
void link_error(char *fmt,...)
{
    va_list ap;
    va_start(ap,fmt);
    handle_exception(STAGE_LINK,LEVEL_ERROR,fmt,ap);
    va_end(ap);
}