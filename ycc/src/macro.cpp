//
// Created by yk on 18-2-18.
//

#include <libgen.h>
#include "../include/path.h"
#include "../include/parser.h"

std::shared_ptr<MacroPreprocessor> MacroPreprocessor::Instance(){
    if (!_cpp)
        _cpp.reset(new MacroPreprocessor());
    return _cpp;
}
std::shared_ptr<MacroPreprocessor> MacroPreprocessor::_cpp= nullptr;
//构建预处理条件
DataStruct::CondIncl MacroPreprocessor::make_cond_incl(bool wastrue) {
    DataStruct::CondIncl r;
    r.ctx = DataStruct::CondInclCtx::IN_THEN;
    r.wastrue = wastrue;
    return r;
}

//函数宏参数token
DataStruct::Token MacroPreprocessor::make_macro_token(int position, bool is_vararg) {
    DataStruct::Token r{DataStruct::TOKEN_TYPE::TMACRO_PARAM,is_vararg,position};
    r.space = false;
    r.bol = false;
    return r;
}

DataStruct::Token MacroPreprocessor::copy_token(const DataStruct::Token& tok) const {
    return DataStruct::Token(tok);
}

//下一个是不是期待的关键字
void MacroPreprocessor::expect( DataStruct::AST_TYPE id) {
    DataStruct::Token tok = lex->lex();
    if (!lex->is_keyword(tok, id))
        Error::errort(tok, "%c expected, but got %s", Lex::get_keywords_string(id), Utils::tok2s(tok));
}

bool MacroPreprocessor::next(DataStruct::AST_TYPE id) {
    DataStruct::Token tok = lex->lex();
    if (lex->is_keyword(tok, id))
        return true;
    lex->retreat_token(tok);
    return false;
}

void MacroPreprocessor::propagate_space(std::vector<DataStruct::Token >&tokens, const DataStruct::Token &tmpl) {
    if (tokens.empty())
        return;
    DataStruct::Token tok = copy_token(tokens[0]);
    tok.space = tmpl.space;
    tokens[0] = tok;
}

DataStruct::Token MacroPreprocessor::read_ident() {
    DataStruct::Token tok = lex->lex();
    if (tok.kind != DataStruct::TOKEN_TYPE ::TIDENT)
        Error::errort(tok, "identifier expected, but got %s", Utils::tok2s(tok));
    return tok;
}

void MacroPreprocessor::expect_newline() {
    DataStruct::Token tok = lex->lex();
    if (tok.kind != DataStruct::TOKEN_TYPE ::TNEWLINE)
        Error::errort(tok, "newline expected, but got %s", Utils::tok2s(tok));
}

DataStruct::Token MacroPreprocessor::read_expand_newline() {
    DataStruct::Token tok = lex->lex();
    if (tok.kind != DataStruct::TOKEN_TYPE ::TIDENT)
        return tok;
    std::string name = *(tok.sval);
    DataStruct::Macro macro{DataStruct::MacroType::MACRO_INVALID};
    if (macros.count(name)>0)
        macro=macros[name];
    if (macro.getKind()==DataStruct::MacroType::MACRO_INVALID|| tok.hideset.count(name)>0)
        return tok;

    switch (macro.getKind()) {
        case DataStruct::MacroType::MACRO_OBJ: {
            tok.hideset.insert(name);
            std::vector<std::vector<DataStruct::Token> > token;
            std::vector<DataStruct::Token> tokens = subst(macro, token, tok.hideset);
            propagate_space(tokens, tok);
            unget_all(tokens);
            return read_expand();
        }
        case DataStruct::MacroType::MACRO_FUNC: {
            if (!next(lex->get_keywords("(")))
                return tok;
            std::vector<std::vector<DataStruct::Token >> args = read_args(tok, macro);
            DataStruct::Token rparen = peek_token();
            expect(lex->get_keywords(")"));
            std::set<std::string> hideset = set_intersection(tok.hideset, rparen.hideset);
            hideset.insert(name);
            std::vector<DataStruct::Token > tokens = subst(macro, args, hideset);
            propagate_space(tokens, tok);
            unget_all(tokens);
            return read_expand();
        }
        case DataStruct::MacroType::MACRO_SPECIAL:
            macro.getFn()(this,tok);
            return read_expand();
        default:
            Error::error("internal error");
    }
}

//读取到一个不是换行符的token
DataStruct::Token MacroPreprocessor::read_expand() {
    while (true)
    {
        DataStruct::Token tok = read_expand_newline();
        if (tok.kind != DataStruct::TOKEN_TYPE::TNEWLINE)
            return tok;
    }
}

//buffers中的token是按照back（）方式存取的，因而需要首先反转vector
std::vector<DataStruct::Token > MacroPreprocessor::expand_all(std::vector<DataStruct::Token >&tokens, const DataStruct::Token &tmpl) {
    std::reverse(tokens.begin(),tokens.end());
    lex->token_buffer_stash(tokens);
    std::vector<DataStruct::Token > r;
    while(true)
    {
        DataStruct::Token tok = read_expand();
        if (tok.kind == DataStruct::TOKEN_TYPE::TEOF)
            break;
        r.push_back(tok);
    }
    propagate_space(r, tmpl);
    lex->token_buffer_unstash();
    return r;
}

