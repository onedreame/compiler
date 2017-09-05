//
// Created by yk on 17-8-28.
//

#ifndef SC_COMPLIER_SYN_CONST_H
#define SC_COMPLIER_SYN_CONST_H
/*数据类型编码*/
#include "Symbol.h"
/*
 * 数据类型编码
 * T_INT       =0,     整型
 * T_CHAR      =1,     字符型
 * T_SHORT     =2,     短整型
 * T_VOID      =3,     空类型
 * T_PTR       =4,     指针
 * T_FUNC      =5,     函数
 * T_STRUCT    =6,     结构体
 * T_BTYPE     =0xf,   基本类型掩码
 * T_ARRAY     =0X10,  数组
 */
enum e_TypeCode
{
    T_INT       =0,
    T_CHAR      =1,
    T_SHORT     =2,
    T_VOID      =3,
    T_PTR       =4,
    T_FUNC      =5,
    T_STRUCT    =6,
    T_BTYPE     =0xf,
    T_ARRAY     =0X10,
};

/*字符串指针*/
extern Type char_pointer_type;

/*int类型*/
extern Type int_type;

/*缺省函数类型*/
extern Type default_func_type;

/*存储类型
 *SC_GLOBAL: =0XF0,   包括整型变量，字符常量，字符串常量，全局变量，函数定义
 *SC_LOCAL:  =0XF1,   栈中变量
 *SC_LLOCAL: =0XF2,   寄存器溢出存放栈中
 *SC_CMP:    =0XF3,   使用标志寄存器
 *C_VALMASK: =0XFF,   存储类型掩码
 *SC_LVAL:   =0X100,  左值
 *SC_SYM:    =0X200,  符号
 *SC_ANOM:   =0X10000000,   匿名符号
 *SC_STRUCT: =0X20000000,   结构体符号
 *SC_MEMBER: =0X40000000,   结构体成员变量
 *SC_PAPAMS: =0X80000000,   函数参数
 */
enum e_StorageClass
{
    SC_GLOBAL    =0XF0,
    SC_LOCAL     =0XF1,
    SC_LLOCAL    =0XF2,
    SC_CMP       =0XF3,
    SC_VALMASK   =0XFF,
    SC_LVAL      =0X100,
    SC_SYM       =0X200,

    SC_ANOM      =0X10000000,
    SC_STRUCT    =0X20000000,
    SC_MEMBER    =0X40000000,
    SC_PAPAMS    =0X80000000,
};
#endif //SC_COMPLIER_SYN_CONST_H
