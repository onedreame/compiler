//
// Created by yk on 18-2-8.
//

#include "../include/parser.h"
#include <algorithm>

std::shared_ptr<DataStruct::Type> Parser::make_rectype(bool is_struct){
    return std::make_shared<DataStruct::Type>(DataStruct::TYPE_KIND::KIND_STRUCT,is_struct);
}
//构建basic type的内存大小，对齐方式， C11 6.2.5 types
std::shared_ptr<DataStruct::Type> Parser::make_numtype(DataStruct::TYPE_KIND kind, bool usig) {
    std::shared_ptr<DataStruct::Type> r=std::make_shared<DataStruct::Type>(kind,0,0,usig);
    switch (kind){
        case DataStruct::TYPE_KIND::KIND_VOID:
            r->size = r->align = 0;
            break;
        case DataStruct::TYPE_KIND::KIND_BOOL:
            r->size = r->align = 1;
            break;
        case DataStruct::TYPE_KIND::KIND_CHAR:
            r->size = r->align = 1;
            break;
        case DataStruct::TYPE_KIND::KIND_SHORT:
            r->size = r->align = 2;
            break;
        case DataStruct::TYPE_KIND::KIND_INT:
            r->size = r->align = 4;
            break;
        case DataStruct::TYPE_KIND::KIND_LONG:
            r->size = r->align = 8;
            break;
        case DataStruct::TYPE_KIND::KIND_LLONG:
            r->size = r->align = 8;
            break;
        case DataStruct::TYPE_KIND::KIND_FLOAT:
            r->size = r->align = 4;
            break;
        case DataStruct::TYPE_KIND::KIND_DOUBLE:
            r->size = r->align = 8;
            break;
        case DataStruct::TYPE_KIND::KIND_LDOUBLE:
            r->size = r->align = 8;
            break;
        default:
            Error::error("internal error");;
    }
    return r;
}
std::shared_ptr<DataStruct::Type> Parser::make_func_type(const std::shared_ptr<DataStruct::Type> &rettype, const std::vector<DataStruct::Type> &paramtypes, bool has_varargs, bool oldstyle) {
    return std::make_shared<DataStruct::Type>(DataStruct::TYPE_KIND::KIND_FUNC,rettype,paramtypes,has_varargs,oldstyle);
}
std::shared_ptr<DataStruct::Type> Parser::make_ptr_type(const std::shared_ptr<DataStruct::Type> &ty) {
    return std::make_shared<DataStruct::Type>(DataStruct::TYPE_KIND::KIND_PTR,8,8,ty);
}
std::shared_ptr<DataStruct::Type> Parser::make_array_type(std::shared_ptr<DataStruct::Type>&type, int len){
    int size;
    if (len < 0)
        size = -1;
    else
        size = type->size * len;
    return std::make_shared<DataStruct::Type>(DataStruct::TYPE_KIND::KIND_ARRAY,size,type->align,type,len);
}
//标记出错文件名和行号
DataStruct::SourceLoc& Parser::mark_location() {
    CHECK_CPP();
    DataStruct::Token tok = macro->peek_token();
    sl={tok.file->name,tok.line};
    return sl;
}

char *make_tempname() {
    static int c = 0;
    return format(".T%d", c++);
}

char *make_label() {
    static int c = 0;
    return format(".L%d", c++);
}

static char *make_static_label(char *name) {
    static int c = 0;
    return format(".S%d.%s", c++, name);
}

static Case *make_case(int beg, int end, char *label) {
    Case *r = malloc(sizeof(Case));
    r->beg = beg;
    r->end = end;
    r->label = label;
    return r;
}

bool Parser::is_inttype(const std::shared_ptr<DataStruct::Type> &ty) {
    switch (ty->kind) {
        case DataStruct::TYPE_KIND::KIND_BOOL: case DataStruct::TYPE_KIND::KIND_CHAR:
        case DataStruct::TYPE_KIND::KIND_SHORT: case DataStruct::TYPE_KIND::KIND_INT:
        case DataStruct::TYPE_KIND::KIND_LONG: case DataStruct::TYPE_KIND::KIND_LLONG:
            return true;
        default:
            return false;
    }
}

bool Parser::is_flotype(const std::shared_ptr<DataStruct::Type> &ty) {
    switch (ty->kind) {
        case DataStruct::TYPE_KIND::KIND_FLOAT: case DataStruct::TYPE_KIND::KIND_DOUBLE:
        case DataStruct::TYPE_KIND::KIND_LDOUBLE:
            return true;
        default:
            return false;
    }
}

bool Parser::is_arithtype(const std::shared_ptr<DataStruct::Type> &ty) {
    return is_inttype(ty) || is_flotype(ty);
}

bool Parser::is_string(const std::shared_ptr<DataStruct::Type> &ty) {
    return ty->kind == DataStruct::TYPE_KIND::KIND_ARRAY && ty->ptr->kind == DataStruct::TYPE_KIND::KIND_CHAR;
}

int Parser::eval_struct_ref(const std::shared_ptr<DataStruct::Node> &node, int offset) {
    if (node->getKind() == DataStruct::AST_TYPE::AST_STRUCT_REF)
        return eval_struct_ref(node->struc, node->getTy()->offset + offset);
    return eval_intexpr(node, NULL) + offset;
}

