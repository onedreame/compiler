//
// Created by yk on 18-2-6.
//

#ifndef YCC_UTILS_H
#define YCC_UTILS_H

#include <cstdlib>
#include <ostream>
#include <iostream>
#include <memory>
#include <vector>
#include "Token.h"
class Parser;
class CodeGen;
namespace Utils
{
    void usage(int exitcode) ;
    void delete_temp_files();
    std::string get_base_file();
    void preprocess();
    std::string base(const std::string& );
    //根据是否设置输出文件来决定汇编文件是生成输出文件还是临时文件
    std::string replace_suffix(std::string , char );
    std::ostream& open_asmfile();
    void parse_warnings_arg(const std::string& );
    void parse_f_arg(const std::string& );
    void parse_m_arg(const std::string& );
    void parseopt(int argc, char **argv);
    void preprocess();

    void ycc_setup_and_work(int argc, char**argv);
}
#endif //YCC_UTILS_H
