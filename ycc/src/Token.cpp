//
// Created by yk on 17-12-31.
//

#include "../include/Token.h"
#include "../include/error.h"


//DataStruct::Node& DataStruct::Node::copyUnion(const DataStruct::Node& r){
//    if (r.tok!=tok){
//        clear();
//        tok=r.tok;
//    }
//    switch (r.tok){
//        case NODETYPE::CIL:
//            ival=r.ival;
//            return *this;
//        case NODETYPE::FD:
//            fd=r.fd;
//            return *this;
//        case NODETYPE::STR:   //string
//            str=r.str;
//            return *this;
//        case NODETYPE::LGV:   //local/global 变量
//            lgv=r.lgv;
//            return *this;
//        case NODETYPE::BIOP:  //binary op
//            biop=r.biop;
//            return *this;
//        case NODETYPE::UNOP:  //unary op
//            unop=r.unop;
//            return *this;
//        case NODETYPE::FCFD:  //函数调用或声明
//            fcfd=r.fcfd;
//            return *this;
//        case NODETYPE::DEC:   //声明
//            dec=r.dec;
//            return *this;
//        case NODETYPE::INIT:  //初始化
//            init=r.init;
//            return *this;
//        case NODETYPE::IFTOP: //if语句或ternary op
//            iftop=r.iftop;
//            return *this;
//        case NODETYPE::GOLA:  //goto label
//            gola=r.gola;
//            return *this;
//        case NODETYPE::RET:   //return 语句
//            retval=r.retval;
//            return *this;
//        case NODETYPE::COMPO: //compound 语句
//            stmts=r.stmts;
//            return *this;
//        case NODETYPE::STRREF://struct引用
//            strref=r.strref;
//            return *this;
//        default:
//        Error::error("unknown type");
//    }
//}
//DataStruct::BASE_Node::~BASE_Node(){}
//void DataStruct::Node::clear() {
//    switch (tok){
//        case NODETYPE::CIL:   // char int or long
//            break;
//        case NODETYPE::FD:   // return 语句
////            fd.~shared_ptr();
//            fd.reset();
//            break;
//        case NODETYPE::STR:   //string
////            str.~shared_ptr();
//            str.reset();
//            break;
//        case NODETYPE::LGV:   //local/global 变量
////            lgv.~shared_ptr();
//            lgv.reset();
//            break;
//        case NODETYPE::BIOP:  //binary op
////            biop.~shared_ptr();
//            biop.reset();
//            break;
//        case NODETYPE::UNOP:  //unary op
////            unop.~shared_ptr();
//            unop.reset();
//            break;
//        case NODETYPE::FCFD:  //函数调用或声明
////            fcfd.~shared_ptr();
//            fcfd.reset();
//            break;
//        case NODETYPE::DEC:   //声明
////            dec.~shared_ptr();
//            dec.reset();
//            break;
//        case NODETYPE::INIT:  //初始化
////            init.~shared_ptr();
//            init.reset();
//            break;
//        case NODETYPE::IFTOP: //if语句或ternary op
////            iftop.~shared_ptr();
//            iftop.reset();
//            break;
//        case NODETYPE::GOLA:  //goto label
////            gola.~shared_ptr();
//            gola.reset();
//            break;
//        case NODETYPE::RET:   //return 语句
////            retval.~shared_ptr();
//            retval.reset();
//            break;
//        case NODETYPE::COMPO: //compound 语句
////            stmts.~vector();
//            stmts.clear();
//            break;
//        case NODETYPE::STRREF: //struct引用
////            strref.~shared_ptr();
//            strref.reset();
//            break;
//        default:
//            Error::error("unknown type");
//    }
//}

//DataStruct::Node& DataStruct::Node::operator=(const DataStruct::Node& t){
//    kind=t.getKind();
//    ty=t.getTy();
//    sourceloc=t.getSourceloc();
//    return copyUnion(t);
//}
long DataStruct::Node::getIval() const {
    return ival;
}

void DataStruct::Node::setIval(long ival) {
    Node::ival = ival;
}

DataStruct::AST_TYPE DataStruct::Node::getKind() const {
    return kind;
}

void DataStruct::Node::setKind(DataStruct::AST_TYPE kind) {
    Node::kind = kind;
}

const std::shared_ptr<DataStruct::Type> &DataStruct::Node::getTy() const {
    return ty;
}

void DataStruct::Node::setTy(const std::shared_ptr<DataStruct::Type> &ty) {
    Node::ty = ty;
}

