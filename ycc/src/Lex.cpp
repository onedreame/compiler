//
// Created by yk on 18-2-8.
//

#include <sys/stat.h>
#include "../include/Lex.h"
#include "../include/encode.h"

//根据文件构建文件结构
std::shared_ptr<DataStruct::File> Lex::make_file(std::shared_ptr<std::ifstream> ifi, std::string filename) {
    std::shared_ptr<DataStruct::File> fi=std::make_shared<DataStruct::File>();
    fi->column=1;
    fi->line=1;
    fi->name=filename;
    fi->file=ifi;
    fi->ntok=0;
    struct stat buf;
    if(stat(filename.c_str(),&buf)==-1)
        Error::error("Can not get file stat:%s",strerror(errno));
    fi->mtime=buf.st_mtime;
    return fi;
}
std::shared_ptr<DataStruct::File > Lex::current_file(){
    if (files.empty())
        return nullptr;
    return files.back();
}
std::string Lex::encoding_prefix(DataStruct::ENCODE enc) {
    switch (enc) {
        case DataStruct::ENCODE::ENC_CHAR16:
            return "u";
        case DataStruct::ENCODE::ENC_CHAR32:
            return "U";
        case DataStruct::ENCODE::ENC_UTF8:
            return "u8";
        case DataStruct::ENCODE::ENC_WCHAR:
            return "L";
    }
    return "";
}

std::string Lex::tok2s(const DataStruct::Token &tok) {
    if (tok.kind == DataStruct::TOKEN_TYPE::TPLACEHOLDER)
        return "(null)";
    switch (tok.kind) {
        case DataStruct::TOKEN_TYPE::TIDENT:
            return *(tok.sval);
        case DataStruct::TOKEN_TYPE::TKEYWORD:
            switch (tok.id) {
#define op(id, str)         case DataStruct::AST_TYPE::id: return str;
#define keyword(id, str, _) case DataStruct::AST_TYPE::id: return str;

#include "../include/keyword.inc"

#undef keyword
#undef op
                default:
                    return Lex::get_keywords_string(tok.id);
            }
        case DataStruct::TOKEN_TYPE::TCHAR:
            return Utils::format("%s'%s'",
                                 encoding_prefix(tok.enc),
                                 Utils::quote_char(tok.c));
        case DataStruct::TOKEN_TYPE::TNUMBER:
            return *(tok.sval);
        case DataStruct::TOKEN_TYPE::TSTRING:
            return Utils::format("%s\"%s\"",
                          encoding_prefix(tok.enc),
                          Utils::parse_cstring((*(tok.sval)).c_str()));
        case DataStruct::TOKEN_TYPE::TEOF:
            return "(eof)";
        case DataStruct::TOKEN_TYPE::TINVALID:
            return std::string{static_cast<char >(tok.c)};
        case DataStruct::TOKEN_TYPE::TNEWLINE:
            return "(newline)";
        case DataStruct::TOKEN_TYPE::TSPACE:
            return "(space)";
        case DataStruct::TOKEN_TYPE::TMACRO_PARAM:
            return "(macro-param)";
    }
    Error::error("internal error: unknown token kind: %d", static_cast<int>(tok.kind));
}
DataStruct::AST_TYPE Lex::get_keywords(const std::string&s) const {
    if (keywords.find(s)!=keywords.end())
        return keywords.at(s);
    Error::error("%s is not keyword",s);
}
std::string Lex::pos_string(Pos p)
{
    return Utils::format("%s:%d:%d",filename.size()?filename:"(unknown)",p.line,p.column);
}
void Lex::stream_stash(std::shared_ptr<DataStruct::File> fi){
    stashed.push_back(files);
    files.clear();
    files.push_back(fi);
}
void Lex::stream_unstash(){
    files=stashed.back();
    stashed.pop_back();
}
void Lex::add_file(const std::string& s)
{
    fs=std::make_shared<std::ifstream>(s);
    if (!(*fs)){
        Error::error("%s does not exits,please check:%s",s,strerror(errno));
    }
    files.push_back(make_file(fs,s));
//    std::cout<<"fs is open?"<<fs->is_open()<<std::endl;
}
void Lex::retreat_token(DataStruct::Token& tok)
{
    if (tok.kind==DataStruct::TOKEN_TYPE::TEOF)
        return;
    buffers.back().push_back(tok);
}
bool Lex::is_keyword(const DataStruct::Token& tok, DataStruct::AST_TYPE c) const {
    return (tok.kind == DataStruct::TOKEN_TYPE::TKEYWORD) && (tok.id == c);
}
std::shared_ptr<Lex> Lex::Instance(){
    if (!_lex)
        _lex.reset(new Lex());
    return _lex;
}
std::shared_ptr<Lex> Lex::_lex= nullptr;
//解析字符串
std::shared_ptr<DataStruct::File> Lex::make_file_string(const std::string& s)
{
    std::shared_ptr<DataStruct::File> fi=std::make_shared<DataStruct::File>();
    fi->p=s;
    fi->cur=0;
    fi->line=1;
    fi->column=1;
    return fi;
}

