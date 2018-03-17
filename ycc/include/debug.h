//
// Created by yk on 18-3-17.
//

#ifndef YCC_DEBUG_H
#define YCC_DEBUG_H

#include <string>
#include "Token.h"

namespace Utils{
    std::string ty2s(const std::shared_ptr<DataStruct::Type>&);
    std::string tok2s(const DataStruct::Token& tok);
    std::string node2s(const std::shared_ptr<DataStruct::Node> &node);
    std::string encoding_prefix(DataStruct::ENCODE enc);
    void do_node2s(std::string&b, const std::shared_ptr<DataStruct::Node> &node);
    void binop_to_string(std::string &b, std::string &op, const std::shared_ptr<DataStruct::Node> &node);
}
#endif //YCC_DEBUG_H
