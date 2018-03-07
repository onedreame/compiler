//
// Created by yk on 18-2-8.
//

#ifndef YCC_PARSER_H
#define YCC_PARSER_H

#include "macro.h"

class Parser
{
public:
    struct Case{
        int beg;
        int end;
        char *label;
    };
    explicit Parser(){}
    bool is_funcdef();
    std::shared_ptr<std::unordered_map<std::string,std::shared_ptr<DataStruct::Node>>> env(){
        return localenv?localenv:globalenv;
    };
    std::shared_ptr<std::vector<DataStruct::Node>>& read_toplevels();
    DataStruct::Token read_funcdef();
private:
    MacroPreprocessor* macro= nullptr;
    Lex *lex= nullptr;
    const int MAX_ALIGN=16;
    DataStruct::SourceLoc sl;   //错误位置
    std::shared_ptr<std::unordered_map<std::string,std::shared_ptr<DataStruct::Node>>> globalenv=std::make_shared<std::unordered_map<std::string,std::shared_ptr<DataStruct::Node>>>();    //全局变量
    std::shared_ptr<std::unordered_map<std::string,std::shared_ptr<DataStruct::Node>>> localenv= nullptr;     //局部变量
    std::unordered_map<std::shared_ptr<std::unordered_map<std::string,std::shared_ptr<DataStruct::Node>>>,
            std::shared_ptr<std::unordered_map<std::string,std::shared_ptr<DataStruct::Node>>>> level;
    std::unordered_map<std::string,std::shared_ptr<DataStruct::Type>> tags;         //struct/union/enum
    std::unordered_map<std::string,DataStruct::Token> labels;       //goto label

    std::shared_ptr<std::vector<DataStruct::Node>> toplevels=std::make_shared<std::vector<DataStruct::Node>>();
    std::shared_ptr<std::vector<DataStruct::Node>> localvars= nullptr;
    std::vector<DataStruct::Token> gotos;
    std::vector<DataStruct::Token> cases;
    DataStruct::Type current_func_type;

    std::string defaultcase;
    std::string lbreak;
    std::string lcontinue;

    // 基本类型对象
//    const DataStruct::Type TYPE_VOID = DataStruct::Type(DataStruct::TYPE_KIND::KIND_VOID, 0, 0, false );
//    const DataStruct::Type TYPE_BOOL = DataStruct::Type(DataStruct::TYPE_KIND::KIND_BOOL, 1, 1, true );
//    const DataStruct::Type TYPE_CHAR = DataStruct::Type(DataStruct::TYPE_KIND::KIND_CHAR, 1, 1, false );
//    const DataStruct::Type TYPE_SHORT = DataStruct::Type( DataStruct::TYPE_KIND::KIND_SHORT, 2, 2, false );
//    const DataStruct::Type TYPE_INT = DataStruct::Type( DataStruct::TYPE_KIND::KIND_INT, 4, 4, false );
//    const DataStruct::Type TYPE_LONG = DataStruct::Type( DataStruct::TYPE_KIND::KIND_LONG, 8, 8, false );
//    const DataStruct::Type TYPE_LLONG = DataStruct::Type( DataStruct::TYPE_KIND::KIND_LLONG, 8, 8, false );
//    const DataStruct::Type TYPE_UCHAR = DataStruct::Type( DataStruct::TYPE_KIND::KIND_CHAR, 1, 1, true );
//    const DataStruct::Type TYPE_USHORT = DataStruct::Type( DataStruct::TYPE_KIND::KIND_SHORT, 2, 2, true );
//    const DataStruct::Type TYPE_UINT = DataStruct::Type( DataStruct::TYPE_KIND::KIND_INT, 4, 4, true );
//    const DataStruct::Type TYPE_ULONG = DataStruct::Type( DataStruct::TYPE_KIND::KIND_LONG, 8, 8, true );
//    const DataStruct::Type TYPE_ULLONG = DataStruct::Type( DataStruct::TYPE_KIND::KIND_LLONG, 8, 8, true );
//    const DataStruct::Type TYPE_FLOAT = DataStruct::Type( DataStruct::TYPE_KIND::KIND_FLOAT, 4, 4, false );
//    const DataStruct::Type TYPE_DOUBLE = DataStruct::Type( DataStruct::TYPE_KIND::KIND_DOUBLE, 8, 8, false );
//    const DataStruct::Type TYPE_LDOUBLE = DataStruct::Type( DataStruct::TYPE_KIND::KIND_LDOUBLE, 8, 8, false );
//    const DataStruct::Type TYPE_ENUM = DataStruct::Type( DataStruct::TYPE_KIND::KIND_ENUM, 4, 4, false );

