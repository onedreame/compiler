//
// Created by yk on 17-8-28.
//

#include <cstdlib>
#include <cstring>
#include "../token_analysier/Exception_Handler.h"
#include "Stack.h"

//typedef struct{
//    void **base;   //栈底指针
//    void **top;     //栈顶指针
//    int stacksize;  //栈当前可用的最大容量，以元素个数计
//} Stack;

Stack global_sym_stack, local_sym_stack;

/**********************************************
 * 功能：             初始化栈存储容量
 *stack：            栈存储结构
 *initsize：         栈初始化分配空间
 *********************************************/
void stack_init(Stack * stack,int initsize)
{
    void **base =(void**)malloc(initsize*sizeof(void**));
    if(!base)
        error("内存分配失败");
    stack->base=base;
    stack->top=base;
    stack->stacksize=initsize;
}

/*
 * 由这里可以看出，本次实现的栈是栈顶指向要存的位置
 * 以字节为单位从源端拷贝到目的端
 * 这里原本的stacktop直接指向了锌元素，没有新分配内存，结果就当之了
 * 打印语句以后当前存储的符号改变了，真是坑爹
 */
void * stack_push(Stack * stack,void * element,int size)
{
//    printf("添加新元素\n");
    if(stack->top>=stack->base+stack->stacksize)
    {
        int newsize=stack->stacksize*2;
//        printf("栈分配空间");
        stack->base=(void**)realloc(stack->base,(sizeof(void**)*newsize));
        if(!stack->base)
            return NULL;
        stack->top=stack->base+stack->stacksize;
        stack->stacksize=newsize;
    }
    *stack->top=(void**)malloc(size);
    memcpy(*stack->top,element,size);
    stack->top++;
    return *(stack->top-1);
}

void stack_pop(Stack *stack)
{
//    printf("stack pop\n");
    if(stack->top>stack->base)
    {
        free(*(--stack->top));
    }
}

void *stack_get_top(Stack *stack)
{
//    printf("stack_get_top\n");
    if(stack->top>stack->base)
    {
        void * data=*(stack->top-1);
        return data;
    }
    return NULL;
}

bool stack_is_empty(Stack *stack)
{
//    printf("stack_empty\n");
    return stack->top==stack->base;
}

void stack_destroy(Stack *stack)
{
//    printf("stack_destroy\n");
    while(stack->top!=stack->base)
        stack_pop(stack);
    if (stack->base)
        free(*stack->base);
    stack->base=NULL;
    stack->top=NULL;
    stack->stacksize=0;
}