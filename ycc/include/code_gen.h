//
// Created by yk on 18-3-10.
//

#ifndef YCC_CODE_GEN_H
#define YCC_CODE_GEN_H

#include <string>
#include <vector>
#include <memory>
#include "utils.h"

#ifdef __GNUC__
#define SAVE                                                 \
int save_hook __attribute__((unused, cleanup(pop_function)));\
if (Utils::dumpstack)                     \
    functions.push_back(__func__);
#else
#define SAVE
#endif
#define emit(...)        emitf(__LINE__, "\t" __VA_ARGS__)
#define emit_noindent(...)  emitf(__LINE__, __VA_ARGS__)
class CodeGen{
public:
    static std::shared_ptr<CodeGen> Instance();
    void set_output_file(const std::string&);
    template <class T,class... Args>
    void save_to_file(const std::string& p, decltype(p.size()) cur,T t,Args... arg){
        while (cur<p.size()){
            if (p[cur]=='%')
                if (cur+1<p.size()&&p[cur+1]=='%'){
                    outputfp<<"%";
                    cur+=2;
                }
                else {
                    outputfp<<t;
                    save_to_file(p,cur+2,arg...);
                    return;
                }
            else outputfp<<p[cur++];
        }
        Error::error("extra parameters sent to save_to_file");
    }
    template <class... Args>
    void emitf(int line, const std::string&fmt,Args... arg){
        auto ffmt=replace_char(fmt);
        if (!outputfp.is_open())
            Error::error("output file is not set");
        save_to_file(ffmt,0,arg...);
        if (Utils::dumpstack) {
            outputfp<<"\n";
            outputfp<<get_caller_list()<<":"<<line;
        }
        outputfp<<"\n";
    }
private:
    static std::shared_ptr<CodeGen> _codegen;
    const std::vector<std::string> REGS = {"rdi", "rsi", "rdx", "rcx", "r8", "r9"};
    const std::vector<std::string> SREGS = {"dil", "sil", "dl", "cl", "r8b", "r9b"};
    const std::vector<std::string> MREGS = {"edi", "esi", "edx", "ecx", "r8d", "r9d"};
    const int TAB = 8;
    const int REGAREA_SIZE = 176;
    std::vector<std::string> functions ;
    int stackpos;
    int numgp;
    int numfp;
    std::ofstream outputfp;
//    static Map *source_files = &EMPTY_MAP;
//    static Map *source_lines = &EMPTY_MAP;
    std::string last_loc;

    CodeGen()= default;
    CodeGen(const CodeGen&)= delete;
    CodeGen(CodeGen&&)= delete;
    CodeGen&operator=(const CodeGen&)= delete;
    CodeGen&operator=(CodeGen&&)= delete;

    void pop_function(void *ignore);

    std::string get_caller_list();
    std::string replace_char(const std::string&str);
    void save_to_file(const std::string& p,decltype(p.size()) cur);

    //assemble
    template <class T,class... Args>
    void emit_nostack(const std::string&fmt, Args... args) {
        outputfp<< "\t";
        save_to_file(fmt,0,args...);
        outputfp<<"\n";
    }
    std::string get_load_inst(const std::shared_ptr<DataStruct::Type> &ty);
    int align(int n, int m);
    void push(const std::string& reg);
    void pop(const std::string &reg);
    void push_xmm(int reg);
    void pop_xmm(int reg);
    int push_struct(int size);
    void maybe_emit_bitshift_load(const std::shared_ptr<DataStruct::Type> &ty);
    void maybe_emit_bitshift_save(const std::shared_ptr<DataStruct::Type> &ty, const std::string &addr);
    void emit_gload(const std::shared_ptr<DataStruct::Type> &ty, const std::string &label, int off);
    void emit_intcast(const std::shared_ptr<DataStruct::Type> &ty);

    void emit_toplevel(std::shared_ptr<DataStruct::Node> );
    void emit_func_prologue(std::shared_ptr<DataStruct::Node> func);

};
#endif //YCC_CODE_GEN_H