    const std::shared_ptr<DataStruct::Type> TYPE_VOID = std::make_shared<DataStruct::Type>(DataStruct::TYPE_KIND::KIND_VOID, 0, 0, false );
    const std::shared_ptr<DataStruct::Type> TYPE_BOOL = std::make_shared<DataStruct::Type>(DataStruct::TYPE_KIND::KIND_BOOL, 1, 1, true );
    const std::shared_ptr<DataStruct::Type> TYPE_CHAR = std::make_shared<DataStruct::Type>(DataStruct::TYPE_KIND::KIND_CHAR, 1, 1, false );
    const std::shared_ptr<DataStruct::Type> TYPE_SHORT = std::make_shared<DataStruct::Type>( DataStruct::TYPE_KIND::KIND_SHORT, 2, 2, false );
    const std::shared_ptr<DataStruct::Type> TYPE_INT = std::make_shared<DataStruct::Type>( DataStruct::TYPE_KIND::KIND_INT, 4, 4, false );
    const std::shared_ptr<DataStruct::Type> TYPE_LONG = std::make_shared<DataStruct::Type>( DataStruct::TYPE_KIND::KIND_LONG, 8, 8, false );
    const std::shared_ptr<DataStruct::Type> TYPE_LLONG = std::make_shared<DataStruct::Type>( DataStruct::TYPE_KIND::KIND_LLONG, 8, 8, false );
    const std::shared_ptr<DataStruct::Type> TYPE_UCHAR = std::make_shared<DataStruct::Type>( DataStruct::TYPE_KIND::KIND_CHAR, 1, 1, true );
    const std::shared_ptr<DataStruct::Type> TYPE_USHORT = std::make_shared<DataStruct::Type>( DataStruct::TYPE_KIND::KIND_SHORT, 2, 2, true );
    const std::shared_ptr<DataStruct::Type> TYPE_UINT = std::make_shared<DataStruct::Type>( DataStruct::TYPE_KIND::KIND_INT, 4, 4, true );
    const std::shared_ptr<DataStruct::Type> TYPE_ULONG = std::make_shared<DataStruct::Type>( DataStruct::TYPE_KIND::KIND_LONG, 8, 8, true );
    const std::shared_ptr<DataStruct::Type> TYPE_ULLONG = std::make_shared<DataStruct::Type>( DataStruct::TYPE_KIND::KIND_LLONG, 8, 8, true );
    const std::shared_ptr<DataStruct::Type> TYPE_FLOAT = std::make_shared<DataStruct::Type>( DataStruct::TYPE_KIND::KIND_FLOAT, 4, 4, false );
    const std::shared_ptr<DataStruct::Type> TYPE_DOUBLE = std::make_shared<DataStruct::Type>( DataStruct::TYPE_KIND::KIND_DOUBLE, 8, 8, false );
    const std::shared_ptr<DataStruct::Type> TYPE_LDOUBLE = std::make_shared<DataStruct::Type>( DataStruct::TYPE_KIND::KIND_LDOUBLE, 8, 8, false );
    const std::shared_ptr<DataStruct::Type> TYPE_ENUM = std::make_shared<DataStruct::Type>( DataStruct::TYPE_KIND::KIND_ENUM, 4, 4, false );

    void CHECK_CPP();
    void CHECK_LEX();
    auto lower=[](const std::string&s){std::string b;for(auto&e:s) b+=tolower(e);return b;};
    std::shared_ptr<DataStruct::Type> make_rectype(bool);
    std::shared_ptr<DataStruct::Type> make_numtype(DataStruct::TYPE_KIND, bool);
    std::shared_ptr<DataStruct::Type> make_func_type(const std::shared_ptr<DataStruct::Type> &, const std::vector<DataStruct::Type> &, bool, bool) ;
    std::shared_ptr<DataStruct::Type> make_ptr_type(const std::shared_ptr<DataStruct::Type> &);
    std::shared_ptr<DataStruct::Type> make_array_type(const std::shared_ptr<DataStruct::Type>&, int );