//读取一个字符，char无法代表负值，所以用int代替返回
int Lex::readc_file(std::shared_ptr<DataStruct::File> fi) {
    int res=fi->file->get();
    if (fi->file->eof()) {
        res = (fi->last == '\n' || fi->last == EOF) ? EOF : '\n';
    } else if (res == '\r') {
        int c2=fi->file->get();
        if (c2 != '\n')
            fi->file->seekg(-1,std::ios::cur);
        res = '\n';
    }
    fi->last = res;
    return res;
}

int Lex::readc_string(std::shared_ptr<DataStruct::File> &f) {
    int c;
    if (f->p.empty()||f->p.size()<=f->cur) {
        c = (f->last == '\n' || f->last == EOF) ? EOF : '\n';
        f->cur++;
    } else if (f->p[f->cur] == '\r') {
        f->cur++;
        if (f->cur<f->p.size()&&f->p[f->cur] == '\n')
            f->cur++;
        c = '\n';
    } else {
        c = f->p[f->cur++];
    }
    f->last = c;
    return c;
}

//读取字符，以便readc函数做进一步处理
int Lex::get() {
    std::shared_ptr<DataStruct::File> &f= files.back();
    int c;
    if (f->file) {
        c = readc_file(f);
    } else {
        c = readc_string(f);
    }
    if (c == '\n') {
        f->line++;
        f->column = 1;
    } else if (c != EOF) {
        f->column++;
    }
    return c;
}

//处理读取到的字符，可能读取多个文件，所以要处理多文件操作，以及多行情况处理
int Lex::readc() {
    for (;;) {
        int c = get();
        if (c == EOF) {
            if (files.size() == 1)
                return c;
            files.pop_back();
            continue;
        }
        if (c != '\\')
            return c;
        int c2 = get();
        if (c2 == '\n')
            continue;
        retreat(c2);
        return c;
    }
}

void Lex::retreat(int c) {
    if (c==EOF) return;
    if (files.back()->file)
        if(!files.back()->file->eof())
            files.back()->file->seekg(-1,std::ios::cur);
        else files.back()->last=0;
    else
        files.back()->cur--;
    if (c=='\n')
    {
        files.back()->line--;
        files.back()->column=1;
    }else{
        files.back()->column--;
    }
    if (!files.back()->p.empty()&&files.back()->last=='\n')
        files.back()->last=0;
}

std::string Lex::current_file_postion() {
    if (files.empty()) return std::string("(unknown)");
    std::shared_ptr<DataStruct::File>& f=files.back();
    return Utils::format("%s:%d:%d",f->name,f->line,f->column);
}

Lex::Pos Lex::get_pos(int delta){
    std::shared_ptr<DataStruct::File>& f=files.back();
    return Pos{.line=f->line,.column=f->column+delta };
}

DataStruct::Token Lex::make_token(DataStruct::Token tmpl) {
    DataStruct::Token r=tmpl;
    r.hideset.clear();
    r.line=cur.line;
    r.column=cur.column;
    r.file=files.back();
    r.count= (r.file->ntok)++;
    return r;
}

//对于数字，以字符串的形式保存其内容
//对于关键字，保存其id
//对于字符串和字符，保存其内容和编码方式
//非法字符只保存其内容，不保存编码方式
DataStruct::Token Lex::make_ident(std::string p) {
    return make_token(DataStruct::Token(DataStruct::TOKEN_TYPE::TIDENT, std::move(p)));
}

DataStruct::Token Lex::make_strtok(std::string s, DataStruct::ENCODE enc) {
    return make_token(DataStruct::Token(DataStruct::TOKEN_TYPE::TSTRING, std::move(s), '\0',enc ));
}

