//
// Created by yk on 17-7-26.
//

#ifndef SC_COMPLIER_DYNSTRINGDEF_H
#define SC_COMPLIER_DYNSTRINGDEF_H


typedef struct DynString{
    int count;
    int capacity;
    char *data;
}DynString;
#endif //SC_COMPLIER_DYNSTRINGDEF_H
