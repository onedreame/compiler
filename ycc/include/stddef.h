//
// Created by yk on 18-1-1.
//

#ifndef YCC_STDDEF_H
#define YCC_STDDEF_H
typedef unsigned long size_t;
typedef long ptrdiff_t;
typedef unsigned int wchar_t;
typedef long double max_align_t;
#define offsetof(type,number) ((size_t)&(((type*)0)->member))
#endif //YCC_STDDEF_H