DataStruct::Token Lex::make_keyword(DataStruct::AST_TYPE id) {
    return make_token(DataStruct::Token(DataStruct::TOKEN_TYPE::TKEYWORD,id ));
}

DataStruct::Token Lex::make_number(std::string s) {
    return make_token(DataStruct::Token(DataStruct::TOKEN_TYPE::TNUMBER,std::move(s) ));
}

DataStruct::Token Lex::make_invalid(char c) {
    return make_token(DataStruct::Token(DataStruct::TOKEN_TYPE::TINVALID, "", c ));
}

DataStruct::Token Lex::make_char(int c, DataStruct::ENCODE enc) {
    return make_token(DataStruct::Token(DataStruct::TOKEN_TYPE::TCHAR, "", c, enc ));
}

int Lex::peek() {
    int r = readc();
    retreat(r);
    return r;
}

bool Lex::next(int expect){
    int r=readc();
    if (r==expect)
        return true;
    retreat(r);
    return false;
}

void Lex::skip_line()
{
    while (true)
    {
        int r=readc();
        if (r==EOF)
            return;
        if (r=='\n')
        {
            retreat(r);
            return;
        }
    }
}

void Lex::skip_block_comment() {
    Pos p = get_pos(-2);
    bool maybe_end = false;
    for (;;) {
        int c = readc();
        if (c == EOF)
            Error::errorp(pos_string(p),"premature end of block comment");
        if (c == '/' && maybe_end)
            return;
        maybe_end = (c == '*');
    }
}

bool Lex::do_skip_space() {
    int r=readc();
    if (r==EOF)
        return false;
    if (iswhitespace(r))
        return true;
    if (r=='/')
    {
        if (next('/'))
        {
            skip_line();
            return true;
        }
        if (next('*'))
        {
            skip_block_comment();
            return true;
        }
    }
    retreat(r);
    return false;
}

bool Lex::skip_space() {
    if (!do_skip_space())
        return false;
    while (do_skip_space());
    return true;
}

void Lex::skip_char() {
    if (readc()=='\\')
        readc();
    int r=readc();
    while(r!=EOF&&r!='\'')
        r=readc();
}

void Lex::skip_string() {
    for (int r = readc(); r!=EOF&&r!='"' ; r=readc())
        if (r=='\\')
            readc();
}

//读取数字序列，标准的数字序列只会包含数字，字母，小数点，正负号，科学计数法
DataStruct::Token Lex::read_number(char c) {
    std::string s;
    s+=c;
    int last='\0';
    std::string tok("eEpP");
    std::string pn("+-");
    while (true)
    {
        int r=readc();
        bool flonum=tok.find(last)!=std::string::npos&&pn.find(r)!=std::string::npos;
        if (!isdigit(r)&&!isalnum(r)&&r!='.'&&!flonum)
        {
            retreat(r);
            return make_number(s);
        }
        s+=r;
        last=r;
    }
}

bool Lex::nextoct() {
    int r=peek();
    return '0'<=r&&r<='7';
}

//八进制数字，最多只有3位
int Lex::read_oct_char(int c) {
    int r=c-'0';
    if (!nextoct())
        return r;
    int r1=readc();
    r=r<<3+r1-'0';
    if(!nextoct())
        return r;
    r1=readc();
    return r<<3+r1-'0';
}

//16进制是在一个int范围内的，这里有可能会出问题。
int Lex::read_hex_char() {
    Pos p = get_pos(-2);
    int c = readc();
    if (!isxdigit(c))
        Error::errorp(pos_string(p), "\\x is not followed by a hexadecimal character: %c", c);
    int r = 0;
    for (;; c = readc()) {
        switch (c) {
            case '0' ... '9': r = (r << 4) | (c - '0'); continue;
            case 'a' ... 'f': r = (r << 4) | (c - 'a' + 10); continue;
            case 'A' ... 'F': r = (r << 4) | (c - 'A' + 10); continue;
            default: retreat(c); return r;
        }
    }
}

bool Lex::is_valid_ucn(unsigned int c) {
    // C11 6.4.3p2: U+D800 to U+DFFF are reserved for surrogate pairs.
    // A codepoint within the range cannot be a valid character.
    if (0xD800 <= c && c <= 0xDFFF)
        return false;
    // It's not allowed to encode ASCII characters using \U or \u.
    // Some characters not in the basic character set (C11 5.2.1p3)
    // are allowed as exceptions.
    return 0xA0 <= c || c == '$' || c == '@' || c == '`';
}