//把args中的所有token转化为字符串的形式
DataStruct::Token MacroPreprocessor::stringize(const DataStruct::Token &tmpl, std::vector<DataStruct::Token >&args) {
    std::string b;
    auto len=args.size();
    for (decltype(args.size()) i = 0; i < len; i++) {
        DataStruct::Token &tok = args[i];
        if (!b.empty() && tok.space)
            b+=" ";
        b+=Utils::format("%s",Utils::tok2s(tok));
    }
    DataStruct::Token r = copy_token(tmpl);
    r.kind = DataStruct::TOKEN_TYPE::TSTRING;
    r.sval.reset(new std::string(std::move(b)));
    r.enc = DataStruct::ENCODE::ENC_NONE;
    return r;
}

//宏展开操作，注意有#，##特殊宏的地方不会再展开
std::vector<DataStruct::Token > MacroPreprocessor::subst(const DataStruct::Macro &macro, std::vector<std::vector<DataStruct::Token >>&args, std::set<std::string> &hideset) {
    std::vector<DataStruct::Token > r;
    auto len = macro.getBody().size();
    for (decltype(macro.getBody().size()) i = 0; i < len; i++) {
        const DataStruct::Token &t0 = macro.getBody()[i];
        const DataStruct::Token &t1 = (i == len - 1) ? DataStruct::Token(DataStruct::TOKEN_TYPE::TPLACEHOLDER,-1) : macro.getBody()[i+1];
        bool t0_param = (t0.kind == DataStruct::TOKEN_TYPE::TMACRO_PARAM);
        bool t1_param = t1.kind == DataStruct::TOKEN_TYPE::TMACRO_PARAM;

        //#token,把token转化为字符串
        if (lex->is_keyword(t0, DataStruct::AST_TYPE::KHASH) && t1_param) {
            r.emplace_back(stringize(t0, args[t1.position]));
            i++;
            continue;
        }
        if (lex->is_keyword(t0, DataStruct::AST_TYPE::KHASHHASH) && t1_param) {
// 一般在调试打印Debug 信息的时候, 需要可变参数的宏. 从C99开始可以使编译器标准支持可变参数宏(variadic macros), 另外GCC 也支持可变参数宏, 但是两种在细节上可能存在区别.
// 1. __VA_ARGS__

//  __VA_ARGS__ 将"..." 传递给宏.如

//  #define debug(format, ...) fprintf(stderr, fmt, __VA_ARGS__)

//  在GCC中也支持这类表示, 但是在G++ 中不支持这个表示.
//
//  2. GCC 的复杂宏
//
//  GCC使用一种不同的语法从而可以使你可以给可变参数一个名字，如同其它参数一样。
//  #define debug(format, args...) fprintf (stderr, format, args)
//
//  这和上面举的那个定义的宏例子是完全一样的，但是这么写可读性更强并且更容易进行描述。
//
//  3. ##__VA_ARGS__
//
//  上面两个定义的宏, 如果出现debug("A Message") 的时候, 由于宏展开后有个多余的逗号, 所以将导致编译错误. 为了解决这个问题，CPP使用一个特殊的‘##’操作。
//
//  #define debug(format, ...) fprintf (stderr, format, ## __VA_ARGS__)
//  这里，如果可变参数被忽略或为空，‘##’操作将使预处理器（preprocessor）去除掉它前面的那个逗号。如果你在宏调用时，确实提供了一些可变参数，GNU CPP也会工作正常，它会把这些可变参数放到逗号的后面。
            std::vector<DataStruct::Token >&arg = args[t1.position];
            if (t1.is_vararg && !r.empty() && lex->is_keyword(r.back(), lex->get_keywords(","))) {
                if (!arg.empty())
                    r.insert(r.end(),arg.begin(),arg.end());
                else
                    r.pop_back();
            } else if (!arg.empty()) {
                glue_push(r, arg.front());
                for (int J = 1; J < arg.size(); J++)
                    r.push_back(arg[J]);
            }
            i++;
            continue;
        }
        //##token，普通的glue操作
        if (lex->is_keyword(t0, DataStruct::AST_TYPE::KHASHHASH) && t1.kind!=DataStruct::TOKEN_TYPE::TPLACEHOLDER) {
            hideset = t1.hideset;
            glue_push(r, t1);
            i++;
            continue;
        }
        //token##，转化成上面的##token形式
        if (t0_param && t1.kind!=DataStruct::TOKEN_TYPE::TPLACEHOLDER && lex->is_keyword(t1, DataStruct::AST_TYPE ::KHASHHASH)) {
            hideset = t1.hideset;
            std::vector<DataStruct::Token >&arg = args[t0.position];
            if (arg.empty())
                i++;
            else{
                r.reserve(r.size()+std::distance(arg.begin(),arg.end()));
                r.insert(r.end(),arg.begin(),arg.end());
            }
            continue;
        }
        if (t0_param) {
            std::vector<DataStruct::Token >& arg = args[t0.position];
            r.reserve(r.size()+std::distance(arg.begin(),arg.end()));
            auto res=expand_all(arg,t0);
            r.insert(r.end(),res.begin(),res.end());
            continue;
        }
        r.push_back(t0);
    }
    return add_hide_set(r, hideset);
}
//置于后备buffers中，以便解析展开后的token
void MacroPreprocessor::unget_all(std::vector<DataStruct::Token >&tokens) {
    for (int i = tokens.size() - 1; i >= 0; --i)
        lex->retreat_token(tokens[i]);
}

DataStruct::Token MacroPreprocessor::peek_token() {
    DataStruct::Token r=read_token();
    lex->retreat_token(r);
    return r;
}

