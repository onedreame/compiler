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
    };

    enum {
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
        // used only in parser
                KIND_STUB,
    };
}
#endif //YCC_ENUMERATE_H
