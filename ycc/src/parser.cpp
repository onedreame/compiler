//
// Created by yk on 18-2-8.
//
#include "../include/parser.h"
#include "../include/encode.h"

void Parser::CHECK_CPP(){
    if (macro== nullptr) {
        Error::error("MacroPropressor in Parser should be initialized.");
    }
}
void Parser::CHECK_LEX(){
    if (lex== nullptr)
        Error::error("Lex in Parser should be initialized.");
}
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
//array type：details include the total bytes size,length,aiign, element type
//summary: the array includes how many elements,and what is the element,how much memory they will consume.
std::shared_ptr<DataStruct::Type> Parser::make_array_type(const std::shared_ptr<DataStruct::Type>&type, int len){
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
//构建temp name，格式为.T(id)
std::string  Parser::make_tempname() {
    static int c = 0;
    return Utils::format(".T%d", c++);
}
//构建编号标签，格式为 .L(id)
std::string Parser::make_label() {
    static int c = 0;
    return Utils::format(".L%d", c++);
}

//构建static label，格式为 .S(id).(name)
std::string Parser::make_static_label(const std::string &name) {
    static int c = 0;
    return Utils::format(".S%d.%s", c++, name);
}
//mark begin,end and label name
Parser::Case Parser::make_case(int beg, int end, const std::string &label) {
    return {beg,end,label};
}
//local value: 左值有名字和类型两种属性
std::shared_ptr<DataStruct::Node> Parser::ast_lvar(const std::shared_ptr<DataStruct::Type> &ty, const std::string &name) {
    auto r=std::make_shared<DataStruct::Node>(DataStruct::AST_TYPE::AST_LVAR,ty,sl);
    r->varname=name;
    if (localenv)
        localenv->operator[](name)= r;
    if (localvars)
        localvars->push_back(*r);
    return r;
}
//global value: unlike local value,global value has additional global name
std::shared_ptr<DataStruct::Node> Parser::ast_gvar(const std::shared_ptr<DataStruct::Type> &ty, const std::string &name) {
    auto r=std::make_shared<DataStruct::Node>(DataStruct::AST_TYPE::AST_LVAR,ty,sl);
    r->glabel=name;
    r->varname=name;
    globalenv->operator[](name)=r;
    return r;
}
std::shared_ptr<DataStruct::Node> Parser::ast_inttype(const std::shared_ptr<DataStruct::Type> &ty, long val) {
    auto r=std::make_shared<DataStruct::Node>();
    r->ival=val;
    r->setKind(DataStruct::AST_TYPE::AST_LITERAL);
    r->setSourceloc(sl);
    r->setTy(ty);
    return r;
}
std::shared_ptr<DataStruct::Node> Parser::ast_floattype(const std::shared_ptr<DataStruct::Type>&ty,double v){
    auto r=std::make_shared<DataStruct::Node>();
    r->fval=v;
    r->setTy(ty);
    r->setSourceloc(sl);
    r->setKind(DataStruct::AST_TYPE::AST_LITERAL);
    return r;
}
//string: its attr is obvious
std::shared_ptr<DataStruct::Node> Parser::ast_string(DataStruct::ENCODE enc, const std::string &str){
    std::shared_ptr<DataStruct::Type> ty;
    std::string body;

    switch (enc) {
        case DataStruct::ENCODE::ENC_NONE:
        case DataStruct::ENCODE::ENC_UTF8:
            ty = make_array_type(TYPE_CHAR, str.length());
            body = str;
            break;
        case DataStruct::ENCODE::ENC_CHAR16: {
            auto b = Utils::to_utf16(const_cast<char *>(&str[0]), str.length());
            ty = make_array_type(TYPE_USHORT, b.size() / TYPE_USHORT->size);
            body = b;
            break;
        }
        case DataStruct::ENCODE::ENC_CHAR32:
        case DataStruct::ENCODE::ENC_WCHAR: {
            auto b = Utils::to_utf32(const_cast<char *>(&str[0]), str.size());
            ty = make_array_type(TYPE_UINT, b.size() / TYPE_UINT->size);
            body = b;
            break;
        }
    }
    auto node =std::make_shared<DataStruct::Node>();
    node->setTy(ty);
    node->setKind(DataStruct::AST_TYPE::AST_LITERAL);
    node->setSourceloc(sl);
    node->sval=body;
    return node;
}
//function describe: we only need its name,and its detailed information is stored in its type.
std::shared_ptr<DataStruct::Node> Parser::ast_funcdesg(const std::shared_ptr<DataStruct::Type> &ty, const std::string &fname) {
    auto node=std::make_shared<DataStruct::Node>();
    node->setSourceloc(sl);
    node->setKind(DataStruct::AST_TYPE::AST_FUNCDESG);
    node->setTy(ty);
    node->fname=fname;
    return node;
}
//unary operator
std::shared_ptr<DataStruct::Node> Parser::ast_uop(DataStruct::AST_TYPE kind, const std::shared_ptr<DataStruct::Type> &ty, const std::shared_ptr<DataStruct::Node> &operand) {
    auto node=std::make_shared<DataStruct::Node>();
    node->setKind(kind);
    node->setTy(ty);
    node->setSourceloc(sl);
    node->unop=operand;
    return node;
}
//binary operator
std::shared_ptr<DataStruct::Node> Parser::ast_biop(DataStruct::AST_TYPE kind,
                                           const std::shared_ptr<DataStruct::Type> &ty,
                                           const std::shared_ptr<DataStruct::Node> &left,
                                           const std::shared_ptr<DataStruct::Node> &right){
    auto node=std::make_shared<DataStruct::Node>(kind,ty,sl);
    node->left=left;
    node->right=right;
    return node;
}
//converison: for conversion,we need to know that which node to conversion and which type converted to.
std::shared_ptr<DataStruct::Node> Parser::ast_conv(const std::shared_ptr<DataStruct::Type> &totype, const std::shared_ptr<DataStruct::Node> &val) {
    auto node=std::make_shared<DataStruct::Node>();
    node->setKind(DataStruct::AST_TYPE::AST_CONV);
    node->setSourceloc(sl);
    node->setTy(totype);
    node->unop=val;
    return node;
}
//typedef just creates a alias name of a type,so we simply put it into localenv or global env
std::shared_ptr<DataStruct::Node> Parser::ast_typedef(const std::shared_ptr<DataStruct::Type>&ty, const std::string&name){
    auto node=std::make_shared<DataStruct::Node>();
    node->setSourceloc(sl);
    node->setTy(ty);
    node->setKind(DataStruct::AST_TYPE::AST_TYPEDEF);
    env()->operator[](name)=node;
    return node;
}
//static local value: static local value is global in its context,so it has global name
std::shared_ptr<DataStruct::Node> Parser::ast_static_lvar(const std::shared_ptr<DataStruct::Type> &ty, const std::string &name) {
    auto node=std::make_shared<DataStruct::Node>();
    node->setKind(DataStruct::AST_TYPE::AST_GVAR);
    node->setTy(ty);
    node->setSourceloc(sl);
    node->varname=name;
    node->glabel=make_static_label(name);
    if(!localenv)
        Error::error("localenv is null");
    localenv->operator[](name)= node;
    return node;
}
//function call: what we want is the return value (if exits) of the call.
std::shared_ptr<DataStruct::Node> Parser::ast_funcall(const std::shared_ptr<DataStruct::Type> &ftype,
                                              const std::string &fname,
                                              const std::vector<DataStruct::Type> &args){
    auto node=std::make_shared<DataStruct::Node>(DataStruct::AST_TYPE::AST_FUNCALL,ftype->rettype,sl);
    node->fname=fname;
    node->args=args;
    node->ftype=ftype;
    return node;
}
//function ptr call: for function ptr,we should first resolve its real type,then the last is the same as funcall.
std::shared_ptr<DataStruct::Node> Parser::ast_funcptr_call(std::shared_ptr<DataStruct::Node>&fptr,
                                                   const std::vector<DataStruct::Type> &args) {
    if (fptr->getTy()->kind!=DataStruct::TYPE_KIND::KIND_PTR)
        Error::error("%s is not a func pointer",Utils::ty2s(fptr->getTy()));
    if (fptr->getTy()->ptr->kind!=DataStruct::TYPE_KIND::KIND_FUNC)
        Error::error("%s is not a func",Utils::ty2s(fptr->getTy()));
    auto node=std::make_shared<DataStruct::Node>(DataStruct::AST_TYPE::AST_FUNCPTR_CALL,fptr->getTy()->ptr->rettype,sl);
    node->fptr=fptr;
    node->args=args;
    return node;
}
//func: for func,we need to know its name,params,emmmm..I will think it later
std::shared_ptr<DataStruct::Node> Parser::ast_func(const std::shared_ptr<DataStruct::Type> &ty,
                                            const std::string &fname,
                                            const std::vector<DataStruct::Node>&params,
                                            const std::shared_ptr<DataStruct::Node> &body,
                                            const std::vector<DataStruct::Node> &localvars) {
    auto node=std::make_shared<DataStruct::Node>(DataStruct::AST_TYPE::AST_FUNC,ty,sl);
    node->fname=fname;
    node->params=params;
    node->localvars=localvars;
    node->body=body;
    return node;
}
//declaration: for declaration,it may have initialzier,so we need to know its node info and initializer info.
std::shared_ptr<DataStruct::Node> Parser::ast_decl(const std::shared_ptr<DataStruct::Node> &var,
                                                   const std::vector<std::shared_ptr<DataStruct::Node>> &init) {
    auto node=std::make_shared<DataStruct::Node>(DataStruct::AST_TYPE::AST_DECL, nullptr,sl);
    node->declvar=var;
    node->declinit=init;
    return node;
}
//initializer: for initializer,we need to know its initial value,and there may be conversions,so we also
//need to know the type it will converts to. for array or aggregate,we need to know the offset of the element to be initialized.
std::shared_ptr<DataStruct::Node> Parser::ast_init(const std::shared_ptr<DataStruct::Node> &val,
                                           const std::shared_ptr<DataStruct::Type> &totype, int off) {
    auto node=std::make_shared<DataStruct::Node>(DataStruct::AST_TYPE::AST_INIT, nullptr,sl);
    node->initval=val;
    node->initoff=off;
    node->totype=totype;
    return node;
}
//if: for if, we need to know its condition ahd its branches.
std::shared_ptr<DataStruct::Node> Parser::ast_if(const std::shared_ptr<DataStruct::Node> &cond,
                                         const std::shared_ptr<DataStruct::Node> &then,
                                         const std::shared_ptr<DataStruct::Node> &els){
    auto node=std::make_shared<DataStruct::Node>(DataStruct::AST_TYPE::AST_IF, nullptr,sl);
    node->cond=cond;
    node->then=then;
    node->els=els;
    return node;
}
//ternary: like if,but what is the meaning of the type?
std::shared_ptr<DataStruct::Node> Parser::ast_ternary(const std::shared_ptr<DataStruct::Type> &ty,
                                              const std::shared_ptr<DataStruct::Node> &cond,
                                              const std::shared_ptr<DataStruct::Node> &then,
                                              const std::shared_ptr<DataStruct::Node> &els){
    auto node=std::make_shared<DataStruct::Node>(DataStruct::AST_TYPE::AST_TERNARY, ty,sl);
    node->cond=cond;
    node->then=then;
    node->els=els;
    return node;
}
//return: for return ,we only want to know the value it returns.
std::shared_ptr<DataStruct::Node> Parser::ast_return(const std::shared_ptr<DataStruct::Node> &retval){
    auto node=std::make_shared<DataStruct::Node>(DataStruct::AST_TYPE::AST_RETURN, nullptr,sl);
    node->retval=retval;
    return node;
}
//struct: we need to know members of a named struct;
std::shared_ptr<DataStruct::Node> Parser::ast_struct_ref(const std::shared_ptr<DataStruct::Type> &ty,
                                                 const std::shared_ptr<DataStruct::Node> &struc,
                                                 const std::string &name){
    auto node=std::make_shared<DataStruct::Node>(DataStruct::AST_TYPE::AST_STRUCT_REF, ty,sl);
    node->struc=struc;
    node->field=name;
    return node;
}
//compound: save std::vector<std::shared_ptr<DataStruct::Node>> stmts params,means all node in this block
std::shared_ptr<DataStruct::Node> Parser::ast_compound_stmt(const std::vector<std::shared_ptr<DataStruct::Node>>*stmts){
    auto node=std::make_shared<DataStruct::Node>(DataStruct::AST_TYPE::AST_COMPOUND_STMT, nullptr,sl);
    node->stmts=*stmts;
    return node;
}
//goto: for goto,we need to know the label where to jump
std::shared_ptr<DataStruct::Node> Parser::ast_goto(const std::string &label){
    auto node=std::make_shared<DataStruct::Node>(DataStruct::AST_TYPE::AST_GOTO, nullptr,sl);
    node->label=label;
    return node;
}
//jump: for jump,what is the meaning of newlabe?
std::shared_ptr<DataStruct::Node> Parser::ast_jump(const std::string &label){
    auto node=std::make_shared<DataStruct::Node>(DataStruct::AST_TYPE::AST_GOTO, nullptr,sl);
    node->label=label;
    node->newlabel=label;
    return node;
}
//computed_goto: we need to know the expression to compute
std::shared_ptr<DataStruct::Node> Parser::ast_computed_goto( const std::shared_ptr<DataStruct::Node> &expr){
    auto node=std::make_shared<DataStruct::Node>(DataStruct::AST_TYPE::AST_COMPUTED_GOTO, nullptr,sl);
    node->unop=expr;
    return node;
}
//label: just create a node with label name
std::shared_ptr<DataStruct::Node> Parser::ast_label(const std::string &label){
    auto node=std::make_shared<DataStruct::Node>(DataStruct::AST_TYPE::AST_LABEL, nullptr,sl);
    node->label=label;
    return node;
}
//destion: like label,with a additional newlabel attribute.
std::shared_ptr<DataStruct::Node> Parser::ast_dest(const std::string &label){
    auto node=std::make_shared<DataStruct::Node>(DataStruct::AST_TYPE::AST_LABEL, nullptr,sl);
    node->label=label;
    node->newlabel=label;
    return node;
}
//label address??????
std::shared_ptr<DataStruct::Node> Parser::ast_label_addr(const std::string &label){
    auto node=std::make_shared<DataStruct::Node>(DataStruct::AST_TYPE::OP_LABEL_ADDR, make_ptr_type(TYPE_VOID),sl);
    node->label=label;
    return node;
}
//local var,global var,struct ref, deref(???,what is this?)
void Parser::ensure_lvalue(const std::shared_ptr<DataStruct::Node> &node){
    switch (node->getKind()) {
        case DataStruct::AST_TYPE::AST_LVAR: case DataStruct::AST_TYPE::AST_GVAR:
        case DataStruct::AST_TYPE::AST_DEREF: case DataStruct::AST_TYPE::AST_STRUCT_REF:
            return;
        default:
            Error::error("lvalue expected, but got %s", Utils::node2s(node));
    }
}
//ensure node is int type
void Parser::ensure_inttype(const std::shared_ptr<DataStruct::Node> &node){
    if (!is_inttype(node->getTy()))
        Error::error("integer type expected, but got %s", Utils::node2s(node));
}
//only bool, char, short,int,long,long long are int type.
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
//ensure node is int type or float type
void Parser::ensure_arithtype(const std::shared_ptr<DataStruct::Node> &node) {
    if (!is_arithtype(node->getTy()))
        Error::error("arithmetic type expected, but got %s", Utils::node2s(node));
}
//float, double, long double
bool Parser::is_flotype(const std::shared_ptr<DataStruct::Type> &ty) {
    switch (ty->kind) {
        case DataStruct::TYPE_KIND::KIND_FLOAT: case DataStruct::TYPE_KIND::KIND_DOUBLE:
        case DataStruct::TYPE_KIND::KIND_LDOUBLE:
            return true;
        default:
            return false;
    }
}
//whether ty is int type or float type
bool Parser::is_arithtype(const std::shared_ptr<DataStruct::Type> &ty) {
    return is_inttype(ty) || is_flotype(ty);
}
//wheather ty is char array
bool Parser::is_string(const std::shared_ptr<DataStruct::Type> &ty) {
    return ty->kind == DataStruct::TYPE_KIND::KIND_ARRAY && ty->ptr->kind == DataStruct::TYPE_KIND::KIND_CHAR;
}

//if ty->len==-1,ther return its copy,else return itself.
std::shared_ptr<DataStruct::Type> copy_incomplete_type(const std::shared_ptr<DataStruct::Type> &ty) {
    if (!ty) return nullptr;
    return (ty->len == -1) ? std::make_shared<DataStruct::Type>(*ty) : ty;
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

int Parser::eval_struct_ref(const std::shared_ptr<DataStruct::Node> &node, int offset) {
    if (node->getKind() == DataStruct::AST_TYPE::AST_STRUCT_REF)
        return eval_struct_ref(node->struc, node->getTy()->offset + offset);
    return eval_intexpr(node, nullptr) + offset;
}
int Parser::eval_intexpr(const std::shared_ptr<DataStruct::Node> &node, const std::shared_ptr<DataStruct::Node> *addr) {
    switch (node->getKind()) {
        case DataStruct::AST_TYPE::AST_LITERAL:
            if (is_inttype(node->getTy()))
                return node->ival;
            Error::error("Integer expression expected, but got %s", Utils::node2s(node));
        case DataStruct::AST_TYPE::EXCLAMATION: return !eval_intexpr(node->unop, addr);
        case DataStruct::AST_TYPE::NEG: return ~eval_intexpr(node->unop, addr);
        case DataStruct::AST_TYPE::OP_CAST: return eval_intexpr(node->unop, addr);
        case DataStruct::AST_TYPE::AST_CONV: return eval_intexpr(node->unop, addr);
        case DataStruct::AST_TYPE::AST_ADDR:
            if (node->unop->getKind() == DataStruct::AST_TYPE::AST_STRUCT_REF)
                return eval_struct_ref(node->unop, 0);
            // fallthrough
        case DataStruct::AST_TYPE::AST_GVAR:
            if (addr) {
                addr = conv(node);
                return 0;
            }
            goto error;
            goto error;
        case DataStruct::AST_TYPE::AST_DEREF:
            if (node->unop->getTy()->kind == DataStruct::TYPE_KIND::KIND_PTR)
                return eval_intexpr(node->unop, addr);
            goto error;
        case DataStruct::AST_TYPE::AST_TERNARY: {
            long cond = eval_intexpr(node->cond, addr);
            if (cond)
                return node->then ? eval_intexpr(node->then, addr) : cond;
            return eval_intexpr(node->els, addr);
        }
#define L (eval_intexpr(node->left, addr))
#define R (eval_intexpr(node->right, addr))
        case DataStruct::AST_TYPE::PLUS: return L + R;
        case DataStruct::AST_TYPE::SUB: return L - R;
        case DataStruct::AST_TYPE::MUL: return L * R;
        case DataStruct::AST_TYPE::DIV: return L / R;
        case DataStruct::AST_TYPE::LOW: return L < R;
        case DataStruct::AST_TYPE::NOT: return L ^ R;
        case DataStruct::AST_TYPE::AND: return L & R;
        case DataStruct::AST_TYPE::OR: return L | R;
        case DataStruct::AST_TYPE::LEFT: return L % R;
        case DataStruct::AST_TYPE::OP_EQ: return L == R;
        case DataStruct::AST_TYPE::OP_LE: return L <= R;
        case DataStruct::AST_TYPE::OP_NE: return L != R;
        case DataStruct::AST_TYPE::OP_SAL: return L << R;
        case DataStruct::AST_TYPE::OP_SAR: return L >> R;
        case DataStruct::AST_TYPE::OP_SHR: return ((unsigned long)L) >> R;
        case DataStruct::AST_TYPE::OP_LOGAND: return L && R;
        case DataStruct::AST_TYPE::OP_LOGOR:  return L || R;
#undef L
#undef R
        default:
        error:
            Error::error("Integer expression expected, but got %s", Utils::node2s(node));
    }
}
int Parser::read_intexpr() {
    return eval_intexpr(read_conditional_expr(), nullptr);
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
    CHECK_LEX();
    int nest=0;
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
std::vector<DataStruct::Node> Parser::read_oldstyle_param_args() {
    CHECK_CPP();
    CHECK_LEX();
    auto orig = localenv;
    localenv = nullptr;
    std::vector<DataStruct::Node> r;
    for (;;) {
        if (lex->is_keyword(macro->peek_token(), lex->get_keywords("{")))
            break;
        if (!is_type(macro->peek_token()))
            Error::errort(macro->peek_token(), "K&R-style declarator expected, but got %s", Utils::tok2s(macro->peek_token()));
        read_decl(&r, false);
    }
    localenv = orig;
    return r;
}
//at the begining,we set all params type as int,so here,we fix this problem.
//@params:placeholder param, int type
//@declvars: the true type we parse
void Parser::update_oldstyle_param_type(std::vector<DataStruct::Node>  *params, std::vector<DataStruct::Node> &declvars) {
    for (auto&decl:declvars) {
        if (decl.getKind()!=DataStruct::AST_TYPE::AST_DECL)
            Error::error("%s is not a declation type.",Utils::node2s(std::make_shared<DataStruct::Node>(decl)));
        auto var = decl.declvar;
        if (var->getKind()!=DataStruct::AST_TYPE::AST_LVAR)
            Error::error("%s is not a local var type",Utils::node2s(decl.declvar));
        for(auto& param:*params){
            if(param.getKind()!=DataStruct::AST_TYPE::AST_LVAR)
                Error::error("%s is not a local var type",Utils::node2s(std::make_shared<DataStruct::Node>(param)));
            if (param.varname!= var->varname)
                continue;
            param.setTy(var->getTy());
            goto found;
        }
        Error::error("missing parameter: %s", var->varname);
        found:;
    }
}

void Parser::read_oldstyle_param_type(std::vector<DataStruct::Node> *params) {
    auto vars = read_oldstyle_param_args();
    update_oldstyle_param_type(params, vars);
}
//get all parameters types,without name.
std::vector<std::shared_ptr<DataStruct::Type>> Parser::param_types(std::vector<DataStruct::Node> *params) {
    std::vector<std::shared_ptr<DataStruct::Type>> r;
    for (auto&param:*params) {
        r.emplace_back(param.getTy());
    }
    return r;
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
            read_decl(toplevels.get(), true);
    }
}

DataStruct::Node Parser::read_funcdef() {
    CHECK_LEX();
    CHECK_CPP();
    DataStruct::QUALITIFIER sclass;
    std::shared_ptr<DataStruct::Type> basetype = read_decl_spec_opt(&sclass);
    localenv=std::make_shared<std::unordered_map<std::string,std::shared_ptr<DataStruct::Node>>>();
    scope[localenv]=globalenv;
    gotos.clear();
    labels.clear();
    std::string name;
    std::vector<DataStruct::Node> params;
    auto functype = read_declarator(&name, basetype, &params, DataStruct::DECL_TYPE::DECL_BODY);
    if (functype->oldstyle) {
        if (params.empty())
            functype->hasva = false;
        read_oldstyle_param_type(&params);
        functype->params = param_types(&params);
    }
    functype->isstatic = (sclass == DataStruct::QUALITIFIER::S_STATIC);
    ast_gvar(functype, name);
    macro->expect(lex->get_keywords("{"));
    auto r = read_func_body(functype, name, params);
    backfill_labels();
    localenv = nullptr;
    return *r;
}
std::shared_ptr<DataStruct::Node> Parser::read_func_body(const std::shared_ptr<DataStruct::Type>&functype, const std::string &fname, std::vector<DataStruct::Node> &params){
    decltype(localenv) tmp=std::make_shared<std::unordered_map<std::string,std::shared_ptr<DataStruct::Node>>>();
    scope[tmp]=localenv;
    localenv = tmp;
    localvars=std::make_shared<std::vector<DataStruct::Node>>();
    current_func_type = functype;
    auto funcname = ast_string(DataStruct::ENCODE::ENC_NONE, fname);
    localenv->operator[]("__func__") = funcname;
    localenv->operator[]("__FUNCTION__") = funcname;
    auto body = read_compound_stmt();
    auto r = ast_func(functype, fname, params, body, *localvars);
    current_func_type = nullptr;
    localenv = nullptr;
    localvars = nullptr;
    return r;
}
std::shared_ptr<DataStruct::Type> Parser::read_int_suffix(const std::string &s){
    if (lower(s)=="u")
        return TYPE_UINT;
    if (lower(s)=="l")
        return TYPE_LONG;
    if (lower(s) =="ul" || lower(s)== "lu")
        return TYPE_ULONG;
    if (lower(s) == "ll")
        return TYPE_LLONG;
    if (lower(s) == "ull" || lower(s) == "llu")
        return TYPE_ULLONG;
    return nullptr;
}
//read int from tok.sval
std::shared_ptr<DataStruct::Node> Parser::read_int(const DataStruct::Token &tok){
    auto& s = *(tok.sval);
    char *end;
    long v = lower(s).substr(0,2)== "0b"
             ? strtoul(&s[2], &end, 2) : strtoul(&s[0], &end, 0);
    auto ty = read_int_suffix(end);
    if (ty)
        return ast_inttype(ty, v);
    if (*end != '\0')
        Error::errort(tok, "invalid character '%c': %s", *end, s);

    // C11 6.4.4.1 p 5 long, long longjunshi均是8字节，要注意是否溢出
    bool base10 = (s[0] != '0');
    if (base10) {
        ty = !(v & ~(long)INT_MAX) ? TYPE_INT : TYPE_LONG;
        return ast_inttype(ty, v);
    }
    // Octal or hexadecimal constant type 可能是unsigned
    ty = !(v & ~(unsigned long)INT_MAX) ? TYPE_INT
                                        : !(v & ~(unsigned long)UINT_MAX) ? TYPE_UINT
                                                                          : !(v & ~(unsigned long)LONG_MAX) ? TYPE_LONG
                                                                                                            : TYPE_ULONG;
    return ast_inttype(ty, v);
}
//read float from tok.sval
std::shared_ptr<DataStruct::Node> Parser::read_float(const DataStruct::Token &tok){
    auto &s = *(tok.sval);
    char *end;
    double v = strtod(&s[0], &end);
    // C11 6.4.4.2 p4:
    if (lower(std::string(end))== "l")
        return ast_floattype(TYPE_LDOUBLE, v);
    if (lower(std::string(end))== "f")
        return ast_floattype(TYPE_FLOAT, v);
    if (*end != '\0')
        Error::errort(tok, "invalid character '%c': %s", *end, s);
    return ast_floattype(TYPE_DOUBLE, v);
}
//read int or float from tok.sval
std::shared_ptr<DataStruct::Node> Parser::read_number(const DataStruct::Token &tok){
    auto &s = *(tok.sval);
    bool isfloat = strpbrk(&s[0], ".pP") || (lower(s).substr(0,2) !="0x"&& strpbrk(&s[0], "eE"));
    return isfloat ? read_float(tok) : read_int(tok);
}
std::shared_ptr<DataStruct::Node> Parser::read_generic(){
    CHECK_CPP();
    CHECK_LEX();
    macro->expect(lex->get_keywords("("));
    const DataStruct::Token &tok = macro->peek_token();
    auto contexpr = read_assignment_expr();
    macro->expect(lex->get_keywords(","));
    decltype(contexpr) defaultexpr = nullptr;
    Vector *list = read_generic_list(&defaultexpr);
    for (int i = 0; i < vec_len(list); i++) {
        void **pair = vec_get(list, i);
        Type *ty = pair[0];
        Node *expr = pair[1];
        if (type_compatible(contexpr->ty, ty))
            return expr;
    }
    if (!defaultexpr)
        Error::errort(tok, "no matching generic selection for %s: %s", Utils::node2s(contexpr), Utils::ty2s(contexpr->getTy()));
    return defaultexpr;
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
//create static local var, may be initialized if has a following "=", staitc var should be put into global
void Parser::read_static_local_var(const std::shared_ptr<DataStruct::Type>& ty, const std::string& name){
    CHECK_CPP();
    CHECK_LEX();
    auto var = ast_static_lvar(ty, name);
    std::vector<std::shared_ptr<DataStruct::Node>> init;
    if (macro->next(lex->get_keywords("="))) {
        auto orig = localenv;
        localenv = nullptr;
        init = read_decl_init(ty);
        localenv = orig;
    }
    toplevels->emplace_back(ast_decl(var, init));
}
//6.7 Declarations,
//paramsters type:  6.7.1 At most, one storage-class specifier may be given in the declaration specifiers in a
//declaration, except that _Thread_local may appear with static or extern
//(1) typedef
//(2) static, can't be global
//(3)local
//(4)global,may be extern
void Parser::read_decl(std::vector<DataStruct::Node>* block, bool isglobal){
    CHECK_LEX();
    CHECK_CPP();
    DataStruct::QUALITIFIER sclass = DataStruct::QUALITIFIER::S_PLACEHOLDER;
    auto basetype = read_decl_spec_opt(&sclass);
    if (macro->next(lex->get_keywords(";")))
        return;
    for (;;) {
        std::string name;
        auto ty = read_declarator(&name, copy_incomplete_type(basetype), nullptr, DataStruct::DECL_TYPE::DECL_BODY);
        ty->isstatic = (sclass == DataStruct::QUALITIFIER::S_STATIC);
        if (sclass == DataStruct::QUALITIFIER::S_TYPEDEF) {
            ast_typedef(ty, name);
        } else if (ty->isstatic && !isglobal) {
            ensure_not_void(ty);
            read_static_local_var(ty, name);
        } else {
            ensure_not_void(ty);
            auto var = (isglobal ? ast_gvar : ast_lvar)(ty, name);
            if (macro->next(DataStruct::AST_TYPE::ASSIGN)) {
                block->emplace_back(ast_decl(var, read_decl_init(ty)));
            } else if (sclass != DataStruct::QUALITIFIER::S_EXTERN && ty->kind != DataStruct::TYPE_KIND::KIND_FUNC) {
                block->emplace_back(ast_decl(var, nullptr));
            }
        }
        if (macro->next(lex->get_keywords(";")))
            return;
        if (!macro->next(lex->get_keywords(",")))
            Error::errort(macro->peek_token(), "';' or ',' are expected, but got %s", Utils::tok2s(macro->peek_token()));
    }
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
                // C11 6.7.5 p6
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
        //这里也是c++11新增强枚举类型的原因之一
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
    CHECK_LEX();CHECK_CPP();
    std::vector<std::pair<std::string,std::shared_ptr<DataStruct::Type>>> r;
    while (1){
        if (macro->next(lex->get_keywords("_Static_assert"))) {
            //http://zh.cppreference.com/w/c/language/_Static_assert
            read_static_assert();
            continue;
        }
        if (!is_type(macro->peek_token()))
            break;
        //6.7.2.1 注意struct内部只能是type-specifiers或type-qualifiers
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
    size = maxsize + compute_padding(maxsize, align);  //p 17 extra paddings
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
    for (auto&ele:unnamed->fields) {
        std::shared_ptr<DataStruct::Type> t=std::make_shared<DataStruct::Type>(ele.second);
        t->offset += offset;
        dict.emplace_back(std::make_shared(ele.first,t));
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
std::shared_ptr<DataStruct::Node> Parser::binop(DataStruct::AST_TYPE op, const std::shared_ptr<DataStruct::Node> &lhs, const std::shared_ptr<DataStruct::Node> &rhs){
    if (lhs->getTy()->kind == DataStruct::TYPE_KIND::KIND_PTR && rhs->getTy()->kind == DataStruct::TYPE_KIND::KIND_PTR) {
        if (!valid_pointer_binop(op))
            Error::error("invalid pointer arith");
        // C11 6.5.6.9: Pointer subtractions have type ptrdiff_t.
        if (op == DataStruct::AST_TYPE::SUB)
            return ast_biop(op, TYPE_LONG, lhs, rhs);
        // C11 6.5.8.6, 6.5.9.3: Pointer comparisons have type int.
        return ast_biop(op, TYPE_INT, lhs, rhs);
    }
    if (lhs->getTy()->kind == DataStruct::TYPE_KIND::KIND_PTR)
        return ast_biop(op, lhs->getTy(), lhs, rhs);
    if (rhs->getTy()->kind == DataStruct::TYPE_KIND::KIND_PTR)
        return ast_biop(op, rhs->getTy(), rhs, lhs);
    if (!is_arithtype(lhs->getTy()))
        Error::error("lhs shoudle be arithtype");
    if (!is_arithtype(rhs->getTy()))
        Error::error("rhs shoudle be arithtype");
    auto r = usual_arith_conv(lhs->getTy(), rhs->getTy());
    return ast_biop(op, r, wrap(r, lhs), wrap(r, rhs));
}
bool Parser::is_same_struct(const std::shared_ptr<DataStruct::Type> &a, const std::shared_ptr<DataStruct::Type> &b){
    if (a->kind != b->kind)
        return false;
    switch (a->kind) {
        case DataStruct::TYPE_KIND::KIND_ARRAY:
            return a->len == b->len &&
                   is_same_struct(a->ptr, b->ptr);
        case DataStruct::TYPE_KIND::KIND_PTR:
            return is_same_struct(a->ptr, b->ptr);
        case DataStruct::TYPE_KIND::KIND_STRUCT: {
            if (a->is_struct != b->is_struct)
                return false;
            auto key_selector=[](decltype(a->fields[0]) & ele){ return ele.second;};
//            Vector *ka = dict_keys(a->fields);
//            Vector *kb = dict_keys(b->fields);
            std::vector<decltype(a->fields[0].second)> ka;
            std::vector<decltype(a->fields[0].second)> kb;
            ka.reserve(a->fields.size());
            kb.reserve(b->fields.size());
            std::transform(a->fields.begin(),a->fields.end(),ka.begin(),key_selector);
            std::transform(b->fields.begin(),b->fields.end(),kb.begin(),key_selector);
            if (ka.size() != kb.size())
                return false;
            for (int i = 0; i < vec_len(ka); i++)
                if (!is_same_struct(vec_get(ka, i), vec_get(kb, i)))
                    return false;
            return true;
        }
        default:
            return true;
    }
}
void Parser::ensure_assignable(const std::shared_ptr<DataStruct::Type> &totype, const std::shared_ptr<DataStruct::Type> &fromtype){
    if ((is_arithtype(totype) || totype->kind == DataStruct::TYPE_KIND::KIND_PTR) &&
        (is_arithtype(fromtype) || fromtype->kind == DataStruct::TYPE_KIND::KIND_PTR))
        return;
    if (is_same_struct(totype, fromtype))
        return;
    Error::error("incompatible kind: <%s> <%s>", Utils::ty2s(totype), Utils::ty2s(fromtype));
}
//6.3.1 下面是类型转换处理，标准中说的很清楚，对照实现就好了
std::shared_ptr<DataStruct::Node> Parser::conv(const std::shared_ptr<DataStruct::Node> &node) {
    if (!node)
        return nullptr;
    auto ty = node->getTy();
    switch (ty->kind) {
        case DataStruct::TYPE_KIND::KIND_ARRAY:
            // C11 6.3.2.1p3: An array of T is converted to a pointer to T.
            return ast_uop(DataStruct::AST_TYPE::AST_CONV, make_ptr_type(ty->ptr), node);
        case DataStruct::TYPE_KIND::KIND_FUNC:
            // C11 6.3.2.1p4: A function designator is converted to a pointer to the function.
            return ast_uop(DataStruct::AST_TYPE::AST_ADDR, make_ptr_type(ty), node);
        case DataStruct::TYPE_KIND::KIND_SHORT: case DataStruct::TYPE_KIND::KIND_CHAR: case DataStruct::TYPE_KIND::KIND_BOOL:
            // C11 6.3.1.1p2: The integer promotions
            return ast_conv(TYPE_INT, node);
        case DataStruct::TYPE_KIND::KIND_INT:
            if (ty->bitsize > 0)
                return ast_conv(TYPE_INT, node);
    }
    return node;
}
//whether the two type object's kind and sign is the same
bool Parser::same_arith_type(const std::shared_ptr<DataStruct::Type> &t, const std::shared_ptr<DataStruct::Type> &u) {
    return t->kind == u->kind && t->usig == u->usig;
}

std::shared_ptr<DataStruct::Node> Parser::wrap(const std::shared_ptr<DataStruct::Type> &t, const std::shared_ptr<DataStruct::Node> &node) {
    if (same_arith_type(t, node->getTy()))
        return node;
    return ast_uop(DataStruct::AST_TYPE::AST_CONV, t, node);
}

// C11 6.3.1.8
std::shared_ptr<DataStruct::Type> Parser::usual_arith_conv(std::shared_ptr<DataStruct::Type> t, std::shared_ptr<DataStruct::Type> u) {
    if (!is_arithtype(t))
        Error::error("%s is not arithtype.",Utils::ty2s(t));
    if (!is_arithtype(u))
        Error::error("%s is not arithtype.",Utils::ty2s(u));
    if (t->kind < u->kind) {
        auto tmp = t;
        t = u;
        u = tmp;
    }
    if (is_flotype(t))
        return t;
    if (!is_inttype(t)||t->size >= TYPE_INT->size)
        Error::error("%s is not int type or size less than int",Utils::ty2s(t));
    if (!is_inttype(u)||u->size >= TYPE_INT->size)
        Error::error("%s is not int type or size less than int",Utils::ty2s(u));
    if (t->size > u->size)
        return t;
    if (t->size!=u->size)
        Error::error("%s and %s are not the same size",Utils::ty2s(t),Utils::ty2s(u));
    if (t->usig == u->usig)
        return t;
    decltype(t) r = std::make_shared<DataStruct::Type>(*t);
    r->usig = true;
    return r;
}

bool Parser::valid_pointer_binop(DataStruct::AST_TYPE op) {
    switch (op) {
        case DataStruct::AST_TYPE::SUB: case DataStruct::AST_TYPE::LOW: case DataStruct::AST_TYPE::HIG: case DataStruct::AST_TYPE::OP_EQ:
        case DataStruct::AST_TYPE::OP_NE: case DataStruct::AST_TYPE::OP_GE: case DataStruct::AST_TYPE::OP_LE:
            return true;
        default:
            return false;
    }
}
std::shared_ptr<DataStruct::Node> binop(DataStruct::AST_TYPE op, const std::shared_ptr<DataStruct::Node> &lhs, const std::shared_ptr<DataStruct::Node> &rhs) {
    if (lhs->ty->kind == KIND_PTR && rhs->ty->kind == KIND_PTR) {
        if (!valid_pointer_binop(op))
            Error::error("invalid pointer arith");
        // C11 6.5.6.9: Pointer subtractions have type ptrdiff_t.
        if (op == '-')
            return ast_binop(type_long, op, lhs, rhs);
        // C11 6.5.8.6, 6.5.9.3: Pointer comparisons have type int.
        return ast_binop(type_int, op, lhs, rhs);
    }
    if (lhs->ty->kind == KIND_PTR)
        return ast_binop(lhs->ty, op, lhs, rhs);
    if (rhs->ty->kind == KIND_PTR)
        return ast_binop(rhs->ty, op, rhs, lhs);
    assert(is_arithtype(lhs->ty));
    assert(is_arithtype(rhs->ty));
    Type *r = usual_arith_conv(lhs->ty, rhs->ty);
    return ast_binop(r, op, wrap(r, lhs), wrap(r, rhs));
}
bool is_same_struct(const std::shared_ptr<DataStruct::Type> &a, const std::shared_ptr<DataStruct::Type> &b) {
    if (a->kind != b->kind)
        return false;
    switch (a->kind) {
        case DataStruct::TYPE_KIND::KIND_ARRAY:
            return a->len == b->len &&
                   is_same_struct(a->ptr, b->ptr);
        case DataStruct::TYPE_KIND::KIND_PTR:
            return is_same_struct(a->ptr, b->ptr);
        case DataStruct::TYPE_KIND::KIND_STRUCT: {
            if (a->is_struct != b->is_struct)
                return false;
            Vector *ka = dict_keys(a->fields);
            Vector *kb = dict_keys(b->fields);
            if (a.size() != b.size())
                return false;
            for (int i = 0; i < vec_len(ka); i++)
                if (!is_same_struct(vec_get(ka, i), vec_get(kb, i)))
                    return false;
            return true;
        }
        default:
            return true;
    }
}

static void ensure_assignable(Type *totype, Type *fromtype) {
    if ((is_arithtype(totype) || totype->kind == KIND_PTR) &&
        (is_arithtype(fromtype) || fromtype->kind == KIND_PTR))
        return;
    if (is_same_struct(totype, fromtype))
        return;
    error("incompatible kind: <%s> <%s>", ty2s(totype), ty2s(fromtype));
}
std::shared_ptr<DataStruct::Node> Parser::read_var_or_func(const std::string &name){
    CHECK_LEX();
    CHECK_CPP();
    std::shared_ptr<DataStruct::Node> v= nullptr;
    if(env()->find(name)!=env()->end())
        v=env()->at(name);
    if (!v) {
        const DataStruct::Token &tok = macro->peek_token();
        if (!lex->is_keyword(tok, lex->get_keywords("(")))
            Error::errort(tok, "undefined variable: %s", name);
        auto ty = make_func_type(TYPE_INT,std::vector<DataStruct::Type> () , true, false);
        Error::warnt(tok, "assume returning int: %s()", name);
        return ast_funcdesg(ty, name);
    }
    if (v->getTy()->kind == DataStruct::TYPE_KIND::KIND_FUNC)
        return ast_funcdesg(v->getTy(), name);
    return v;
}
int Parser::get_compound_assign_op(const DataStruct::Token &tok){

}
std::shared_ptr<DataStruct::Node> Parser::read_stmt_expr(){

}
std::shared_ptr<DataStruct::Type> Parser::char_type(DataStruct::ENCODE enc){
    switch (enc) {
        case DataStruct::ENCODE::ENC_NONE:
        case DataStruct::ENCODE::ENC_WCHAR:
            return TYPE_INT;
        case DataStruct::ENCODE::ENC_CHAR16:
            return TYPE_USHORT;
        case DataStruct::ENCODE::ENC_CHAR32:
            return TYPE_UINT;
    }
    Error::error("internal error");
}
//6.5.1 Primary Expression
//identifier
//constant:    6.4.4  Integer Floating Enumeration Charater
//string-literal
//( expression )
//generic-selectionz
std::shared_ptr<DataStruct::Node> Parser::read_primary_expr(){
    CHECK_LEX();
    CHECK_CPP();
    DataStruct::Token tok = macro->read_token();
    if (tok.kind==DataStruct::TOKEN_TYPE::TEOF) return nullptr;
    if (lex->is_keyword(tok, lex->get_keywords("("))) {
        if (macro->next(lex->get_keywords("{")))
            return read_stmt_expr();
        auto r = read_expr();
        macro->expect(lex->get_keywords(")"));
        return r;
    }
    if (lex->is_keyword(tok, DataStruct::AST_TYPE ::KGENERIC)) {
        return read_generic();
    }
    switch (tok.kind) {
        case DataStruct::TOKEN_TYPE::TIDENT:
            return read_var_or_func(*(tok.sval));
        case DataStruct::TOKEN_TYPE::TNUMBER:
            return read_number(tok);
        case DataStruct::TOKEN_TYPE::TCHAR:
            return ast_inttype(char_type(tok.enc), tok.c);
        case DataStruct::TOKEN_TYPE::TSTRING:
            return ast_string(tok.enc, *(tok.sval));
        case DataStruct::TOKEN_TYPE::TKEYWORD:
            lex->retreat_token(tok);
            return nullptr;
        default:
            Error::error("internal error: unknown token kind: %d", static_cast<int>(tok.kind));
    }
}
std::shared_ptr<DataStruct::Node>  Parser::read_subscript_expr(const std::shared_ptr<DataStruct::Node> &node){

}
std::shared_ptr<DataStruct::Node> Parser::read_postfix_expr_tail(const std::shared_ptr<DataStruct::Node> &node){
    CHECK_CPP();
    CHECK_LEX();
    if (!node) return nullptr;
    for (;;) {
        if (macro->next(lex->get_keywords("("))) {
            const DataStruct::Token &tok = macro->peek_token();
            node = conv(node);
            auto t = node->getTy();
            if (t->kind != DataStruct::TYPE_KIND::KIND_PTR || t->ptr->kind != DataStruct::TYPE_KIND::KIND_FUNC)
                errort(tok, "function expected, but got %s", node2s(node));
            node = read_funcall(node);
            continue;
        }
        if (next_token('[')) {
            node = read_subscript_expr(node);
            continue;
        }
        if (next_token('.')) {
            node = read_struct_field(node);
            continue;
        }
        if (next_token(OP_ARROW)) {
            if (node->ty->kind != KIND_PTR)
                error("pointer type expected, but got %s %s",
                      ty2s(node->ty), node2s(node));
            node = ast_uop(AST_DEREF, node->ty->ptr, node);
            node = read_struct_field(node);
            continue;
        }
        Token *tok = peek();
        if (next_token(OP_INC) || next_token(OP_DEC)) {
            ensure_lvalue(node);
            int op = is_keyword(tok, OP_INC) ? OP_POST_INC : OP_POST_DEC;
            return ast_uop(op, node->ty, node);
        }
        return node;
    }
}
std::shared_ptr<DataStruct::Node> Parser::read_postfix_expr(){
    auto node = read_primary_expr();
    return read_postfix_expr_tail(node);
}
std::shared_ptr<DataStruct::Node> Parser::read_unary_incdec(int op){

}
std::shared_ptr<DataStruct::Node> Parser::read_label_addr(const std::shared_ptr<DataStruct::Token> &tok){

}
std::shared_ptr<DataStruct::Node> Parser::read_unary_addr(){

}
std::shared_ptr<DataStruct::Node> Parser::read_unary_deref(const std::shared_ptr<DataStruct::Token> &tok){

}
std::shared_ptr<DataStruct::Node> Parser::read_unary_minus(){

}
std::shared_ptr<DataStruct::Node> Parser::read_unary_bitnot(const std::shared_ptr<DataStruct::Token> &tok){

}
std::shared_ptr<DataStruct::Node> Parser::read_unary_lognot(){

}
std::shared_ptr<DataStruct::Node> Parser::read_unary_expr(){

}
std::shared_ptr<DataStruct::Node> Parser::read_compound_literal(const std::shared_ptr<DataStruct::Type> &ty){

}
std::shared_ptr<DataStruct::Node> Parser::read_cast_expr(){

}
std::shared_ptr<DataStruct::Node> Parser::read_multiplicative_expr(){

}
std::shared_ptr<DataStruct::Node> Parser::read_additive_expr(){

}
std::shared_ptr<DataStruct::Node> Parser::read_shift_expr(){

}
std::shared_ptr<DataStruct::Node> Parser::read_relational_expr(){

}
std::shared_ptr<DataStruct::Node> Parser::read_equality_expr(){

}
std::shared_ptr<DataStruct::Node> Parser::read_bitand_expr(){

}
std::shared_ptr<DataStruct::Node> Parser::read_assignment_expr(){

}
std::shared_ptr<DataStruct::Node> Parser::read_comma_expr(){

}
std::shared_ptr<DataStruct::Node> Parser::read_expr(){

}
std::shared_ptr<DataStruct::Node> Parser::do_read_conditional_expr(const std::shared_ptr<DataStruct::Node> &cond) {
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

std::shared_ptr<DataStruct::Node> Parser::read_conditional_expr() {
    CHECK_LEX();
    CHECK_CPP();
    auto cond = read_logor_expr();
    if (!macro->next(lex->get_keywords("?")))
        return cond;
    return do_read_conditional_expr(cond);
}
std::shared_ptr<DataStruct::Node> Parser::read_logor_expr(){
    CHECK_CPP();
    CHECK_LEX();
    auto node = read_logand_expr();
    while (macro->next(DataStruct::AST_TYPE::OP_LOGOR))
        node = ast_binop(TYPE_INT, DataStruct::AST_TYPE::OP_LOGOR, node, read_logand_expr());
    return node;
}
std::shared_ptr<DataStruct::Node> Parser::read_logand_expr(){
    CHECK_LEX();
    CHECK_CPP();
    auto node = read_bitor_expr();
    while (macro->next(DataStruct::AST_TYPE::OP_LOGAND))
        node = ast_binop(TYPE_INT, DataStruct::AST_TYPE::OP_LOGAND, node, read_bitor_expr());
    return node;
}
std::shared_ptr<DataStruct::Node> Parser::read_bitor_expr(){
    CHECK_CPP();
    CHECK_LEX();
    auto node = read_bitxor_expr();
    while (macro->next(lex->get_keywords("|")))
        node = binop('|', conv(node), conv(read_bitxor_expr()));
    return node;
}
std::shared_ptr<DataStruct::Node> Parser::read_bitxor_expr(){

}
//make node with every element in p and put it into inits,for char array, the left room is initialized to zero
void Parser::assign_string(std::vector<std::shared_ptr<DataStruct::Node>>& inits,
                   const std::shared_ptr<DataStruct::Type> &ty, const std::string &p, int off) {
    if (ty->len == -1)
        ty->len = ty->size = p.size() + 1;
    int i = 0;
    for (auto beg=p.cbegin(); i < ty->len && beg!=p.cend(); i++)
        inits.emplace_back(ast_init(ast_inttype(TYPE_CHAR, *beg++), TYPE_CHAR, off + i));
    for (; i < ty->len; i++)
        inits.emplace_back(ast_init(ast_inttype(TYPE_CHAR, 0), TYPE_CHAR, off + i));
}

bool Parser::maybe_read_brace() {
    CHECK_CPP();
    CHECK_LEX();
    return macro->next(lex->get_keywords("{"));
}

void Parser::maybe_skip_comma() {
    CHECK_LEX();
    CHECK_CPP();
    macro->next(lex->get_keywords(","));
}

void Parser::skip_to_brace() {
    CHECK_CPP();
    CHECK_LEX();
    for (;;) {
        if (macro->next(lex->get_keywords("}")))
            return;
        if (macro->next(lex->get_keywords("."))) {
            macro->read_token();
            macro->expect(lex->get_keywords("="));
        }
        auto tok = macro->peek_token();
        auto ignore = read_assignment_expr();
        if (!ignore)
            return;
        Error::warnt(tok, "excessive initializer: %s", Utils::node2s(ignore));
        maybe_skip_comma();
    }
}
void Parser::read_initializer_elem(std::vector<std::shared_ptr<DataStruct::Node>>&inits,
                           const std::shared_ptr<DataStruct::Type> &ty, int off, bool designated) {
    CHECK_CPP();
    CHECK_LEX();
    macro->next(lex->get_keywords("="));
    //aggregate objects,recursively 6.7.9 P 20
    if (ty->kind == DataStruct::TYPE_KIND::KIND_ARRAY || ty->kind == DataStruct::TYPE_KIND::KIND_STRUCT) {
        read_initializer_list(inits, ty, off, designated);
    } else if (macro->next(lex->get_keywords("{"))) {
        read_initializer_elem(inits, ty, off, true);
        macro->expect(lex->get_keywords("}"));
    } else {
        auto expr = conv(read_assignment_expr());
        ensure_assignable(ty, expr->getTy());
        inits.emplace_back(ast_init(expr, ty, off));
    }
}
//for aggregate or array,sort by their offset,by ascending
void Parser::sort_inits(std::vector<std::shared_ptr<DataStruct::Node>>&inits) {
    std::sort(inits.begin(),inits.end(),[](decltype(inits[0]) ele, decltype(inits[0]) another)
                                                        {if (ele->initoff<another->initoff) return 1;
                                                            return 0;
                                                        });
}

void Parser::read_struct_initializer_sub(std::vector<std::shared_ptr<DataStruct::Node>>&inits,
                                        const std::shared_ptr<DataStruct::Type> &ty, int off, bool designated) {
    CHECK_LEX();
    CHECK_CPP();
    bool has_brace = maybe_read_brace();
    std::vector<std::string> keys;
    keys.reserve(ty->fields.size());
    auto key_selector=[](decltype(ty->fields[0]) ele){ return ele.first;};
    auto find_key=[](decltype(ty->fields)& ele,const std::string&name){
        for (auto &e:ele)
            if (e.first==name)
                return e.second;
        return nullptr;
    };
    std::transform(ty->fields.begin(),ty->fields.end(),keys.begin(),key_selector);
    int i = 0;
    for (;;) {
        auto tok = macro->read_token();
        if (lex->is_keyword(tok, lex->get_keywords("}"))) {
            if (!has_brace)
                lex->retreat_token(tok);
            return;
        }
        std::string fieldname;
        std::shared_ptr<DataStruct::Type> fieldtype;
        if ((lex->is_keyword(tok, lex->get_keywords(".")) || lex->is_keyword(tok, lex->get_keywords("[")))
            && !has_brace && !designated) {
            lex->retreat_token(tok);
            return;
        }
        if (lex->is_keyword(tok,lex->get_keywords("."))) {
            tok = macro->read_token();
            if (tok.kind != DataStruct::TOKEN_TYPE::TIDENT)
                Error::errort(tok, "malformed desginated initializer: %s", Utils::tok2s(tok));
            fieldname = *(tok.sval);
            fieldtype = find_key(ty->fields, fieldname);
            if (!fieldtype)
                Error::errort(tok, "field does not exist: %s", Utils::tok2s(tok));
//            keys = dict_keys(ty->fields);
            i = 0;
            while (i < keys.size()) {
                if(fieldname==keys[i++])
                    break;
            }
            designated = true;
        } else {
            lex->retreat_token(tok);
            if (i == keys.size())
                break;
            fieldname = keys[i++];
            fieldtype = find_key(ty->fields, fieldname);
        }
        read_initializer_elem(inits, fieldtype, off + fieldtype->offset, designated);
        maybe_skip_comma();
        designated = false;
        if (!ty->is_struct)  //maybe union
            break;
    }
    if (has_brace)
        skip_to_brace();
}
void Parser::read_struct_initializer(std::vector<std::shared_ptr<DataStruct::Node>>&inits,
                                    const std::shared_ptr<DataStruct::Type> &ty, int off, bool designated) {
    read_struct_initializer_sub(inits, ty, off, designated);
    sort_inits(inits);
}
//There are so many initializer methods
void Parser::read_array_initializer_sub(std::vector<std::shared_ptr<DataStruct::Node>>&inits,
                                const std::shared_ptr<DataStruct::Type> &ty, int off, bool designated) {
    CHECK_LEX();
    CHECK_CPP();
    bool has_brace = maybe_read_brace();
    bool flexible = (ty->len <= 0);
    int elemsize = ty->ptr->size;
    int i;
    for (i = 0; flexible || i < ty->len; i++) {
        auto tok = macro->read_token();
        if (lex->is_keyword(tok, lex->get_keywords("}"))) {  //boundary
            if (!has_brace)
                lex->retreat_token(tok);
            goto finish;
        }
        if ((lex->is_keyword(tok,lex->get_keywords(".")) || lex->is_keyword(tok,lex->get_keywords("[")))
            && !has_brace && !designated) {         //invalid
            lex->retreat_token(tok);
            return;
        }
        if (lex->is_keyword(tok, lex->get_keywords("["))) {
            auto tok = macro->peek_token();
            int idx = read_intexpr();
            if (idx < 0 || (!flexible && ty->len <= idx))
                Error::errort(tok, "array designator exceeds array bounds: %d", idx);
//            6.7.9 p17
//            When no designations are present, subobjects of the current object are initialized in order according
//            to the type of the current object: array elements in increasing subscript order, structure
//            members in declaration order, and the first named member of a union.148) In contrast, a
//            designation causes the following initializer to begin initialization of the subobject
//            described by the designator. Initialization then continues forward in order, beginning
//            with the next subobject after that described by the designator.
            i = idx;
            macro->expect(lex->get_keywords("]"));
            designated = true;
        } else {
            lex->retreat_token(tok);
        }
        read_initializer_elem(inits, ty->ptr, off + elemsize * i, designated);
        maybe_skip_comma();
        designated = false;
    }
    if (has_brace)
        skip_to_brace();
    finish:
    if (ty->len < 0) {
        ty->len = i;
        ty->size = elemsize * i;
    }
}
void Parser::read_array_initializer(std::vector<std::shared_ptr<DataStruct::Node>>&inits,
                            const std::shared_ptr<DataStruct::Type> &ty, int off, bool designated) {
    read_array_initializer_sub(inits, ty, off, designated);
    sort_inits(inits);
}
void Parser::read_initializer_list(std::vector<std::shared_ptr<DataStruct::Node>>&inits,
                           const std::shared_ptr<DataStruct::Type> &ty, int off, bool designated) {
    auto tok = macro->read_token();
    if (is_string(ty)) {
        if (tok.kind == DataStruct::TOKEN_TYPE::TSTRING) {
            assign_string(inits, ty, *(tok.sval), off);
            return;
        }
        //6.7.9 p14 p15 An array of character type may be initialized by a character string literal or UTF−8 string
//        literal, optionally enclosed in braces.
        if (lex->is_keyword(tok, lex->get_keywords("{")) && macro->peek_token().kind == DataStruct::TOKEN_TYPE::TSTRING) {
            tok = macro->read_token();
            assign_string(inits, ty, *(tok.sval), off);
            macro->expect(lex->get_keywords("}"));
            return;
        }
    }
    lex->retreat_token(tok);
    if (ty->kind == DataStruct::TYPE_KIND::KIND_ARRAY) {
        read_array_initializer(inits, ty, off, designated);
    } else if (ty->kind == DataStruct::TYPE_KIND::KIND_STRUCT) {
        read_struct_initializer(inits, ty, off, designated);
    } else {   //scalar
        auto arraytype = make_array_type(ty, 1);
        read_array_initializer(inits, arraytype, off, designated);
    }
}
std::vector<std::shared_ptr<DataStruct::Node>> Parser::read_decl_init(const std::shared_ptr<DataStruct::Type> &ty){
    CHECK_LEX();
    CHECK_CPP();
    std::vector<std::shared_ptr<DataStruct::Node>> r;
    if (lex->is_keyword(macro->peek_token(), lex->get_keywords("{")) || is_string(ty)) {
        read_initializer_list(r, ty, 0, false);
    } else {
        auto init = conv(read_assignment_expr());
        if (is_arithtype(init->getTy()) && init->getTy()->kind != ty->kind)
            init = ast_conv(ty, init);
        r.emplace_back(ast_init(init, ty, 0));
    }
    return r;
}
std::shared_ptr<DataStruct::Node> Parser::read_compound_stmt(){
    auto orig = localenv;
    localenv=std::make_shared<std::unordered_map<std::string,std::shared_ptr<DataStruct::Node>>>();
    scope[localenv]=orig;
    std::vector<DataStruct::Node> list ;
    for (;;) {
        if (macro->next(lex->get_keywords("}")))
            break;
        read_decl_or_stmt(list);
    }
    localenv = orig;
    std::vector<std::shared_ptr<DataStruct::Node>> slist;
    slist.reserve(list.size());
    for(auto &e:list)
        slist.push_back(std::make_shared<DataStruct::Node>(e));
    return ast_compound_stmt(&slist);
}
//syntax:
//compound-statement:
//     { block-item-listopt }
//  block-item-list:
//     block-item
//     block-item-list block-item
//  block-item:
//     declaration
//     statement
void Parser::read_decl_or_stmt(std::vector<DataStruct::Node> &list){
    CHECK_CPP();
    CHECK_LEX();
    auto tok = macro->peek_token();
    if (tok.kind == DataStruct::TOKEN_TYPE::TEOF)
        Error::error("premature end of input");
    mark_location();
    if (is_type(tok)) {
        read_decl(&list, false);
    } else if (macro->next(DataStruct::AST_TYPE::KSTATIC_ASSERT)) {
        read_static_assert();
    } else {
        auto stmt = read_stmt();
        if (stmt)
            list.push_back(*stmt);
    }
}
//statement:
//  labeled-statement
//        identifier : statement
//        case constant-expression : statement                              7
//        default : statement                                               8
//  compound-statement
//  expression-statement
//        expressionopt ;
//  selection-statement
//        if ( expression ) statement                                        1
//        if ( expression ) statement else statement                         1
//        switch ( expression ) statement                                    6
//  iteration-statement
//        while ( expression ) statement                                     3
//        do statement while ( expression ) ;                                4
//        for ( expressionopt ; expressionopt ; expressionopt ) statement    2
//        for ( declaration expressionopt ; expressionopt ) statement        2
//  jump-statement
//        goto identifier ;                                                  11
//        continue ;                                                         10
//        break ;                                                            9
//        return expressionopt ;                                             5
std::shared_ptr<DataStruct::Node> Parser::read_stmt(){
    auto tok = macro->read_token();
    if (tok.kind == DataStruct::TOKEN_TYPE::TKEYWORD) {
        switch (tok.id) {
            case DataStruct::AST_TYPE::LBRACE:       return read_compound_stmt();
            case DataStruct::AST_TYPE::KIF:       return read_if_stmt();
            case DataStruct::AST_TYPE::KFOR:      return read_for_stmt();
            case DataStruct::AST_TYPE::KWHILE:    return read_while_stmt();
            case DataStruct::AST_TYPE::KDO:       return read_do_stmt();
            case DataStruct::AST_TYPE::KRETURN:   return read_return_stmt();
            case DataStruct::AST_TYPE::KSWITCH:   return read_switch_stmt();
            case DataStruct::AST_TYPE::KCASE:     return read_case_label(tok);
            case DataStruct::AST_TYPE::KDEFAULT:  return read_default_label(tok);
            case DataStruct::AST_TYPE::KBREAK:    return read_break_stmt(tok);
            case DataStruct::AST_TYPE::KCONTINUE: return read_continue_stmt(tok);
            case DataStruct::AST_TYPE::KGOTO:     return read_goto_stmt();
        }
    }
    if (tok.kind == DataStruct::TOKEN_TYPE::TIDENT && macro->next(lex->get_keywords(":")))
        return read_label(tok);
    lex->retreat_token(tok);
    auto r = read_comma_expr();
    macro->expect(lex->get_keywords(";"));
    return r;
}
//create a label node with the content of tok.sval
std::shared_ptr<DataStruct::Node> Parser::read_label(const DataStruct::Token &tok){
    std::string &label = *(tok.sval);
    if (labels.find(label) !=labels.end())
        Error::errort(tok, "duplicate label: %s", Utils::tok2s(tok));
    auto r = ast_label(label);
    labels[label]= r;
    return read_label_tail(r);
}
std::shared_ptr<DataStruct::Node> Parser::read_label_tail(const std::shared_ptr<DataStruct::Node> &label) {
    auto stmt = read_stmt();
    std::vector<std::shared_ptr<DataStruct::Node>> v;
    v.push_back(label);
    if (stmt)
        v.push_back(stmt);
    return ast_compound_stmt(&v);
}

std::shared_ptr<DataStruct::Node> Parser::read_case_label(const DataStruct::Token &tok) {
    CHECK_LEX();CHECK_CPP();
    if (cases.empty())
        Error::errort(tok, "stray case label");
    auto label = make_label();
    int beg = read_intexpr();
    if (macro->next(DataStruct::AST_TYPE ::KELLIPSIS)) {
        int end = read_intexpr();
        macro->expect(lex->get_keywords(":"));
        if (beg > end)
            Error::errort(tok, "case region is not in correct order: %d ... %d", beg, end);
        cases.emplace_back(make_case(beg, end, label));
    } else {
        macro->expect(lex->get_keywords(":"));
        cases.emplace_back(make_case(beg, beg, label));
    }
    check_case_duplicates();
    return read_label_tail(ast_dest(label));
}
std::shared_ptr<DataStruct::Node> Parser::read_default_label(const DataStruct::Token &tok) {
    CHECK_CPP();CHECK_LEX();
    macro->expect(lex->get_keywords(":"));
    if (!defaultcase.empty())
        Error::errort(tok, "duplicate default");
    defaultcase = make_label();
    return read_label_tail(ast_dest(defaultcase));
}
void Parser::check_case_duplicates(){
    const Case &x = cases.back();
    for(auto beg=cases.cbegin(),end=--(cases.end());beg!=end;++beg) {
        if (x.end < beg->beg || beg->end < x.beg)
            continue;
        if (x.beg == x.end)
            Error::error("duplicate case value: %d", x.beg);
        Error::error("duplicate case value: %d ... %d", x.beg, x.end);
    }
}
//IF FLOAT,THEN CASTED TO BOOL
std::shared_ptr<DataStruct::Node> Parser::read_boolean_expr(){
    auto cond = read_expr();
    return is_flotype(cond->getTy()) ? ast_conv(TYPE_BOOL, cond) : cond;
}
std::shared_ptr<DataStruct::Node> Parser::read_if_stmt(){
    CHECK_LEX();
    CHECK_CPP();
    macro->expect(lex->get_keywords("("));
    auto cond = read_boolean_expr();
    macro->expect(lex->get_keywords(")"));
    auto then = read_stmt();
    if (!macro->next(DataStruct::AST_TYPE::KELSE))
        return ast_if(cond, then, nullptr);
    auto els = read_stmt();
    return ast_if(cond, then, els);
}
std::shared_ptr<DataStruct::Node> Parser::read_opt_decl_or_stmt(){
    CHECK_CPP();
    CHECK_LEX();
    if (macro->next(lex->get_keywords(";")))
        return nullptr;
    std::vector<DataStruct::Node> list;
    read_decl_or_stmt(list);
    std::vector<std::shared_ptr<DataStruct::Node>> slist;
    slist.reserve(list.size());
    for(auto&e:list)
        slist.emplace_back(std::make_shared<DataStruct::Node>(e));
    return ast_compound_stmt(&slist);
}
std::shared_ptr<DataStruct::Node> Parser::read_for_stmt(){
    CHECK_LEX();
    CHECK_CPP();
    macro->expect(lex->get_keywords("("));
    auto beg = make_label();
    auto mid = make_label();
    auto end = make_label();
    auto orig = localenv;
    localenv=std::make_shared<std::unordered_map<std::string,std::shared_ptr<DataStruct::Node>>>();
    scope[localenv]=orig;
    auto init = read_opt_decl_or_stmt();
    auto cond = read_comma_expr();
    if (cond && is_flotype(cond->getTy()))
        cond = ast_conv(TYPE_BOOL, cond);
    macro->expect(lex->get_keywords(";"));
    auto step = read_comma_expr();
    macro->expect(lex->get_keywords(")"));
    auto ocontinue = lcontinue;
    auto obreak = lbreak;
    lcontinue = mid;
    lbreak = end;
    auto body = read_stmt();
    lcontinue = ocontinue;
    lbreak = obreak;
    localenv = orig;

    std::vector<std::shared_ptr<DataStruct::Node>> v;
    if (init)
        v.push_back(init);
    v.emplace_back(ast_dest(beg));
    if (cond)
        v.emplace_back(ast_if(cond, nullptr, ast_jump(end)));
    if (body)
        v.push_back(body);
    v.emplace_back(ast_dest(mid));
    if (step)
        v.push_back(step);
    v.emplace_back(ast_jump(beg));
    v.emplace_back(ast_dest(end));
    return ast_compound_stmt(&v);
}
std::shared_ptr<DataStruct::Node> Parser::read_while_stmt(){
    CHECK_CPP();
    CHECK_LEX();
    macro->expect(lex->get_keywords("("));
    auto cond = read_boolean_expr();
    macro->expect(lex->get_keywords(")"));

    auto beg = make_label();
    auto end = make_label();
    auto ocontinue = lcontinue;
    auto obreak = lbreak;
    lcontinue = beg;
    lbreak = end;
    auto body = read_stmt();
    lcontinue = ocontinue;
    lbreak = obreak;

    std::vector<std::shared_ptr<DataStruct::Node>> v;
    v.emplace_back(ast_dest(beg));
    v.emplace_back(ast_if(cond, body, ast_jump(end)));
    v.emplace_back(ast_jump(beg));
    v.emplace_back(ast_dest(end));
    return ast_compound_stmt(&v);
}
std::shared_ptr<DataStruct::Node> Parser::read_do_stmt(){
    CHECK_LEX();
    CHECK_CPP();
    auto beg = make_label();
    auto end = make_label();
    auto ocontinue = lcontinue;
    auto obreak = lbreak;
    lcontinue = beg;
    lbreak = end;
    auto body = read_stmt();
    lcontinue = ocontinue;
    lbreak = obreak;
    auto tok = macro->read_token();
    if (!lex->is_keyword(tok, DataStruct::AST_TYPE::KWHILE))
        Error::errort(tok, "'while' is expected, but got %s", Utils::tok2s(tok));
    macro->expect(lex->get_keywords("("));
    auto cond = read_boolean_expr();
    macro->expect(lex->get_keywords(")"));
    macro->expect(lex->get_keywords(";"));

    std::vector<std::shared_ptr<DataStruct::Node>> v;
    v.emplace_back(ast_dest(beg));
    if (body)
        v.emplace_back(body);
    v.emplace_back(ast_if(cond, ast_jump(beg), nullptr));
    v.emplace_back(ast_dest(end));
    return ast_compound_stmt(&v);
}
std::shared_ptr<DataStruct::Node> Parser::make_switch_jump(const std::shared_ptr<DataStruct::Node> &var, const Case &c) {
    std::shared_ptr<DataStruct::Node> cond;
    if (c.beg == c.end) {
        cond = ast_biop(DataStruct::AST_TYPE::OP_EQ, TYPE_INT, var, ast_inttype(TYPE_INT, c.beg));
    } else {
        // [GNU] case i ... j is compiled to if (i <= cond && cond <= j) goto <label>.
        auto x = ast_biop(DataStruct::AST_TYPE::OP_LE, TYPE_INT, ast_inttype(TYPE_INT, c.beg), var);
        auto y = ast_biop(DataStruct::AST_TYPE::OP_LE, TYPE_INT, var, ast_inttype(TYPE_INT, c.end));
        cond = ast_biop(DataStruct::AST_TYPE::OP_LOGAND, TYPE_INT, x, y);
    }
    return ast_if(cond, ast_jump(c.label), nullptr);
}
std::shared_ptr<DataStruct::Node> Parser::read_switch_stmt(){
    CHECK_CPP();
    CHECK_LEX();
    macro->expect(lex->get_keywords("("));
    auto expr = conv(read_expr());
    ensure_inttype(expr);
    macro->expect(lex->get_keywords(")"));

    auto end = make_label();
    auto ocases = cases;
    auto odefaultcase = defaultcase;
    auto obreak = lbreak;
    cases.clear();
    defaultcase = "";
    lbreak = end;
    auto body = read_stmt();
    std::vector<std::shared_ptr<DataStruct::Node>> v;
    auto var = ast_lvar(expr->getTy(), make_tempname());
    v.emplace_back(ast_biop(lex->get_keywords("="), expr->getTy(), var, expr));
    for (auto&e:cases)
        v.emplace_back(make_switch_jump(var, e));
    v.emplace_back(ast_jump(!defaultcase.empty() ? defaultcase : end));
    if (body)
        v.push_back(body);
    v.emplace_back(ast_dest(end));
    cases = ocases;
    defaultcase = odefaultcase;
    lbreak = obreak;
    return ast_compound_stmt(&v);
}
std::shared_ptr<DataStruct::Node> Parser::read_break_stmt(const DataStruct::Token &tok){
    CHECK_LEX();
    CHECK_CPP();
    macro->expect(lex->get_keywords(";"));
    if (lbreak.empty())
        Error::errort(tok, "stray break statement");
    return ast_jump(lbreak);
}
std::shared_ptr<DataStruct::Node> Parser::read_continue_stmt(const DataStruct::Token &tok) {
    CHECK_LEX();
    CHECK_CPP();
    macro->expect(lex->get_keywords(";"));
    if (!lcontinue.empty())
        Error::errort(tok, "stray continue statement");
    return ast_jump(lcontinue);
}
std::shared_ptr<DataStruct::Node> Parser::read_return_stmt() {
    auto retval = read_comma_expr();
    macro->expect(lex->get_keywords(";"));
    if (retval)
        return ast_return(ast_conv(current_func_type->rettype, retval));
    return ast_return(nullptr);
}

std::shared_ptr<DataStruct::Node> Parser::read_goto_stmt() {
    CHECK_CPP();
    CHECK_LEX();
    if (macro->next(lex->get_keywords("*"))) {
        // [GNU] computed goto. "goto *p" jumps to the address pointed by p.
        auto tok = macro->peek_token();
        auto expr = read_cast_expr();
        if (expr->getTy()->kind != DataStruct::TYPE_KIND::KIND_PTR)
            Error::errort(tok, "pointer expected for computed goto, but got %s", Utils::node2s(expr));
        return ast_computed_goto(expr);
    }
    auto tok = macro->read_token();
    if (tok.kind != DataStruct::TOKEN_TYPE::TIDENT)
        Error::errort(tok, "identifier expected, but got %s", Utils::tok2s(tok));
    macro->expect(lex->get_keywords(";"));
    auto r = ast_goto(*(tok.sval));
    gotos.push_back(r);
    return r;
}