//读取一个token，必要的时候进行宏展开，
//c11标准5.1.1.2中是说：Adjacent string literal tokens are concatenated.这里需要特殊处理一下。
DataStruct::Token MacroPreprocessor::read_token() {
    DataStruct::Token tok;
    while (true)
    {
        tok=read_expand();
        if (tok.bol&&lex->is_keyword(tok,lex->get_keywords("#"))&&tok.hideset.empty())
        {
            read_directive(tok);
            continue;
        }
        if (tok.kind==DataStruct::TOKEN_TYPE::TINVALID)
            Error::errort(tok,"stray character in program.%c", static_cast<char>(tok.c));
        if (tok.kind==DataStruct::TOKEN_TYPE::TSTRING)
        {
            std::string b;
            DataStruct::ENCODE enc=tok.enc;
            while(tok.kind==DataStruct::TOKEN_TYPE::TSTRING)
            {
                DataStruct::ENCODE enc2=tok.enc;
                if (enc!=DataStruct::ENCODE::ENC_NONE&&enc2!=DataStruct::ENCODE::ENC_NONE&&enc!=enc2)
                    Error:: errort(tok, "unsupported non-standard concatenation of string literals: %s", Utils::tok2s(tok));
                b+=*(tok.sval);
                if (enc==DataStruct::ENCODE::ENC_NONE)
                    enc=enc2;
                tok=read_expand();
            }
            lex->retreat_token(tok);
            return lex->make_strtok(b,enc);
        }
        return may_convert_keyword(tok);
    }
}

//在宏展开的过程中，可能有些token是关键字，需要转化为关键字的形式
DataStruct::Token MacroPreprocessor::may_convert_keyword(const DataStruct::Token &tok) {
    if (tok.kind != DataStruct::TOKEN_TYPE::TIDENT)
        return tok;
    if (keywords.count(*(tok.sval))==0)
        return tok;
    DataStruct::Token r = copy_token(tok);
    r.kind = DataStruct::TOKEN_TYPE::TKEYWORD;
    r.id = keywords[*(tok.sval)];
    return r;
}

//读取参数宏的一个参数
std::vector<DataStruct::Token > MacroPreprocessor::read_one_arg(const DataStruct::Token &ident, bool &end,
                                                                bool readall) {
    std::vector<DataStruct::Token > r;
    int level = 0;
    for (;;) {
        DataStruct::Token tok = lex->lex();
        if (tok.kind == DataStruct::TOKEN_TYPE::TEOF)
            Error::errort(ident, "unterminated macro argument list");
        if (tok.kind == DataStruct::TOKEN_TYPE::TNEWLINE)
            continue;
        if (tok.bol && lex->is_keyword(tok, lex->get_keywords("#"))) {
            read_directive(tok);
            continue;
        }
        if (level == 0 && lex->is_keyword(tok, lex->get_keywords(")"))) {
            lex->retreat_token(tok);
            end = true;
            return r;
        }
        if (level == 0 && lex->is_keyword(tok, lex->get_keywords(",")) && !readall)
            return r;
        if (lex->is_keyword(tok, lex->get_keywords("(")))
            level++;
        if (lex->is_keyword(tok, lex->get_keywords(")")))
            level--;
        //宏参数列表中，换行符被认为是一个普通的空白字符，因而对于#进行字符串话处理需要特殊对待
        if (tok.bol) {
            tok = copy_token(tok);
            tok.bol = false;
            tok.space = true;
        }
        r.push_back(tok);
    }
}

//读取函数宏的参数
std::vector<std::vector<DataStruct::Token >> MacroPreprocessor::do_read_args(const DataStruct::Token &ident, const DataStruct::Macro &macro) {
    std::vector<std::vector<DataStruct::Token >> r;
    bool end = false;
    while (!end) {
        bool in_ellipsis = (macro.isIs_varg() && r.size() + 1 == macro.getNargs());
        r.emplace_back(read_one_arg(ident, end, in_ellipsis));
    }
    if (macro.isIs_varg() && r.size() == macro.getNargs() - 1)
        r.emplace_back(std::vector<DataStruct::Token>());
    return r;
}

//读取函数宏参数的预处理操作
std::vector<std::vector<DataStruct::Token> > MacroPreprocessor::read_args(const DataStruct::Token &tok, const DataStruct::Macro &macro) {
    if (macro.getNargs() == 0 && lex->is_keyword(peek_token(), lex->get_keywords(")"))) {
        return std::vector<std::vector<DataStruct::Token> >();
    }
    std::vector<std::vector<DataStruct::Token >> args = do_read_args(tok, macro);
    if (args.size() != macro.getNargs())
        Error::errort(tok, "macro argument number does not match");
    return args;
}

//宏展开过程中的替换集合
std::vector<DataStruct::Token > MacroPreprocessor::add_hide_set(std::vector<DataStruct::Token >& tokens,std::set<std::string >& hideset) {
    std::vector<DataStruct::Token > r;
    for (auto &e:tokens) {
        DataStruct::Token t = copy_token(e);
        t.hideset = set_union(t.hideset, hideset);
        r.push_back(t);
    }
    return r;
}

//把两个token以字符串的形式连接,并重新解析该token
DataStruct::Token MacroPreprocessor::glue_tokens(const DataStruct::Token& t, const DataStruct::Token &u) const {
    std::string b;
    b+=Utils::format("%s",Utils::tok2s(t));
    b+=Utils::format("%s", Utils::tok2s(u));
    DataStruct::Token r = lex->lex_string(b);
    return r;
}

void MacroPreprocessor::glue_push(std::vector<DataStruct::Token >&tokens, const DataStruct::Token &tok) const {
    DataStruct::Token last = tokens.back();
    tokens.pop_back();
    tokens.emplace_back(glue_tokens(last, tok));
}

