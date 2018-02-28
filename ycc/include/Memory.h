// Created by yk on 18-2-7.
//

#ifndef YCC_MEMORY_H
#define YCC_MEMORY_H

#include <memory>
#include <vector>
#include <iostream>
#include <unordered_map>
#include "Token.h"

template <typename T,typename U=int>
class memory{
public:
    explicit memory():count(0){}
    memory(const memory& )= delete;
    memory&operator=(const memory&)= delete;
    memory(memory&&)= delete;
    memory&operator=(memory&&)= delete;
    memory(T&& t) noexcept :count(0) {
        _append(std::forward(t));
    }
    memory(T& t):count(0){
        _append(t);
    }
    void append(T&&t) noexcept {
        _append(std::forward<T>(t));
    }
    void append(T& t){
        _append(t);
    }
    void erase(T& t){

    }

    void iter(){
        for (int i = 0; i < token_list.size(); ++i) {
            for(int j=0;j<BLOCK_SIZE;++j)
                std::cout<<token_list[i][j]<<" ";
        }
    }

private:
    int count;
    enum {BLOCK_SIZE=8};
    std::vector<std::unique_ptr<T[]>> token_list;
    std::unordered_map<U,T> dict;


    template <class UU>
    void _append(UU&&t){
//        if (count == 0)
//            token_list.push_back(std::unique_ptr<UU[]>(new UU[BLOCK_SIZE]));
//
//        auto it=token_list.size()-1;
//        token_list[it][count++]=t;
//        if (count==BLOCK_SIZE)
//            count=0;

    }
};
#endif //YCC_MEMORY_H
