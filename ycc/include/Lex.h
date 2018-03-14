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
    void add_file(const std::string& s);
    DataStruct::Token lex();
    bool is_keyword(const DataStruct::Token& tok, DataStruct::AST_TYPE c) const ;
    static std::string get_keywords_string(DataStruct::AST_TYPE);
    void retreat_token(DataStruct::Token& tok);
    DataStruct::AST_TYPE get_keywords(const std::string&s) const;
    void token_buffer_stash(std::vector<DataStruct::Token>&tokens){buffers.push_back(tokens);}
    void token_buffer_unstash(){if (!buffers.empty()) buffers.pop_back();}
    DataStruct::Token make_strtok(std::string s, DataStruct::ENCODE enc);
    std::string current_file_postion();
    std::shared_ptr<DataStruct::File > current_file();
    void skip_cond_incl();
    bool is_ident(const DataStruct::Token &, const std::string&);
    std::string read_header_file_name(bool &);
    std::shared_ptr<DataStruct::File> make_file(std::shared_ptr<std::ifstream> ifi,std::string filename);
    void stream_push(std::shared_ptr<DataStruct::File> fi){ files.push_back(fi); }
    void init_keywords();
    std::shared_ptr<DataStruct::File> make_file_string(const std::string& s);
    int steam_depth(){ return files.size();}
    void stream_unstash();
    void stream_stash(std::shared_ptr<DataStruct::File> fi);
    DataStruct::Token lex_string(std::string&);
    static std::shared_ptr<Lex> Instance();
    bool in_keyword(const DataStruct::Token& tok){ return keywords.find(*tok.sval)!=keywords.end();}

    void debug(){
        std::cout<<"________________"<<std::endl;
        for (int i = 0; i < files.size(); ++i) {
           std::cout<<"文件名："<<files[i]->name;
           std::cout<<"\t行号："<<files[i]->line;
            std::cout<<"\t列号："<<files[i]->column<<std::endl;
        }
    }
private:
    static std::shared_ptr<Lex> _lex;
    std::vector<std::shared_ptr<DataStruct::File>> files;
    std::vector<std::vector<std::shared_ptr<DataStruct::File>>> stashed;
    std::shared_ptr<std::ifstream> fs;
    std::string filename;

    Pos cur;
    std::vector<std::vector<DataStruct::Token>> buffers{std::vector<DataStruct::Token>()};

    std::unordered_map<std::string,DataStruct::AST_TYPE> keywords;
    static std::unordered_map<int ,std::string> rkeywords;

    Lex()= default;
    Lex(const Lex&)= delete;
    Lex(Lex&&)= delete;
    Lex&operator=(const Lex&)= delete;
    Lex&operator=(Lex&&)= delete;
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
    int readc_file(std::shared_ptr<DataStruct::File>);
    int get();
    int readc_string(std::shared_ptr<DataStruct::File> &);
    int readc();
    std::string pos_string(Pos p);
    Pos get_pos(int delta);
    DataStruct::Token make_token(DataStruct::Token tmpl);
    DataStruct::Token make_ident(std::string p);
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

    DataStruct::Token read_number(char c);
    DataStruct::Token read_ident(int );

    DataStruct::Token do_read_token();
    bool buffer_empty() { return buffers.size()==1&& buffers[0].size()==0;};
    const DataStruct::Token SPACE_TOKEN=DataStruct::Token(DataStruct::TOKEN_TYPE::TSPACE,-1);
    const DataStruct::Token NEWLINE_TOKEN=DataStruct::Token(DataStruct::TOKEN_TYPE::TNEWLINE,-1);
    const DataStruct::Token EOF_TOKEN=DataStruct::Token(DataStruct::TOKEN_TYPE::TEOF,-1);

};
#endif //YCC_LEX_H
