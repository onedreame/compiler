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
    std::unordered_map<std::string,DataStruct::Token> env(){
        return localenv.size()?localenv:globalenv;
    };

private:
    MacroPreprocessor* macro= nullptr;
    Lex *lex= nullptr;
    const int MAX_ALIGN=16;
    DataStruct::SourceLoc sl;   //错误位置
    std::unordered_map<std::string,DataStruct::Token> globalenv;    //全局变量
    std::unordered_map<std::string,DataStruct::Token> localenv;     //局部变量
    std::unordered_map<std::string,DataStruct::Token> tags;         //struct/union/enum
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
    const DataStruct::Type type_void = DataStruct::Type(DataStruct::TYPE_KIND::KIND_VOID, 0, 0, false );
    const DataStruct::Type type_bool = DataStruct::Type(DataStruct::TYPE_KIND::KIND_BOOL, 1, 1, true );
    const DataStruct::Type type_char = DataStruct::Type(DataStruct::TYPE_KIND::KIND_CHAR, 1, 1, false );
    const DataStruct::Type type_short = DataStruct::Type( DataStruct::TYPE_KIND::KIND_SHORT, 2, 2, false );
    const DataStruct::Type type_int = DataStruct::Type( DataStruct::TYPE_KIND::KIND_INT, 4, 4, false );
    const DataStruct::Type type_long = DataStruct::Type( DataStruct::TYPE_KIND::KIND_LONG, 8, 8, false );
    const DataStruct::Type type_llong = DataStruct::Type( DataStruct::TYPE_KIND::KIND_LLONG, 8, 8, false );
    const DataStruct::Type type_uchar = DataStruct::Type( DataStruct::TYPE_KIND::KIND_CHAR, 1, 1, true );
    const DataStruct::Type type_ushort = DataStruct::Type( DataStruct::TYPE_KIND::KIND_SHORT, 2, 2, true );
    const DataStruct::Type type_uint = DataStruct::Type( DataStruct::TYPE_KIND::KIND_INT, 4, 4, true );
    const DataStruct::Type type_ulong = DataStruct::Type( DataStruct::TYPE_KIND::KIND_LONG, 8, 8, true );
    const DataStruct::Type type_ullong = DataStruct::Type( DataStruct::TYPE_KIND::KIND_LLONG, 8, 8, true );
    const DataStruct::Type type_float = DataStruct::Type( DataStruct::TYPE_KIND::KIND_FLOAT, 4, 4, false );
    const DataStruct::Type type_double = DataStruct::Type( DataStruct::TYPE_KIND::KIND_DOUBLE, 8, 8, false );
    const DataStruct::Type type_ldouble = DataStruct::Type( DataStruct::TYPE_KIND::KIND_LDOUBLE, 8, 8, false );
    const DataStruct::Type type_enum = DataStruct::Type( DataStruct::TYPE_KIND::KIND_ENUM, 4, 4, false );
};
#endif //YCC_PARSER_H