//宏展开处理
void MacroPreprocessor::read_directive(const DataStruct::Token& hash)
{
    const DataStruct::Token& tok = lex->lex();
    if (tok.kind == DataStruct::TOKEN_TYPE::TNEWLINE)
        return;
    if (tok.kind == DataStruct::TOKEN_TYPE::TNUMBER) {
        read_linemarker(tok);
        return;
    }
    std::string s;   //gcc这里必须首先定义s，否则编译不通过
    if (tok.kind != DataStruct::TOKEN_TYPE::TIDENT)
        goto err;
    s = *(tok.sval);
    if (s == "define")            read_define();
    else if (s == "elif")         read_elif(hash);
    else if (s == "else")         read_else(hash);
    else if (s == "endif")        read_endif(hash);
    else if (s == "error")        read_error(hash);
    else if (s == "if")           read_if();
    else if (s == "ifdef")        read_ifdef();
    else if (s == "ifndef")       read_ifndef();
    else if (s == "import")       read_include(hash, tok.file, true);
    else if (s == "include")      read_include(hash, tok.file, false);
    else if (s == "include_next") read_include_next(hash, tok.file);
    else if (s == "line")         read_line();
    else if (s == "pragma")       read_pragma();
    else if (s == "undef")        read_undef();
    else if (s == "warning")      read_warning(hash);
    else goto err;
    return;

err:
    Error::errort(hash, "unsupported preprocessor directive: %s", Utils::tok2s(tok));
}
//函数宏不支持括号的嵌套
bool MacroPreprocessor::read_funclike_macro_params(const DataStruct::Token &name, std::unordered_map<std::string ,DataStruct::Token >&param)
{
    int pos = 0;
    for (;;) {
        DataStruct::Token tok = lex->lex();
        if (lex->is_keyword(tok, lex->get_keywords(")")))
            return false;
        if (pos) {
            if (!lex->is_keyword(tok, lex->get_keywords(",")))
                Error::errort(tok, ", expected, but got %s", Utils::tok2s(tok));
            tok = lex->lex();
        }
        if (tok.kind == DataStruct::TOKEN_TYPE::TNEWLINE)
            Error::errort(name, "missing ')' in macro parameter list");
        if (lex->is_keyword(tok, DataStruct::AST_TYPE ::KELLIPSIS)) {
            param["__VA_ARGS__"]= make_macro_token(pos++, true);
            expect(lex->get_keywords(")"));
            return true;
        }
        if (tok.kind != DataStruct::TOKEN_TYPE::TIDENT)
            Error::errort(tok, "identifier expected, but got %s", Utils::tok2s(tok));
        std::string arg = *(tok.sval);
        if (next(DataStruct::AST_TYPE::KELLIPSIS)) { //命名可变参数
            expect(lex->get_keywords(")"));
            param[arg] =make_macro_token(pos++, true);
            return true;
        }
        param [arg] = make_macro_token(pos++, false);
    }
}

void MacroPreprocessor::read_funclike_macro(const DataStruct::Token& name)
{
    std::unordered_map<std::string ,DataStruct::Token > params;
    bool is_varg= read_funclike_macro_params(name, params);
    std::vector<DataStruct::Token > body=read_funclike_macro_body(params);
    hashhash_check(body);
    DataStruct::Macro macro = make_func_macro(body, params.size(), is_varg);
    macros[*(name.sval)]= macro;
}

void MacroPreprocessor::hashhash_check(const std::vector<DataStruct::Token >&body) const
{
    if (body.empty())
        return;
    if (lex->is_keyword(body.front(), DataStruct::AST_TYPE::KHASHHASH))
        Error::errort(body.front(), "'##' cannot appear at start of macro expansion");
    if (lex->is_keyword(body.back(), DataStruct::AST_TYPE::KHASHHASH))
        Error::errort(body.back(), "'##' cannot appear at end of macro expansion");
}

//读取函数体，同时必要的时候进行token参数替换
std::vector<DataStruct::Token > MacroPreprocessor::read_funclike_macro_body(const std::unordered_map<std::string,DataStruct::Token >&params) const
{
    std::vector<DataStruct::Token >r;
    for (;;) {
        DataStruct::Token tok = lex->lex();
        if (tok.kind == DataStruct::TOKEN_TYPE::TNEWLINE)
            return r;
        if (tok.kind == DataStruct::TOKEN_TYPE::TIDENT) {
            DataStruct::Token subst{DataStruct::TOKEN_TYPE::TPLACEHOLDER,-1};
            if (params.count(*(tok.sval))>0)
                subst=params.at(*(tok.sval));
            if (subst.kind!=DataStruct::TOKEN_TYPE::TPLACEHOLDER) {
                subst = copy_token(subst);
                subst.space = tok.space;   //主要注意这里，前导空格关系到宏的展开处理
                r.push_back(subst);
                continue;
            }
        }
        r.push_back(tok);
    }
}

void MacroPreprocessor::read_obj_macro(const std::string& sval)
{
    std::vector<DataStruct::Token > body;
    while(true) {
        DataStruct::Token tok = lex->lex();
        if (tok.kind == DataStruct::TOKEN_TYPE::TNEWLINE)
            break;
        body.push_back(tok);
    }
    hashhash_check(body);
    macros[sval]=make_obj_macro(body);
}

void MacroPreprocessor::read_define()
{
    DataStruct::Token name=read_ident();
    DataStruct::Token tok = lex->lex();
    //注意，函数宏的括号前面不能有空格
    if (lex->is_keyword(tok, lex->get_keywords("(")) && !tok.space) {
        read_funclike_macro(name);
        return;
    }
    lex->retreat_token(tok);
    read_obj_macro(*(name.sval));
}

bool MacroPreprocessor::is_digit_seq(std::string &seq) const {
    for(auto &e:seq)
        if (!isdigit(e))
            return false;
    return true;
}