int Lex::read_universal_char(int len) {
    Pos p = get_pos(-2);
    unsigned int r = 0;
    for (int i = 0; i < len; i++) {
        int c = readc();
        switch (c) {
            case '0' ... '9': r = (r << 4) | (c - '0'); continue;
            case 'a' ... 'f': r = (r << 4) | (c - 'a' + 10); continue;
            case 'A' ... 'F': r = (r << 4) | (c - 'A' + 10); continue;
            default: Error::errorp(pos_string(p), "invalid universal character: %c", c);
        }
    }
    if (!is_valid_ucn(r))
        Error::errorp(pos_string(p), "invalid universal character: \\%c%x", (len == 4) ? 'u' : 'U', r);
    return r;
}

int Lex::read_escaped_char() {
    Pos p = get_pos(-1);
    int c = readc();
    switch (c) {
        case '\'': case '"': case '?': case '\\':
            return c;
        case 'a': return '\a';
        case 'b': return '\b';
        case 'f': return '\f';
        case 'n': return '\n';
        case 'r': return '\r';
        case 't': return '\t';
        case 'v': return '\v';
        case 'e': return '\033';  // '\e' is GNU extension
        case 'x': return read_hex_char();
        case 'u': return read_universal_char(4);
        case 'U': return read_universal_char(8);
        case '0' ... '7': return read_oct_char(c);
    }
    Error::warnp(pos_string(p), "unknown escape character: \\%c", c);
    return c;
}

DataStruct::Token Lex::read_string(DataStruct::ENCODE enc)
{
    std::string src;
    while (true)
    {
        int c = readc();
        if (c == EOF)
            Error::errorp(pos_string(cur), "unterminated string");
        if (c == '"')
            break;
        if (c != '\\') {
            src+=c;
            continue;
        }
        bool isucs = (peek() == 'u' || peek() == 'U');
        c = read_escaped_char();
        if (isucs) {
            Utils::write_utf8(src, c);
            continue;
        }
        src+=c;
    }
    return make_strtok(src, enc);
}

DataStruct::Token Lex::read_char(DataStruct::ENCODE enc) {
    Pos p=get_pos(-1);
    int r=readc();
    if (r=='\\')
        r=read_escaped_char();
    int c=readc();
    if (c!='\'')
        Error::errorp(pos_string(p),"unterminated char");
    if (enc==DataStruct::ENCODE::ENC_NONE)
        return make_char(static_cast<char >(r),enc);
    return make_char(r,enc);
}

DataStruct::Token Lex::read_ident(int c) {
    std::string ident{c};
    while(true)
    {
        int r=readc();
        if (isalnum(r) || (r & 0x80) || r == '_' || r == '$'){
            ident+=r;
            continue;
        }
        if (r == '\\' && (peek() == 'u' || peek() == 'U')) {
            Utils::write_utf8(ident, read_escaped_char());
            continue;
        }
        retreat(r);
        return make_ident(ident);
    }
}

DataStruct::Token Lex::read_rep2(char a1,DataStruct::AST_TYPE id1,char a2,DataStruct::AST_TYPE id2,DataStruct::AST_TYPE id3)
{
    if (next(a1))
        return make_keyword(id1);
    if (next(a2))
        return make_keyword(id2);
    return make_keyword(id3);
}

DataStruct::Token Lex::read_rep(char a1,DataStruct::AST_TYPE id1,DataStruct::AST_TYPE id2)
{
    if (next(a1))
        return make_keyword(id1);
    return make_keyword(id2);
}
std::unordered_map<int ,std::string> Lex::rkeywords;

std::string Lex::get_keywords_string(DataStruct::AST_TYPE s)
{
    int r= static_cast<int >(s);
    if (rkeywords.find(r)!=rkeywords.end())
        return rkeywords[r];
    Error::error("%d is not a valid AST_TYPE", static_cast<int>(s));
}