const DataStruct::SourceLoc &DataStruct::Node::getSourceloc() const {
    return sourceloc;
}

void DataStruct::Node::setSourceloc(const DataStruct::SourceLoc &sourceloc) {
    Node::sourceloc = sourceloc;
}
//DataStruct::Node::Node(const Node&r){
//    std::cout<<"Node copy"<<std::endl;
//    //uanry操作
//    this->unop=unop;
//    // compound 语句
//    this->stmts=stmts;
//    // float or double
//    this->fval=fval;
//    this->flabel=flabel;
//    // char int or long
//    this->ival=ival;
//    // return 语句
//    this->retval=retval;
//    // string
//    this->sval=sval;
//    this->slabel=slabel;
//    // local/global 变量
//    this->varname=varname;
//    // local
//    this->loff=loff;
//    lvarinit;
//    // global
//    this->glabel=glabel;
//    // binary op
//    this->left=left;
//    this->right=right;
//    // 函数调用或声明
//    this->fname=fname;
//    // Function call
//    this->args=args;
//    this->ftype=ftype;
//    // Function pointer or function designator
//    this->fptr=fptr;
//    // Function declaration
//    this->params=params;
//    this->localvars=localvars;
//    this->body=body;
//    // 声明
//    this->declvar=declvar;
//    this->declinit=declinit;
//    // 初始化
//    this->initval=initval;
//    this->initoff=initoff;
//    this->totype=totype;
//    // if语句或ternary op
//    this->cond=cond;
//    this->then=then;
//    this->els=els;
//    // goto label
//    this->label=label;
//    this->newlabel=newlabel;
//    // struct引用
//    this->struc=struc;
//    this->field=field;
//    this->fieldtype=fieldtype;
//}
static int count=0;
template <class T,class U>
void format_print(U t){
    std::cout<<"  ";
    if (typeid(t)== typeid(std::shared_ptr<T>))
        std::cout<<t.use_count();
    else std::cout<<t;
    std::cout<<std::endl;
}
void DataStruct::f(Node*thi) {
    std::cout<<__LINE__<< " ";
    std::cout<<"~node__________________________________"<<++count<<"\n";
    std::cout<<STR(thi->unop);
    format_print<Node>(thi->unop);
    std::cout<<STR(thi->stmts);
    for(auto&e:thi->stmts)
        format_print<Node>(e);
    if (thi->stmts.empty()) std::cout<<std::endl;
    std::cout<<STR(thi->retval);
    format_print<Node>(thi->retval);
//    std::cout<<STR(thi->sval);
//    format_print<std::string>(thi->sval);
//    std::cout<<STR(thi->slabel);
//    format_print<std::string>(thi->slabel);
//    std::cout<<STR(thi->varname);
//    format_print<std::string>(thi->varname);
    std::cout<<"  lvarinit.size="<<thi->lvarinit.size()<<std::endl;
    std::cout<<STR(thi->left);
    format_print<Node>(thi->left);
    std::cout<<STR(thi->right);
    format_print<Node>(thi->right);
    std::cout<<STR(thi->ftype);
    format_print<Node>(thi->ftype);
    std::cout<<STR(thi->fptr);
    format_print<Node>(thi->fptr);
    std::cout<<STR(thi->body);
    format_print<Node>(thi->body);
    std::cout<<STR(thi->declvar);
    format_print<Node>(thi->declvar);
    std::cout<<STR(thi->declinit);
    for (auto& e:thi->declinit)
        format_print<Node>(e);
    if (thi->declinit.empty()) std::cout<<std::endl;
    std::cout<<STR(thi->initval);
    format_print<Node>(thi->initval);
    std::cout<<STR(thi->totype);
    format_print<Type>(thi->totype);
    std::cout<<STR(thi->cond);
    format_print<Node>(thi->cond);
    std::cout<<STR(thi->then);
    format_print<Node>(thi->then);
    std::cout<<STR(thi->els);
    format_print<Node>(thi->els);
    std::cout<<STR(thi->struc);
    format_print<Node>(thi->struc);
    std::cout<<STR(thi->fieldtype);
    format_print<Node>(thi->fieldtype);
}

