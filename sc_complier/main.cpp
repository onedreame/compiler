//
// Created by yk on 17-7-25.
//

#include <cstdio>
#include "token_analysier/TokenParser.h"
#include "token_analysier/DynArray.h"
#include "token_analysier/TokenCode.h"
#include "token_analysier/TKTable.h"
#include "syntax_analysier/syntax_analysier.h"
#include "syntax_analysier/Stack.h"
#include "syntax_analysier/syn_const.h"

int token;              //单词编码
char *filename;     //文件名
int line_num;       //行数
char ch;        //当前取到的源码字符
int tkvalue;
FILE *fin;
void init()
{
    line_num=1;
    init_lex();
    stack_init(&local_sym_stack,8);
    stack_init(&global_sym_stack,8);
    sec_sym_put(".rdata",0);
    int_type.t=T_INT;
    char_pointer_type.t=T_CHAR;
    mk_pointer(char_pointer_type);
    default_func_type.t=T_FUNC;
    default_func_type.ref=sym_push(SC_ANOM,&int_type,KW_CDECL,0);
}
void cleanup() {
    printf("\ntktable.count=%d\n",tktable.count);
    sym_pop(&global_sym_stack,NULL);
    stack_destroy(&local_sym_stack);
    stack_destroy(&global_sym_stack);
    for (int i=TK_IDENT;i<tktable.count;++i)
    {
        free(tktable.data[i]);
    }

}
int main(int argc, char **argv)
{
    fin=fopen(argv[1],"rb");
    filename=argv[1];
    if (!fin)
    {
        printf("不能打开sc源文件！\n");
        return 0;
    }
    printf("打开文件成功\n");
    init();
    getch();

    get_token();
    translate_unit();

    printf("\n代码行数：%d行\n",line_num);
    cleanup();
    fclose(fin);
    printf("%s 词法分析通过！\n",argv[1]);
    return 1;
}