int Parser::eval_intexpr(const std::shared_ptr<DataStruct::Node> &node, const std::shared_ptr<DataStruct::Node> *addr) {
    switch (node->getKind()) {
        case DataStruct::AST_TYPE::AST_LITERAL:
            if (is_inttype(node->getTy()))
                return node->ival;
            Error::error("Integer expression expected, but got %s", node2s(node));
        case '!': return !eval_intexpr(node->operand, addr);
        case '~': return ~eval_intexpr(node->operand, addr);
        case OP_CAST: return eval_intexpr(node->operand, addr);
        case AST_CONV: return eval_intexpr(node->operand, addr);
        case AST_ADDR:
            if (node->operand->kind == AST_STRUCT_REF)
                return eval_struct_ref(node->operand, 0);
            // fallthrough
        case AST_GVAR:
            if (addr) {
                *addr = conv(node);
                return 0;
            }
            goto error;
            goto error;
        case AST_DEREF:
            if (node->operand->ty->kind == KIND_PTR)
                return eval_intexpr(node->operand, addr);
            goto error;
        case AST_TERNARY: {
            long cond = eval_intexpr(node->cond, addr);
            if (cond)
                return node->then ? eval_intexpr(node->then, addr) : cond;
            return eval_intexpr(node->els, addr);
        }
#define L (eval_intexpr(node->left, addr))
#define R (eval_intexpr(node->right, addr))
        case '+': return L + R;
        case '-': return L - R;
        case '*': return L * R;
        case '/': return L / R;
        case '<': return L < R;
        case '^': return L ^ R;
        case '&': return L & R;
        case '|': return L | R;
        case '%': return L % R;
        case OP_EQ: return L == R;
        case OP_LE: return L <= R;
        case OP_NE: return L != R;
        case OP_SAL: return L << R;
        case OP_SAR: return L >> R;
        case OP_SHR: return ((unsigned long)L) >> R;
        case OP_LOGAND: return L && R;
        case OP_LOGOR:  return L || R;
#undef L
#undef R
        default:
        error:
            Error::error("Integer expression expected, but got %s", node2s(node));
    }
}
int Parser::read_intexpr() {
    return eval_intexpr(read_conditional_expr(), NULL);
}

std::shared_ptr<DataStruct::Node> Parser::ast_lvar(const std::shared_ptr<DataStruct::Type> &ty, const std::string &name) {
    std::shared_ptr<DataStruct::Node> r=std::make_shared<DataStruct::Node>(DataStruct::AST_TYPE::AST_LVAR,ty,mark_location());
    r->lgv=std::make_shared<DataStruct::LGV_Node>();
    r->lgv->varname=name;
    if (localenv)
        localenv->operator[](name)= r;
    if (localvars)
        localvars->push_back(*r);
    return r;
}
std::shared_ptr<DataStruct::Node> Parser::ast_inttype(const std::shared_ptr<DataStruct::Type> &ty, long val) {
    std::shared_ptr<DataStruct::Node> r=std::make_shared<DataStruct::Node>();
    r->ival=val;
    r->setKind(DataStruct::AST_TYPE::AST_LITERAL);
    return r;
}
//c中的类型，可以是关键字，auto，const，char等，或者采用typedef自定义的类型
bool Parser::is_type(const DataStruct::Token&tok){
    if (tok.kind == DataStruct::TOKEN_TYPE::TIDENT)
        return get_typedef(*(tok.sval))!= nullptr;
    if (tok.kind != DataStruct::TOKEN_TYPE::TKEYWORD)
        return false;
    switch (tok.id) {
#define op(x, y)
#define keyword(id, _, istype) case DataStruct::TOKENTYPE::id: return istype;
#include "../include/keyword.inc"
#undef keyword
#undef op
        default:
            return false;
    }
}
//name是否是typedef定义的类型，是则返回定义的类型，否则返回nullptr
std::shared_ptr<DataStruct::Type> Parser::get_typedef(const std::string&name){
    std::shared_ptr<DataStruct::Node> node=nullptr;
    if (env()->find(name)!=env()->end())
        node=env()->at(name);
    return (node && node->getKind() == DataStruct::AST_TYPE ::AST_TYPEDEF) ? node->getTy() : nullptr;
}
//是否是函数定义，C中的函数定义可以包括函数定义和函数声明
bool Parser::is_funcdef() {
    std::vector<DataStruct::Token> vec;
    bool r=false;
    CHECK_CPP();
    CHECK_LEX();
    while (true)
    {
        const DataStruct::Token tok=macro->read_token();
        vec.push_back(tok);
        if (tok.kind==DataStruct::TOKEN_TYPE::TEOF)
            Error::error("premature end of input");
        if (lex->is_keyword(tok,lex->get_keywords(";")))
            break;
        if(is_type(tok))
            continue;
        if (lex->is_keyword(tok,lex->get_keywords("("))){
            skip_parentheses(vec);
            continue;
        }
        if (tok.kind!=DataStruct::TOKEN_TYPE::TIDENT)
            continue;
        if (!lex->is_keyword(macro->peek_token(),lex->get_keywords("(")))
            continue;
        vec.emplace_back(macro->read_token());
        skip_parentheses(vec);
        r=lex->is_keyword(macro->peek_token(),lex->get_keywords("{"))||is_type(macro->peek_token());
        break;
    }
    for (auto rbeg=vec.rbegin(),rend=vec.rend();rbeg!=rend;++rbeg) {
        lex->retreat_token(*rbeg);
    }
    return r;
}
void Parser::skip_parentheses(std::vector<DataStruct::Token>&buf){
    int nest=0;
    CHECK_LEX();
    while (true){
        const DataStruct::Token& tok=lex->lex();
        if (tok.kind==DataStruct::TOKEN_TYPE::TEOF)
            Error::error("premature end of input");
        if (!nest&&lex->is_keyword(tok,lex->get_keywords(")")))
            break;
        if (lex->is_keyword(tok,lex->get_keywords("(")))
            ++nest;
        if (lex->is_keyword(tok,lex->get_keywords(")")))
            --nest;
        buf.push_back(tok);
    }
}