//DataStruct::Node DataStruct::Node::make_CIL_node(long a)
//{
//    Node node;
//    node.tok=CIL;
//    node.ival=a;
//    return node;
//}
//DataStruct::Node DataStruct::Node::make_FD_node(double a, const std::string& b)
//{
//    Node node;
//    node.tok=FD;
//    node.fd=std::make_shared<DataStruct::FD_Node >();
//    node.fd->fval=a;
//    node.fd->flabel=b;
//    return node;
//}
//DataStruct::Node DataStruct::Node::make_STR_node(const std::string&a, const std::string&b)
//{
//    Node node;
//    node.str=std::make_shared<STR_Node>();
//    node.str->sval=a;
//    node.str->slabel=b;
//    node.tok=STR;
//    return node;
//}
//DataStruct::Node DataStruct::Node::make_LGV_node(const std::string&a,int b, const std::vector<Token>&c, const std::string&d)
//{
//    Node node;
//    node.tok=LGV;
//    node.lgv=std::make_shared<LGV_Node>();
//    node.lgv->varname=a;
//    node.lgv->loff=b;
//    node.lgv->lvarinit=c;
//    node.lgv->glabel=d;
//    return node;
//}
//DataStruct::Node DataStruct::Node::make_BIOP_node(const std::shared_ptr<Node>&left, const std::shared_ptr<Node>&right )
//{
//    Node node;
//    node.tok=BIOP;
//    node.biop=std::make_shared<BIOP_Node>();
//    node.biop->left=left;
//    node.biop->right=right;
//    return node;
//}
//DataStruct::Node DataStruct::Node::make_RET_node(const std::shared_ptr<Node>&ret)
//{
//    Node node;
//    node.tok=RET;
//    node.retval=ret;
//    return node;
//}
//DataStruct::Node DataStruct::Node::make_FCFD_node(const std::string&fname, const std::vector<Token>&args,const std::shared_ptr<Type>&ftype, std::shared_ptr<Node>&fptr,const std::vector<Token>&params,
//                    const std::vector<Token>&localvars, const std::shared_ptr<Node>&body)
//{
//    Node node;
//    node.tok=FCFD;
//    node.fcfd=std::make_shared<FCFD_Node>();
//    node.fcfd->fname=fname;
//    node.fcfd->args=args;
//    node.fcfd->ftype=ftype;
//    node.fcfd->fptr=fptr;
//    node.fcfd->params=params;
//    node.fcfd->localvars=localvars;
//    node.fcfd->body=body;
//    return node;
//
//}
//DataStruct::Node DataStruct::Node::make_DEC_node(const std::shared_ptr<Node>&declvar, const std::vector<Token>&declinit)
//{
//    Node node;
//    node.tok=DEC;
//    node.dec=std::make_shared<DEC_Node>();
//    node.dec->declvar=declvar;
//    node.dec->declinit=declinit;
//    return node;
//}
//DataStruct::Node DataStruct::Node::make_INIT_node(const std::shared_ptr<Node>&initval,int initoff, const std::shared_ptr<Type>&totype)
//{
//    Node node;
//    node.tok=INIT;
//    node.init=std::make_shared<INIT_Node>();
//    node.init->initval=initval;
//    node.init->initoff=initoff;
//    node.init->totype=totype;
//    return node;
//}
//DataStruct::Node DataStruct::Node::make_IFTOP_node(const std::shared_ptr<Node>&cond, const std::shared_ptr<Node>&then, const std::shared_ptr<Node>&els)
//{
//    Node node;
//    node.tok=IFTOP;
//    node.iftop=std::make_shared<IFTOP_Node>();
//    node.iftop->cond=cond;
//    node.iftop->then=then;
//    node.iftop->els=els;
//    return node;
//}
//DataStruct::Node DataStruct::Node::make_STRREF_node(const std::shared_ptr<Node>&struc, const std::string&field, const std::shared_ptr<Type>&fieldtype)
//{
//    Node node;
//    node.tok=STRREF;
//    node.strref->struc=struc;
//    node.strref->field=field;
//    node.strref->fieldtype=fieldtype;
//    return node;
//}
//DataStruct::Node DataStruct::Node::make_COMPO_node(const std::vector<Token>&stmts)
//{
//    Node node;
//    node.tok=COMPO;
//    node.stmts=stmts;
//    return node;
//}
//DataStruct::Node DataStruct::Node::make_GOLA_node(const std::string&label, const std::string&newlabel)
//{
//    Node node;
//    node.tok=GOLA;
//    node.gola=std::make_shared<GOLA_Node>();
//    node.gola->label=label;
//    node.gola->newlabel=newlabel;
//    return node;
//}
//DataStruct::Node DataStruct::Node::make_UNOP_node(const std::shared_ptr<Node>&a)
//{
//    Node node;
//    node.tok=UNOP;
//    node.unop=a;
//    return node;
//}