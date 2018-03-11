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
#include <unordered_map>
#include "enumerate.h"
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
        std::string file;
        int line;
    } ;

    struct Type {
        Type(DataStruct::TYPE_KIND k,int s=0,int a=0,bool ui=false):kind(k),size(s),align(a),usig(ui){}
        Type(DataStruct::TYPE_KIND k,bool is_struct=false):kind(k),is_struct(is_struct){}
        Type(DataStruct::TYPE_KIND k,int s,int a,const std::shared_ptr<Type>& ptr,int len=0):kind(k),size(s),align(a),ptr(ptr),len(len){}
        Type(DataStruct::TYPE_KIND k,const std::shared_ptr<DataStruct::Type>&ret,const std::vector<std::shared_ptr<Type>>& params,bool hasva,
             bool oldstype):kind(k),rettype(ret),params(params),hasva(hasva),oldstyle(oldstype){}
        Type()= default;
        Type(const Type&)= default;
        DataStruct::TYPE_KIND kind;
        int size;       //该type所占字节数
        int align;
        bool usig; // 是否是unsigned
        bool isstatic;
        // pointer or array
        std::shared_ptr<Type> ptr= nullptr;
        // array length
        int len;
        // struct
        std::vector<std::pair<std::string, std::shared_ptr<Type>>> fields;
        int offset;
        bool is_struct; // true：struct, false： union
        // bitfield
        int bitoff;
        int bitsize;
        std::shared_ptr<Type> rettype;   //函数返回类型
        std::vector<std::shared_ptr<Type>> params;        //函数参数
        bool hasva;                      //是否有参数
        //http://en.cppreference.com/w/c/language/function_declaration
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
        Token(const Token&)= default;
        Token&operator=(const Token&)= default;
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
    class Node;

    struct CTL_Node{
//        CTL_Node():BASE_Node(NODETYPE::CIL){}
        long ival;

    };
    struct FD_Node{
//        FD_Node():BASE_Node(NODETYPE::FD){}
        double fval;
        std::string flabel;
    };
    struct STR_Node{
//        STR_Node():BASE_Node(NODETYPE::STR){}
        std::string sval;
        std::string slabel;
    };
    struct LGV_Node{
//        LGV_Node():BASE_Node(NODETYPE::LGV){}
        std::string varname;
        // local
        int loff;
        std::vector<DataStruct::Token> lvarinit;
        // global
        std::string glabel;
    };
    struct BIOP_Node{
//        BIOP_Node():BASE_Node(NODETYPE::BIOP){}
        std::shared_ptr<Node> left;
        std::shared_ptr<Node> right;
    };
    struct RET_Node{
//        RET_Node():BASE_Node(NODETYPE::RET){}
        std::shared_ptr<Node> retval;
    };
    struct FCFD_Node{
//        FCFD_Node():BASE_Node(NODETYPE::FCFD){}
        std::string fname;
        // Function call
        std::vector<Token> args;
        std::shared_ptr<Type> ftype;
        // Function pointer or function designator
        std::shared_ptr<Node> fptr;
        // Function declaration
        std::vector<Token> params;
        std::vector<Token> localvars;
        std::shared_ptr<Node> body;
    };
    struct DEC_Node{
//        DEC_Node():BASE_Node(NODETYPE::DEC){}
        std::shared_ptr<Node> declvar;
        std::vector<Token> declinit;
    };
    struct INIT_Node{
//        INIT_Node():BASE_Node(NODETYPE::INIT){}
        std::shared_ptr<Node> initval;
        int initoff;
        std::shared_ptr<Type> totype;
    };
    struct IFTOP_Node{
//        IFTOP_Node():BASE_Node(NODETYPE::IFTOP){}
        std::shared_ptr<Node> cond;
        std::shared_ptr<Node> then;
        std::shared_ptr<Node> els;
    };
    struct STRREF_Node{
//        STRREF_Node():BASE_Node(NODETYPE::STRREF){}
        std::shared_ptr<Node> struc;
        std::string field;
        std::shared_ptr<Type> fieldtype;
    };
    struct COMPO_Node{
//        COMPO_Node():BASE_Node(NODETYPE::COMPO){}
        std::vector<DataStruct::Token> stmts;
    };
    struct GOLA_Node{
//        GOLA_Node():BASE_Node(NODETYPE::GOLA){}
        std::string label;
        std::string newlabel;
    };
    struct UNOP_Node{
//        UNOP_Node():BASE_Node(NODETYPE::UNOP){}
        std::shared_ptr<Node> unop;
    };


    class Node{
    public:
//        using Fd =struct {
//            double fval;
//            std::string flabel;
//        }; // float or double
//        using Str=struct {
//            std::string sval;
//            std::string slabel;
//        };// string
//        using LGv=struct {
//            std::string varname;
//            // local
//            int loff;
//            std::vector<DataStruct::Token> lvarinit;
//            // global
//            std::string glabel;
//        };            // local/global 变量
//        using Biop=struct {
//            std::shared_ptr<Node> left;
//            std::shared_ptr<Node> right;
//        };      // binary op
//        using Fcfd=struct {
//            std::string fname;
//            // Function call
//            std::vector<Token> args;
//            std::shared_ptr<Type> ftype;
//            // Function pointer or function designator
//            std::shared_ptr<Node> fptr;
//            // Function declaration
//            std::vector<Token> params;
//            std::vector<Token> localvars;
//            std::shared_ptr<Node> body;
//        };// 函数调用或声明
//        using Dec=struct {
//            std::shared_ptr<Node> declvar;
//            std::vector<Token> declinit;
//        };    // 声明
//        using Init=struct {
//            std::shared_ptr<Node> initval;
//            int initoff;
//            std::shared_ptr<Type> totype;
//        };  // 初始化
//        using Iftop=struct {
//            std::shared_ptr<Node> cond;
//            std::shared_ptr<Node> then;
//            std::shared_ptr<Node> els;
//        };// if语句或ternary op
//        using Gola=struct {
//            std::string label;
//            std::string newlabel;
//        };// goto label
//        using Strref=struct {
//            std::shared_ptr<Node> struc;
//            std::string field;
//            std::shared_ptr<Type> fieldtype;
//        };// struct引用
        Node():tok(NODETYPE::CIL),ival(0){}
        Node(const Node&r)= default;
        Node&operator=(const Node&)= default;
        Node(AST_TYPE k,const std::shared_ptr<Type>&ty, const SourceLoc& sor):kind(k),ty(ty),sourceloc(sor){}
//        ~Node(){clear();}
//        static Node make_CIL_node(long);
//        static Node make_FD_node(double, const std::string&);
//        static Node make_STR_node(const std::string&, const std::string&);
//        static Node make_LGV_node(const std::string&,int , const std::vector<Token>&, const std::string&);
//        static Node make_BIOP_node(const std::shared_ptr<Node>&, const std::shared_ptr<Node>& );
//        static Node make_RET_node(const std::shared_ptr<Node>&);
//        static Node make_FCFD_node(const std::string&, const std::vector<Token>&,const std::shared_ptr<Type>&, std::shared_ptr<Node>&,const std::vector<Token>&,
//                const std::vector<Token>&, const std::shared_ptr<Node>&);
//        static Node make_DEC_node(const std::shared_ptr<Node>&, const std::vector<Token>&);
//        static Node make_INIT_node(const std::shared_ptr<Node>&,int, const std::shared_ptr<Type>&);
//        static Node make_IFTOP_node(const std::shared_ptr<Node>&, const std::shared_ptr<Node>&, const std::shared_ptr<Node>&);
//        static Node make_STRREF_node(const std::shared_ptr<Node>&, const std::string&, const std::shared_ptr<Type>&);
//        static Node make_COMPO_node(const std::vector<Token>&);
//        static Node make_GOLA_node(const std::string&, const std::string&);
//        static Node make_UNOP_node(const std::shared_ptr<Node>&);


        long getIval() const;

        void setIval(long);

        AST_TYPE getKind() const;

        void setKind(AST_TYPE);

        const std::shared_ptr<Type> &getTy() const;

        void setTy(const std::shared_ptr<Type> &ty);

        const SourceLoc &getSourceloc() const;

        void setSourceloc(const SourceLoc &sourceloc);


//        union {
//        std::shared_ptr<FD_Node> fd;
//        std::shared_ptr<STR_Node> str;
//        std::shared_ptr<LGV_Node> lgv;
//        std::shared_ptr<BIOP_Node> biop;
//        std::shared_ptr<FCFD_Node> fcfd;
//        std::shared_ptr<DEC_Node> dec;
//        std::shared_ptr<INIT_Node> init;
//        std::shared_ptr<IFTOP_Node> iftop;
//        std::shared_ptr<STRREF_Node> strref;
//        std::shared_ptr<GOLA_Node> gola;
//            std::shared_ptr<Strref> strref;
//            std::shared_ptr<Gola> gola;
//            std::shared_ptr<Iftop> iftop;
//            std::shared_ptr<Init> init;
//            std::shared_ptr<Dec> dec;
//            std::shared_ptr<Fcfd> fcfd;
//            std::shared_ptr<Node> unop;
//            std::shared_ptr<Biop> biop;
//            std::shared_ptr<LGv> lgv;
//            std::shared_ptr<Str> str;
//            std::shared_ptr<Fd> fd;
//        };
        //uanry操作
        std::shared_ptr<Node> unop;
        // compound 语句
        std::vector<std::shared_ptr<DataStruct::Node>> stmts;
        // float or double
        double fval;
        std::string flabel;
        // char int or long
        long ival;
        // return 语句
        std::shared_ptr<Node> retval;
        // string
        std::string sval;
        std::string slabel;
        // local/global 变量
        std::string varname;
        // local
        int loff;
        std::vector<std::shared_ptr<Node>> lvarinit;
        // global
        std::string glabel;
        // binary op
        std::shared_ptr<Node> left;
        std::shared_ptr<Node> right;
        // 函数调用或声明
        std::string fname;
        // Function call
        std::vector<Node> args;
        std::shared_ptr<Type> ftype;
        // Function pointer or function designator
        std::shared_ptr<Node> fptr;
        // Function declaration
        std::vector<Node> params;
        std::vector<Node> localvars;
        std::shared_ptr<Node> body;
        // 声明
        std::shared_ptr<Node> declvar;
        std::vector<std::shared_ptr<Node>> declinit;
        // 初始化
        std::shared_ptr<Node> initval;
        int initoff;
        std::shared_ptr<Type> totype;
        // if语句或ternary op
        std::shared_ptr<Node> cond;
        std::shared_ptr<Node> then;
        std::shared_ptr<Node> els;
        // goto label
        std::string label;
        std::string newlabel;
        // struct引用
        std::shared_ptr<Node> struc;
        std::string field;
        std::shared_ptr<Type> fieldtype;
    private:
        AST_TYPE kind;
        std::shared_ptr<Type> ty;
        SourceLoc sourceloc;
        NODETYPE tok;
//        Node& copyUnion(const Node& r);
//        void clear();
    };
}
#endif //YCC_TOKEN_H