    bool is_inttype(const std::shared_ptr<DataStruct::Type>&);
    bool is_flotype(const std::shared_ptr<DataStruct::Type>&);
    bool is_arithtype(const std::shared_ptr<DataStruct::Type>&);
    bool is_string(const std::shared_ptr<DataStruct::Type>&);

    std::shared_ptr<DataStruct::Type> read_int_suffix(const std::string &);
    std::shared_ptr<DataStruct::Node> read_int(const DataStruct::Token &);
    std::shared_ptr<DataStruct::Node> read_float(const DataStruct::Token &);
    std::shared_ptr<DataStruct::Node> read_number(const DataStruct::Token &);

    int read_intexpr();
    int eval_intexpr(const std::shared_ptr<DataStruct::Node> &, const std::shared_ptr<DataStruct::Node> *);
    int eval_struct_ref(const std::shared_ptr<DataStruct::Node> &, int);

    DataStruct::SourceLoc& mark_location();
    std::shared_ptr<DataStruct::Node> ast_lvar(const std::shared_ptr<DataStruct::Type> &, const std::string &);
    std::shared_ptr<DataStruct::Node> ast_inttype(const std::shared_ptr<DataStruct::Type> &, long ) ;
    std::shared_ptr<DataStruct::Node> ast_floattype(const std::shared_ptr<DataStruct::Type>&,double);
    std::shared_ptr<DataStruct::Node> ast_string(DataStruct::ENCODE , const std::string &);

    std::shared_ptr<DataStruct::Node> read_generic();
    Vector *read_generic_list(Node **defaultexpr);
    bool type_compatible(Type *a, Type *b);
    void read_static_assert();

    bool is_type(const DataStruct::Token&);
    std::shared_ptr<DataStruct::Type> get_typedef(const std::string&);
    void skip_parentheses(std::vector<DataStruct::Token>&);
    std::shared_ptr<DataStruct::Type> read_decl_spec_opt(DataStruct::QUALITIFIER *);
    std::shared_ptr<DataStruct::Type> read_decl_spec(DataStruct::QUALITIFIER *);

    std::shared_ptr<DataStruct::Type> read_typeof();
    std::shared_ptr<DataStruct::Type> read_struct_def();
    std::shared_ptr<DataStruct::Type> read_union_def();
    std::shared_ptr<DataStruct::Type> read_enum_def();
    std::shared_ptr<DataStruct::Type> read_rectype_def(bool);
    std::string read_rectype_tag();
    std::vector<std::pair<std::string,std::shared_ptr<DataStruct::Type>>>  read_rectype_fields_sub();
    void fix_rectype_flexible_member(std::vector<std::pair<std::string,std::shared_ptr<DataStruct::Type>>> &);
    std::vector<std::pair<std::string,std::shared_ptr<DataStruct::Type>>> update_struct_offset(int&,int&,std::vector<std::pair<std::string,std::shared_ptr<DataStruct::Type>>>&);
    std::vector<std::pair<std::string,std::shared_ptr<DataStruct::Type>>> update_union_offset(int&,int&,std::vector<std::pair<std::string,std::shared_ptr<DataStruct::Type>>>&);
    void finish_bitfield(int&,int &);
    int compute_padding(int , int );

    void squash_unnamed_struct(std::vector<std::pair<std::string,std::shared_ptr<DataStruct::Type>>>&, std::shared_ptr<DataStruct::Type> &, int ) ;
    int read_bitsize(std::string&,const std::shared_ptr<DataStruct::Type>&);
    std::shared_ptr<DataStruct::Type> read_declarator(std::string*,const std::shared_ptr<DataStruct::Type>&,std::vector<DataStruct::Node>*,DataStruct::DECL_TYPE);