//构建关键字词典
void Lex::init_keywords() {
#define op(id,str) keywords[str]=DataStruct::AST_TYPE::id;
#define keyword(id,str,_) keywords[str]=DataStruct::AST_TYPE::id;

#include "../include/keyword.inc"

#undef op
#undef keyword

    keywords["["]=DataStruct::AST_TYPE::LSQUARE_BRACKET;
    keywords["]"]=DataStruct::AST_TYPE::RSQUARE_BRACKET;
    keywords["("]=DataStruct::AST_TYPE::LPARENTHESE;
    keywords[")"]=DataStruct::AST_TYPE::RPARENTHESE;
    keywords["{"]=DataStruct::AST_TYPE::LBRACE;
    keywords["}"]=DataStruct::AST_TYPE::RBRACE;
    keywords["."]=DataStruct::AST_TYPE::DOT;
    keywords[","]=DataStruct::AST_TYPE::COMMA;
    keywords[":"]=DataStruct::AST_TYPE::COLON;
    keywords["+"]=DataStruct::AST_TYPE::PLUS;
    keywords["-"]=DataStruct::AST_TYPE::SUB;
    keywords["*"]=DataStruct::AST_TYPE::MUL;
    keywords["/"]=DataStruct::AST_TYPE::DIV;
    keywords["="]=DataStruct::AST_TYPE::ASSIGN;
    keywords["!"]=DataStruct::AST_TYPE::EXCLAMATION;
    keywords["&"]=DataStruct::AST_TYPE::AND;
    keywords["|"]=DataStruct::AST_TYPE::OR;
    keywords["^"]=DataStruct::AST_TYPE::XOR;
    keywords["~"]=DataStruct::AST_TYPE::NEG;
    keywords["?"]=DataStruct::AST_TYPE::QUES;
    keywords[";"]=DataStruct::AST_TYPE::SEMI;
    keywords["<"]=DataStruct::AST_TYPE::LOW;
    keywords[">"]=DataStruct::AST_TYPE::HIG;
    keywords["%"]=DataStruct::AST_TYPE::LEFT;
#define op(id,str) rkeywords[static_cast<int>(DataStruct::AST_TYPE::id)]=str;
#define keyword(id,str,_) rkeywords[static_cast<int>(DataStruct::AST_TYPE::id)]=str;

#include "../include/keyword.inc"

#undef op
#undef keyword

    rkeywords[static_cast<int >(DataStruct::AST_TYPE::LSQUARE_BRACKET)]="[";
    rkeywords[static_cast<int >(DataStruct::AST_TYPE::RSQUARE_BRACKET)]="]";
    rkeywords[static_cast<int >(DataStruct::AST_TYPE::LPARENTHESE)]="(";
    rkeywords[static_cast<int >(DataStruct::AST_TYPE::RPARENTHESE)]=")";
    rkeywords[static_cast<int >(DataStruct::AST_TYPE::LBRACE)]="{";
    rkeywords[static_cast<int >(DataStruct::AST_TYPE::RBRACE)]="}";
    rkeywords[static_cast<int >(DataStruct::AST_TYPE::DOT)]=".";
    rkeywords[static_cast<int >(DataStruct::AST_TYPE::COMMA)]=",";
    rkeywords[static_cast<int >(DataStruct::AST_TYPE::COLON)]=":";
    rkeywords[static_cast<int >(DataStruct::AST_TYPE::PLUS)]="+";
    rkeywords[static_cast<int >(DataStruct::AST_TYPE::SUB)]="-";
    rkeywords[static_cast<int >(DataStruct::AST_TYPE::MUL)]="*";
    rkeywords[static_cast<int >(DataStruct::AST_TYPE::DIV)]="/";
    rkeywords[static_cast<int >(DataStruct::AST_TYPE::ASSIGN)]="=";
    rkeywords[static_cast<int >(DataStruct::AST_TYPE::EXCLAMATION)]="!";
    rkeywords[static_cast<int >(DataStruct::AST_TYPE::AND)]="&";
    rkeywords[static_cast<int >(DataStruct::AST_TYPE::OR)]="|";
    rkeywords[static_cast<int >(DataStruct::AST_TYPE::XOR)]="^";
    rkeywords[static_cast<int >(DataStruct::AST_TYPE::NEG)]="~";
    rkeywords[static_cast<int >(DataStruct::AST_TYPE::QUES)]="?";
    rkeywords[static_cast<int >(DataStruct::AST_TYPE::SEMI)]=";";
    rkeywords[static_cast<int >(DataStruct::AST_TYPE::LOW)]="<";
    rkeywords[static_cast<int >(DataStruct::AST_TYPE::HIG)]=">";
    rkeywords[static_cast<int >(DataStruct::AST_TYPE::LEFT)]="%";
}

