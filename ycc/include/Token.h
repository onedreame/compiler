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
//#include "macro.h"
class MacroPreprocessor;
namespace DataStruct {
    struct File{
        std::shared_ptr<std::ifstream> file;  // 文件流
        std::string p;     // 字符串流
        std::string name;  //文件名
        int cur;
        int line;
        int column;
        int ntok;     // 该流中有多少个token
        int last;     // 从流中读取的最后一个字符
        time_t mtime; // 最后修改时间
    } ;

    struct SourceLoc{
        char *file;
        int line;
    } ;

    struct Type {
        Type(DataStruct::TYPE_KIND k,int s=0,int a=0,bool ui=false):kind(k),size(s),align(a),usig(ui){}
        Type()= default;
        DataStruct::TYPE_KIND kind;
        int size;
        int align;
        bool usig; // 是否是unsigned
        bool isstatic;
        // pointer or array
        Type *ptr;
        // array length
        int len;
        // struct
        std::map<std::map<const char *, void *>, std::vector<void *>> *fields;
        int offset;
        bool is_struct; // true：struct, false： union
        // bitfield
        int bitoff;
        int bitsize;
        Type *rettype;   //函数返回类型
        std::vector<void *> *params;
        bool hasva;
        bool oldstyle;
    };

    struct Token{
        TOKEN_TYPE kind;
        std::shared_ptr<File> file;
        int line=1;
        int column=1;
        bool space= false;   // 该token是否有个前导空格
        bool bol= false;     // 该token是否是一行的开头
        int count=0;    // token number in a file, counting from 0.
        std::set<std::string > hideset; // 宏展开
        // TKEYWORD
        AST_TYPE id;
        // TSTRING or TCHAR
        std::shared_ptr<std::string> sval;
        int c='\0';
        DataStruct::ENCODE enc=DataStruct::ENCODE::ENC_NONE;
        // TMACRO_PARAM
        bool is_vararg= false;   //是否是可变参数宏
        int position=-1;        //对于函数宏，代表该token是第几个参数
        Token()= default;

        Token(DataStruct::TOKEN_TYPE id,std::string str="",int c='\0',DataStruct::ENCODE enc=DataStruct::ENCODE::ENC_NONE):kind(id),sval(std::make_shared<std::string>(str)),c(c),enc(enc){}
        Token(DataStruct::TOKEN_TYPE id, bool is_var= false,int pos=-1):kind(id),is_vararg(is_var),position(pos){}
        Token(DataStruct::TOKEN_TYPE id,DataStruct::AST_TYPE id2):kind(id),id(id2){}
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
        Macro()= default;
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

    class Node{
    public:


    private:
        enum {CIL,   //char int or long
              FD,    //float or double
              STR,   //string
              LGV,   //local/global 变量
              BIOP,  //binary op
              UNOP,  //unary op
              FCFD,  //函数调用或声明
              DEC,   //声明
              INIT,  //初始化
              IFTOP, //if语句或ternary op
              GOLA,  //goto label
              RET,   //return 语句
              COMPO, //compound 语句
              STRREF,//struct引用
        } tok;
        union {
            // char int or long
            long ival;
            // float or double
            struct {
                double fval;
                char *flabel;
            };
            // string
            struct {
                char *sval;
                char *slabel;
            };
            // local/global 变量
            struct {
                char *varname;
                // local
                int loff;
                std::vector<DataStruct::Token> lvarinit;
                // global
                char *glabel;
            };
            // binary op
            struct {
                Node *left;
                Node *right;
            };
            // unary op
            struct {
                Node *operand;
            };
            // 函数调用或声明
            struct {
                char *fname;
                // Function call
                std::vector<DataStruct::Token> args;
                struct Type *ftype;
                // Function pointer or function designator
                struct Node *fptr;
                // Function declaration
                std::vector<DataStruct::Token> params;
                std::vector<DataStruct::Token> localvars;
                struct Node *body;
            };
            // 声明
            struct {
                struct Node *declvar;
                std::vector<DataStruct::Token> declinit;
            };
            // 初始化
            struct {
                Node *initval;
                int initoff;
                Type *totype;
            };
            // if语句或ternary op
            struct {
                Node *cond;
                Node *then;
                Node *els;
            };
            // goto label
            struct {
                char *label;
                char *newlabel;
            };
            // return 语句
            Node *retval;
            // compound 语句
            std::vector<DataStruct::Token> stmts;
            // struct引用
            struct {
                Node *struc;
                std::string field;
                Type *fieldtype;
            };
        };
    };
    typedef struct Node {
        int kind;
        Type *ty;
        SourceLoc *sourceLoc;
        union {
            // Char, int, or long
            long ival;
            // Float or double
            struct {
                double fval;
                char *flabel;
            };
            // String
            struct {
                char *sval;
                char *slabel;
            };
            // Local/global variable
            struct {
                char *varname;
                // local
                int loff;
                Vector *lvarinit;
                // global
                char *glabel;
            };
            // Binary operator
            struct {
                struct Node *left;
                struct Node *right;
            };
            // Unary operator
            struct {
                struct Node *operand;
            };
            // Function call or function declaration
            struct {
                char *fname;
                // Function call
                Vector *args;
                struct Type *ftype;
                // Function pointer or function designator
                struct Node *fptr;
                // Function declaration
                Vector *params;
                Vector *localvars;
                struct Node *body;
            };
            // Declaration
            struct {
                struct Node *declvar;
                Vector *declinit;
            };
            // Initializer
            struct {
                struct Node *initval;
                int initoff;
                Type *totype;
            };
            // If statement or ternary operator
            struct {
                struct Node *cond;
                struct Node *then;
                struct Node *els;
            };
            // Goto and label
            struct {
                char *label;
                char *newlabel;
            };
            // Return statement
            struct Node *retval;
            // Compound statement
            Vector *stmts;
            // Struct reference
            struct {
                struct Node *struc;
                char *field;
                Type *fieldtype;
            };
        };
    } Node;
}
#endif //YCC_TOKEN_H
