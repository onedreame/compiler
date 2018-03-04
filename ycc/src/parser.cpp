//
// Created by yk on 18-2-8.
//

#include "../include/parser.h"

std::shared_ptr<DataStruct::Type> Parser::make_rectype(bool is_struct){
    return std::make_shared<DataStruct::Type>(DataStruct::TYPE_KIND::KIND_STRUCT,is_struct);
}
//c中的类型，可以是关键字，auto，const，char等，或者自定义的类型
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
    int sclass = 0;
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

std::shared_ptr<DataStruct::Type> Parser::read_decl_spec_opt(DataStruct::QUALITIFIER &sclass){
    CHECK_CPP();
    if (is_type(macro->peek_token()))
        return read_decl_spec(sclass);
    Error::warnt(macro->peek_token(), "type specifier missing, assuming int");
    return TYPE_INT;
}
std::shared_ptr<DataStruct::Type> Parser::read_decl_spec(DataStruct::QUALITIFIER &rsclass){
    DataStruct::QUALITIFIER sclass = DataStruct::QUALITIFIER::S_PLACEHOLDER;
    CHECK_CPP();
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
        if (kind == 0 && tok.kind == DataStruct::TOKEN_TYPE::TIDENT && usertype== nullptr) {
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
            case DataStruct::AST_TYPE ::KTYPEOF: {
                if (usertype) goto err;
                usertype = read_typeof();
                break;
            }
            default:
                lex->retreat_token(tok);
                goto done;
        }
        errcheck:
        if (kind == kbool && (size != 0 && sig != 0))
            goto err;
        if (size == kshort && (kind != 0 && kind != kint))
            goto err;
        if (size == klong && (kind != 0 && kind != kint && kind != kdouble))
            goto err;
        if (sig != 0 && (kind == kvoid || kind == kfloat || kind == kdouble))
            goto err;
        if (usertype && (kind != 0 || size != 0 || sig != 0))
            goto err;
    }
    done:
    if (rsclass)
        rsclass = sclass;
    if (usertype)
        return usertype;
    if (align != -1 && !is_poweroftwo(align))
        Error::errort(tok, "alignment must be power of 2, but got %d", align);
    std::shared_ptr<DataStruct::Type> ty;
    switch (kind) {
        case kvoid:   ty = TYPE_VOID; goto end;
        case kbool:   ty = make_numtype(KIND_BOOL, false); goto end;
        case kchar:   ty = make_numtype(KIND_CHAR, sig == kunsigned); goto end;
        case kfloat:  ty = make_numtype(KIND_FLOAT, false); goto end;
        case kdouble: ty = make_numtype(size == klong ? KIND_LDOUBLE : KIND_DOUBLE, false); goto end;
        default: break;
    }
    switch (size) {
        case kshort: ty = make_numtype(KIND_SHORT, sig == kunsigned); goto end;
        case klong:  ty = make_numtype(KIND_LONG, sig == kunsigned); goto end;
        case kllong: ty = make_numtype(KIND_LLONG, sig == kunsigned); goto end;
        default:     ty = make_numtype(KIND_INT, sig == kunsigned); goto end;
    }
    Error::error("internal error: kind: %d, size: %d", kind, size);
    end:
    if (align != -1)
        ty->align = align;
    return ty;
    err:
    Error::errort(tok, "type mismatch: %s", Utils::tok2s(tok));
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
    Dict *fields = read_rectype_fields(&size, &align, is_struct);
    r->align = align;
    if (fields) {
        r->fields = fields;
        r->size = size;
    }
    return r;
}
std::string Parser::read_rectype_tag(){
    CHECK_CPP();
    if (!next_token('{'))
        return NULL;
    Vector *fields = read_rectype_fields_sub();
    fix_rectype_flexible_member(fields);
    if (is_struct)
        return update_struct_offset(rsize, align, fields);
    return update_union_offset(rsize, align, fields);
}

void read_rectype_fields(int &size,int &align,bool is_struct){

}