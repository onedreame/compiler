//
// Created by yk on 18-2-8.
//

#include "../include/parser.h"

bool Parser::is_funcdef() {
    std::vector<DataStruct::Token> vec;
    bool r;
    if (macro== nullptr)
        Error::error("MacroPropressor in Parser should be initialized.");
    if (lex== nullptr)
        Error::error("Lex in Parser should be initialized.");
    while (true)
    {
        DataStruct::Token tok=macro->read_token();
        vec.push_back(tok);
        if (tok.kind==DataStruct::TOKEN_TYPE::TEOF)
            Error::error("premature end of input");
        if (lex->is_keyword(tok,lex->get_keywords(";")))
            break;

    }
}