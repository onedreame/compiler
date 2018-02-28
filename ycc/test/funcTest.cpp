//
// Created by yk on 18-2-18.
//

#include "funcTest.h"
#include "../include/Lex.h"
#include <string>
#include <unistd.h>
#include <dirent.h>
#include <cstring>
#include <fstream>

namespace Test{
    void lexTokenTest(std::string path){
        DIR *dir;
        struct dirent *ptr;

        char base[1000];

        if ((dir=opendir(path.c_str())) == NULL)
        {
            perror("Open dir error...");
            exit(1);
        }

        int id=0;
        bool isflush= false;
        while ((ptr=readdir(dir)) != NULL)
        {
            if(strcmp(ptr->d_name,".")==0 || strcmp(ptr->d_name,"..")==0)
//                    ||ptr->d_name[strlen(ptr->d_name)-1]!='c'||ptr->d_name[strlen(ptr->d_name)-1]!='h')    ///current dir OR parrent dir
                continue;
            if(ptr->d_type == 8)    ///file
            {
                //printf("d_name:%s/%s\n",basePath,ptr->d_name);
//                files.push_back(ptr->d_name);
                std::cout<<"#################第"<<++id<<"个文件："<<ptr->d_name<<std::endl;
                Lex lex(path+std::string(ptr->d_name));
                DataStruct::Token tok=lex.lex();
                int count=0;
                while (tok.kind!=DataStruct::TOKEN_TYPE::TEOF)
                {
                    std::cout<<count++<<":";
                    switch (tok.kind){
                        case DataStruct::TOKEN_TYPE::TSPACE:
                            std::cout<<"空格";
                            if(isflush)
                            std::cout<<std::endl;
                            break;
                        case DataStruct::TOKEN_TYPE::TNEWLINE:
                            std::cout<<"换行";
                            if (isflush)
                            std::cout<<std::endl;
                            break;
                        case DataStruct::TOKEN_TYPE::TKEYWORD:
                            std::cout<<KRED<< static_cast<char >(tok.id)<<RST;
                            if (isflush)
                            std::cout<<std::endl;
                            break;
                        case DataStruct::TOKEN_TYPE::TIDENT:
                            std::cout<<KGRN<<*(tok.sval)<<RST;
                            if (isflush)
                            std::cout<<std::endl;
                            break;
                        case DataStruct::TOKEN_TYPE::TNUMBER:
                        case DataStruct::TOKEN_TYPE::TSTRING:
                            std::cout<<*(tok.sval);
                            if (isflush)
                            std::cout<<std::endl;
                            break;
                        default:
                            std::cout<< static_cast<char >(tok.c);
                            if (isflush)
                            std::cout<<std::endl;
                            break;
                    }

                    tok=lex.lex();
                }
                std::cout<<std::endl;
            }
//            else if(ptr->d_type == 10)    ///link file
//                //printf("d_name:%s/%s\n",basePath,ptr->d_name);
//                continue;
//            else if(ptr->d_type == 4)    ///dir
//            {
//                files.push_back(ptr->d_name);
//                /*
//                    memset(base,'\0',sizeof(base));
//                    strcpy(base,basePath);
//                    strcat(base,"/");
//                    strcat(base,ptr->d_nSame);
//                    readFileList(base);
//                */
//            }
        }
        closedir(dir);
    }
    void newLineTest()
    {
        std::ifstream fs("/tmp/hello");
        if (!fs){
            perror("打开失败");
            exit(1);
        }
        char c=fs.get();
        fs.seekg(-1,std::ios::cur);
        while(!fs.eof())
        {
            c=fs.get();
            std::cout<<c;
            if (c=='\n')
                std::cout<<": 这是个换行符";
            if (isspace(c))
                std::cout<<":这是个空格符号";
            std::cout<<std::endl;
        }
    }
    void macroExpandTest(std::string& path)
    {
        DIR *dir;
        struct dirent *ptr;

        char base[1000];

        if ((dir=opendir(path.c_str())) == NULL)
        {
            perror("Open dir error...");
            exit(1);
        }

        int id=0;
        bool isflush= false;
        std::ofstream ofs("../testFilename.txt");
        while ((ptr=readdir(dir)) != NULL)
        {
            if(strcmp(ptr->d_name,".")==0 || strcmp(ptr->d_name,"..")==0)
//                    ||ptr->d_name[strlen(ptr->d_name)-1]!='c'||ptr->d_name[strlen(ptr->d_name)-1]!='h')    ///current dir OR parrent dir
                continue;
            if(ptr->d_type == 8)    ///file
            {
                ofs<<ptr->d_name<<std::endl;
//                std::cout<<"#################第"<<++id<<"个文件："<<ptr->d_name<<std::endl;
//                Lex lex(path+std::string(ptr->d_name));
//                DataStruct::Token tok=lex.lex();
//                int count=0;
//                while (tok.kind!=DataStruct::TOKEN_TYPE::TEOF)
//                {
//                    std::cout<<count++<<":";
//                    switch (tok.kind){
//                        case DataStruct::TOKEN_TYPE::TSPACE:
//                            std::cout<<"空格";
//                            if(isflush)
//                                std::cout<<std::endl;
//                            break;
//                        case DataStruct::TOKEN_TYPE::TNEWLINE:
//                            std::cout<<"换行";
//                            if (isflush)
//                                std::cout<<std::endl;
//                            break;
//                        case DataStruct::TOKEN_TYPE::TKEYWORD:
//                            std::cout<<KRED<< static_cast<char >(tok.id)<<RST;
//                            if (isflush)
//                                std::cout<<std::endl;
//                            break;
//                        case DataStruct::TOKEN_TYPE::TIDENT:
//                            std::cout<<KGRN<<*(tok.sval)<<RST;
//                            if (isflush)
//                                std::cout<<std::endl;
//                            break;
//                        case DataStruct::TOKEN_TYPE::TNUMBER:
//                        case DataStruct::TOKEN_TYPE::TSTRING:
//                            std::cout<<*(tok.sval);
//                            if (isflush)
//                                std::cout<<std::endl;
//                            break;
//                        default:
//                            std::cout<< static_cast<char >(tok.c);
//                            if (isflush)
//                                std::cout<<std::endl;
//                            break;
//                    }
//
//                    tok=lex.lex();
//                }
//                std::cout<<std::endl;
            }
        }
        closedir(dir);
    }
}