//读取声明和函数定义
std::shared_ptr<std::vector<DataStruct::Node>>& Parser::read_toplevels()
{
    toplevels->clear();
    CHECK_CPP();
    for (;;) {
        if (macro->peek_token().kind == DataStruct::TOKEN_TYPE::TEOF)
            return toplevels;
        if (is_funcdef())
            toplevels->emplace_back(read_funcdef());
        else
            read_decl(toplevels, true);
    }
}

void Parser::CHECK_CPP(){
    if (macro== nullptr)
        Error::error("MacroPropressor in Parser should be initialized.");
}
void Parser::CHECK_LEX(){
    if (lex== nullptr)
        Error::error("Lex in Parser should be initialized.");
}

DataStruct::Token Parser::read_funcdef() {
    CHECK_LEX();
    CHECK_CPP();
    DataStruct::QUALITIFIER sclass;
    std::shared_ptr<DataStruct::Type> basetype = read_decl_spec_opt(&sclass);
    localenv = make_map_parent(globalenv);
    gotos = make_vector();
    labels = make_map();
    std::string name;
    std::vector<DataStruct::Node> params;
    auto functype = read_declarator(&name, basetype, &params, DataStruct::DECL_TYPE::DECL_BODY);
    if (functype->oldstyle) {
        if (params.empty())
            functype->hasva = false;
        read_oldstyle_param_type(params);
        functype->params = param_types(params);
    }
    functype->isstatic = (sclass == DataStruct::QUALITIFIER::S_STATIC);
    ast_gvar(functype, name);
    macro->expect(lex->get_keywords("{"));
    Node *r = read_func_body(functype, name, params);
    backfill_labels();
    localenv = NULL;
    return r;
}
//函数如果未定义返回类型，则默认返回int
std::shared_ptr<DataStruct::Type> Parser::read_decl_spec_opt(DataStruct::QUALITIFIER *sclass){
    CHECK_CPP();
    if (is_type(macro->peek_token()))
        return read_decl_spec(sclass);
    Error::warnt(macro->peek_token(), "type specifier missing, assuming int");
    return TYPE_INT;
}
std::shared_ptr<DataStruct::Type> Parser::read_decl_spec(DataStruct::QUALITIFIER *rsclass){
    DataStruct::QUALITIFIER sclass = DataStruct::QUALITIFIER::S_PLACEHOLDER;
    CHECK_CPP();
    CHECK_LEX();
    DataStruct::Token tok = macro->peek_token();
    if (!is_type(tok))
        Error::errort(tok, "type name expected, but got %s", Utils::tok2s(tok));

    std::shared_ptr<DataStruct::Type> usertype = nullptr;
    enum { kvoid = 1, kbool, kchar, kint, kfloat, kdouble } kind = 0;
    enum { kshort = 1, klong, kllong } size = 0;
    enum { ksigned = 1, kunsigned } sig = 0;
    int align = -1;

    for (;;) {
        tok = macro->read_token();
        if (tok.kind == DataStruct::TOKEN_TYPE::TEOF)
            Error::error("premature end of input");
        if (kind == 0 && tok.kind == DataStruct::TOKEN_TYPE::TIDENT && usertype== nullptr) {  //typedef自定义类型
            std::shared_ptr<DataStruct::Type> def = get_typedef(*(tok.sval));
            if (def!= nullptr) {
                if (usertype!= nullptr) goto err;
                usertype = def;
                goto errcheck;
            }
        }
        if (tok.kind != DataStruct::TOKEN_TYPE::TKEYWORD) {
            lex->retreat_token(tok);
            break;
        }
        switch (tok.id) {
            case DataStruct::AST_TYPE ::KTYPEDEF:
                if (sclass!=DataStruct::QUALITIFIER::S_PLACEHOLDER) goto err; sclass = DataStruct::QUALITIFIER ::S_TYPEDEF; break;
            case DataStruct::AST_TYPE ::KEXTERN:
                if (sclass!=DataStruct::QUALITIFIER::S_PLACEHOLDER) goto err; sclass = DataStruct::QUALITIFIER::S_EXTERN; break;
            case DataStruct::AST_TYPE ::KSTATIC:
                if (sclass!=DataStruct::QUALITIFIER::S_PLACEHOLDER) goto err; sclass = DataStruct::QUALITIFIER::S_STATIC; break;
            case DataStruct::AST_TYPE ::KAUTO:
                if (sclass!=DataStruct::QUALITIFIER::S_PLACEHOLDER) goto err; sclass = DataStruct::QUALITIFIER::S_AUTO; break;
            case DataStruct::AST_TYPE ::KREGISTER:
                if (sclass!=DataStruct::QUALITIFIER::S_PLACEHOLDER) goto err; sclass = DataStruct::QUALITIFIER::S_REGISTER; break;
            case DataStruct::AST_TYPE ::KCONST:    break;
            case DataStruct::AST_TYPE ::KVOLATILE: break;
            case DataStruct::AST_TYPE ::KINLINE:   break;
//            http://zh.cppreference.com/w/c/language/_Noreturn
//            https://stackoverflow.com/questions/13691361/use-of-noreturn-in-c11
            case DataStruct::AST_TYPE ::KNORETURN: break;
            case DataStruct::AST_TYPE ::KVOID:     if (kind) goto err; kind = kvoid; break;
            case DataStruct::AST_TYPE ::KBOOL:     if (kind) goto err; kind = kbool; break;
            case DataStruct::AST_TYPE ::KCHAR:     if (kind) goto err; kind = kchar; break;
            case DataStruct::AST_TYPE ::KINT:      if (kind) goto err; kind = kint; break;
            case DataStruct::AST_TYPE ::KFLOAT:    if (kind) goto err; kind = kfloat; break;
            case DataStruct::AST_TYPE ::KDOUBLE:   if (kind) goto err; kind = kdouble; break;
            case DataStruct::AST_TYPE ::KSIGNED:   if (sig) goto err; sig = ksigned; break;
            case DataStruct::AST_TYPE ::KUNSIGNED: if (sig) goto err; sig = kunsigned; break;
            case DataStruct::AST_TYPE ::KSHORT:    if (size) goto err; size = kshort; break;
            case DataStruct::AST_TYPE ::KSTRUCT:   if (usertype!= nullptr) goto err; usertype = read_struct_def(); break;
            case DataStruct::AST_TYPE ::KUNION:    if (usertype!= nullptr) goto err; usertype = read_union_def(); break;
            case DataStruct::AST_TYPE ::KENUM:     if (usertype!= nullptr) goto err; usertype = read_enum_def(); break;
            case DataStruct::AST_TYPE ::KALIGNAS: {
                int val = read_alignas();
                if (val < 0)
                    Error::errort(tok, "negative alignment: %d", val);
                // C11 6.7.5p6: alignas(0) should have no effect.
                if (val == 0)
                    break;
                if (align == -1 || val < align)
                    align = val;
                break;
            }
            case DataStruct::AST_TYPE::KLONG: {
                if (size == 0) size = klong;
                else if (size == klong) size = kllong;
                else goto err;
                break;
            }
            //https://gcc.gnu.org/onlinedocs/gcc/Typeof.html
            case DataStruct::AST_TYPE ::KTYPEOF: {   //gcc拓展，c11没有这个关键字 C11 6.4.1 keywords
                if (usertype) goto err;
                usertype = read_typeof();
                break;
            }
            default:
                lex->retreat_token(tok);
                goto done;
        }
        errcheck:
//        C11 6.2.5  types
        if (kind == kbool && (size != 0 || sig != 0))
            goto err;
        if (size == kshort && (kind != 0 && kind != kint))
            goto err;
        if (size == klong && (kind != 0 && kind != kint && kind != kdouble))
            goto err;
        if (sig != 0 && (kind == kvoid || kind == kfloat || kind == kdouble||kind==kbool))
            goto err;
        if (usertype && (kind != 0 || size != 0 || sig != 0))
            goto err;
    }
    done:
    if (rsclass)
        *rsclass = sclass;
    if (usertype)
        return usertype;
    if (align != -1 && !is_poweroftwo(align))
        Error::errort(tok, "alignment must be power of 2, but got %d", align);
    std::shared_ptr<DataStruct::Type> ty;
    switch (kind) {
        case kvoid:   ty = TYPE_VOID; goto end;
        case kbool:   ty = make_numtype(DataStruct::TYPE_KIND::KIND_BOOL, false); goto end;
        case kchar:   ty = make_numtype(DataStruct::TYPE_KIND::KIND_CHAR, sig == kunsigned); goto end;
        case kfloat:  ty = make_numtype(DataStruct::TYPE_KIND::KIND_FLOAT, false); goto end;
        case kdouble: ty = make_numtype(size == klong ? DataStruct::TYPE_KIND::KIND_LDOUBLE : DataStruct::TYPE_KIND::KIND_DOUBLE, false); goto end;
        default: break;
    }
    switch (size) {
        case kshort: ty = make_numtype(DataStruct::TYPE_KIND::KIND_SHORT, sig == kunsigned); goto end;
        case klong:  ty = make_numtype(DataStruct::TYPE_KIND::KIND_LONG, sig == kunsigned); goto end;
        case kllong: ty = make_numtype(DataStruct::TYPE_KIND::KIND_LLONG, sig == kunsigned); goto end;
        default:     ty = make_numtype(DataStruct::TYPE_KIND::KIND_INT, sig == kunsigned); goto end;
    }
    Error::error("internal error: kind: %d, size: %d", kind, size);
    end:
    if (align != -1)
        ty->align = align;
    return ty;
    err:
    Error::errort(tok, "type mismatch: %s", Utils::tok2s(tok));
}
std::shared_ptr<DataStruct::Type> Parser::read_typeof(){
    CHECK_LEX();
    CHECK_CPP();
    macro->expect(lex->get_keywords("("));
    std::shared_ptr<DataStruct::Type> r = is_type(macro->peek_token())
              ? read_cast_type()
              : read_comma_expr()->ty;
    macro->expect(lex->get_keywords(")"));
    return r;
}
//align是否是2的幂
bool Parser::is_poweroftwo(int align){
    return align<=0?false:!(align&(align-1));
}
//C11 6.7.2.2  Enumeration Specifiers
std::shared_ptr<DataStruct::Type> Parser::read_enum_def(){
    CHECK_CPP();
    CHECK_LEX();
    std::string tag;
    DataStruct::Token tok = macro->read_token();

    // Enum is handled as a synonym for int. We only check if the enum
    // is declared.
    if (tok.kind == DataStruct::TOKEN_TYPE::TIDENT) {
        tag = *(tok.sval);
        tok = macro->read_token();
    }
    if (!tag.empty()) {
        std::shared_ptr<DataStruct::Type> ty= nullptr;
        if (tags.find(tag)!=tags.end())
               ty = tags[tag];
        if (ty && ty->kind != DataStruct::TYPE_KIND::KIND_ENUM)
            Error::errort(tok, "declarations of %s does not match", tag);
    }
    if (!lex->is_keyword(tok, lex->get_keywords("{"))) {
        if (tag.empty() || tags.find(tag)==tags.end())
            Error::errort(tok, "enum tag %s is not defined", tag);
        lex->retreat_token(tok);
        return TYPE_INT;
    }
    if (!tag.empty())
        tags[tag] = TYPE_ENUM;

    int val = 0;
    for (;;) {
        tok = macro->read_token();
        if (lex->is_keyword(tok, lex->get_keywords("}")))
            break;
        if (tok.kind != DataStruct::TOKEN_TYPE::TIDENT)
            Error::errort(tok, "identifier expected, but got %s", Utils::tok2s(tok));
        std::string name = *(tok.sval);

        if (macro->next(lex->get_keywords("=")))
            val = read_intexpr();
        std::shared_ptr<DataStruct::Node> constval = ast_inttype(TYPE_INT, val++);
        env()->emplace(std::make_pair(name,constval));
        if (macro->next(lex->get_keywords(",")))
            continue;
        if (macro->next(lex->get_keywords("}")))
            break;
        Error::errort(macro->peek_token(), "',' or '}' expected, but got %s", Utils::tok2s(macro->peek_token()));
    }
    return TYPE_INT;
}
std::shared_ptr<DataStruct::Type> Parser::read_struct_def(){
    return read_rectype_def(true);
}
std::shared_ptr<DataStruct::Type> Parser::read_union_def(){
    return read_rectype_def(false);
}
std::shared_ptr<DataStruct::Type> Parser::read_rectype_def(bool is_struct){
    std::string tag = read_rectype_tag();
    std::shared_ptr<DataStruct::Type> r= nullptr;
    if (!tag.empty()) {
        if (tags.find(tag)!=tags.end())
            r =tags.at(tag);
        if (r!= nullptr && (r->kind == DataStruct::TYPE_KIND::KIND_ENUM || r->is_struct != is_struct))
            Error::error("declarations of %s does not match", tag);
        if (!r) {
            r = make_rectype(is_struct);
            tags[tag]=r;
        }
    } else {
        r = make_rectype(is_struct);
    }
    int size = 0, align = 1;
    auto  fields = read_rectype_fields(size, align, is_struct);
    r->align = align;
    if (!fields.empty()) {
        r->fields = fields;
        r->size = size;
    }
    return r;
}
//读取聚合对象名字
std::string Parser::read_rectype_tag(){
    CHECK_CPP();CHECK_LEX();
    DataStruct::Token tok = macro->read_token();
    if (tok.kind == DataStruct::TOKEN_TYPE::TIDENT)
        return *(tok.sval);
    lex->retreat_token(tok);
    return "";
}
//读取结构体成员变量
//6.7.2.1 注意位域的处理
std::vector<std::pair<std::string,std::shared_ptr<DataStruct::Type>>>  Parser::read_rectype_fields_sub(){
    std::vector<std::pair<std::string,std::shared_ptr<DataStruct::Type>>> r;
    CHECK_LEX();CHECK_CPP();
    while (1){
        if (macro->next(lex->get_keywords("_Static_assert"))) {
            //http://zh.cppreference.com/w/c/language/_Static_assert
            read_static_assert();
            continue;
        }
        if (!is_type(macro->peek_token()))
            break;
        std::shared_ptr<DataStruct::Type> basetype = read_decl_spec(nullptr);
        if (basetype->kind == DataStruct::TYPE_KIND::KIND_STRUCT && macro->next(lex->get_keywords(";"))) {
            r.emplace_back(std::make_pair<std::string>("", basetype));
            continue;
        }
        while(1) {
            std::string name;
            std::shared_ptr<DataStruct::Type> fieldtype = read_declarator(&name, basetype, nullptr, DataStruct::DECL_TYPE::DECL_PARAM_TYPEONLY);
            ensure_not_void(fieldtype);
            fieldtype->bitsize = macro->next(lex->get_keywords(":")) ? read_bitsize(name, fieldtype) : -1;
            r.emplace_back(std::make_pair(name, fieldtype));
            if (macro->next(lex->get_keywords(",")))
                continue;
            if (lex->is_keyword(macro->peek_token(), lex->get_keywords("}")))
                Error::warnt(macro->peek_token(), "missing ';' at the end of field list");
            else
                macro->expect(lex->get_keywords(";"));
            break;
        }
    }
    macro->expect(lex->get_keywords("}"));
    return r;
}

