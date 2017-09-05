//
// Created by yk on 17-8-17.
//

#ifndef SC_COMPLIER_SYNTAX_ANALYSIER_H
#define SC_COMPLIER_SYNTAX_ANALYSIER_H

#include "../token_analysier/Exception_Handler.h"
#include "Symbol.h"

void print_tab(int l);

void syntax_indent();

void external_declaration(int l);

void translate_unit();

bool type_specifier(Type &type);

void funcbody(Symbol *s);

void mk_pointer(Type &type);

void struct_specifier(Type &type);

void struct_member_alignment(int *align);

void struct_declaration_list(const Type *type);

int cal_align(int offset,int align);

void struct_declaration(int *maxalign,int *offset,Symbol ***ps);

void struct_declarator_list();

int function_calling_convention(int &fc);

int type_size(Type &type,int &align);

void declarator(int *align,int *v,Type &type,int &fc);

void direct_declarator(Type &type,int *v, int &fc);

void direct_declarator_postfix(Type &type,int fc);

void parameter_type_list(Type &type,int fc);

void initializer(Type *type);

void compound_statement(int *bsym,int *csym);

void assignment_expression();

void equality_expression();

void relational_expression();

void additive_expression();

void multiplicative_expression();

void unary_expression();

void postfix_expression();

void sizeof_expression();

void primary_expression();

void expression();

void argument_expression_list();

void statement();

void expression_statement();

void if_statement();

void for_statement();

void continue_statement();

void break_statement();

void return_statement();
#endif //SC_COMPLIER_SYNTAX_ANALYSIER_H
