//
// Created by yk on 17-12-31.
//

#include <cstring>
#include <iomanip>
#include "../include/buffer.h"
namespace Utils{
    std::string parse_cstring(const char* p)
    {
        std::string src;
        while (*p!='\0')
            print(src,*p++);
        return src;
    }

    std::string quote_cstring_len(const char *p,int len)
    {
        std::string src;
        for (int i=0;i<len;++i)
            print(src,p[i]);
        return src;
    }

    std::string quoto_char(char c)
    {
        if(c=='\\') return std::string("\\\\");
        if (c=='\'') return std::string("\\\'");
        return std::string()+c;

    }

    std::string quote(char c)
    {
        switch (c){
            case '"':
                return std::string("\\\"");
            case '\\':
                return std::string("\\\\");
            case '\b':
                return std::string("\\b");
            case '\f':
                return std::string("\\f");
            case '\n':
                return std::string("\\n");
            case '\r':
                return std::string("\\r");
            case '\t':
                return std::string("\\r");
        }
        return "";
    }

    void print(std::string& src,char c)
    {
        std::string q=quote(c);
        if (q!= ""){
            src+=q;
        }else if(isprint(c))
            src+=c;
        else
        {
            src+="\\x";
            std::stringstream ss;
            ss<<std::hex<<std::setw(2)<<std::setfill('0')<<c;
            src+=ss.str();
//        buf_printf("\\x%02x",c);
        }
    }

    void buf_printf(std::string &src, char *fmt)
    {
        while (*fmt!='\0')
            src+=*fmt++;
    }

    std::string format(char* fmt)
    {
        std::string src;
        while (*fmt!='\0')
            src+=*fmt++;
        return src;
    }
}