    std::vector<std::pair<std::string,std::shared_ptr<DataStruct::Type>>> read_rectype_fields(int &,int &,bool);
    void ensure_not_void(std::shared_ptr<DataStruct::Type>&);
    int read_alignas();
    bool is_poweroftwo(int);
    std::shared_ptr<DataStruct::Type> read_cast_type();
    std::shared_ptr<DataStruct::Type> read_abstract_declarator(const std::shared_ptr<DataStruct::Type>&);
    std::shared_ptr<DataStruct::Type> read_declarator_func(const std::shared_ptr<DataStruct::Type>&, std::vector<DataStruct::Node>*);
    std::shared_ptr<DataStruct::Type> read_func_param_list(std::vector<DataStruct::Node>*, const std::shared_ptr<DataStruct::Type>&);
    std::shared_ptr<DataStruct::Type> read_declarator_tail(const std::shared_ptr<DataStruct::Type>&, std::vector<DataStruct::Node>*);
    void read_declarator_params(std::vector<DataStruct::Type>&, std::vector<DataStruct::Node>*, bool &);
    std::shared_ptr<DataStruct::Type> read_func_param(std::string& ,bool);
    void read_declarator_params_oldstyle(std::vector<DataStruct::Node>*);
    std::shared_ptr<DataStruct::Type> read_declarator_array(const std::shared_ptr<DataStruct::Type> &);
    void skip_type_qualifiers();

    std::shared_ptr<DataStruct::Node> read_var_or_func(const std::string &);
    int get_compound_assign_op(const DataStruct::Token &);
    std::shared_ptr<DataStruct::Node> read_stmt_expr();
    std::shared_ptr<DataStruct::Type> char_type(DataStruct::ENCODE);
    std::shared_ptr<DataStruct::Node> read_primary_expr();
    std::shared_ptr<DataStruct::Node>  read_subscript_expr(const std::shared_ptr<DataStruct::Node> &);
    std::shared_ptr<DataStruct::Node> read_postfix_expr_tail(const std::shared_ptr<DataStruct::Node> &);
    std::shared_ptr<DataStruct::Node> read_postfix_expr();
    std::shared_ptr<DataStruct::Node> read_unary_incdec(int);
    std::shared_ptr<DataStruct::Node> read_label_addr(const std::shared_ptr<DataStruct::Token> &);
    std::shared_ptr<DataStruct::Node> read_unary_addr();
    std::shared_ptr<DataStruct::Node> read_unary_deref(const std::shared_ptr<DataStruct::Token> &);
    std::shared_ptr<DataStruct::Node> read_unary_minus();
    std::shared_ptr<DataStruct::Node> read_unary_bitnot(const std::shared_ptr<DataStruct::Token> &);
    std::shared_ptr<DataStruct::Node> read_unary_lognot();
    std::shared_ptr<DataStruct::Node> read_unary_expr();
    std::shared_ptr<DataStruct::Node> read_compound_literal(const std::shared_ptr<DataStruct::Type> &);
    std::shared_ptr<DataStruct::Node> read_cast_expr();
    std::shared_ptr<DataStruct::Node> read_multiplicative_expr();
    std::shared_ptr<DataStruct::Node> read_additive_expr();
    std::shared_ptr<DataStruct::Node> read_shift_expr();
    std::shared_ptr<DataStruct::Node> read_relational_expr();
    std::shared_ptr<DataStruct::Node> read_equality_expr();
    std::shared_ptr<DataStruct::Node> read_bitand_expr();
    std::shared_ptr<DataStruct::Node> read_assignment_expr();
    std::shared_ptr<DataStruct::Node> read_comma_expr();
    std::shared_ptr<DataStruct::Node> do_read_conditional_expr(const std::shared_ptr<DataStruct::Node> &);
    std::shared_ptr<DataStruct::Node> read_conditional_expr();
    std::shared_ptr<DataStruct::Node> read_logor_expr();
    std::shared_ptr<DataStruct::Node> read_logand_expr();
    std::shared_ptr<DataStruct::Node> read_bitor_expr();
    std::shared_ptr<DataStruct::Node> read_bitxor_expr();
    std::shared_ptr<DataStruct::Node> read_expr();

};
#endif //YCC_PARSER_H
