//
// Created by yk on 17-7-24.
//

#ifndef SC_COMPLIER_EXCEPTION_HANDLER_H
#define SC_COMPLIER_EXCEPTION_HANDLER_H

#include <stdio.h>

/*
 * 异常处理函数
 *
 */
extern void handle_exception(int stage,int level,char *fmt,va_list ap);

/*
 * 编译警告错误处理
 */
extern void warning(char *fmt,...);
/*
 * 错误消息处理
 */
extern void error(char *msg,...);

/*
 * 提示错误，此处缺少某个语法成分
 * msg：需要什么语法成分
 */
extern void expect(char *msg);

/*
 * 跳过单词c，取下一单词，如果当前单词不是c，提示错误
 * c：要跳过的单词
 */
extern void skip(int c);

/*
 * 取得单词v所代表的源码字符串
 * v：单词编号
 */
extern char * get_tkstr(int v);

/*
 * 链接错误处理
 */
extern void link_error(char *fmt,...);
#endif //SC_COMPLIER_EXCEPTION_HANDLER_H
