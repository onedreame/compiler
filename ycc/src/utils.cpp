//
// Created by yk on 18-2-13.
//

#include <string>
#include <zconf.h>
#include <iostream>
#include <libgen.h>
#include <vector>
#include "../include/buffer.h"
#include "../include/error.h"
#include "../include/Lex.h"

namespace Utils{
    std::string infile;
    std::string outfile;
    std::string asmfile;
    std::string BUILD_DIR=".";
    bool dumpast;
    bool cpponly;
    bool dumpasm;
    bool dontlink;
    extern bool dumpstack;
    extern bool dumpsource;
    std::vector<std::string> cppdefs;  //保存D参数定义的宏
    std::vector<std::string> tmpfiles;
    void delete_temp_files() {
        for (const auto &e:tmpfiles)
            unlink(e.c_str());
    }

    void usage(int exitcode) {
        std::ostream &os=exitcode?std::cerr:std::cout;
        os<<"Usage: ycc [ -E ][ -a ] [ -h ] <file>\n\n"
                "\n"
                "  -I<path>          add to include path\n"
                "  -E                print preprocessed source code\n"
                "  -D name           Predefine name as a macro\n"
                "  -D name=def\n"
                "  -S                Stop before assembly (default)\n"
                "  -c                Do not run linker (default)\n"
                "  -U name           Undefine name\n"
                "  -fdump-ast        print AST\n"
                "  -fdump-stack      Print stacktrace\n"
                "  -fno-dump-source  Do not emit source code as assembly comment\n"
                "  -o filename       Output to the specified file\n"
                "  -g                Do nothing at this moment\n"
                "  -Wall             Enable all warnings\n"
                "  -Werror           Make all warnings into errors\n"
                "  -O<number>        Does nothing at this moment\n"
                "  -m64              Output 64-bit code (default)\n"
                "  -w                Disable all warnings\n"
                "  -h            no    print this help\n"
                "\n"
                "One of -a, -c, -E or -S must be specified.\n\n";
        exit(exitcode);
    }

    std::string base(const std::string& path)  {
        return basename(const_cast<char *>(path.c_str()));
    }

    std::string replace_suffix(std::string r, char suffix) {
        if (r.back() != 'c')
            Error::error("filename suffix is not .c");
        *(--r.end()) = suffix;
        return r;
    }

    std::ostream& open_asmfile() {
        if (dumpasm) {
            asmfile = !outfile.empty() ? outfile : replace_suffix(base(infile), 's');
        } else {
            asmfile = std::string("/tmp/8ccXXXXXX.s");
            if (!mkstemps(const_cast<char *>(asmfile.c_str()), 2))
                perror("mkstemps");
            tmpfiles.push_back(asmfile);
        }
        if (asmfile== "-")
            return std::cout;
        static  std::ofstream fp(asmfile);
        if (fp.is_open())
            fp.close();
        fp.open(asmfile);
        if (!fp)
            perror("fopen");
        return fp;
    }

    void parse_warnings_arg(const std::string& s) {
        if (s == "error")
            Error::warning_is_error = true;
        else if (s != "all")
            Error::error("unknown -W option: %s", s);
    }

    void parse_f_arg(const std::string& s) {
        if (s == "dump-ast")
            dumpast = true;
//        else if (s == "dump-stack")
//            dumpstack = true;
//        else if (s == "no-dump-source")
//            dumpsource = false;
        else
            usage(1);
    }

    void parse_m_arg(const std::string& s) {
        if (s != "64")
            Error::error("Only 64 is allowed for -m, but got %s", s);
    }

/*
 * 解析参数，就是usage中的参数
 */
    void parseopt(int argc, char **argv) {
        for (;;) {
            int opt = getopt(argc, argv, "I:ED:O:SU:W:acd:f:gm:o:hw");
            //解析字符串，按照后面的string解析，如果正确解析返回正值，否则返回-1
            if (opt == -1)
                break;
            switch (opt) {
                //optarg是getopt内的char*，可能是解析到的参数
//                case 'I': add_include_path(optarg); break;
                    //把参数加入vector中
                case 'E': cpponly = true; break;
                case 'D': {
//                    char *p = strchr(optarg, '='); //查找=在optarg中首次出现的位置
//                    if (p)
//                        *p = ' ';
//                    buf_printf(cppdefs, "#define %s\n", optarg);//
                    break;
                }
                case 'O': break;
                case 'S': dumpasm = true; break;
                case 'U':
//                    buf_printf(cppdefs, "#undef %s\n", optarg);
                    break;
                case 'W': parse_warnings_arg(optarg); break;
                case 'c': dontlink = true; break;
                case 'f': parse_f_arg(optarg); break;
                case 'm': parse_m_arg(optarg); break;
                case 'g': break;
                case 'o': outfile = optarg; break;
                case 'w': Error::enable_warning = false; break;
                case 'h':
                    usage(0);
                default:
                    usage(1);
            }
        }
        if (optind != argc - 1)
            usage(1);

        if (!dumpast && !cpponly && !dumpasm && !dontlink)
            Error::error("One of -a, -c, -E or -S must be specified");
        infile = argv[optind];
    }

    std::string get_base_file() {
        return infile;
    }

    void preprocess() {
//        for (;;) {
//            Token *tok = read_token();
//            if (tok->kind == TEOF)
//                break;
//            if (tok->bol)
//                printf("\n");
//            if (tok->space)
//                printf(" ");
//            printf("%s", tok2s(tok));
//        }
//        printf("\n");
//        exit(0);
    }

    std::string encoding_prefix(DataStruct::ENCODE enc) {
        switch (enc) {
            case DataStruct::ENCODE::ENC_CHAR16: return "u";
            case DataStruct::ENCODE::ENC_CHAR32: return "U";
            case DataStruct::ENCODE::ENC_UTF8:   return "u8";
            case DataStruct::ENCODE::ENC_WCHAR:  return "L";
        }
        return "";
    }

    std::string tok2s(const DataStruct::Token& tok)
    {
        if (tok.kind==DataStruct::TOKEN_TYPE::TPLACEHOLDER)
            return "(null)";
        switch (tok.kind) {
            case DataStruct::TOKEN_TYPE::TIDENT:
                return *(tok.sval);
            case DataStruct::TOKEN_TYPE::TKEYWORD:
                switch (tok.id) {
#define op(id, str)         case DataStruct::AST_TYPE::id: return str;
#define keyword(id, str, _) case DataStruct::AST_TYPE::id: return str;
#include "../include/keyword.inc"
#undef keyword
#undef op
                    default: return Lex::get_keywords_string(tok.id);
                }
            case DataStruct::TOKEN_TYPE::TCHAR:
                return Utils::format("%s'%s'",
                              encoding_prefix(tok.enc),
                              quote_char(tok.c));
            case DataStruct::TOKEN_TYPE::TNUMBER:
                return *(tok.sval);
            case DataStruct::TOKEN_TYPE::TSTRING:
                return format("%s\"%s\"",
                              encoding_prefix(tok.enc),
                              Utils::parse_cstring((*(tok.sval)).c_str()));
            case DataStruct::TOKEN_TYPE::TEOF:
                return "(eof)";
            case DataStruct::TOKEN_TYPE::TINVALID:
                return format("%c", tok.c);
            case DataStruct::TOKEN_TYPE::TNEWLINE:
                return "(newline)";
            case DataStruct::TOKEN_TYPE::TSPACE:
                return "(space)";
            case DataStruct::TOKEN_TYPE::TMACRO_PARAM:
                return "(macro-param)";
        }
        Error::error("internal error: unknown token kind: %d", static_cast<int>(tok.kind));
    }
}