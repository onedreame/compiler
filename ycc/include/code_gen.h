//
// Created by yk on 18-3-10.
//

#ifndef YCC_CODE_GEN_H
#define YCC_CODE_GEN_H

#include <string>
#include <vector>
#include <memory>
#include "parser.h"

#ifdef __GNUC__
#define SAVE                              \
std::shared_ptr<Hook> pr;                 \
if (Utils::dumpstack)                     \
    pr.reset(new Hook(__func__))
#else
#define SAVE
#endif
#define emit(...)        emitf(__LINE__, "\t" __VA_ARGS__)
#define emit_noindent(...)  emitf(__LINE__, __VA_ARGS__)
class CodeGen{
public:
    static std::shared_ptr<CodeGen> Instance();
    void set_output_file(const std::ostream&);
    void set_depency(const std::shared_ptr<Parser>& parser){_parser=parser;}
    void emit_toplevel(std::shared_ptr<DataStruct::Node> );
    template <class T,class... Args>
    void save_to_file(const std::string& p, decltype(p.size()) cur,T t,Args... arg){
        while (cur<p.size()){
            if (p[cur]=='%')
                if (cur+1<p.size()&&p[cur+1]=='%'){
                    *outputfp<<"%";
                    cur+=2;
                }
                else {
                    *outputfp<<t;
                    save_to_file(p,cur+2,arg...);
                    return;
                }
            else *outputfp<<p[cur++];
        }
        Error::error("extra parameters sent to save_to_file");
    }
    template <class... Args>
    void emitf(int line, const std::string&fmt,Args... arg){
        auto ffmt=replace_char(fmt);
        if (!outputfp)
            Error::error("output file is not set");
        save_to_file(ffmt,0,arg...);
        if (Utils::dumpstack) {
            *outputfp<<"\n";
            *outputfp<<get_caller_list()<<":"<<line;
        }
        *outputfp<<"\n";
    }
private:
    static std::shared_ptr<CodeGen> _codegen;
    std::shared_ptr<Parser> _parser;
    const std::vector<std::string> REGS = {"rdi", "rsi", "rdx", "rcx", "r8", "r9"};
    const std::vector<std::string> SREGS = {"dil", "sil", "dl", "cl", "r8b", "r9b"};
    const std::vector<std::string> MREGS = {"edi", "esi", "edx", "ecx", "r8d", "r9d"};
    const int TAB = 8;
    const int REGAREA_SIZE = 176;
    static std::vector<std::string> functions ;
    int stackpos;
    int numgp;
    int numfp;
    std::shared_ptr<std::ostream> outputfp;
    std::unordered_map<std::string,long > source_files;
    std::unordered_map<std::string,std::vector<std::string>> source_lines;
    std::string last_loc;

    CodeGen()= default;
    CodeGen(const CodeGen&)= delete;
    CodeGen(CodeGen&&)= delete;
    CodeGen&operator=(const CodeGen&)= delete;
    CodeGen&operator=(CodeGen&&)= delete;
    class Hook{
    public:
        Hook(const std::string& func){functions.push_back(func);}
        ~Hook(){if (!functions.empty()) functions.pop_back();}
    };

    std::string get_caller_list();
    std::string replace_char(const std::string&str);
    void save_to_file(const std::string& p,decltype(p.size()) cur);

