//
// Created by yk on 18-2-6.
//

#include <iostream>
#include <cstring>
#include <zconf.h>
#include "../include/path.h"
#include "../include/buffer.h"
#include "../include/error.h"

namespace Path
{
    std::string clean(const std::string& path)
    {
        std::string newpath;
        if (!path.empty()&&path[0]=='/')
            newpath+='/';
        auto len=path.size();
        for (decltype(path.size()) i = 0; i < len; ++i) {
            if (path[i]=='/')
                continue;
            if (path[i]=='.'&&i+1<len&&path[i+1]=='/')
            {
                ++i;
                continue;
            }
            if (path[i]=='.'&&i+2<len&&path[i+1]=='.'&&path[i+2]=='/')
            {
                if (newpath.empty()||newpath.size()==1&&newpath[0]=='/') {
                    i += 2;
                    continue;
                }
                newpath.pop_back();
                auto pos=newpath.size()-1;
                while (pos>=0)
                {
                    if (newpath[pos]!='/')
                    {
                        newpath.pop_back();
                        --pos;
                    }   else
                        break;
                }
                continue;
            }
            while (i<len&&path[i]!='/') newpath+=path[i++];
            if (i<len&&path[i]=='/') {
                newpath+='/';
                continue;
            }
            return newpath;
        }
    }
    //获取绝对路径
    std::string fullpath(const std::string& path)
    {
        static char cwd[PATH_MAX];
        if (!path.empty()&&path[0]=='/')
            return clean(path);
        if (cwd[0]=='\0'&&!getcwd(cwd,PATH_MAX))
            Error::errorf(__FILE__":"STR(__LINE__),"","Can not get current work dictory: %s",strerror(errno));
        return clean(std::string(cwd)+"/"+path);
    }

}