//这是gun的一个拓展功能，格式信心为# linenum，filename，flags，保持文件的原始信息
void MacroPreprocessor::read_linemarker(const DataStruct::Token &tok)
{
    if (!is_digit_seq(*(tok.sval)))
        Error::errort(tok, "line number expected, but got %s", Utils::tok2s(tok));
    int line=stoi(*(tok.sval));
    DataStruct::Token filename=lex->lex();
    if (filename.kind!=DataStruct::TOKEN_TYPE::TSTRING)
        Error::errort(tok, "filename expected, but got %s", Utils::tok2s(tok));
    std::string name=*(filename.sval);
    while (filename.kind!=DataStruct::TOKEN_TYPE::TNEWLINE)
        filename=lex->lex();
    std::shared_ptr<DataStruct::File> fi=lex->current_file();
    fi->line=line;
    fi->name=name;
}
void MacroPreprocessor::read_elif(const DataStruct::Token &hash)
{
    if (cond_incl_stack.empty())
        Error::errort(hash, "stray #elif");
    auto ci = cond_incl_stack.back();
    if (ci.ctx == DataStruct::CondInclCtx ::IN_ELSE)
        Error::errort(hash, "#elif after #else");
    ci.ctx = DataStruct::CondInclCtx::IN_ELIF;
    ci.include_guard = "";
    if (ci.wastrue || !read_constexpr()) {
        lex->skip_cond_incl();
        return;
    }
    ci.wastrue = true;
}

//读取else条件，因为只有else才需要和上面的条件编译条件做交互，所以这里的else的wastrue并不需要额外设置
void MacroPreprocessor::read_else(const DataStruct::Token &hash)
{
    if (cond_incl_stack.empty())
        Error::errort(hash, "stray #else");
    DataStruct::CondIncl &ci = cond_incl_stack.back();
    if (ci.ctx == DataStruct::CondInclCtx::IN_ELSE)
        Error::errort(hash, "#else appears in #else");
    expect_newline();
    ci.ctx = DataStruct::CondInclCtx::IN_ELSE;
    ci.include_guard = "";
    if (ci.wastrue)
        lex->skip_cond_incl();
}

DataStruct::Token MacroPreprocessor::skip_newlines() {
    DataStruct::Token tok = lex->lex();
    while (tok.kind == DataStruct::TOKEN_TYPE::TNEWLINE)
        tok = lex->lex();
    lex->retreat_token(tok);
    return tok;
}
//endif的处理需要注意一点，就是作为包含哨使用的时候，
void MacroPreprocessor::read_endif(const DataStruct::Token&hash)
{
    if (cond_incl_stack.empty())
        Error::errort(hash, "stray #endif");
    DataStruct::CondIncl ci = cond_incl_stack.back();
    cond_incl_stack.pop_back();
    expect_newline();

    if (ci.include_guard.empty() || ci.file->name!= hash.file->name)
        return;
    DataStruct::Token last = skip_newlines();
    if (ci.file->name != last.file->name)
        include_guard[ci.file->name]= ci.include_guard;
}
std::string MacroPreprocessor::read_error_message() {
    std::string b;
    for (;;) {
        DataStruct::Token tok = lex->lex();
        if (tok.kind == DataStruct::TOKEN_TYPE::TNEWLINE)
            return b;
        if (!b.empty() && tok.space)
            b+=' ';
        b+=Utils::format( "%s", Utils::tok2s(tok));
    }
}
void MacroPreprocessor::read_error(const DataStruct::Token&hash)
{
    Error::errort(hash,"#error: %s", read_error_message());
}
DataStruct::Token MacroPreprocessor::read_defined_op(){
    auto tok = lex->lex();
    if (lex->is_keyword(tok, lex->get_keywords("("))) {
        tok = lex->lex();
        expect(lex->get_keywords(")"));
    }
    if (tok.kind != DataStruct::TOKEN_TYPE::TIDENT)
        Error::errort(tok, "identifier expected, but got %s", Utils::tok2s(tok));
    return macros.find(*tok.sval)!=macros.end() ? CPP_TOKEN_ONE : CPP_TOKEN_ZERO;
}
std::vector<DataStruct::Token> MacroPreprocessor::read_intexpr_line(){
    std::vector<DataStruct::Token> r;
    for (;;) {
        auto tok = read_expand_newline();
        if (tok.kind == DataStruct::TOKEN_TYPE::TNEWLINE)
            return r;
        if (lex->is_ident(tok, "defined")) {
            r.emplace_back(read_defined_op());
        } else if (tok.kind == DataStruct::TOKEN_TYPE::TIDENT) {
            // C11 6.10.1.4
            r.push_back(CPP_TOKEN_ZERO);
        } else {
            r.push_back(tok);
        }
    }
}
void MacroPreprocessor::do_read_if(bool istrue) {
    cond_incl_stack.emplace_back(make_cond_incl(istrue));
    if (!istrue)
        lex->skip_cond_incl();
}
bool MacroPreprocessor::read_constexpr() {
    auto val=read_intexpr_line();
    std::reverse(val.begin(),val.end());
    lex->token_buffer_stash(val);
    auto expr = parser->read_expr();
    auto tok = lex->lex();
    if (tok.kind != DataStruct::TOKEN_TYPE::TEOF)
        Error::errort(tok, "stray token: %s", Utils::tok2s(tok));
    lex->token_buffer_unstash();
    return parser->eval_intexpr(expr, nullptr);
}

