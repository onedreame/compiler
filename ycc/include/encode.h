//
// Created by yk on 18-3-7.
//

#ifndef YCC_ENCODE_H
#define YCC_ENCODE_H

#include <cstdint>
#include <string>

namespace Utils{
    int count_leading_ones(char);
    int read_rune(uint32_t &, const std::string &, char *);
    std::string to_utf32(char *, int);
    void write16(std::string &, uint16_t );
    void write32(std::string& , uint32_t );
    void write_utf8(std::string &, uint32_t);
    std::string to_utf16(char *, int);
}
#endif //YCC_ENCODE_H
