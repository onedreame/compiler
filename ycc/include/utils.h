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

namespace Utils
{
    void usage(int exitcode) ;
    void delete_temp_files();
    std::string get_base_file();
    void preprocess();
    std::string base(const std::string& );
    //根据是否设置输出文件来决定汇编文件是生成输出文件还是临时文件
    std::ostream& open_asmfile();
    std::string replace_suffix(std::string , char );
    void parse_warnings_arg(const std::string& );
    void parse_f_arg(const std::string& );
    void parse_m_arg(const std::string& );
    void parseopt(int argc, char **argv);
    void preprocess();

    extern std::string infile;
    extern std::string outfile;
    extern std::string asmfile;
    extern bool dumpast;
    extern bool cpponly;
    extern bool dumpasm;
    extern bool dontlink;
    extern std::vector<std::string> cppdefs;  //保存D参数定义的宏
    extern std::vector<std::string> tmpfiles;
    extern bool dumpstack;
    extern bool dump;
    extern std::string BUILD_DIR;
    std::string ty2s(const std::shared_ptr<DataStruct::Type>&);
    std::string tok2s(const DataStruct::Token& tok);
    std::string encoding_prefix(DataStruct::ENCODE enc);
}
#endif //YCC_UTILS_H