void MacroPreprocessor::read_if()
{
    do_read_if(read_constexpr());
}
void MacroPreprocessor::read_ifdef()
{
    DataStruct::Token tok = lex->lex();
    if (tok.kind != DataStruct::TOKEN_TYPE::TIDENT)
        Error::errort(tok, "identifier expected, but got %s", Utils::tok2s(tok));
    expect_newline();
    do_read_if(macros.find(*(tok.sval))!=macros.end());
}
//这里主要是注意包含哨处理
void MacroPreprocessor::read_ifndef()
{
    DataStruct::Token tok = lex->lex();
    if (tok.kind != DataStruct::TOKEN_TYPE::TIDENT)
        Error::errort(tok, "identifier expected, but got %s", Utils::tok2s(tok));
    expect_newline();
    do_read_if(macros.find(*(tok.sval))==macros.end());
    if (tok.count == 2) {
        DataStruct::CondIncl &ci = cond_incl_stack.back();
        ci.include_guard = *(tok.sval);
        ci.file = tok.file;
    }
}
std::string MacroPreprocessor::join_paths(std::vector<DataStruct::Token >& args) {
    std::string b;
    for (auto &e:args)
        b+=Utils::format("%s", Utils::tok2s(e));
    return b;
}

std::string MacroPreprocessor::read_cpp_header_name(const DataStruct::Token &hash, bool &std) {

    std::string path = lex->read_header_file_name(std);
    if (!path.empty())
        return path;

    //8cc中写，如果不是采用<>或“”包含的，那么宏展开也可能是合法的文件路径
    DataStruct::Token tok = read_expand_newline();
    if (tok.kind == DataStruct::TOKEN_TYPE::TNEWLINE)
        Error::errort(hash, "expected filename, but got newline");
    if (tok.kind == DataStruct::TOKEN_TYPE::TSTRING) {
        std = false;
        return *(tok.sval);
    }
    if (!lex->is_keyword(tok, lex->get_keywords("<")))
        Error::errort(tok, "< expected, but got %s", Utils::tok2s(tok));
    std::vector<DataStruct::Token > tokens;
    for (;;) {
        DataStruct::Token tok = read_expand_newline();
        if (tok.kind == DataStruct::TOKEN_TYPE::TNEWLINE)
            Error::errort(hash, "premature end of header name");
        if (lex->is_keyword(tok, lex->get_keywords(">")))
            break;
        tokens.push_back(tok);
    }
    std = true;
    return join_paths(tokens);
}
void MacroPreprocessor::define_obj_macro(const std::string &name, const DataStruct::Token &value) {
    macros[name]=make_obj_macro({value});
}
//查询包含哨是否已经被定义
bool MacroPreprocessor::guarded(std::string &path) {
    std::string guard;
    //获取文件对应的包含哨
    if (include_guard.find(path)!=include_guard.end())
        guard= include_guard.at(path);
    //查看包含哨是否已经定义
    bool r = (!guard.empty() && macros.find(guard)!=macros.end());
    define_obj_macro("__ycc_include_guard", r ? CPP_TOKEN_ONE : CPP_TOKEN_ZERO);
    return r;
}
//注意只包含一次的两种情况
bool MacroPreprocessor::try_include(std::string dir, const std::string& filename, bool isimport) {
    std::string path = Path::fullpath(Utils::format("%s/%s", dir, filename));
    if (once.find(path)!=once.end())
        return true;
    if (guarded(path))
        return true;
    std::shared_ptr<std::ifstream> ifi=std::make_shared<std::ifstream>(path);
    if (!(*ifi))
        return false;
    if (isimport)
        once[path]=1;
    lex->stream_push(lex->make_file(ifi, path));
    return true;
}
//绝对路径包含，相对路径包含，标准路径包含
void MacroPreprocessor::read_include(const DataStruct::Token &hash, std::shared_ptr<DataStruct::File > file, bool isimport)
{
    bool std;
    std::string filename = read_cpp_header_name(hash, std);
    expect_newline();
    if (filename[0] == '/') {
        if (try_include("/", filename, isimport))
            return;
        goto err;
    }
    if (!std) {
        std::string dir=".";
        char tpath[256];
        if (!file->name.empty()){
            auto len=file->name.copy(tpath,file->name.size(),0);
            tpath[len]='\0';
            dir=dirname(tpath);
        }

        if (try_include(dir, filename, isimport))
            return;
    }
    for(auto &e:std_include_path)
        if (try_include(e, filename, isimport))
            return;
    err:
    Error::errort(hash, "cannot find header file: %s", filename);
}
//这是gun的一个拓展指令，他的作用是跳过搜索路径上的第一个找到的文件，而包含下一个同名文件
void MacroPreprocessor::read_include_next(const DataStruct::Token &hash, std::shared_ptr<DataStruct::File> file)
{
    bool std;
    std::string filename = read_cpp_header_name(hash, std);
    expect_newline();
    std::string cur = Path::fullpath(file->name);
    int i = 0;
    if (filename[0] == '/') {
        if (try_include("/", filename, false)) {
            return;
        }
        goto err;
    }
    for (; i < std_include_path.size(); i++)
    {
        if (cur == Path::fullpath(Utils::format("%s/%s", std_include_path[i], filename)))
            break;
    }
    for (i++; i < std_include_path.size(); i++)
        if (try_include(std_include_path[i], filename, false))
            return;
    err:
    Error::errort(hash, "cannot find header file: %s", filename);
}
//#line 的作用是改变当前行数和文件名称，它们是在编译程序中预先定义的标识符命令的基本形式如下：
//
//#line number["filename"]
//
//其中[]内的文件名可以省略。
void MacroPreprocessor::read_line()
{
    DataStruct::Token tok = read_expand_newline();
    if (tok.kind != DataStruct::TOKEN_TYPE::TNUMBER || !is_digit_seq(*(tok.sval)))
        Error::errort(tok, "number expected after #line, but got %s", Utils::tok2s(tok));
    int line = stoi(*(tok.sval));
    tok = read_expand_newline();
    std::string filename;
    if (tok.kind == DataStruct::TOKEN_TYPE::TSTRING) {
        filename = *(tok.sval);
        expect_newline();
    } else if (tok.kind != DataStruct::TOKEN_TYPE::TNEWLINE) {
        Error::errort(tok, "newline or a source name are expected, but got %s", Utils::tok2s(tok));
    }
    std::shared_ptr<DataStruct::File > f = lex->current_file();
    f->line = line;
    if (!filename.empty())
        f->name = filename;
}
void MacroPreprocessor::parse_pragma_operand(const DataStruct::Token &tok) {
    std::string s = *(tok.sval);
    if (s ==  "once") {
        std::string path = Path::fullpath(tok.file->name);
        once[path]=1;
    } else if (s == "enable_warning") {
        Error::enable_warning = true;
    } else if (s == "disable_warning") {
        Error::enable_warning = false;
    } else {
        Error::errort(tok, "unknown #pragma: %s", s);
    }
}
void MacroPreprocessor::read_pragma()
{
    DataStruct::Token tok = read_ident();
    parse_pragma_operand(tok);
}
void MacroPreprocessor::read_undef()
{
    DataStruct::Token name = read_ident();
    expect_newline();
    macros.erase(*(name.sval));
}
void MacroPreprocessor::read_warning(const DataStruct::Token&hash)
{
    Error::warnt(hash, "#warning: %s", read_error_message());
}
void MacroPreprocessor::make_token_pushback(const DataStruct::Token &tmpl, DataStruct::TOKEN_TYPE kind, const std::string &sval) {
    DataStruct::Token tok(tmpl);
    tok.kind = kind;
    tok.sval.reset(new std::string(sval));
    tok.enc = DataStruct::ENCODE::ENC_NONE;
    lex->retreat_token(tok);
}
////特殊宏 __TIME__
////返回当前时间
//void MacroPreprocessor::handle_time_macro(const DataStruct::Token &tmpl) {
//    char buf[10];
//    strftime(buf, sizeof(buf), "%T", &now);
//    make_token_pushback(tmpl, DataStruct::TOKEN_TYPE::TSTRING, std::string(buf));
//}
////特殊宏 __DATE__
////%b 月份缩写，如 Aug
////%e 月中的某一天
////%Y 年份
////...
////具体的格式可以从这里查到http://www.cplusplus.com/reference/ctime/strftime/
//void MacroPreprocessor::handle_date_macro(const DataStruct::Token &tmpl) {
//    char buf[20];
//    strftime(buf, sizeof(buf), "%b %e %Y", &(now));
//    make_token_pushback(tmpl, DataStruct::TOKEN_TYPE::TSTRING, std::string(buf));
//}
//// 特殊宏__FILE_
//// 返回文件名_
//void MacroPreprocessor::handle_file_macro(const DataStruct::Token &tmpl) {
//    make_token_pushback(tmpl, DataStruct::TOKEN_TYPE::TSTRING, tmpl.file->name);
//}
////特殊宏 __LINE__
////返回行号
//void MacroPreprocessor::handle_line_macro(const DataStruct::Token &tmpl) {
//    make_token_pushback(tmpl, DataStruct::TOKEN_TYPE::TNUMBER, std::to_string(tmpl.file->line));
//}
////特殊宏__PAAGMA__
////
//void MacroPreprocessor::handle_pragma_macro(const DataStruct::Token &tmpl) {
//    expect(lex->get_keywords("("));
//    const DataStruct::Token &operand = read_token();
//    if (operand.kind != DataStruct::TOKEN_TYPE::TSTRING)
//        Error::errort(operand, "_Pragma takes a string literal, but got %s", Utils::tok2s(operand));
//    expect(lex->get_keywords(")"));
//    parse_pragma_operand(operand);
//    make_token_pushback(tmpl, DataStruct::TOKEN_TYPE::TNUMBER, "1");
//}
////https://gcc.gnu.org/onlinedocs/cpp/Common-Predefined-Macros.html
////下面4个是GUN的拓展宏
//void MacroPreprocessor::handle_base_file_macro(const DataStruct::Token &tmpl) {
//    make_token_pushback(tmpl, DataStruct::TOKEN_TYPE::TSTRING, Utils::infile);
//}
////GUN中的特殊宏 __TIMESTAMP__
//// 返回一个字符串，描述当前源文件的最后一次的修改时间，格式为日期和时间
//void MacroPreprocessor::handle_timestamp_macro(const DataStruct::Token &tmpl) {
//    char buf[30];
//    strftime(buf, sizeof(buf), "%a %b %e %T %Y", localtime(&(tmpl.file->mtime)));
//    make_token_pushback(tmpl, DataStruct::TOKEN_TYPE::TSTRING, std::string(buf));
//}
//
//void MacroPreprocessor::handle_counter_macro(const DataStruct::Token &tmpl) {
//    static int counter = 0;
//    make_token_pushback(tmpl, DataStruct::TOKEN_TYPE::TNUMBER, std::to_string(counter++));
//}
//
//void MacroPreprocessor::handle_include_level_macro(const DataStruct::Token &tmpl) {
//    make_token_pushback(tmpl, DataStruct::TOKEN_TYPE::TNUMBER, std::to_string(lex->steam_depth()-1));
//}
void MacroPreprocessor::add_include_path(const std::string &path) {
    std_include_path.push_back(path);
}
void MacroPreprocessor::init_path_and_macros() {
//    vec_push(std_include_path, BUILD_DIR "/include");
    std_include_path.emplace_back("/usr/local/lib/ycc/include");
    std_include_path.emplace_back("/usr/local/include");
    std_include_path.emplace_back("/usr/include");
    std_include_path.emplace_back("/usr/include/linux");
    std_include_path.emplace_back("/usr/include/x86_64-linux-gnu");

    macros["__DATE__"]=make_special_macro(handle_date_macro);
    macros["__TIME__"]=make_special_macro(handle_time_macro);
    macros["__FILE__"]= make_special_macro(handle_file_macro);
    macros["__LINE__"]= make_special_macro(handle_line_macro);
    macros["_Pragma"]= make_special_macro(handle_pragma_macro);
    macros["__BASE_FILE__"]= make_special_macro(handle_base_file_macro);
    macros["__COUNTER__"]= make_special_macro(handle_counter_macro);
    macros["__INCLUDE_LEVEL__"]= make_special_macro(handle_include_level_macro);
    macros["__TIMESTAMP__"]=make_special_macro(handle_timestamp_macro);
    macros["__BASE_FILE__"]=make_special_macro(handle_base_file_macro);
    read_from_string(std::string("#include <")+ Utils::BUILD_DIR+std::string("/include/8cc.h>"));
}

