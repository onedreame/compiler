//
// Created by yk on 18-3-10.
//

#include "../include/code_gen.h"

std::shared_ptr<CodeGen> CodeGen::Instance() {
    if (_codegen)
        return _codegen;
    _codegen.reset(new CodeGen());
    return _codegen;
}

std::shared_ptr<CodeGen> CodeGen::_codegen= nullptr;
/*
 * 离开作用域时栈信息的回调函数，清理函数名
 */
void CodeGen::pop_function(void *ignore){
    if (Utils::dumpstack&&!functions.empty())
        functions.pop_back();
}

/**
 * 设置输出文件，注意如果打开一个已存在的文件，文件内容首先会被清空
 * @param filename 要打开的输出文件名
 */
void CodeGen::set_output_file(const std::string& filename){
    if (outputfp.is_open())
        outputfp.close();
    outputfp.open(filename);
}
/**
 * 字符#替换为汇编中的%
 * @param str 要替换的字符串
 * @return 替换后的字符串
 */
std::string CodeGen::replace_char(const std::string&str){
    std::string buf;
    int i = 0;
    for (auto&e:str) {
        if (e == '#') {
            buf += '%';
            buf += '%';
        } else {
            buf += e;
        }
    }
    return buf;
}
/**
 * 解析函数调用链
 * @return 函数调用链，以 -> 分割
 */
std::string CodeGen::get_caller_list() {
    std::string b;
    for (int i = 0; i < functions.size(); i++) {
        if (i > 0)
            b+= " -> ";
        b += functions[i];
    }
    return b;
}

void CodeGen::save_to_file(const std::string& p, decltype(p.size()) cur){
    if (p.size()==cur)
        return;
    outputfp<<p.substr(cur);
}
/**
 * 根据ty.size选择ax，cx的一部分还是全部
 * @param ty
 * @param r register prefix
 * @return register name
 */
std::string get_int_reg(const std::shared_ptr<DataStruct::Type> &ty, char r) {
    if (r!='a'&&r!='c')
        Error::error("unknown register prefix");
    switch (ty->size) {
        case 1: return (r == 'a') ? "al" : "cl";
        case 2: return (r == 'a') ? "ax" : "cx";
        case 4: return (r == 'a') ? "eax" : "ecx";
        case 8: return (r == 'a') ? "rax" : "rcx";
        default:
            Error::error("Unknown data size: %s: %d", Utils::ty2s(ty), ty->size);
    }
}
/**
 * ycc是工作于64bit机器上的，所以会用到拓展，根据ty的size设置符号拓展到8字节，选择相关的mov指令
 * @param ty
 * @return movsbq movswq movslq mov
 */
std::string CodeGen::get_load_inst(const std::shared_ptr<DataStruct::Type> &ty) {
    switch (ty->size) {
        case 1: return "movsbq";
        case 2: return "movswq";
        case 4: return "movslq";
        case 8: return "mov";
        default:
            Error::error("Unknown data size: %s: %d", Utils::ty2s(ty), ty->size);
    }
}
int CodeGen::align(int n, int m) {
    int rem = n % m;
    return (rem == 0) ? n : n - rem + m;
}
/**
 * push %register_name
 * stackspos+=8
 * @param reg  register_name
 */
void CodeGen::push(const std::string& reg) {
    SAVE;
    emit("push #%s", reg);
    stackpos += 8;
}
/**
 * pop %register_name
 * stackpos-=8
 * @param reg register_name
 */
void CodeGen::pop(const std::string &reg) {
    SAVE;
    emit("pop #%s", reg);
    stackpos -= 8;
    if (stackpos<0)
        perror("stackpos<0");
}

void CodeGen::push_xmm(int reg) {
    SAVE;
    emit("sub $8, #rsp");
    emit("movsd #xmm%d, (#rsp)", reg);
    stackpos += 8;
}
void CodeGen::pop_xmm(int reg) {
    SAVE;
    emit("movsd (#rsp), #xmm%d", reg);
    emit("add $8, #rsp");
    stackpos -= 8;
    if (stackpos<0)
        perror("stackpos<0");
}
/**
 * struct对齐到8字节开始处
 * @param size struct size
 * @return
 */
