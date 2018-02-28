//
// Created by yk on 17-8-28.
//

#ifndef SC_COMPLIER_STACK_H
#define SC_COMPLIER_STACK_H

/*
 * void **base;   //栈底指针
 * void **top;     //栈顶指针
 * int stacksize;  //栈当前可用的最大容量，以元素个数计
 */
typedef struct{
    void **base;
    void **top;
    int stacksize;
} Stack;

extern Stack global_sym_stack, local_sym_stack;

extern void stack_init(Stack * stack,int initsize);

extern void * stack_push(Stack * stack,void * element,int size);

extern void stack_pop(Stack *stack);

extern void *stack_get_top(Stack *stack);

extern bool stack_is_empty(Stack *stack);

extern void stack_destroy(Stack *stack);
#endif //SC_COMPLIER_STACK_H
