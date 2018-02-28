//
// Created by yk on 17-12-31.
//

#ifndef YCC_BUFFER_H
#define YCC_BUFFER_H

#include <zconf.h>
#include <string>
#include <memory>
#include <string>
#include <sstream>
namespace Utils{
    //把字符数组转化为字符串，主要注意是转义字符的转换
    std::string parse_cstring(const char *p);
    //解析指定长度的字符数组为字符串，主要注意是转义字符的转换
    std::string quote_cstring_len(const char *p,int);
    //解析单个字符为字符串,主要是‘\’情况的处理，因为需要保持format打印后的字符次保持
    std::string quote_char(char);
    //c++风格的printf的实现
    std::string format(char* fmt);
    template <typename T,typename... Args>
    std::string format(char *fmt, const T& value,Args... args);
    template <typename T,typename... Args>
    std::string format(char *fmt, const T& value,Args... args)
    {
        std::string src;
        while (*fmt)
        {
            if (*fmt=='%'&&*++fmt!='%')
            {
                std::stringstream ss;
                ss<<value;
                src+=ss.str();
                src+=format(++fmt,args...);
                return src;
            }
            src+=*fmt++;
        }
        throw std::runtime_error("extra params provided to vformat");
    };
    //按照printf的格式打印字符串，打印的字符串保存在第一个参数src中
    void buf_printf(std::string &src, char *fmt);
    template <typename T,typename... Args>
    void buf_printf(std::string& src,char* fmt,T value,Args... args)
    {
        while (*fmt)
        {
            if (*fmt=='%'&&*++fmt!='%')
            {
                std::stringstream ss;
                ss<<value;
                src+=ss.str();
                buf_printf(src,++fmt,args...);
                return;
            }
            src+=*fmt++;
        }
        throw std::runtime_error("extra arguments provided to buf_printf");
    };
    //转义字符转化为字符串格式
    std::string quote(char c);
    void print(std::string& src,char c);
}
#endif //YCC_BUFFER_H