int CodeGen::push_struct(int size) {
    SAVE;
    int aligned = align(size, 8);
    emit("sub $%d, #rsp", aligned);
    emit("mov #rcx, -8(#rsp)");
    emit("mov #r11, -16(#rsp)");
    emit("mov #rax, #rcx");
    int i = 0;
    for (; i < size; i += 8) {
        emit("movq %d(#rcx), #r11", i);
        emit("mov #r11, %d(#rsp)", i);
    }
    for (; i < size; i += 4) {
        emit("movl %d(#rcx), #r11", i);
        emit("movl #r11d, %d(#rsp)", i);
    }
    for (; i < size; i++) {
        emit("movb %d(#rcx), #r11", i);
        emit("movb #r11b, %d(#rsp)", i);
    }
    emit("mov -8(#rsp), #rcx");
    emit("mov -16(#rsp), #r11");
    stackpos += aligned;
    return aligned;
}
/**
 * 位域操作
 * @param ty
 */
void CodeGen::maybe_emit_bitshift_load(const std::shared_ptr<DataStruct::Type> &ty) {
    SAVE;
    if (ty->bitsize <= 0)
        return;
    emit("shr $%d, #rax", ty->bitoff);
    push("rcx");
    emit("mov $%u, #rcx", static_cast<unsigned long>((1 << static_cast<long >(ty->bitsize)) - 1));
    emit("and #rcx, #rax");
    pop("rcx");
}
void CodeGen::maybe_emit_bitshift_save(const std::shared_ptr<DataStruct::Type> &ty, const std::string &addr) {
    SAVE;
    if (ty->bitsize <= 0)
        return;
    push("rcx");
    push("rdi");
    emit("mov $%u, #rdi", static_cast<unsigned long>((1 << static_cast<long >(ty->bitsize) - 1)));
    emit("and #rdi, #rax");
    emit("shl $%d, #rax", ty->bitoff);
    emit("mov %s, #%s", addr, get_int_reg(ty, 'c'));
    emit("mov $%u, #rdi", ~static_cast<unsigned  long>((((1 << static_cast<long >(ty->bitsize) - 1) << ty->bitoff))));
    emit("and #rdi, #rcx");
    emit("or #rcx, #rax");
    pop("rdi");
    pop("rcx");
}
void CodeGen::emit_gload(const std::shared_ptr<DataStruct::Type> &ty, const std::string &label, int off) {
    SAVE;
    if (ty->kind == DataStruct::TYPE_KIND::KIND_ARRAY) {
        if (off)
            emit("lea %s+%d(#rip), #rax", label, off);
        else
            emit("lea %s(#rip), #rax", label);
        return;
    }
    auto inst = get_load_inst(ty);
    emit("%s %s+%d(#rip), #rax", inst, label, off);
    maybe_emit_bitshift_load(ty);
}
void Codeemit_intcast(const std::shared_ptr<DataStruct::Type> &ty) {
    switch(ty->kind) {
        case DataStruct::TYPE_KIND::KIND_BOOL:
        case DataStruct::TYPE_KIND::KIND_CHAR:
            ty->usig ? emit("movzbq #al, #rax") : emit("movsbq #al, #rax");
            return;
        case DataStruct::TYPE_KIND::KIND_SHORT:
            ty->usig ? emit("movzwq #ax, #rax") : emit("movswq #ax, #rax");
            return;
        case KIND_INT:
            ty->usig ? emit("mov #eax, #eax") : emit("cltq");
            return;
        case KIND_LONG:
        case KIND_LLONG:
            return;
    }
}
void CodeGen::emit_func_prologue(std::shared_ptr<DataStruct::Node> func) {
    SAVE;
    emit(".text");
    if (!func->getTy()->isstatic)
        emit_noindent(".global %s", func->fname);
    emit_noindent("%s:", func->fname);
    emit("nop");
    push("rbp");
    emit("mov #rsp, #rbp");
    int off = 0;
    if (func->getTy()->hasva) {
        set_reg_nums(func->params);
        off -= emit_regsave_area();
    }
    push_func_params(func->params, off);
    off -= vec_len(func->params) * 8;

    int localarea = 0;
    for (int i = 0; i < vec_len(func->localvars); i++) {
        Node *v = vec_get(func->localvars, i);
        int size = align(v->ty->size, 8);
        if(size%8)
            Error::error("%s is not aligned to base 8",Utils::node2s(v));
        off -= size;
        v->loff = off;
        localarea += size;
    }
    if (localarea) {
        emit("sub $%d, #rsp", localarea);
        stackpos += localarea;
    }
}

void CodeGen::emit_toplevel(std::shared_ptr<DataStruct::Node> v) {
    stackpos = 8;
    if (v->getKind() == DataStruct::AST_TYPE::AST_FUNC) {
        emit_func_prologue(v);
        emit_expr(v->body);
        emit_ret();
    } else if (v->getKind() == DataStruct::AST_TYPE::AST_DECL) {
        emit_global_var(v);
    } else {
        Error::error("internal error");
    }
}