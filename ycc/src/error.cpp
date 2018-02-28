//
// Created by yk on 17-12-30.
//

#include <iostream>
#include <memory>
#include "../include/error.h"
#include "../include/utils.h"
#include "../include/buffer.h"

namespace Error{
    void Print_error(const char *s)
    {
        if (s== nullptr) return;
        while(*s)
        {
            if (*s=='%'&&*++s!='%') throw std::runtime_error("invalid format string:missing arguments");
            std::cerr<<*s++;
        }
    }

    std::string token_pos(const DataStruct::Token& tok)
    {
        std::shared_ptr<DataStruct::File> f=tok.file;
        if (!f)
            return std::string("(unknown)");
        std::string filename=f->name.size()?f->name: std::move("(unknown)");
        return Utils::format("%s:%d:%d",filename,tok.line,tok.column);
    }
    bool enable_warning= true;
    bool warning_is_error= false;
}