//
// Created by yk on 18-2-18.
//

#ifndef YCC_MACRO_H
#define YCC_MACRO_H

#include <unordered_map>
#include <list>
#include <algorithm>
#include "Token.h"
#include "Lex.h"
typedef void (*SpecialMacroHandler)(MacroPreprocessor* ,const DataStruct::Token&);
class MacroPreprocessor{
public:
    MacroPreprocessor&operator=( const MacroPreprocessor&)= delete;
    MacroPreprocessor(const MacroPreprocessor&)= delete;
    MacroPreprocessor&operator=(MacroPreprocessor&&)= delete;
    MacroPreprocessor(MacroPreprocessor&&)= delete;
    MacroPreprocessor(std::shared_ptr<Lex> lex):lex(lex){}
    DataStruct::Token read_expand_newline();
    DataStruct::Token read_expand();
    DataStruct::Token peek_token();
    DataStruct::Token read_token();
    struct tm getNow(){ return now;}
    void define_obj_macro(const std::string& name, const DataStruct::Token& value);
    void read_directive(const DataStruct::Token& hash);
    DataStruct::CondIncl make_cond_incl(bool);
    DataStruct::Token read_ident( );
    void expect_newline();
    DataStruct::Macro make_obj_macro(const std::vector<DataStruct::Token >& body) {
        return make_macro(DataStruct::Macro(DataStruct::MacroType::MACRO_OBJ,body ));
    }
    DataStruct::Macro make_func_macro(std::vector<DataStruct::Token >& body, int nargs, bool is_varg) {
        return make_macro(DataStruct::Macro(
                DataStruct::MacroType::MACRO_FUNC, body, is_varg,  nargs));
    }
    DataStruct::Macro make_special_macro(SpecialMacroHandler fn) {
        return make_macro(DataStruct::Macro( DataStruct::MacroType::MACRO_SPECIAL, fn ));
    }
    void propagate_space(std::vector<DataStruct::Token >&, const DataStruct::Token &);
    std::vector<DataStruct::Token > read_one_arg(const DataStruct::Token &ident, bool &end, bool readall);
    std::vector<DataStruct::Token > subst(const DataStruct::Macro &, std::vector<std::vector<DataStruct::Token >>&, std::set<std::string> &);

    std::vector<DataStruct::Token > expand_all(std::vector<DataStruct::Token >&, const DataStruct::Token &);
    std::vector<DataStruct::Token > add_hide_set(std::vector<DataStruct::Token >&,std::set<std::string >&);
    DataStruct::Token stringize(const DataStruct::Token &, std::vector<DataStruct::Token >&);
    std::vector<std::vector<DataStruct::Token> > do_read_args(const DataStruct::Token &, const DataStruct::Macro &);
    std::vector<std::vector<DataStruct::Token >> read_args(const DataStruct::Token &, const DataStruct::Macro &);
    DataStruct::Token glue_tokens(const DataStruct::Token& , const DataStruct::Token &)const ;
    void glue_push(std::vector<DataStruct::Token >&tokens, const DataStruct::Token &tok) const ;
    void read_funclike_macro(const DataStruct::Token& name) ;
    void read_obj_macro(const std::string& sval);
    bool read_funclike_macro_params(const DataStruct::Token &, std::unordered_map<std::string ,DataStruct::Token >&);
    std::vector<DataStruct::Token > read_funclike_macro_body(const std::unordered_map<std::string,DataStruct::Token >&) const;
    std::string read_error_message();
    void unget_all(std::vector<DataStruct::Token >&);
    void do_read_if(bool istrue);
    void parse_pragma_operand(const DataStruct::Token&);
    std::string join_paths(std::vector<DataStruct::Token >& );
    std::string read_cpp_header_name(const DataStruct::Token &, bool &);
    bool guarded(std::string &);
    bool try_include(std::string , const std::string& , bool );
    DataStruct::Token may_convert_keyword(const DataStruct::token&);

    void read_define();
    void read_linemarker(const DataStruct::Token &);
    void read_elif(const DataStruct::Token &);
    void read_else(const DataStruct::Token &);
    void read_endif(const DataStruct::Token&);
    void read_error(const DataStruct::Token&);
    void read_if();
    void read_ifdef();
    void read_ifndef();
    void read_include(const DataStruct::Token &, std::shared_ptr<DataStruct::File >, bool);
    void read_include_next(const DataStruct::Token &, std::shared_ptr<DataStruct::File>);
    void read_line();
    void read_pragma();
    void read_undef();
    void read_warning(const DataStruct::Token&);

