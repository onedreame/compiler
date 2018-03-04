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
    std::unordered_map<std::string,std::shared_ptr<DataStruct::Node>>& env(){
        return localenv.size()?localenv:globalenv;
    };
    std::vector<DataStruct::Token>& read_toplevels();
    DataStruct::Token read_funcdef();
private:
    MacroPreprocessor* macro= nullptr;
    Lex *lex= nullptr;
    const int MAX_ALIGN=16;
    DataStruct::SourceLoc sl;   //错误位置
    std::unordered_map<std::string,std::shared_ptr<DataStruct::Node>> globalenv;    //全局变量
    std::unordered_map<std::string,std::shared_ptr<DataStruct::Node>> localenv;     //局部变量
    std::unordered_map<std::string,std::shared_ptr<DataStruct::Type>> tags;         //struct/union/enum
    std::unordered_map<std::string,DataStruct::Token> labels;       //goto label

    std::vector<DataStruct::Token> toplevels;
    std::vector<DataStruct::Token> localvars;
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
    std::shared_ptr<DataStruct::Type> make_rectype(bool);

    bool is_type(const DataStruct::Token&);
    std::shared_ptr<DataStruct::Type> get_typedef(const std::string&);
    void skip_parentheses(std::vector<DataStruct::Token>&);
    std::shared_ptr<DataStruct::Type> read_decl_spec_opt(DataStruct::QUALITIFIER &);
    std::shared_ptr<DataStruct::Type> read_decl_spec(DataStruct::QUALITIFIER &);
    std::shared_ptr<DataStruct::Type> read_struct_def();
    std::shared_ptr<DataStruct::Type> read_union_def();
    std::shared_ptr<DataStruct::Type> read_enum_def();
    std::shared_ptr<DataStruct::Type> read_rectype_def(bool);
    std::string read_rectype_tag();
};
#endif //YCC_PARSER_H