std::vector<std::pair<std::string,std::shared_ptr<DataStruct::Type>>>  Parser::read_rectype_fields(int &size,int &align,bool is_struct){
    CHECK_CPP();
    CHECK_LEX();
    if (!macro->next(lex->get_keywords("{")))
        return std::vector<std::pair<std::string,std::shared_ptr<DataStruct::Type>>>();
    std::vector<std::pair<std::string,std::shared_ptr<DataStruct::Type>>> fields = read_rectype_fields_sub();
    fix_rectype_flexible_member(fields);
    if (is_struct)
        return update_struct_offset(size, align, fields);
    return update_union_offset(size, align, fields);
}
void Parser::read_static_assert(){
    CHECK_CPP();CHECK_LEX();
    macro->expect(lex->get_keywords("("));
    int val = read_intexpr();
    macro->expect(lex->get_keywords(","));
    DataStruct::Token tok = macro->read_token();
    if (tok.kind != DataStruct::TOKEN_TYPE::TSTRING)
        Error::errort(tok, "string expected as the second argument for _Static_assert, but got %s", Utils::tok2s(tok));
    macro->expect(lex->get_keywords(")"));
    macro->expect(lex->get_keywords(";"));
    if (!val)
        Error::errort(tok, "_Static_assert failure: %s", *(tok.sval));
}
//位域： <0 invalid
// =0  ananymous
//不能超过类型的存储范围
int Parser::read_bitsize(std::string&name,const std::shared_ptr<DataStruct::Type>&type){
    if (!is_inttype(type))
        Error::error("non-integer type cannot be a bitfield: %s", Utils::ty2s(type));
    CHECK_CPP();
    CHECK_LEX();
    const DataStruct::Token tok = macro->peek_token();
    int r = read_intexpr();
    int maxsize = type->kind == DataStruct::TYPE_KIND::KIND_BOOL ? 1 : type->size * 8;
    if (r < 0 || maxsize < r)
        Error::errort(tok, "invalid bitfield size for %s: %d", Utils::ty2s(type), r);
    if (r == 0 && name != NULL)
        Error::errort(tok, "zero-width bitfield needs to be unnamed: %s", name);
    return r;
}
//6.7.2.1 p18 flexible array member
void Parser::fix_rectype_flexible_member(std::vector<std::pair<std::string,std::shared_ptr<DataStruct::Type>>> &fields){
    for (int len=fields.size(), i=0;i<len;++i) {
        std::string name=fields[i].first;
        std::shared_ptr<DataStruct::Type> ty=fields[i].second;
        if (ty->kind!=DataStruct::TYPE_KIND::KIND_ARRAY)
            continue;
        if (ty->len==-1){
            if (i!=len-1)
                Error::error("flexible member may only appear as the last member: %s %s", Utils::ty2s(ty), name);
            if (len==1)
                Error::error("flexible member with no other fields: %s %s", Utils::ty2s(ty), name);
            ty->len=0;
            ty->size=0;
        }
    }
}
//C11 6.7.2.1  p 16
std::vector<std::pair<std::string,std::shared_ptr<DataStruct::Type>>> Parser::update_union_offset(int&size,int&align,std::vector<std::pair<std::string,std::shared_ptr<DataStruct::Type>>>&fields){
    int maxsize = 0;
    std::vector<std::pair<std::string,std::shared_ptr<DataStruct::Type>>> r;
    for (auto&e:fields) {
        std::string name=e.first;
        auto fieldtype=e.second;
        maxsize = std::max(maxsize, fieldtype->size);
        align = std::max(align, fieldtype->align);
        if (name.empty() && fieldtype->kind == DataStruct::TYPE_KIND::KIND_STRUCT) {
            squash_unnamed_struct(r, fieldtype, 0);
            continue;
        }
        fieldtype->offset = 0;
        if (fieldtype->bitsize >= 0)
            fieldtype->bitoff = 0;
        if (!name.empty())
            r.emplace_back(std::make_pair(name,fieldtype));
    }
    size = maxsize + compute_padding(maxsize, align);
    return r;
};
//C11 6.7.2.1
std::vector<std::pair<std::string,std::shared_ptr<DataStruct::Type>>> Parser::update_struct_offset(int&size,int&align,std::vector<std::pair<std::string,std::shared_ptr<DataStruct::Type>>>&fields){
    int off = 0, bitoff = 0;
    std::vector<std::pair<std::string,std::shared_ptr<DataStruct::Type>>> r;
    for (auto& e:fields) {
//        void **pair = vec_get(fields, i);
        std::string name = e.first;
        std::shared_ptr<DataStruct::Type> &fieldtype = e.second;
        // C11 6.7.2.1 p 14
        if (!name.empty())
            align = std::max(align,fieldtype->align);

        if (name.empty() && fieldtype->kind == DataStruct::TYPE_KIND::KIND_STRUCT) {
            // C11 6.7.2.1 p13  Anonymous struct
            finish_bitfield(off, bitoff);
            off += compute_padding(off, fieldtype->align);
            squash_unnamed_struct(r, fieldtype, off);
            off += fieldtype->size;
            continue;
        }
        if (fieldtype->bitsize == 0) {
            // C11 6.7.2.1   p12:
            finish_bitfield(off, bitoff);
            off += compute_padding(off, fieldtype->align);
            bitoff = 0;
            continue;
        }
        if (fieldtype->bitsize > 0) {  //bit足够则直接在当前位置pack，否则对齐到下一位置去pack
            int bit = fieldtype->size * 8;
            int room = bit - (off * 8 + bitoff) % bit;
            if (fieldtype->bitsize <= room) {
                fieldtype->offset = off;
                fieldtype->bitoff = bitoff;
            } else {
                finish_bitfield(off, bitoff);
                off += compute_padding(off, fieldtype->align);
                fieldtype->offset = off;
                fieldtype->bitoff = 0;
            }
            bitoff += fieldtype->bitsize;
        } else {
            finish_bitfield(off, bitoff);
            off += compute_padding(off, fieldtype->align);
            fieldtype->offset = off;
            off += fieldtype->size;
        }
        if (!name.empty())
            r.emplace_back(std::make_pair(name,fieldtype));
    }
    finish_bitfield(off, bitoff);
    size = off + compute_padding(off, align);
    return r;
};
//只有bitoff为0的时候才偏移为0,否则偏移到下一字节
void Parser::finish_bitfield(int&off,int &bitoff){
    off += (bitoff + 7) / 8;
    bitoff = 0;
}
int Parser::compute_padding(int offset, int align) {
    return (offset % align == 0) ? 0 : align - offset % align;
}
void Parser::squash_unnamed_struct(std::vector<std::pair<std::string,std::shared_ptr<DataStruct::Type>>>&dict, std::shared_ptr<DataStruct::Type> &unnamed, int offset) {
    auto key_selector=[](std::pair<std::string, std::shared_ptr<DataStruct::Type>>& pair){ return pair.first;};
    std::vector<std::string> keys(dict.size());
    std::transform(dict.cbegin(),dict.cend(),keys.begin(),key_selector);
    for (auto&name:keys) {
        std::shared_ptr<DataStruct::Type> t=std::make_shared<DataStruct::Type>(*(unnamed->fields.at(name)));
        t->offset += offset;
        dict.emplace_back(std::make_shared(name,t));
    }
}
// C11 6.7.6: Declarators
std::shared_ptr<DataStruct::Type> Parser::read_declarator(std::string *name,const std::shared_ptr<DataStruct::Type>&basetype,
                                                  std::vector<DataStruct::Node>*params,DataStruct::DECL_TYPE type){
    CHECK_CPP();
    CHECK_LEX();
    //C11 6.7.6.1.3 Function declarators
    if (macro->next(lex->get_keywords("("))) {
        if (is_type(macro->peek_token()))
            return read_declarator_func(basetype, params);   //int (int...)
        //int (*)(int...)
        std::shared_ptr<DataStruct::Type> stub = std::make_shared<DataStruct::Type>(DataStruct::TYPE_KIND::KIND_PLACEHOLDER,0);
        std::shared_ptr<DataStruct::Type> t = read_declarator(name, stub, params, type);  //过滤掉第一个括号内的内容并提取出函数指针
        macro->expect(lex->get_keywords(")"));
        stub = read_declarator_tail(basetype, params);
        return t;
    }
    //C11 6.7.6.1 Pointer Declarators
    if (macro->next(lex->get_keywords("*"))) {
        skip_type_qualifiers();
        return read_declarator(name, make_ptr_type(basetype), params, type);  //递归构造多重指针
    }
    //C11 6.7.6.2 Array Declarators
    DataStruct::Token tok = macro->read_token();
    if (tok.kind == DataStruct::TOKEN_TYPE ::TIDENT) {
        if (type == DataStruct::DECL_TYPE::DECL_CAST)
            Error::errort(tok, "identifier is not expected, but got %s", Utils::tok2s(tok));
        *name = *(tok.sval);
        return read_declarator_tail(basetype, params);
    }
    if (type == DataStruct::DECL_TYPE::DECL_BODY || type == DataStruct::DECL_TYPE::DECL_PARAM)
        Error::errort(tok, "identifier, ( or * are expected, but got %s", Utils::tok2s(tok));
    lex->retreat_token(tok);
    return read_declarator_tail(basetype, params);
}
std::shared_ptr<DataStruct::Type> Parser::read_declarator_tail(const std::shared_ptr<DataStruct::Type>&basetype, std::vector<DataStruct::Node>*params){
    CHECK_LEX();
    CHECK_CPP();
    if (macro->next(lex->get_keywords("[")))
        return read_declarator_array(basetype);
    if (macro->next(lex->get_keywords("(")))
        return read_declarator_func(basetype, params);
    return basetype;
}
//int a[][3]
std::shared_ptr<DataStruct::Type> Parser::read_declarator_array(const std::shared_ptr<DataStruct::Type> &basetype){
    CHECK_LEX();
    CHECK_CPP();
    int len;
    if (macro->next(lex->get_keywords("]"))) {
        len = -1;
    } else {
        len = read_intexpr();
        macro->expect(lex->get_keywords("]"));
    }
    const DataStruct::Token &tok = macro->peek_token();
    std::shared_ptr<DataStruct::Type> t = read_declarator_tail(basetype, nullptr);
    if (t->kind == DataStruct::TYPE_KIND::KIND_FUNC)
        Error::errort(tok, "array of functions");
    return make_array_type(t, len);
}
std::shared_ptr<DataStruct::Type> Parser::read_declarator_func(const std::shared_ptr<DataStruct::Type>&returntype, std::vector<DataStruct::Node>*params){
    if (returntype->kind == DataStruct::TYPE_KIND::KIND_FUNC)
        Error::error("function returning a function");
    if (returntype->kind == DataStruct::TYPE_KIND::KIND_ARRAY)
        Error::error("function returning an array");
    return read_func_param_list(params, returntype);
}
//C11 6.7.6.3
std::shared_ptr<DataStruct::Type> Parser::read_func_param_list(std::vector<DataStruct::Node>*param, const std::shared_ptr<DataStruct::Type>&rettype){
    CHECK_CPP();
    CHECK_LEX();
    DataStruct::Token tok = macro->read_token();
    if (lex->is_keyword(tok, lex->get_keywords("void")) && macro->next(lex->get_keywords(")")))
        return make_func_type(rettype, std::vector<DataStruct::Type>(), false, false);

    // C11 6.7.6.3 p14
    //K&R style函数声明是上古版本的用法了，所以实不实现都无所谓，不过为了更好的兼容旧代码，实现了还是有一定用处的，
    //最起码，可以学到更多的知识。。。
    if (lex->is_keyword(tok, lex->get_keywords(")")))
        return make_func_type(rettype, std::vector<DataStruct::Type>(), true, true);
    lex->retreat_token(tok);

    DataStruct::Token tok2 = macro->peek_token();
    if (macro->next(DataStruct::AST_TYPE::KELLIPSIS))
        Error::errort(tok2, "at least one parameter is required before \"...\"");
    if (is_type(macro->peek_token())) {
        bool ellipsis;
        std::vector<DataStruct::Type> paramtypes ;
        read_declarator_params(paramtypes, param, ellipsis);
        return make_func_type(rettype, paramtypes, ellipsis, false);
    }
    if (!param)
        Error::errort(tok, "invalid function definition");
    read_declarator_params_oldstyle(param);
    std::vector<DataStruct::Type> paramtypes ;
    for (auto &e:*param)
        paramtypes.emplace_back(*TYPE_INT);
    return make_func_type(rettype, paramtypes, false, true);
}
//paramtypes: 参数类型
//paramvars: 命名参数
void Parser::read_declarator_params(std::vector<DataStruct::Type>&paramtypes, std::vector<DataStruct::Node>*paramvars, bool &ellipsis){
    CHECK_LEX();
    CHECK_CPP();
    bool typeonly = paramvars== nullptr;
    ellipsis = false;
    while(1) {
        DataStruct::Token tok = macro->peek_token();
        if (macro->next(DataStruct::AST_TYPE::KELLIPSIS)) {
            if (paramtypes.empty())
                Error::errort(tok, "at least one parameter is required before \"...\"");
            macro->expect(lex->get_keywords(")"));
            ellipsis = true;
            return;
        }
        std::string name;
        std::shared_ptr<DataStruct::Type> ty = read_func_param(name, typeonly);
        ensure_not_void(ty);
        paramtypes.push_back(*ty);
        if (!typeonly)
            paramvars->emplace_back( *(ast_lvar(ty, name)));
        tok = macro->read_token();
        if (lex->is_keyword(tok, lex->get_keywords(")")))
            return;
        if (!lex->is_keyword(tok, lex->get_keywords(",")))
            Error::errort(tok, "comma expected, but got %s", Utils::tok2s(tok));
    }
}
std::shared_ptr<DataStruct::Type> Parser::read_func_param(std::string& name,bool typeonly){
    CHECK_CPP();
    CHECK_LEX();
    DataStruct::QUALITIFIER sclass = DataStruct::QUALITIFIER::S_PLACEHOLDER;
    std::shared_ptr<DataStruct::Type> basety = TYPE_INT;
    if (is_type(macro->peek_token())) {
        basety = read_decl_spec(&sclass);
    } else if (typeonly) {
        Error::errort(macro->peek_token(), "type expected, but got %s", Utils::tok2s(macro->peek_token()));
    }
    std::shared_ptr<DataStruct::Type> ty = read_declarator(&name, basety, nullptr, typeonly ? DataStruct::DECL_TYPE::DECL_PARAM_TYPEONLY : DataStruct::DECL_TYPE::DECL_PARAM);
    // C11 6.7.6.3 p7
    if (ty->kind == DataStruct::TYPE_KIND::KIND_ARRAY)
        return make_ptr_type(ty->ptr);
    // C11 6.7.6.3 p8
    if (ty->kind == DataStruct::TYPE_KIND::KIND_FUNC)
        return make_ptr_type(ty);
    return ty;
}
//K&R style的函数列表中没有具体类型，其类型在括号外部定义，因而可以暂时假设参数列表中的参数均是int类型。
void Parser::read_declarator_params_oldstyle(std::vector<DataStruct::Node>*vars){
    CHECK_CPP();
    CHECK_LEX();
    for (;;) {
        const DataStruct::Token &tok = macro->read_token();
        if (tok.kind != DataStruct::TOKEN_TYPE::TIDENT)
            Error::errort(tok, "identifier expected, but got %s", Utils::tok2s(tok));
        vars->emplace_back(ast_lvar(TYPE_INT, *(tok.sval)));
        if (macro->next(lex->get_keywords(")")))
            return;
        if (!macro->next(lex->get_keywords(",")))
            Error::errort(tok, "comma expected, but got %s", Utils::tok2s(macro->read_token()));
    }
}
//确保类型不为void
void Parser::ensure_not_void(std::shared_ptr<DataStruct::Type>&fieldtype){
    if(fieldtype->kind==DataStruct::TYPE_KIND::KIND_VOID)
        Error::error("void is not allowed");
}
//跳过const volatile，restrict
void Parser::skip_type_qualifiers(){
    CHECK_CPP();
    CHECK_LEX();
    while (macro->next(lex->get_keywords("const"))||macro->next(lex->get_keywords("volatile"))||macro->next(lex->get_keywords("restrict")));
}
//http://zh.cppreference.com/w/c/language/_Alignas
int Parser::read_alignas(){
    macro->expect(lex->get_keywords("("));
    int r = is_type(macro->peek_token())
            ? read_cast_type()->align
            : read_intexpr();
    macro->expect(lex->get_keywords(")"));
    return r;
}
std::shared_ptr<DataStruct::Type> Parser::read_cast_type() {
    return read_abstract_declarator(read_decl_spec(nullptr));
}
//C11 6.7.7给出了type names的文法定义，看起来还是挺麻烦的
std::shared_ptr<DataStruct::Type> Parser::read_abstract_declarator(const std::shared_ptr<DataStruct::Type>& basety) {
    return read_declarator(nullptr, basety, nullptr, DataStruct::DECL_TYPE::DECL_CAST);
}

std::shared_ptr<DataStruct::Node> do_read_conditional_expr(const std::shared_ptr<DataStruct::Node> &cond) {
    Node *then = conv(read_comma_expr());
    expect(':');
    Node *els = conv(read_conditional_expr());
    // [GNU] Omitting the middle operand is allowed.
    Type *t = then ? then->ty : cond->ty;
    Type *u = els->ty;
    // C11 6.5.15p5: if both types are arithemtic type, the result
    // type is the result of the usual arithmetic conversions.
    if (is_arithtype(t) && is_arithtype(u)) {
        Type *r = usual_arith_conv(t, u);
        return ast_ternary(r, cond, (then ? wrap(r, then) : NULL), wrap(r, els));
    }
    return ast_ternary(u, cond, then, els);
}

static Node *read_conditional_expr() {
    Node *cond = read_logor_expr();
    if (!next_token('?'))
        return cond;
    return do_read_conditional_expr(cond);
}