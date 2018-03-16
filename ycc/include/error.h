//
// Created by yk on 17-12-30.
//

#ifndef YCC_ERROR_H
#define YCC_ERROR_H

#include <string>
#include <iostream>
#include "Token.h"
#include "colors.h"
#define STR2(X) #X
#define STR(X) STR2(X)
#define error(...)       errorf(__FILE__ ":" STR(__LINE__),"", __VA_ARGS__)
#define errort(tok, ...) errorf(__FILE__ ":" STR(__LINE__), Error::token_pos(tok), __VA_ARGS__)
#define warn(...)        warnf(__FILE__  ":" STR(__LINE__),"" , __VA_ARGS__)
#define warnt(tok, ...)  warnf(__FILE__ ":" STR(__LINE__), Error::token_pos(tok), __VA_ARGS__)
#define errorp(p, ...) errorf(__FILE__ ":" STR(__LINE__), p, __VA_ARGS__)
#define warnp(p, ...)  warnf(__FILE__ ":" STR(__LINE__), p, __VA_ARGS__)

namespace Error{
    extern bool enable_warning;
    extern bool warning_is_error;
    void Print_error(const char *s);

//    打印错误信息，一般格式为：
//    line：pos（这连个均是参数）
//    自定义格式信息
//    换行符
    template <typename T,typename... Args>
    void Print_error(const char *s,T value,Args... args)
    {
        while (*s)
        {
            if (*s=='%'&&*++s!='%')
            {
                switch (*s)
                {
                    case 'x':
                    case 'X':
                        std::cerr<<std::hex<<value;
                        break;
                    case 'o':
                    case 'O':
                        std::cerr<<std::oct<<value;
                        break;
                    default:
                        std::cerr<<value;
                        break;
                }
                Print_error(++s,args...);
                return;
            }
            std::cerr<<*s++;
        }
        throw std::runtime_error("extra arguments provided to Print_error");
    }

    template <typename T,typename U,typename... Args>
    void errorf(T&& line,U&& pos,char*fmt, Args... args)
    {
        std::cerr<<KRED<<"ERROR"<<RST<<std::endl;
        Print_error("%s: %s\n",std::forward<T>(line),std::forward<U>(pos));
        Print_error(fmt,args...);
        std::cerr<<std::endl;
        quick_exit(-1);
    }

    template <typename T,typename U,typename... Args>
    void warnf(T line, U pos, char* fmt,Args... args)
    {
        if (!enable_warning)
            return;
        std::string label=warning_is_error?"ERROR":"WARN";
        std::cerr<<KRED<<label<<RST<<std::endl;
        Print_error("%s: %s\n",line,pos);
        Print_error(fmt,args...);
        if (warning_is_error)
            quick_exit(1);
    }
//    template <class T,class...Args>
//    void errorp(const T& src,Args... args)
//    {
//        errorf(__FILE__ ":" STR(__LINE__),std::forward(src),args...);
//    };
//可变模板参数推断出来的类型均是引用，因而如果转发的话并不行
//    template <class T,class... Args>
//    void warnp( T&& src,Args... args)
//    {
//        warnf(__FILE__ ":" STR(__LINE__),src,args...);
//    };
    std::string token_pos(const DataStruct::Token& );
}
#endif //YCC_ERROR_H
