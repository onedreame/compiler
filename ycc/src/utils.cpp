//
// Created by yk on 18-2-13.
//

#include <string>
#include <zconf.h>
#include <iostream>
#include <libgen.h>
#include <vector>
#include <algorithm>
#include <wait.h>
#include "../include/buffer.h"
#include "../include/error.h"
#include "../include/utils.h"
#include "../include/code_gen.h"

namespace Utils{
    std::string infile;
    std::string outfile;
    std::string asmfile;
    std::string BUILD_DIR="..";
    bool dumpast;
    bool cpponly;
    bool dumpasm;
    bool dontlink;
    bool dumpstack= false;
    bool dumpsource= true;
    std::vector<std::string> cppdefs;  //保存D参数定义的宏
    std::vector<std::string> tmpfiles;
    extern std::shared_ptr<Lex> _lex;
    extern std::shared_ptr<Parser> _parser;
    extern std::shared_ptr<MacroPreprocessor> _cpp;
    extern std::shared_ptr<CodeGen> _codegen;
    std::ofstream fp;

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
        quick_exit(exitcode);
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

    void parse_warnings_arg(const std::string& s) {
        if (s == "error")
            Error::warning_is_error = true;
        else if (s != "all")
            Error::error("unknown -W option: %s", s);
    }

    void parse_f_arg(const std::string& s) {
        if (s == "dump-ast")
            dumpast = true;
        else if (s == "dump-stack")
            dumpstack = true;
        else if (s == "no-dump-source")
            dumpsource = false;
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
        _lex=Lex::Instance();
        _cpp=MacroPreprocessor::Instance();
        _parser=Parser::Instance();
        _codegen=CodeGen::Instance();
        for (;;) {
            int opt = getopt(argc, argv, "I:ED:O:SU:W:acd:f:gm:o:hw");
            //解析字符串，按照后面的string解析，如果正确解析返回正值，否则返回-1
            if (opt == -1)
                break;
            switch (opt) {
                //optarg是getopt内的char*，可能是解析到的参数
                case 'I': _cpp->add_include_path(optarg); break;
                    //把参数加入vector中
                case 'E': cpponly = true; break;
                case 'D': {
                    char *p = strchr(optarg, '='); //查找=在optarg中首次出现的位置
                    if (p)
                        *p = ' ';
                    cppdefs.emplace_back(format("#define %s", std::string(optarg)));//
                    break;
                }
                case 'O': break;
                case 'S': dumpasm = true; break;
                case 'U':
                    cppdefs.emplace_back(format("#undef %s", std::string(optarg)));
                    break;
                case 'W': parse_warnings_arg(std::string(optarg)); break;
                case 'c': dontlink = true; break;
                case 'f': parse_f_arg(std::string(optarg)); break;
                case 'm': parse_m_arg(std::string(optarg)); break;
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
    std::ostream& open_asmfile() {
        if (dumpasm) {
            asmfile = !outfile.empty() ? outfile : replace_suffix(base(infile), 's');
        } else {
            char temp[256];
            asmfile = "/tmp/8ccXXXXXX.s";
            auto length=asmfile.copy(temp,asmfile.size(),0);
            temp[length]='\0';
            if (!mkstemps(temp, 2))
                perror("mkstemps");
            tmpfiles.push_back(asmfile);
        }
        if (asmfile == "-")
            return std::cout;
        if (fp.is_open())
            fp.close();
        fp.open(asmfile);
        if (!fp)
            perror("ofstream");
        return fp;
    }

    std::string get_base_file() {
        return infile;
    }

    void preprocess() {
        for (;;) {
            auto tok = _cpp->read_token();
            if (tok.kind == DataStruct::TOKEN_TYPE::TEOF)
                break;
            if (tok.bol)
                std::cout<<std::endl;
            if (tok.space)
                std::cout<<" ";
            std::cout<<_lex->tok2s(tok);
        }
        std::cout<<std::endl;
        quick_exit(0);
    }
    void ycc_setup_and_work(int argc, char**argv){
        if (at_quick_exit(delete_temp_files))
            perror("exit func was not setup corrently:");
        parseopt(argc,argv);
        _cpp->set_depency(_lex,_parser);
        _parser->set_depency(_cpp,_lex);
        _codegen->set_depency(_parser);
        _cpp->cpp_init();
        _lex->add_file(infile);
        _codegen->set_output_file(open_asmfile());
        auto top=_parser->read_toplevels();
        for(auto &e: *top){
            if (dumpast)
                std::cout<<_parser->node2s(std::make_shared<DataStruct::Node>(e));
            else
                _codegen->emit_toplevel(std::make_shared<DataStruct::Node>(e));
        }
        if (fp.is_open()) fp.close();
        if (!dumpast && !dumpasm) {
            if (outfile.empty())
                outfile = replace_suffix(base(infile), 'o');
            pid_t pid = fork();
            if (pid < 0) perror("fork");
            if (pid == 0) {
//                char texec[20];
//                auto length=std::string("as").copy(texec,2,0);
//                texec[length]='\0';
                execlp(std::string("as").c_str(), "as", "-o", outfile, "-c", asmfile, (char *) NULL);
                perror("execl failed");
            }
            int status;
            waitpid(pid, &status, 0);
            if (status < 0)
                Error::error("as failed");
        }
    }


}