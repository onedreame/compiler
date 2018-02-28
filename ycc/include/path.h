//
// Created by yk on 18-2-6.
//

#ifndef YCC_PATH_H
#define YCC_PATH_H

#include <string>

namespace Path
{
    std::string clean(const std::string& path);
    std::string fullpath(const std::string& path);
}
#endif //YCC_PATH_H
