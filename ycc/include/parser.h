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
        int beg;   //开始行号
        int end;   //结束行号
        std::string label;   //label name
        Case(Case&&c):beg(c.beg),end(c.end),label(std::move(c.label)){}
        Case()= default;
        Case(int a,int b,const std::string &s):beg(a),end(b),label(s){}
        Case(const Case&)= default;
        Case& operator=(const Case&)= default;
        Case&operator=(Case&&c){
            beg=c.beg;end=c.end;label=std::move(c.label);
        }
    };
    using vec_share=std::vector<std::shared_ptr<DataStruct::Node>>;
    using env_type=std::unordered_map<std::string,std::shared_ptr<DataStruct::Node>>;

    bool is_funcdef();
    std::string node2s(const std::shared_ptr<DataStruct::Node> &node);
    std::string ty2s(const std::shared_ptr<DataStruct::Type> &ty);
    std::shared_ptr<std::unordered_map<std::string,std::shared_ptr<DataStruct::Node>>> env(){
        return localenv?localenv:globalenv;
    };
//    using env_type = std::shared_ptr<std::unordered_map<std::string,std::shared_ptr<DataStruct::Node>>>;

//    void debugenv(int depth,const env_type& env){
//        for(auto&e:*env)
//        {
//            for(int i=0;i<depth;++i)
//                std::cout<<" ";
//            std::cout<<e.first<<std::endl;
//        }
//        if (scope.find(env)!=scope.end())
//            debugenv(depth+4,scope[env]);
//    }

    static std::shared_ptr<Parser> Instance();
    int eval_intexpr(const std::shared_ptr<DataStruct::Node> &, std::shared_ptr<DataStruct::Node> *);
    std::shared_ptr<Parser::vec_share> read_toplevels();
    std::shared_ptr<DataStruct::Node> read_funcdef();
    std::shared_ptr<DataStruct::Node> read_expr();
    bool is_inttype(const std::shared_ptr<DataStruct::Type>&);
    bool is_flotype(const std::shared_ptr<DataStruct::Type>&);
    std::string make_static_label(const std::string &);
    std::string  make_tempname();
    std::string make_label();
    Parser::Case make_case(int beg, int end, const std::string &label);
    void set_depency(std::shared_ptr<MacroPreprocessor>& _macro,std::shared_ptr<Lex>& _lex){
        lex=_lex;
        macro=_macro;
    }
private:
    Parser()= default;
    static std::shared_ptr<Parser> _parser;
    Parser&operator=(const Parser&)= delete;
    Parser&operator=(Parser&&)= delete;
    Parser(const Parser&)= delete;
    Parser(Parser&&)= delete;
    std::shared_ptr<MacroPreprocessor> macro= nullptr;
    std::shared_ptr<Lex> lex= nullptr;
    const int MAX_ALIGN=16;
    DataStruct::SourceLoc sl;   //错误位置
    std::shared_ptr<env_type> globalenv=std::make_shared<env_type >();    //全局变量
    std::shared_ptr<env_type> localenv= nullptr;     //局部变量
    std::unordered_map<std::shared_ptr<env_type>, std::shared_ptr<env_type>> scope;
    std::unordered_map<std::string,std::shared_ptr<DataStruct::Type>> tags;         //struct/union/enum
    std::unordered_map<std::string,std::shared_ptr<DataStruct::Node>> labels;       //goto label

    std::shared_ptr<vec_share> toplevels=std::make_shared<vec_share >();
    std::shared_ptr<vec_share> localvars= nullptr;
    vec_share gotos;
    std::shared_ptr<std::vector<Case>> cases;
    std::shared_ptr<DataStruct::Type> current_func_type;

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

    std::string decorate_int(const std::string &name, const std::shared_ptr<DataStruct::Type> &ty);
    std::string do_ty2s(std::unordered_map<std::shared_ptr<DataStruct::Type>, bool> &dict,
                        const std::shared_ptr<DataStruct::Type> &ty);
    void do_node2s(std::string&b, const std::shared_ptr<DataStruct::Node> &node);

    void a2s_declinit(std::string &b, const std::vector<std::shared_ptr<DataStruct::Node>> &initlist);
    void uop_to_string(std::string &b, std::string &&op, const std::shared_ptr<DataStruct::Node> &node);
    void binop_to_string(std::string &b, std::string &&op, const std::shared_ptr<DataStruct::Node> &node);