//解析字符，生成相应的token，注意，原来c语言还有如下的2字符和3字符之分
//某些标点符号，在一些键盘上是无法输入的，所以C提供了这样的替换方式。其中双字符记号有六个，也成为双字符组(digraph)：
//
//<:     [
//:>     ]
//<%   {
//%>   }
//%:     #
//%:%: ##
//
//如果上述任何字符组出现在字符常量或字符串字面值中，则不会被解释成双字符组；除此之外，它们一律被解释成双字符组，作用和相应的单一字符记号完全一样
DataStruct::Token Lex::do_read_token() {
    if (skip_space())
        return SPACE_TOKEN;
    mark();
    int c=readc();
    switch (c){
        case '\n': return NEWLINE_TOKEN;
        //读了标准才发现，原来c语言的索引括号除了可以是[]外，还可是是<: :>
        case ':': return make_keyword(next('>') ? keywords["]"] : keywords[":"]);
        case '#': return make_keyword(next('#') ? DataStruct::AST_TYPE::KHASHHASH : keywords["#"]);
        case '+': return read_rep2('+', DataStruct::AST_TYPE::OP_INC, '=', DataStruct::AST_TYPE::OP_A_ADD, keywords["+"]);
        case '*': return read_rep('=', DataStruct::AST_TYPE::OP_A_MUL, keywords["*"]);
        case '=': return read_rep('=', DataStruct::AST_TYPE::OP_EQ, keywords["="]);
        case '!': return read_rep('=', DataStruct::AST_TYPE::OP_NE, keywords["!"]);
        case '&': return read_rep2('&', DataStruct::AST_TYPE::OP_LOGAND, '=', DataStruct::AST_TYPE::OP_A_AND, keywords["&"]);
        case '|': return read_rep2('|', DataStruct::AST_TYPE::OP_LOGOR, '=', DataStruct::AST_TYPE::OP_A_OR, keywords["|"]);
        case '^': return read_rep('=', DataStruct::AST_TYPE::OP_A_XOR, keywords["^"]);
        case '"': return read_string(DataStruct::ENCODE::ENC_NONE);
        case '\'': return read_char(DataStruct::ENCODE::ENC_NONE);
        case '/': return make_keyword(next('=') ? DataStruct::AST_TYPE::OP_A_DIV : keywords["/"]);
        case 'a' ... 't': case 'v' ... 'z': case 'A' ... 'K':
        case 'M' ... 'T': case 'V' ... 'Z': case '_': case '$':
        case 0x80 ... 0xFD:
            return read_ident(c);
        case '0' ... '9':
            return read_number(c);
        case 'L': case 'U': {
            // Wide/char32_t character/string literal
            auto enc = (c == 'L') ? DataStruct::ENCODE::ENC_WCHAR : DataStruct::ENCODE::ENC_CHAR32;
            if (next('"'))  return read_string(enc);
            if (next('\'')) return read_char(enc);
            return read_ident(c);
        }
        case 'u':
            if (next('"')) return read_string(DataStruct::ENCODE::ENC_CHAR16);
            if (next('\'')) return read_char(DataStruct::ENCODE::ENC_CHAR16);
            if (next('8')) {
                if (next('"'))
                    return read_string(DataStruct::ENCODE::ENC_UTF8);
                retreat('8');
            }
            return read_ident(c);
        case '.':
            if (isdigit(peek()))
                return read_number(c);  //C语言支持python的浮点数表示
            if (next('.')) {
                if (next('.'))
                    return make_keyword(DataStruct::AST_TYPE ::KELLIPSIS);
                return make_ident("..");
            }
            return make_keyword(keywords["."]);
        case '(': case ')': case ',': case ';': case '[': case ']': case '{':
        case '}': case '?': case '~':
            return make_keyword(keywords[std::string()+ static_cast<char >(c)]);
        case '-':
            if (next('-')) return make_keyword(DataStruct::AST_TYPE::OP_DEC);
            if (next('>')) return make_keyword(DataStruct::AST_TYPE::OP_ARROW);
            if (next('=')) return make_keyword(DataStruct::AST_TYPE::OP_A_SUB);
            return make_keyword(keywords["-"]);
        case '<':
            if (next('<')) return read_rep('=', DataStruct::AST_TYPE::OP_A_SAL, DataStruct::AST_TYPE::OP_SAL);
            if (next('=')) return make_keyword(DataStruct::AST_TYPE::OP_LE);
            //注意c语言中的另类括号表示方式
            if (next(':')) return make_keyword(keywords["["]);
            if (next('%')) return make_keyword(keywords["{"]);
            return make_keyword(keywords["<"]);
        case '>':
            if (next('=')) return make_keyword(DataStruct::AST_TYPE::OP_GE);
            if (next('>')) return read_rep('=', DataStruct::AST_TYPE::OP_A_SAR, DataStruct::AST_TYPE::OP_SAR);
            return make_keyword(keywords[">"]);
        case '%': {
            DataStruct::Token tok = read_hash_digraph();
            if (tok.id!=keywords["%"])
                return tok;
            return read_rep('=', DataStruct::AST_TYPE::OP_A_MOD, keywords["%"]);
        }
        case EOF:
            return EOF_TOKEN;
        default: return make_invalid(c);
    }
}

