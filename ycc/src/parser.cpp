//
// Created by yk on 18-2-8.
//

#include "../include/parser.h"

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
    if (env().find(name)!=env().end())
        node=env().at(name);
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
    for (int i = vec.size(); i >=0 ; --i) {
        lex->retreat_token(vec[i]);
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
std::vector<DataStruct::Token>& Parser::read_toplevels()
{
    toplevels.clear();
    CHECK_CPP();
    for (;;) {
        if (macro->peek_token().kind == DataStruct::TOKEN_TYPE::TEOF)
            return toplevels;
        if (is_funcdef())
            toplevels.emplace_back(read_funcdef());
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
    DataStruct::QUALITIFIER sclass;
    std::shared_ptr<DataStruct::Type> basetype = read_decl_spec_opt(&sclass);
    localenv = make_map_parent(globalenv);
    gotos = make_vector();
    labels = make_map();
    char *name;
    Vector *params = make_vector();
    Type *functype = read_declarator(&name, basetype, params, DECL_BODY);
    if (functype->oldstyle) {
        if (vec_len(params) == 0)
            functype->hasva = false;
        read_oldstyle_param_type(params);
        functype->params = param_types(params);
    }
    functype->isstatic = (sclass == S_STATIC);
    ast_gvar(functype, name);
    expect('{');
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
std::shared_ptr<DataStruct::Type> Parser::read_enum_def(){

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
    std::unordered_map<std::string,std::shared_ptr<DataStruct::Type>>  fields = read_rectype_fields(size, align, is_struct);
    r->align = align;
    if (fields) {
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
std::unordered_map<std::string,std::shared_ptr<DataStruct::Type>>  Parser::read_rectype_fields_sub(){
    std::unordered_map<std::string,std::shared_ptr<DataStruct::Type>> r;
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
            r.emplace(std::make_pair("", basetype));
            continue;
        }
        while(1) {
            std::string name;
            std::shared_ptr<DataStruct::Type> fieldtype = read_declarator(&name, basetype, nullptr, DataStruct::DECL_TYPE::DECL_PARAM_TYPEONLY);
            ensure_not_void(fieldtype);
            fieldtype = copy_type(fieldtype);
            fieldtype->bitsize = macro->next(lex->get_keywords(":")) ? read_bitsize(name, fieldtype) : -1;
            r.emplace(make_pair(name, fieldtype));
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

std::unordered_map<std::string,std::shared_ptr<DataStruct::Type>>  Parser::read_rectype_fields(int &size,int &align,bool is_struct){
    CHECK_CPP();
    CHECK_LEX();
    if (!macro->next(lex->get_keywords("{")))
        return std::unordered_map<std::string,std::shared_ptr<DataStruct::Type>>();
    std::unordered_map<std::string,std::shared_ptr<DataStruct::Type>> fields = read_rectype_fields_sub();
    fix_rectype_flexible_member(fields);
    if (is_struct)
        return update_struct_offset(rsize, align, fields);
    return update_union_offset(rsize, align, fields);
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
// C11 6.7.6: Declarators
std::shared_ptr<DataStruct::Type> Parser::read_declarator(std::string *name,const std::shared_ptr<DataStruct::Type>&basetype,
                                                  std::vector<DataStruct::Type>*params,DataStruct::DECL_TYPE type){
    CHECK_CPP();
    CHECK_LEX();
    if (macro->next(lex->get_keywords("("))) {
        // '(' is either beginning of grouping parentheses or of a function parameter list.
        // If the next token is a type name, a parameter list must follow.
        if (is_type(macro->peek_token()))
            return read_declarator_func(basetype, params);
        // If not, it's grouping. In that case we have to read from outside.
        // For example, consider int (*)(), which is "pointer to function returning int".
        // We have only read "int" so far. We don't want to pass "int" to
        // a recursive call, or otherwise we would get "pointer to int".
        // Here, we pass a dummy object to get "pointer to <something>" first,
        // continue reading to get "function returning int", and then combine them.
        std::shared_ptr<DataStruct::Type> stub = std::make_shared<DataStruct::Type>(DataStruct::TYPE_KIND::KIND_PLACEHOLDER,0);
        std::shared_ptr<DataStruct::Type> t = read_declarator(name, stub, params, type);
        macro->expect(lex->get_keywords("("));
        stub = read_declarator_tail(basetype, params);
        return t;
    }
    if (macro->next(lex->get_keywords("*"))) {
        skip_type_qualifiers();
        return read_declarator(name, make_ptr_type(basetype), params, type);
    }
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
std::shared_ptr<DataStruct::Type> Parser::read_declarator_tail(const std::shared_ptr<DataStruct::Type>&basetype, std::vector<DataStruct::Type>*params){
    CHECK_LEX();
    CHECK_CPP();
    if (macro->next(lex->get_keywords("[")))
        return read_declarator_array(basetype);
    if (macro->next(lex->get_keywords("(")))
        return read_declarator_func(basetype, params);
    return basetype;
}
std::shared_ptr<DataStruct::Type> Parser::read_declarator_func(const std::shared_ptr<DataStruct::Type>&basetype, std::vector<DataStruct::Type>*params){
    if (basetype->kind == DataStruct::TYPE_KIND::KIND_FUNC)
        Error::error("function returning a function");
    if (basetype->kind == DataStruct::TYPE_KIND::KIND_ARRAY)
        Error::error("function returning an array");
    return read_func_param_list(params, basetype);
}
//C11 6.7.6.3
std::shared_ptr<DataStruct::Type> Parser::read_func_param_list(std::vector<DataStruct::Type>*param, const std::shared_ptr<DataStruct::Type>&rettype){
    CHECK_CPP();
    CHECK_LEX();
    DataStruct::Token tok = macro->read_token();
    if (lex->is_keyword(tok, lex->get_keywords("void")) && macro->next(lex->get_keywords(")")))
        return make_func_type(rettype, std::vector<DataStruct::Type>(), false, false);

    // C11 6.7.6.3p14: K&R-style un-prototyped declaration or
    // function definition having no parameters.
    // We return a type representing K&R-style declaration here.
    // If this is actually part of a declartion, the type will be fixed later.
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
    std::vector<std::shared_ptr<DataStruct::Type>> paramtypes ;
    for (auto &e:*param)
        paramtypes.push_back(TYPE_INT);
    return make_func_type(rettype, paramtypes, false, true);
}
void Parser::read_declarator_params(std::vector<DataStruct::Type>&paramtypes, std::vector<DataStruct::Type>*paramvars, bool &ellipsis){
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
        paramtypes.push_back(ty);
        if (!typeonly)
            vec_push(vars, ast_lvar(ty, name));
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
    // C11 6.7.6.3p7: Array of T is adjusted to pointer to T
    // in a function parameter list.
    if (ty->kind == DataStruct::TYPE_KIND::KIND_ARRAY)
        return make_ptr_type(ty->ptr);
    // C11 6.7.6.3p8: Function is adjusted to pointer to function
    // in a function parameter list.
    if (ty->kind == DataStruct::TYPE_KIND::KIND_FUNC)
        return make_ptr_type(ty);
    return ty;
}
//确保类型不为void
void Parser::ensure_not_void(std::shared_ptr<DataStruct::Type>&fieldtype){
    if(fieldtype->kind==DataStruct::TYPE_KIND::KIND_VOID)
        Error::error("void is not allowed");
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