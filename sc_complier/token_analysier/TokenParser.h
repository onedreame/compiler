//
// Created by yk on 17-7-25.
//

#ifndef SC_COMPLIER_TOKENPARSER_H
#define SC_COMPLIER_TOKENPARSER_H
/*
 * 从源文件读取一个字符
 */
extern char getch();

/*
 * 跳过空白字符，忽略空格，Tab和回车
 */
extern void skip_white_space();

/*
 * 解析注释，一行一行的处理
 */
extern void parse_comment();

/*
 * 预处理，忽略空白字符及注释
 * 只有空白和注释需要特殊处理
 */
extern void preprocess();

/*
 * 判断c是否为字母（a-z，A-Z）或下划线（_)
 */
extern bool is_nodigit(char c);

/*
 * 判断c是否为数字
 */
extern bool is_digit(char c);

/*
 * 解析标识符
 */
extern void parse_identifier();

/*
 * 解析整型常量,这里通过两个字符数组，一个用字符数组形式保存，一个用来
 * 处理成相应的变量
 */
extern void parse_num();

/*
 * 解析字符串常量，字符常量
 * sep:  字符常量界符标识为单引号（’）
 *       字符串常量界符标识为双引号(")
 *  这里是个不错的处理，不用判断‘或“，只要判断结尾是否是sep就行
 */
extern void parse_string(char sep);

/*
 * 读取源文件，生成token
 */
extern void get_token();

/*
 * 词法着色
 */
extern void color_token(int lex_state);
#endif //SC_COMPLIER_TOKENPARSER_H
