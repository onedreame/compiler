//
// Created by yk on 18-2-8.
//

#ifndef YCC_ENUMERATE_H
#define YCC_ENUMERATE_H
namespace DataStruct
{
    enum class TOKEN_TYPE:char{
        TIDENT,
        TKEYWORD,
        TNUMBER,
        TCHAR,
        TSTRING,
        TEOF,
        TINVALID,
        // Only in CPP
        MIN_CPP_TOKEN,
        TNEWLINE,
        TSPACE,
        TMACRO_PARAM,
        //占位符，代表非法情况
        TPLACEHOLDER,
    };
//c11 6.4.5 String literals
    enum class ENCODE:int{
        ENC_NONE,
        ENC_CHAR16,
        ENC_CHAR32,
        ENC_UTF8,
        ENC_WCHAR,
    };

    enum class CondInclCtx:char{ IN_THEN, IN_ELIF, IN_ELSE };
    enum class MacroType:char{ MACRO_OBJ, MACRO_FUNC, MACRO_SPECIAL,MACRO_INVALID };

    enum class AST_TYPE{
        AST_LITERAL = 256,
        AST_LVAR,
        AST_GVAR,
        AST_TYPEDEF,
        AST_FUNCALL,
        AST_FUNCPTR_CALL,
        AST_FUNCDESG,
        AST_FUNC,
        AST_DECL,
        AST_INIT,
        AST_CONV,
        AST_ADDR,
        AST_DEREF,
        AST_IF,
        AST_TERNARY,
        AST_DEFAULT,
        AST_RETURN,
        AST_COMPOUND_STMT,
        AST_STRUCT_REF,
        AST_GOTO,
        AST_COMPUTED_GOTO,
        AST_LABEL,
        OP_SIZEOF,
        OP_CAST,
        OP_SHR,
        OP_SHL,
        OP_A_SHR,
        OP_A_SHL,
        OP_PRE_INC,
        OP_PRE_DEC,
        OP_POST_INC,
        OP_POST_DEC,
        OP_LABEL_ADDR,
#define op(name, _) name,
#define keyword(name, x, y) name,
#include "keyword.inc"
#undef keyword
#undef op
        LSQUARE_BRACKET='[',
        RSQUARE_BRACKET=']',
        LPARENTHESE='(',
        RPARENTHESE=')',
        LBRACE='{',
        RBRACE='}',
        DOT='.',
        COMMA=',',
        COLON=':',
        PLUS='+',
        SUB='-',
        MUL='*',
        DIV='/',
        ASSIGN='=',
        EXCLAMATION='!',
        AND='&',
        OR='|',
        NOT='^',
        NEG='~',
        QUES='?',
        SEMI=';',
        LOW='<',
        HIG='>',
        LEFT='%',
        AST_PLACEHOLDER,
    };

    enum class TYPE_KIND{
        KIND_VOID,
        KIND_BOOL,
        KIND_CHAR,
        KIND_SHORT,
        KIND_INT,
        KIND_LONG,
        KIND_LLONG,
        KIND_FLOAT,
        KIND_DOUBLE,
        KIND_LDOUBLE,
        KIND_ARRAY,
        KIND_ENUM,
        KIND_PTR,
        KIND_STRUCT,
        KIND_FUNC,
        // 占位符
        KIND_PLACEHOLDER,
    };
    enum class QUALITIFIER{
        S_PLACEHOLDER=0,
        S_TYPEDEF = 1,
        S_EXTERN,
        S_STATIC,
        S_AUTO,
        S_REGISTER,
    };

    enum class DECL_TYPE{
        DECL_PLACEHOLDER=0,
        DECL_BODY = 1,     //函数体
        DECL_PARAM,        //参数
        DECL_PARAM_TYPEONLY,    //参数
        DECL_CAST,     //typeof
    };

    enum class NODETYPE{
        CIL,   //char int or long
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
    };
}
#endif //YCC_ENUMERATE_H