    //assemble
    template <class... Args>
    void emit_nostack(const std::string&fmt, Args... args) {
        *outputfp<< "\t";
        save_to_file(fmt,0,args...);
        *outputfp<<"\n";
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
    void emit_toint(const std::shared_ptr<DataStruct::Type> &ty);
    std::string get_int_reg(const std::shared_ptr<DataStruct::Type> &ty, char r);
    void emit_lload(const std::shared_ptr<DataStruct::Type> &ty, const std::string &base, int off);
    void maybe_convert_bool(const std::shared_ptr<DataStruct::Type> &ty);
    void emit_gsave(const std::string &varname, const std::shared_ptr<DataStruct::Type> &ty, int off);
    void emit_lsave(const std::shared_ptr<DataStruct::Type> &ty, int off);
    void do_emit_assign_deref(const std::shared_ptr<DataStruct::Type> &ty, int off);
    void emit_assign_deref(const std::shared_ptr<DataStruct::Node> &var);
    void emit_pointer_arith(DataStruct::AST_TYPE kind, const std::shared_ptr<DataStruct::Node> &left, const std::shared_ptr<DataStruct::Node> &right);
    void emit_zero_filler(int start, int end);
    void ensure_lvar_init(const std::shared_ptr<DataStruct::Node> &node);
    void emit_assign_struct_ref(const std::shared_ptr<DataStruct::Node> &struc,
                                const std::shared_ptr<DataStruct::Type> &field, int off);
    void emit_load_struct_ref(const std::shared_ptr<DataStruct::Node> &struc,
                              const std::shared_ptr<DataStruct::Type> &field, int off);
    void emit_store(const std::shared_ptr<DataStruct::Node> &var);
    void emit_to_bool(const std::shared_ptr<DataStruct::Type> &ty);
    void emit_comp(const std::string &inst, const std::string &usiginst, const std::shared_ptr<DataStruct::Node> &node);
    void emit_binop_int_arith(const std::shared_ptr<DataStruct::Node> &node);
    void emit_binop_float_arith(const std::shared_ptr<DataStruct::Node> &node);
    void emit_load_convert(const std::shared_ptr<DataStruct::Type> &to, const std::shared_ptr<DataStruct::Type> &from);
    void emit_ret();
    void emit_binop(const std::shared_ptr<DataStruct::Node> &node);
    void emit_save_literal(const std::shared_ptr<DataStruct::Node> &node,
                           const std::shared_ptr<DataStruct::Type> &totype, int off);
    void emit_addr(const std::shared_ptr<DataStruct::Node> &node);
    void emit_copy_struct(const std::shared_ptr<DataStruct::Node> &left,
                                   const std::shared_ptr<DataStruct::Node> &right);
    void emit_fill_holes(const std::vector<std::shared_ptr<DataStruct::Node>> &inits, int off, int totalsize);
    void emit_decl_init(const std::vector<std::shared_ptr<DataStruct::Node>> &inits, int off, int totalsize);
    void emit_pre_inc_dec(const std::shared_ptr<DataStruct::Node> &node, const std::string &op);
    void emit_post_inc_dec(const std::shared_ptr<DataStruct::Node> &node, const std::string &op);
    void set_reg_nums(const std::vector<std::shared_ptr<DataStruct::Node>> &args);
    void emit_je(const std::string &label);
    void emit_label(const std::string &label);
    void emit_jmp(const std::string &label);
    void emit_literal(const std::shared_ptr<DataStruct::Node> &node);
    std::vector<std::string> split(std::string &buf);
    std::vector<std::string> read_source_file(const std::string &file);
    void maybe_print_source_line(const std::string &file, int line);
    void maybe_print_source_loc(const std::shared_ptr<DataStruct::Node> &node);
    void emit_lvar(const std::shared_ptr<DataStruct::Node> &node);
    void emit_gvar(const std::shared_ptr<DataStruct::Node> node);
    void emit_builtin_return_address(const std::shared_ptr<DataStruct::Node> &node);
    void emit_builtin_reg_class(const std::shared_ptr<DataStruct::Node> &node);
    void emit_builtin_va_start(const std::shared_ptr<DataStruct::Node> &node);
    bool maybe_emit_builtin(const std::shared_ptr<DataStruct::Node> &node);
    void classify_args(std::vector<std::shared_ptr<DataStruct::Node>> &ints,
                       std::vector<std::shared_ptr<DataStruct::Node>> &floats,
                       std::vector<std::shared_ptr<DataStruct::Node>> & rest,
                       const std::vector<std::shared_ptr<DataStruct::Node>> & args);
    void save_arg_regs(int nints, int nfloats);
    void restore_arg_regs(int nints, int nfloats);
    int emit_args(const std::vector<std::shared_ptr<DataStruct::Node>> &vals);
    void pop_int_args(int nints);
    void pop_float_args(int nfloats);
    void maybe_booleanize_retval(const std::shared_ptr<DataStruct::Type> &ty);
    void emit_func_call(const std::shared_ptr<DataStruct::Node> &node);
    void emit_decl(const std::shared_ptr<DataStruct::Node> &node);
    void emit_conv(const std::shared_ptr<DataStruct::Node> &node);
    void emit_deref(const std::shared_ptr<DataStruct::Node> &node);
    void emit_ternary(const std::shared_ptr<DataStruct::Node> &node);
    void emit_goto(const std::shared_ptr<DataStruct::Node> &node);
    void emit_return(const std::shared_ptr<DataStruct::Node> &node);
    void emit_compound_stmt(const std::shared_ptr<DataStruct::Node> &node);
    void emit_logand(const std::shared_ptr<DataStruct::Node> &node);
    void emit_logor(const std::shared_ptr<DataStruct::Node> &node);
    void emit_lognot(const std::shared_ptr<DataStruct::Node> &node);
    void emit_bitand(const std::shared_ptr<DataStruct::Node> &node);
    void emit_bitor(const std::shared_ptr<DataStruct::Node> &node);
    void emit_bitnot(const std::shared_ptr<DataStruct::Node> &node);
    void emit_cast(const std::shared_ptr<DataStruct::Node> &node);
    void emit_comma(const std::shared_ptr<DataStruct::Node> &node);
    void emit_assign(const std::shared_ptr<DataStruct::Node> &node);
    void emit_label_addr(const std::shared_ptr<DataStruct::Node> &node);
    void emit_computed_goto(const std::shared_ptr<DataStruct::Node> &node);
    void emit_expr(const std::shared_ptr<DataStruct::Node> &node);
    void emit_zero(int size);
    void emit_padding(const std::shared_ptr<DataStruct::Node> &node, int off);
    void emit_data_addr(const std::shared_ptr<DataStruct::Node> &operand, int depth);
    void emit_data_charptr(const std::string &s, int depth);
    void emit_data_primtype(const std::shared_ptr<DataStruct::Type> &ty, const std::shared_ptr<DataStruct::Node> &val, int depth);
    void do_emit_data(const std::vector<std::shared_ptr<DataStruct::Node>> &inits, int size, int off, int depth);
    void emit_data(const std::shared_ptr<DataStruct::Node> &v, int off, int depth);
    void emit_bss(const std::shared_ptr<DataStruct::Node> &v);
    void emit_global_var(const std::shared_ptr<DataStruct::Node> &v);
    int emit_regsave_area();
    void push_func_params(const std::vector<std::shared_ptr<DataStruct::Node>> params, int off);

    void emit_func_prologue(std::shared_ptr<DataStruct::Node> func);

};
#endif //YCC_CODE_GEN_H