void MacroPreprocessor::init_now() {
    time_t timet = time(nullptr);
    localtime_r(&timet, &now);
}
//ycc支持UCS编码，所以需要设置locale以支持相应字符的显示
void MacroPreprocessor::cpp_init() {
    std::setlocale(LC_ALL, "C");
    lex->init_keywords();
    init_now();
    init_path_and_macros();
}

//解析一个字符串，适合处理一些宏
void MacroPreprocessor::read_from_string(const std::string& s){
    lex->stream_push(lex->make_file_string(s));
    auto toplevels = parser->read_toplevels();
//    for (auto &e:toplevels)
//        emit_toplevel(e);
    lex->stream_unstash();
}
//特殊宏 __TIME__
//返回当前时间
void handle_time_macro(MacroPreprocessor* thi,const DataStruct::Token &tmpl) {
    char buf[10];
    auto tm=thi->getNow();
    strftime(buf, sizeof(buf), "%T", &tm);
    thi->make_token_pushback(tmpl, DataStruct::TOKEN_TYPE::TSTRING, std::string(buf));
}
//特殊宏 __DATE__
//%b 月份缩写，如 Aug
//%e 月中的某一天
//%Y 年份
//...
//具体的格式可以从这里查到http://www.cplusplus.com/reference/ctime/strftime/
void handle_date_macro(MacroPreprocessor* thi,const DataStruct::Token &tmpl) {
    char buf[20];
    auto tm=thi->getNow();
    strftime(buf, sizeof(buf), "%b %e %Y", &tm);
    thi->make_token_pushback(tmpl, DataStruct::TOKEN_TYPE::TSTRING, std::string(buf));
}
// 特殊宏__FILE_
// 返回文件名_
void handle_file_macro(MacroPreprocessor* thi,const DataStruct::Token &tmpl) {
    thi->make_token_pushback(tmpl, DataStruct::TOKEN_TYPE::TSTRING, tmpl.file->name);
}
//特殊宏 __LINE__
//返回行号
void handle_line_macro(MacroPreprocessor* thi,const DataStruct::Token &tmpl) {
    thi->make_token_pushback(tmpl, DataStruct::TOKEN_TYPE::TNUMBER, std::to_string(tmpl.file->line));
}
//特殊宏__PAAGMA__
//
void handle_pragma_macro(MacroPreprocessor* thi,const DataStruct::Token &tmpl) {
    thi->expect(thi->lex->get_keywords("("));
    const DataStruct::Token &operand = thi->read_token();
    if (operand.kind != DataStruct::TOKEN_TYPE::TSTRING)
        Error::errort(operand, "_Pragma takes a string literal, but got %s", Utils::tok2s(operand));
    thi->expect(thi->lex->get_keywords(")"));
    thi->parse_pragma_operand(operand);
    thi->make_token_pushback(tmpl, DataStruct::TOKEN_TYPE::TNUMBER, "1");
}
//https://gcc.gnu.org/onlinedocs/cpp/Common-Predefined-Macros.html
//下面4个是GUN的拓展宏
void handle_base_file_macro(MacroPreprocessor* thi,const DataStruct::Token &tmpl) {
    thi->make_token_pushback(tmpl, DataStruct::TOKEN_TYPE::TSTRING, Utils::infile);
}
//GUN中的特殊宏 __TIMESTAMP__
// 返回一个字符串，描述当前源文件的最后一次的修改时间，格式为日期和时间
void handle_timestamp_macro(MacroPreprocessor* thi,const DataStruct::Token &tmpl) {
    char buf[30];
    strftime(buf, sizeof(buf), "%a %b %e %T %Y", localtime(&(tmpl.file->mtime)));
    thi->make_token_pushback(tmpl, DataStruct::TOKEN_TYPE::TSTRING, std::string(buf));
}

void handle_counter_macro(MacroPreprocessor* thi,const DataStruct::Token &tmpl) {
    static int counter = 0;
    thi->make_token_pushback(tmpl, DataStruct::TOKEN_TYPE::TNUMBER, std::to_string(counter++));
}

void handle_include_level_macro(MacroPreprocessor* thi,const DataStruct::Token &tmpl) {
    thi->make_token_pushback(tmpl, DataStruct::TOKEN_TYPE::TNUMBER, std::to_string(thi->lex->steam_depth()-1));
}

