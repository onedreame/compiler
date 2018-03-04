//
// Created by yk on 18-2-18.
//

#ifndef YCC_FUNCTEST_H
#define YCC_FUNCTEST_H

#include <string>

namespace Test{
    void lexTokenTest(std::string);
    void newLineTest();
    void macroExpandTest(const std::string&,int);
    void NodeTest();
}
#endif //YCC_FUNCTEST_H
