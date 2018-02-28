//
// Created by yk on 17-12-31.
//

#ifndef YCC_TOKEN_H
#define YCC_TOKEN_H

#include <cstdio>
#include <ctime>
#include <set>
#include <map>
#include <vector>
#include <fstream>
#include <memory>
#include "enumerate.h"
#include "macro.h"

namespace DataStruct {
    using File=struct{
        std::shared_ptr<std::ifstream> file;  // stream backed by FILE *
        std::string p;     // stream backed by string
        std::string name;  //文件名
        int cur;
        int line;
        int column;
        int ntok;     // token counter
        int last;     // the last character read from file
        time_t mtime; // last modified time. 0 if string-backed file
    } ;

    typedef struct {
        char *file;
        int line;
    } SourceLoc;

    typedef struct Type {
        int kind;
        int size;
        int align;
        bool usig; // true if unsigned
        bool isstatic;
        // pointer or array
        struct Type *ptr;
        // array length
        int len;
        // struct
        std::map<std::map<const char *, void *>, std::vector<void *>> *fields;
        int offset;
        bool is_struct; // true if struct, false if union
        // bitfield
        int bitoff;
        int bitsize;
        // function
        struct Type *rettype;
        std::vector<void *> *params;
        bool hasva;
        bool oldstyle;
    } Type;

    using Token= struct token{
        DataStruct::TOKEN_TYPE kind;
        std::shared_ptr<DataStruct::File> file;
        int line=1;
        int column=1;
        bool space= false;   // 该token是否有个前导空格
        bool bol= false;     // 该token是否是一行的开头
        int count=0;    // token number in a file, counting from 0.
        std::set<std::string > hideset; // 宏展开
        // TKEYWORD
        DataStruct::AST_TYPE id;
        // TSTRING or TCHAR
        std::shared_ptr<std::string> sval;
        int c='\0';
        DataStruct::ENCODE enc=DataStruct::ENCODE::ENC_NONE;
        // TMACRO_PARAM
        bool is_vararg= false;   //是否是可变参数宏
        int position=-1;        //对于函数宏，代表该token是第几个参数
        token()= default;

        token(DataStruct::TOKEN_TYPE id,std::string str="",int c='\0',DataStruct::ENCODE enc=DataStruct::ENCODE::ENC_NONE):kind(id),sval(std::make_shared<std::string>(str)),c(c),enc(enc){}
        token(DataStruct::TOKEN_TYPE id, bool is_var= false,int pos=-1):kind(id),is_vararg(is_var),position(pos){}
        token(DataStruct::TOKEN_TYPE id,DataStruct::AST_TYPE id2):kind(id),id(id2){}
    };

    //条件宏的展开用的结构体：
    // ctx： 处于哪种条件宏中，如#if，#else，#elif等
    //include_guard:包含哨
    //file：存在包含哨的时候才可用，代表包含的文件
    //wastrue：该处代码块是否需要展开
    using CondIncl=struct {
        DataStruct::CondInclCtx ctx;
        std::string include_guard;
        std::shared_ptr<DataStruct::File> file;
        bool wastrue;
    } ;

    //宏展开相应结构体
    //kind：宏的类型，可选值为
    //    MACRO_OBJ:对象宏
    //    MACRO_FUNC：函数宏
    //    MACRO_SPECIAL：特殊宏，为c语言内置的宏
    //nargs：对于函数宏代表起参数数目
    //body：参数实体
    class Macro{
    public:
        Macro &operator=(const Macro&t) = default;
        Macro(const Macro&)= default;
        Macro &operator=(Macro&&macro1)noexcept{
            kind=macro1.getKind();
            nargs=macro1.getNargs();
            is_varg=macro1.isIs_varg();
            fn=macro1.getFn();
            using itertype=decltype(macro1.getBody().begin());
            for (itertype beg=macro1.getBody().begin(),endg=macro1.getBody().end();beg!=endg;++beg)
                body.push_back(std::move(*beg));
        }
        Macro(Macro&&macro1)noexcept :kind(macro1.getKind()),nargs(macro1.getNargs()),is_varg(macro1.isIs_varg()),fn(macro1.getFn()){
            using itertype=decltype(macro1.getBody().begin());
            for (itertype beg=macro1.getBody().begin(),endg=macro1.getBody().end();beg!=endg;++beg)
                body.push_back(std::move(*beg));
        }
        Macro(DataStruct::MacroType t,std::vector<DataStruct::Token > body,bool is=false,int nargs=0):kind(t),body(body),is_varg(is),nargs(nargs){}
        Macro(DataStruct::MacroType t,std::function<void(MacroPreprocessor* ,const DataStruct::Token &)> f= nullptr):kind(t),fn(f){}

        DataStruct::MacroType getKind() const {
            return kind;
        }

        void setKind(DataStruct::MacroType kind) {
            this->kind = kind;
        }

        int getNargs() const {
            return nargs;
        }

        void setNargs(int nargs) {
            this->nargs = nargs;
        }

        const std::vector<DataStruct::Token > &getBody() const {
            return body;
        }

        void setBody(std::vector<DataStruct::Token> body) {
            this->body = body;
        }

        bool isIs_varg() const {
            return is_varg;
        }

        void setIs_varg(bool is_varg) {
            this->is_varg = is_varg;
        }

        const std::function<void(MacroPreprocessor* ,const DataStruct::Token &)> &getFn() const {
            return fn;
        }

        void setFn(const std::function<void(MacroPreprocessor* ,const DataStruct::Token &)> &fn) {
            this->fn = fn;
        }

    private:
        DataStruct::MacroType kind=DataStruct::MacroType::MACRO_INVALID;
        int nargs=0;
        std::vector<DataStruct::Token > body;
        bool is_varg= false;
        std::function<void (MacroPreprocessor* ,const DataStruct::Token&)> fn= nullptr;
    };
}
#endif //YCC_TOKEN_H