    void make_token_pushback(const DataStruct::Token &, DataStruct::TOKEN_TYPE , const std::string &);
    friend void handle_date_macro(MacroPreprocessor*,const DataStruct::Token &);
    friend void handle_time_macro(MacroPreprocessor*,const DataStruct::Token &);
    friend void handle_timestamp_macro(MacroPreprocessor*,const DataStruct::Token &);
    friend void handle_file_macro(MacroPreprocessor*,const DataStruct::Token &);
    friend void handle_line_macro(MacroPreprocessor*,const DataStruct::Token &);
    friend void handle_pragma_macro(MacroPreprocessor*,const DataStruct::Token &);
    friend void handle_base_file_macro(MacroPreprocessor*,const DataStruct::Token &);
    friend void handle_counter_macro(MacroPreprocessor*,const DataStruct::Token &);
    friend void handle_include_level_macro(MacroPreprocessor*,const DataStruct::Token &);

//    void handle_date_macro(MacroPreprocessor*const,const DataStruct::Token &);
//    void handle_time_macro(const DataStruct::Token &);
//    void handle_timestamp_macro(const DataStruct::Token &);
//    void handle_file_macro(const DataStruct::Token &);
//    void handle_line_macro(const DataStruct::Token &);
//    void handle_pragma_macro(const DataStruct::Token &);
//    void handle_base_file_macro(const DataStruct::Token &);
//    void handle_counter_macro(const DataStruct::Token &);
//    void handle_include_level_macro(const DataStruct::Token &);
    void add_include_path(const std::string &);

    void cpp_init();
    void read_from_string(const std::string&);
private:

    std::shared_ptr<Lex> lex;
    std::unordered_map<std::string,DataStruct::Macro> macros;
    std::list<DataStruct::Token > tokens;
    std::unordered_map<std::string,DataStruct::AST_TYPE > keywords;
    std::vector<DataStruct::CondIncl >cond_incl_stack;
    //key:文件路径，value：文件的包含哨
    std::unordered_map<std::string,std::string> include_guard;
    std::unordered_map<std::string,int> once;
    const DataStruct::Token CPP_TOKEN_ONE=DataStruct::Token(DataStruct::TOKEN_TYPE::TNUMBER,"1");
    const DataStruct::Token CPP_TOKEN_ZERO=DataStruct::Token(DataStruct::TOKEN_TYPE::TNUMBER,"0");
    std::vector<std::string> std_include_path;
    struct tm now;

    DataStruct::Macro make_macro(const DataStruct::Macro& tmpl) {
        return DataStruct::Macro(tmpl);
    }

    DataStruct::Token make_macro_token(int , bool);
    DataStruct::Token copy_token(const DataStruct::Token&) const ;
    void expect(DataStruct::AST_TYPE);
    DataStruct::Token skip_newlines();
    bool next(DataStruct::AST_TYPE );
    template <class T>
    std::set<T> set_intersection(const std::set<T>&s1,const std::set<T>& s2) const
    {
        std::vector<T> mvec(s1.size()+s2.size());
        auto it=std::set_intersection(s1.begin(),s1.end(),s2.begin(),s2.end(),mvec.begin());
        return std::set<T>(mvec.begin(),it);
    }
    template <class T>
    std::set<T> set_union(const std::set<T>& s1, const std::set<T>& s2) const
    {
        std::vector<T> mvec(s1.size()+s2.size());
        auto it=std::set_union(s1.begin(),s1.end(),s2.begin(),s2.end(),mvec.begin());
        return std::set<T>(mvec.begin(),it);
    }
    void hashhash_check(const std::vector<DataStruct::Token >&) const ;
    bool is_digit_seq(std::string& ) const ;
    void init_path_and_macros();

    void init_now();
};
void handle_date_macro(MacroPreprocessor*,const DataStruct::Token &);
void handle_time_macro(MacroPreprocessor*,const DataStruct::Token &);
void handle_timestamp_macro(MacroPreprocessor*,const DataStruct::Token &);
void handle_file_macro(MacroPreprocessor*,const DataStruct::Token &);
void handle_line_macro(MacroPreprocessor*,const DataStruct::Token &);
void handle_pragma_macro(MacroPreprocessor*,const DataStruct::Token &);
void handle_base_file_macro(MacroPreprocessor*,const DataStruct::Token &);
void handle_counter_macro(MacroPreprocessor*,const DataStruct::Token &);
void handle_include_level_macro(MacroPreprocessor*,const DataStruct::Token &);

#endif //YCC_MACRO_H