//    std::function<std::string(const std::string&)> lower=_lower;
    std::shared_ptr<DataStruct::Type> make_rectype(bool);
    std::shared_ptr<DataStruct::Type> make_numtype(DataStruct::TYPE_KIND, bool);
    std::shared_ptr<DataStruct::Type> make_func_type(const std::shared_ptr<DataStruct::Type> &, const std::vector<DataStruct::Type> &, bool, bool) ;
    std::shared_ptr<DataStruct::Type> make_ptr_type(const std::shared_ptr<DataStruct::Type> &);
    std::shared_ptr<DataStruct::Type> make_array_type(const std::shared_ptr<DataStruct::Type>&, int );
    std::shared_ptr<DataStruct::Node> ast_lvar(const std::shared_ptr<DataStruct::Type> &, const std::string &);
    std::shared_ptr<DataStruct::Node> ast_gvar(const std::shared_ptr<DataStruct::Type> &, const std::string &);
    std::shared_ptr<DataStruct::Node> ast_inttype(const std::shared_ptr<DataStruct::Type> &, long ) ;
    std::shared_ptr<DataStruct::Node> ast_floattype(const std::shared_ptr<DataStruct::Type>&,double);
    std::shared_ptr<DataStruct::Node> ast_string(DataStruct::ENCODE , const std::string &);
    std::shared_ptr<DataStruct::Node> ast_funcdesg(const std::shared_ptr<DataStruct::Type> &, const std::string &);
    std::shared_ptr<DataStruct::Node> ast_funcall(const std::shared_ptr<DataStruct::Type> &, const std::string &, const std::vector<std::shared_ptr<DataStruct::Node>> &);
    std::shared_ptr<DataStruct::Node> ast_funcptr_call(std::shared_ptr<DataStruct::Node>&, const std::vector<std::shared_ptr<DataStruct::Node>> &);
    std::shared_ptr<DataStruct::Node>  ast_func(const std::shared_ptr<DataStruct::Type> &,
                                                const std::string &,
                                                const std::vector<std::shared_ptr<DataStruct::Node>>&,
                                                const std::shared_ptr<DataStruct::Node> &,
                                                const std::vector<std::shared_ptr<DataStruct::Node>> &);
    std::shared_ptr<DataStruct::Node> ast_decl(const std::shared_ptr<DataStruct::Node> &, const std::vector<std::shared_ptr<DataStruct::Node>> &);
    std::shared_ptr<DataStruct::Node> ast_init(const std::shared_ptr<DataStruct::Node> &, const std::shared_ptr<DataStruct::Type> &, int );
    std::shared_ptr<DataStruct::Node> ast_conv(const std::shared_ptr<DataStruct::Type> &, const std::shared_ptr<DataStruct::Node> &);
    std::shared_ptr<DataStruct::Node> ast_uop(DataStruct::AST_TYPE, const std::shared_ptr<DataStruct::Type> &, const std::shared_ptr<DataStruct::Node> &);
    std::shared_ptr<DataStruct::Node> ast_biop(DataStruct::AST_TYPE, const std::shared_ptr<DataStruct::Type> &,const std::shared_ptr<DataStruct::Node> &, const std::shared_ptr<DataStruct::Node> &);
    std::shared_ptr<DataStruct::Node> ast_typedef(const std::shared_ptr<DataStruct::Type>&, const std::string&);
    std::shared_ptr<DataStruct::Node> ast_static_lvar(const std::shared_ptr<DataStruct::Type> &, const std::string &);
    std::shared_ptr<DataStruct::Node> ast_if(const std::shared_ptr<DataStruct::Node> &,
                                             const std::shared_ptr<DataStruct::Node> &,
                                             const std::shared_ptr<DataStruct::Node> &);
    std::shared_ptr<DataStruct::Node> ast_ternary(const std::shared_ptr<DataStruct::Type> &,
                                                  const std::shared_ptr<DataStruct::Node> &,
                                                  const std::shared_ptr<DataStruct::Node> &,
                                                  const std::shared_ptr<DataStruct::Node> &);
    std::shared_ptr<DataStruct::Node> ast_return(const std::shared_ptr<DataStruct::Node> &);
    std::shared_ptr<DataStruct::Node> ast_compound_stmt(const std::vector<std::shared_ptr<DataStruct::Node>>*stmts);
    std::shared_ptr<DataStruct::Node> ast_struct_ref(const std::shared_ptr<DataStruct::Type> &,
                                                     const std::shared_ptr<DataStruct::Node> &,
                                                     const std::string &);
    std::shared_ptr<DataStruct::Node> ast_goto(const std::string &);
    std::shared_ptr<DataStruct::Node> ast_jump(const std::string &);
    std::shared_ptr<DataStruct::Node> ast_computed_goto( const std::shared_ptr<DataStruct::Node> &);
    std::shared_ptr<DataStruct::Node> ast_label(const std::string &);
    std::shared_ptr<DataStruct::Node> ast_dest(const std::string &);
    std::shared_ptr<DataStruct::Node> ast_label_addr(const std::string &);

    bool is_arithtype(const std::shared_ptr<DataStruct::Type>&);
    bool is_type(const DataStruct::Token&);
    bool is_string(const std::shared_ptr<DataStruct::Type>&);
    void ensure_lvalue(const std::shared_ptr<DataStruct::Node> &);
    void ensure_inttype(const std::shared_ptr<DataStruct::Node> &);
    void ensure_arithtype(const std::shared_ptr<DataStruct::Node> &);
    void ensure_not_void(std::shared_ptr<DataStruct::Type>&);
    DataStruct::SourceLoc& mark_location();
    void backfill_labels();
    std::shared_ptr<DataStruct::Node> conv(const std::shared_ptr<DataStruct::Node> &);

    bool same_arith_type(const std::shared_ptr<DataStruct::Type> &, const std::shared_ptr<DataStruct::Type> &);
    std::shared_ptr<DataStruct::Node> wrap(const std::shared_ptr<DataStruct::Type> &, const std::shared_ptr<DataStruct::Node> &);
    bool valid_pointer_binop(DataStruct::AST_TYPE );
    std::shared_ptr<DataStruct::Node> binop(DataStruct::AST_TYPE , const std::shared_ptr<DataStruct::Node> &, const std::shared_ptr<DataStruct::Node> &);
    bool is_same_struct(const std::shared_ptr<DataStruct::Type> &, const std::shared_ptr<DataStruct::Type> &);
    void ensure_assignable(const std::shared_ptr<DataStruct::Type> &, const std::shared_ptr<DataStruct::Type> &);
    std::shared_ptr<DataStruct::Type> usual_arith_conv(std::shared_ptr<DataStruct::Type>, std::shared_ptr<DataStruct::Type>);
    std::shared_ptr<DataStruct::Type> read_int_suffix(const std::string &);

    std::shared_ptr<DataStruct::Node> read_int(const DataStruct::Token &);
    std::shared_ptr<DataStruct::Node> read_float(const DataStruct::Token &);
    std::shared_ptr<DataStruct::Node> read_number(const DataStruct::Token &);
    int eval_struct_ref(const std::shared_ptr<DataStruct::Node> &, int);

    //_Generic
    bool type_compatible(const std::shared_ptr<DataStruct::Type> &a, const std::shared_ptr<DataStruct::Type> &b);
    std::vector<std::pair<std::shared_ptr<DataStruct::Type>,std::shared_ptr<DataStruct::Node>>> read_generic_list(std::shared_ptr<DataStruct::Node> &defaultexpr);
    std::shared_ptr<DataStruct::Node> read_generic();

    std::vector<std::shared_ptr<DataStruct::Node>> read_func_args(const std::vector<std::shared_ptr<DataStruct::Type>> &params);
    std::shared_ptr<DataStruct::Node> read_funcall(std::shared_ptr<DataStruct::Node> fp);
    void read_static_assert();
    std::shared_ptr<DataStruct::Type> get_typedef(const std::string&);
    std::shared_ptr<DataStruct::Type> read_typeof();

    //Expressions
    std::shared_ptr<DataStruct::Type> char_type(DataStruct::ENCODE);
    int read_intexpr();
    std::shared_ptr<DataStruct::Node> read_var_or_func(const std::string &);
    DataStruct::AST_TYPE get_compound_assign_op(const DataStruct::Token &);
    std::shared_ptr<DataStruct::Node> read_stmt_expr();
    std::shared_ptr<DataStruct::Node> read_primary_expr();
    std::shared_ptr<DataStruct::Node> read_subscript_expr(const std::shared_ptr<DataStruct::Node> &);
    std::shared_ptr<DataStruct::Node> read_postfix_expr_tail(std::shared_ptr<DataStruct::Node> &);
    std::shared_ptr<DataStruct::Node> read_postfix_expr();
    std::shared_ptr<DataStruct::Node> read_unary_incdec(DataStruct::AST_TYPE);
    std::shared_ptr<DataStruct::Node> read_label_addr(const DataStruct::Token &);
    std::shared_ptr<DataStruct::Node> read_unary_addr();
    std::shared_ptr<DataStruct::Node> read_unary_deref(const DataStruct::Token &);
    std::shared_ptr<DataStruct::Node> read_unary_minus();
    std::shared_ptr<DataStruct::Node> read_unary_bitnot(const DataStruct::Token &);
    std::shared_ptr<DataStruct::Node> read_unary_lognot();
    std::shared_ptr<DataStruct::Node> read_unary_expr();
    std::shared_ptr<DataStruct::Node> read_sizeof_operand();
    std::shared_ptr<DataStruct::Type> read_sizeof_operand_sub();
    std::shared_ptr<DataStruct::Node> read_alignof_operand();
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

    std::shared_ptr<DataStruct::Node> read_expr_opt();
    //Declarations
    std::shared_ptr<DataStruct::Type> read_decl_spec_opt(DataStruct::QUALITIFIER *);

    std::shared_ptr<DataStruct::Type> read_decl_spec(DataStruct::QUALITIFIER *);
    void read_decl(Parser::vec_share *, bool);
    std::shared_ptr<DataStruct::Type> read_abstract_declarator(const std::shared_ptr<DataStruct::Type>&);
    std::shared_ptr<DataStruct::Type> read_declarator_func(const std::shared_ptr<DataStruct::Type>&, Parser::vec_share*);
    std::shared_ptr<DataStruct::Type> read_struct_def();
    std::shared_ptr<DataStruct::Type> read_union_def();
    std::shared_ptr<DataStruct::Type> read_enum_def();
    std::shared_ptr<DataStruct::Type> read_rectype_def(bool);
    std::string read_rectype_tag();
    std::shared_ptr<DataStruct::Node> read_struct_field(const std::shared_ptr<DataStruct::Node> &struc);
    std::vector<std::pair<std::string,std::shared_ptr<DataStruct::Type>>>  read_rectype_fields_sub();
    void fix_rectype_flexible_member(std::vector<std::pair<std::string,std::shared_ptr<DataStruct::Type>>> &);
    std::vector<std::pair<std::string,std::shared_ptr<DataStruct::Type>>> update_struct_offset(int&,int&,std::vector<std::pair<std::string,std::shared_ptr<DataStruct::Type>>>&);
    std::vector<std::pair<std::string,std::shared_ptr<DataStruct::Type>>> update_union_offset(int&,int&,std::vector<std::pair<std::string,std::shared_ptr<DataStruct::Type>>>&);
    void finish_bitfield(int&,int &);
    int compute_padding(int , int );
    void squash_unnamed_struct(std::vector<std::pair<std::string,std::shared_ptr<DataStruct::Type>>>&, std::shared_ptr<DataStruct::Type> &, int ) ;
    int read_bitsize(std::string&,const std::shared_ptr<DataStruct::Type>&);
    void read_static_local_var(const std::shared_ptr<DataStruct::Type>&, const std::string&);
    std::shared_ptr<DataStruct::Type> read_declarator(std::string*,const std::shared_ptr<DataStruct::Type>&,Parser::vec_share*,DataStruct::DECL_TYPE);
    std::vector<std::pair<std::string,std::shared_ptr<DataStruct::Type>>> read_rectype_fields(int &,int &,bool);
    int read_alignas();
    bool is_poweroftwo(int);
    std::shared_ptr<DataStruct::Type> read_cast_type();
    void skip_parentheses(std::vector<DataStruct::Token>&);
    std::shared_ptr<DataStruct::Node> read_func_body(const std::shared_ptr<DataStruct::Type>&functype, const std::string &name, Parser::vec_share &params);
    Parser::vec_share read_oldstyle_param_args();
    void read_oldstyle_param_type(Parser::vec_share *);
    std::vector<std::shared_ptr<DataStruct::Type>> param_types(Parser::vec_share*params);
    void update_oldstyle_param_type(Parser::vec_share  *params, Parser::vec_share &declvars);
    std::shared_ptr<DataStruct::Type> read_func_param_list(Parser::vec_share*, const std::shared_ptr<DataStruct::Type>&);
    std::shared_ptr<DataStruct::Type> read_declarator_tail(const std::shared_ptr<DataStruct::Type>&, Parser::vec_share*);
    void read_declarator_params(std::vector<DataStruct::Type>&, Parser::vec_share*, bool &);
    std::shared_ptr<DataStruct::Type> read_func_param(std::string& ,bool);
    void read_declarator_params_oldstyle(Parser::vec_share*);
    std::shared_ptr<DataStruct::Type> read_declarator_array(const std::shared_ptr<DataStruct::Type> &);
    void skip_type_qualifiers();
    //Initializers
    void assign_string(std::vector<std::shared_ptr<DataStruct::Node>>& ,
                       const std::shared_ptr<DataStruct::Type> &, const std::string &, int );

    bool maybe_read_brace();
    void maybe_skip_comma();
    void skip_to_brace();
    void sort_inits(std::vector<std::shared_ptr<DataStruct::Node>>&);
    void read_initializer_elem(std::vector<std::shared_ptr<DataStruct::Node>>&,
                               const std::shared_ptr<DataStruct::Type> &, int, bool);
    void read_array_initializer(std::vector<std::shared_ptr<DataStruct::Node>>&,
                                const std::shared_ptr<DataStruct::Type> &, int, bool);
    void read_array_initializer_sub(std::vector<std::shared_ptr<DataStruct::Node>>&,
                                    const std::shared_ptr<DataStruct::Type> &, int , bool);
    void read_struct_initializer(std::vector<std::shared_ptr<DataStruct::Node>>&,
                                const std::shared_ptr<DataStruct::Type> &, int, bool);
    void read_struct_initializer_sub(std::vector<std::shared_ptr<DataStruct::Node>>&,
                                     const std::shared_ptr<DataStruct::Type> &, int, bool);
    std::vector<std::shared_ptr<DataStruct::Node>> read_decl_init(const std::shared_ptr<DataStruct::Type> &ty);
    void read_initializer_list(std::vector<std::shared_ptr<DataStruct::Node>>&,
                               const std::shared_ptr<DataStruct::Type> &, int , bool);
    //Statements and Blocks
    std::shared_ptr<DataStruct::Node> read_boolean_expr();

    std::shared_ptr<DataStruct::Node> read_if_stmt();
    std::shared_ptr<DataStruct::Node> read_opt_decl_or_stmt();
    std::shared_ptr<DataStruct::Node> read_for_stmt();
    std::shared_ptr<DataStruct::Node> read_compound_stmt();
    void read_decl_or_stmt(Parser::vec_share &list);
    std::shared_ptr<DataStruct::Node> read_stmt();
    std::shared_ptr<DataStruct::Node> read_label(const DataStruct::Token &tok);
    std::shared_ptr<DataStruct::Node> read_label_tail(const std::shared_ptr<DataStruct::Node> &label);
    std::shared_ptr<DataStruct::Node> read_case_label(const DataStruct::Token &tok);
    void check_case_duplicates();
    std::shared_ptr<DataStruct::Node> read_default_label(const DataStruct::Token &tok);
    std::shared_ptr<DataStruct::Node> read_while_stmt();
    std::shared_ptr<DataStruct::Node> read_do_stmt();
    std::shared_ptr<DataStruct::Node> make_switch_jump(const std::shared_ptr<DataStruct::Node> &var, const Case &c);
    std::shared_ptr<DataStruct::Node> read_switch_stmt();
    std::shared_ptr<DataStruct::Node> read_break_stmt(const DataStruct::Token &tok);
    std::shared_ptr<DataStruct::Node> read_continue_stmt(const DataStruct::Token &tok);
    std::shared_ptr<DataStruct::Node> read_return_stmt();
    std::shared_ptr<DataStruct::Node> read_goto_stmt();

    std::shared_ptr<DataStruct::Node> var_lookup(const std::string& name);
};
auto lower=[](const std::string&s)->std::string {std::string b;for(auto&e:s) b+=tolower(e);return b;};
#endif //YCC_PARSER_H
