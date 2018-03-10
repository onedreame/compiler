#include <iostream>
#include "include/Token.h"
#include "include/error.h"
#include "include/buffer.h"
#include "include/path.h"
#include "include/Memory.h"
#include "include/Lex.h"
#include "test/funcTest.h"

using namespace Error;
void ifstreamTest(std::string filename);
//void FileTest();
void sharedIoTest();
void warnpTest();

int main() {
//    DataStruct::File fi;
//    DataStruct::Token t;
//    std::cout << "Hello, World!" << std::endl;
//    Print_error("hello,%d,%f,%s\n",1,2.3,"yes");
//    std::string s1="3行";
//    std::string s2="2列";
//    std::cout<<Path::clean("/hello/..//dkfk/")<<std::endl;
//    warnf(__FILE__ ":",__LINE__,"%s,%1,%c\n",1,"yes",'c');
//    warn("%s,%1,%c\n",1,"yes",'c');
////    std::to_string(1);
//    std::cout<<__LINE__;
////    std::cout<<s1<<std::endl;
////    std::cout<<s2<<std::endl;
////    errorf(s1,s2,"%d,%s,%c\n",1,"yes",'c');
////    Buffer b;
////    std::cout<<b.format("%d,%s,%c\n",1,"yes",'c');
////    std::cout<<b.parse_cstring("hello\r\t,world\s\e\n\\\'");
////    Utils::usage(1);
//    memory<int> h;
//    std::cout<<"memery test"<<std::endl;
//    for(int i=0;i<10;++i)
//        h.append(i);
//    h.iter();
//    Lex lex("/home/yk/compiler/ycc/include/buffer.h");
//    Lex::Pos pos{.line=2,.column=3};
//    std::cout<<lex.pos_string(pos);
//    ifstreamTest("/tmp/test");
////    FileTest();
//    sharedIoTest();
////    Error::warnp(std::string("hello"),"%s,%1,%c\n",1,"yes",'c');
//    Test::lexTokenTest("../test/");
//    Test::newLineTest();
    Test::macroExpandTest("../test/",0);
//    Test::NodeTest();
    return 0;
}

void ifstreamTest(std::string filename)
{
    std::ifstream ifs(filename);
    if (!ifs)
    {
        std::cerr<<"无法打开文件"<<std::endl;
        return;
    }
    auto p=ifs.tellg();
    std::cout<<"初始pos："<<p<<std::endl;
    char c;
    ifs>>c;
    std::cout<<"初始字符："<<c<<std::endl;
    std::cout<<"读取一个字符以后的文件指针："<<ifs.tellg()<<std::endl;
    ifs.seekg(-1,std::ios::cur);
    std::cout<<"设置-1以后的文件指针："<<ifs.tellg()<<std::endl;
    ifs>>c;
    std::cout<<"此时的字符为:"<<c<<std::endl;
    int p1=1;
    int c1;
    while(!ifs.eof()||p1<3)
    {
        ifs>>c;
        if (ifs.eof()) ++p1;
        std::cout<<c<<std::endl;
    }
    ifs.close();
    ifs.open("/tmp/test");
    std::cout<<std::endl<<"测试是否有多余字符输出"<<std::endl;
    while(!ifs.eof()){
        ifs>>c;
        std::cout<<c<<" ";
    }
    std::cout<<std::endl;
}
//void FileTest()
//{
//    std::string s1="/tmp/test";
//    std::string s2="/tmp/test1";
//    std::string s3="/tmp/test2";
//    Lex lex;
//    lex.open_file(s1);
//    lex.stash_push();
//    lex.debug();
//    lex.open_file(s2);
//    lex.stash_push();
//    lex.debug();
//    lex.stash_pop();
//    lex.debug();
//}

void sharedIoTest()
{
    std::shared_ptr<std::ifstream> p=std::make_shared<std::ifstream>("/tmp/test");
    std::shared_ptr<std::ifstream> q=p;
    std::cout<<"p的文件位置为"<<p->tellg()<<std::endl;
    std::cout<<"复制品q的文件位置为"<<q->tellg()<<std::endl;
    char c;
    *p>>c;
    std::cout<<"输出一个字符以后"<<std::endl;
    std::cout<<"p的文件位置为"<<p->tellg()<<std::endl;
    std::cout<<"复制品q的文件位置为"<<q->tellg()<<std::endl;
}