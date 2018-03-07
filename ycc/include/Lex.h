//
// Created by yk on 18-2-8.
//

#ifndef YCC_LEX_H
#define YCC_LEX_H


#include <fstream>
#include <iostream>
#include <cstring>
#include <unordered_map>
#include "error.h"
#include "buffer.h"
#include "enumerate.h"
#include "Token.h"


class Lex
{
public:
    typedef struct {int line;int column;} Pos;
    Lex()= default;
    void open_file(const std::string& s)
    {
        fs=std::make_shared<std::ifstream>(s);
        if (!(*fs)){
            Error::error("%s does not exits,please check:%s",s,strerror(errno));
        }
        buffers.push_back(std::vector<DataStruct::Token>());
        files.push_back(make_file(fs,s));
        std::cout<<"fs is open?"<<fs->is_open()<<std::endl;
    }
    explicit Lex(const std::string& fi):filename(fi){
        fs=std::make_shared<std::ifstream>(filename);
        if (!(*fs))
        {
            Error::error("%s does not exits,please check:%s",filename,strerror(errno));
        }
        buffers.push_back(std::vector<DataStruct::Token>());
        files.push_back(make_file(fs,filename));
        std::cout<<"fs is open?"<<fs->is_open()<<std::endl;
    }
    std::string pos_string(Pos p)
    {
        return Utils::format("%s:%d:%d",filename.size()?filename:"(unknown)",p.line,p.column);
    }

    int readc_file(std::shared_ptr<DataStruct::File>);
    int get();
    int readc_string(std::shared_ptr<DataStruct::File> &);
    int readc();
    void stream_stash(std::shared_ptr<DataStruct::File> fi){
        stashed.push_back(files);
        files.clear();
        files.push_back(fi);
    }
    void stream_unstash(){
        files=stashed.back();
        stashed.pop_back();
    }
    std::string current_file_postion();
    Pos get_pos(int delta);
    DataStruct::Token make_token(DataStruct::Token tmpl);
    DataStruct::Token make_ident(std::string p);
    DataStruct::Token make_strtok(std::string s, DataStruct::ENCODE enc);
    DataStruct::Token make_keyword(DataStruct::AST_TYPE) ;
    DataStruct::Token make_number(std::string s);
    DataStruct::Token make_invalid(char c) ;
    DataStruct::Token make_char(int c, DataStruct::ENCODE enc);
    bool iswhitespace(int c) {
        return c == ' ' || c == '\t' || c == '\f' || c == '\v';
    }
    int peek();
    bool next(int);
    void skip_line();
    bool do_skip_space();
    void skip_block_comment();
    bool skip_space();
    void skip_char();
    void skip_string();
    void skip_cond_incl();
    bool is_ident(const DataStruct::Token &, const std::string&);

    DataStruct::Token read_number(char c);
    DataStruct::Token read_ident(int );

    DataStruct::Token do_read_token();
    bool buffer_empty() { return buffers.size()==1&& buffers[0].size()==0;};
    std::string read_header_file_name(bool &);
    bool is_keyword(const DataStruct::Token& tok, DataStruct::AST_TYPE c) const {
        return (tok.kind == DataStruct::TOKEN_TYPE::TKEYWORD) && (tok.id == c);
    }
    void retreat_token(DataStruct::Token& tok)
    {
        if (tok.kind==DataStruct::TOKEN_TYPE::TEOF)
            return;
        buffers.back().push_back(tok);
    }
    DataStruct::Token lex_string(std::string&);
    DataStruct::Token lex();
    void token_buffer_stash(std::vector<DataStruct::Token>&tokens){buffers.push_back(tokens);}
    void token_buffer_unstash(){if (!buffers.empty()) buffers.pop_back();}
    const DataStruct::Token SPACE_TOKEN=DataStruct::Token(DataStruct::TOKEN_TYPE::TSPACE,-1);
    const DataStruct::Token NEWLINE_TOKEN=DataStruct::Token(DataStruct::TOKEN_TYPE::TNEWLINE,-1);
    const DataStruct::Token EOF_TOKEN=DataStruct::Token(DataStruct::TOKEN_TYPE::TEOF,-1);
    std::shared_ptr<DataStruct::File> make_file(std::shared_ptr<std::ifstream> ifi,std::string filename);
    void init_keywords();

    int steam_depth(){ return files.size();}
    DataStruct::AST_TYPE get_keywords(const std::string&s) const {
        if (keywords.find(s)!=keywords.end())
            return keywords.at(s);
        Error::error("%s is not keyword",s);
    }
    static std::string get_keywords_string(DataStruct::AST_TYPE);
    void stream_push(std::shared_ptr<DataStruct::File> fi){
        files.push_back(fi);
    }

    std::shared_ptr<DataStruct::File> make_file_string(const std::string& s);
    std::shared_ptr<DataStruct::File > current_file(){
        if (files.empty())
            return nullptr;
        return files.back();
    }
    void debug(){
        for (int i = 0; i < stashed.size(); ++i) {
            std::cout<<"####################"<<std::endl;
            for (int j = 0; j < stashed[i].size(); ++j) {
                char  c;
                std::cout<<"当前文件为："<<stashed[i][j]->name<<std::endl;
                while(!stashed[i][j]->file->eof())
                {
                    *(stashed[i][j]->file)>>c;
                    std::cout<<c<<" ";
                }
                std::cout<<std::endl;
            }
        }
    }
private:
    std::vector<std::shared_ptr<DataStruct::File>> files;
    std::vector<std::vector<std::shared_ptr<DataStruct::File>>> stashed;
    std::shared_ptr<std::ifstream> fs;
    std::string filename;

    Pos cur;
    std::vector<std::vector<DataStruct::Token>> buffers;

    std::unordered_map<std::string,DataStruct::AST_TYPE> keywords;
    static std::unordered_map<int ,std::string> rkeywords;
    void retreat(int c);
    void mark(){
        cur.line=files.back()->line;
        cur.column=files.back()->column;
    }
    bool nextoct();
    int read_oct_char(int);
    int read_hex_char();
    int read_universal_char(int);
    DataStruct::Token read_rep2(char ,DataStruct::AST_TYPE,char ,DataStruct::AST_TYPE,DataStruct::AST_TYPE);
    DataStruct::Token read_rep(char,DataStruct::AST_TYPE,DataStruct::AST_TYPE);
    DataStruct::Token read_string(DataStruct::ENCODE enc);
    DataStruct::Token read_char(DataStruct::ENCODE);
    int read_escaped_char();
    bool is_valid_ucn(unsigned int);
    DataStruct::Token read_hash_digraph();
};
#endif //YCC_LEX_H
