//
// Created by yk on 18-3-7.
//

#include "../include/encode.h"
#include "../include/error.h"
namespace Utils{
    int count_leading_ones(char c) {
        for (int i = 7; i >= 0; i--)
            if ((c & (1 << i)) == 0)
                return 7 - i;
        return 8;
    }

    int read_rune(uint32_t &r, const std::string &s, char *end) {
        int len = count_leading_ones(s[0]);
        if (len == 0) {
            r = s[0];
            return 1;
        }
        if (&s[0] + len > end)
            Error::error("invalid UTF-8 sequence");
        for (int i = 1; i < len; i++)
            if ((s[i] & 0xC0) != 0x80)
                Error::error("invalid UTF-8 continuation byte");
        switch (len) {
            case 2:
                r = ((s[0] & 0x1F) << 6) | (s[1] & 0x3F);
                return 2;
            case 3:
                r = ((s[0] & 0xF) << 12) | ((s[1] & 0x3F) << 6) | (s[2] & 0x3F);
                return 3;
            case 4:
                r = ((s[0] & 0x7) << 18) | ((s[1] & 0x3F) << 12) | ((s[2] & 0x3F) << 6) | (s[3] & 0x3F);
                return 4;
        }
        Error::error("invalid UTF-8 sequence");
    }

    void write16(std::string &b, uint16_t x) {
        b+= static_cast<char >(x & 0xFF);
        b+= static_cast<char >(x >> 8);
    }

    void write32(std::string& b, uint32_t x) {
        write16(b, x & 0xFFFF);
        write16(b, x >> 16);
    }

    std::string to_utf16(char *p, int len) {
        std::string b;
        char *end = p + len;
        while (p != end) {
            uint32_t rune;
            p += read_rune(rune, p, end);
            if (rune < 0x10000) {
                write16(b, rune);
            } else {
                write16(b, (rune >> 10) + 0xD7C0);
                write16(b, (rune & 0x3FF) + 0xDC00);
            }
        }
        return b;
    }

    std::string to_utf32(char *p, int len) {
        std::string b;
        char *end = p + len;
        while (p != end) {
            uint32_t rune;
            p += read_rune(rune, p, end);
            write32(b, rune);
        }
        return b;
    }

    void write_utf8(std::string &b, uint32_t rune) {
        if (rune < 0x80) {
            b+= static_cast<char >(rune);
            return;
        }
        if (rune < 0x800) {
            b+= static_cast<char >( 0xC0 | (rune >> 6));
            b+= static_cast<char >(0x80 | (rune & 0x3F));
            return;
        }
        if (rune < 0x10000) {
            b+= static_cast<char>( 0xE0 | (rune >> 12));
            b+= static_cast<char >(0x80 | ((rune >> 6) & 0x3F));
            b+= static_cast<char >(0x80 | (rune & 0x3F));
            return;
        }
        if (rune < 0x200000) {
            b+= static_cast<char >( 0xF0 | (rune >> 18));
            b+= static_cast<char >(0x80 | ((rune >> 12) & 0x3F));
            b+= static_cast<char >(0x80 | ((rune >> 6) & 0x3F));
            b+= static_cast<char >(0x80 | (rune & 0x3F));
            return;
        }
        Error::error("invalid UCS character: \\U%08x", rune);
    }
}