DataStruct::Token Lex::read_hash_digraph() {
    if (next('>'))
        return make_keyword(keywords["}"]);
    if (next(':'))
    {
        if (next('%'))
        {
            if (next(':'))
                return make_keyword(keywords["##"]);
            retreat('%');
        }
        return make_keyword(keywords["#"]);
    }
    return make_keyword(keywords["%"]); //如果不是双字符，那么用%代表未读取到有效的token
}

//读取include文件名字
std::string Lex::read_header_file_name(bool &std) {
    if (!buffer_empty())
        return "";
    skip_space();
    Pos p = get_pos(0);
    char close;
    if (next('"')) {
        std = false;
        close = '"';
    } else if (next('<')) {
        std = true;
        close = '>';
    } else {
        return "";
    }
    std::string b;
    while (!next(close)) {
        int c = readc();
        if (c == EOF || c == '\n')
            Error::errorp(pos_string(p), "premature end of header name");
        b+=c;
    }
    if (b.empty())
        Error::errorp(pos_string(p), "header name should not be empty");
    return b;
}

DataStruct::Token Lex::lex_string(std::string& s) {
    files.push_back(make_file_string(s));
    DataStruct::Token r = do_read_token();
    next('\n');
    Pos p = get_pos(0);
    if (peek() != EOF)
        Error::errorp(pos_string(p), "unconsumed input: %s", s);
    files=stashed.back();
    stashed.pop_back();
    return r;
}

DataStruct::Token Lex::lex() {
    auto & buf = buffers.back();
    if (!buf.empty())
    {
        DataStruct::Token res=buf.back();
        buf.pop_back();
        return res;
    }
    if (buffers.size() > 1)
        return EOF_TOKEN;
    bool bol = (files.back()->column == 1);
    DataStruct::Token tok = do_read_token();
    while (tok.kind == DataStruct::TOKEN_TYPE::TSPACE) {
        tok = do_read_token();
        tok.space = true;
    }
    tok.bol = bol;
    return tok;
}

void Lex::skip_cond_incl() {
    int nest = 0;
    for (;;) {
        bool bol = (current_file()->column == 1);
        skip_space();
        int c = readc();
        if (c == EOF)
            return;
        if (c == '\'') {
            skip_char();
            continue;
        }
        if (c == '\"') {
            skip_string();
            continue;
        }
        if (c != '#' || !bol)
            continue;
        int column = current_file()->column - 1;
        DataStruct::Token tok = lex();
        if (tok.kind != DataStruct::TOKEN_TYPE::TIDENT)
            continue;
        if (!nest && (is_ident(tok, "else") || is_ident(tok, "elif") || is_ident(tok, "endif"))) {
            retreat_token(tok);
            DataStruct::Token hash = make_keyword(keywords["#"]);
            hash.bol = true;
            hash.column = column;
            retreat_token(hash);
            return;
        }
        if (is_ident(tok, "if") || is_ident(tok, "ifdef") || is_ident(tok, "ifndef"))
            nest++;
        else if (nest && is_ident(tok, "endif"))
            nest--;
        skip_line();
    }
}

bool Lex::is_ident(const DataStruct::Token &tok, const std::string& s) {
    return tok.kind == DataStruct::TOKEN_TYPE ::TIDENT && *(tok.sval)== s;
}