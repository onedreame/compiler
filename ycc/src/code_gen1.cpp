//
// Created by yk on 18-3-10.
//

#include <sys/stat.h>
#include "../include/code_gen.h"

std::shared_ptr<CodeGen> CodeGen::Instance() {
    if (_codegen)
        return _codegen;
    _codegen.reset(new CodeGen());
    return _codegen;
}

std::shared_ptr<CodeGen> CodeGen::_codegen= nullptr;

/**
 * 设置输出文件，注意如果打开一个已存在的文件，文件内容首先会被清空
 * @param filename 要打开的输出文件名
 */
void CodeGen::set_output_file(const std::ostream& stream){
    if (!outputfp){
        outputfp=std::make_shared<std::ostream>(stream.rdbuf());
        return;
    }
    outputfp->rdbuf(stream.rdbuf());
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
std::vector<std::string> CodeGen::functions;
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
    while (cur<p.size()){
        if (p[cur]=='%'&&cur+1<p.size()&&p[cur+1]=='%'){
            *outputfp<<"%";
            cur+=2;
        }
        else{
            *outputfp<<p[cur++];
        }
    }
    *outputfp<<std::flush;
}
/**
 * 根据ty.size选择ax，cx的一部分还是全部
 * @param ty
 * @param r register prefix
 * @return register name
 */
std::string CodeGen::get_int_reg(const std::shared_ptr<DataStruct::Type> &ty, char r) {
    if (r!='a'&&r!='c')
        Error::error("unknown register prefix");
    switch (ty->size) {
        case 1: return (r == 'a') ? "al" : "cl";
        case 2: return (r == 'a') ? "ax" : "cx";
        case 4: return (r == 'a') ? "eax" : "ecx";
        case 8: return (r == 'a') ? "rax" : "rcx";
        default:
            Error::error("Unknown data size: %s: %d", _parser->ty2s(ty), ty->size);
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
            Error::error("Unknown data size: %s: %d", _parser->ty2s(ty), ty->size);
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
 * @return struct对齐后的尺寸
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
 * 位域操作，提取位域范围内的位
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
    //构造mask
    emit("mov $%u, #rdi", static_cast<unsigned long>((1 << static_cast<long >(ty->bitsize) - 1)));
    //&，这里求的位域的结果
    emit("and #rdi, #rax");
    //复位，位域的信息已经处理好
    emit("shl $%d, #rax", ty->bitoff);
    //从地址处取值
    emit("mov %s, #%s", addr, get_int_reg(ty, 'c'));
    //构建相应位的mask
    emit("mov $%u, #rdi", ~static_cast<unsigned  long>((((1 << static_cast<long >(ty->bitsize) - 1) << ty->bitoff))));
    //相应位域的bit设置为0
    emit("and #rdi, #rcx");
    //设置剩余位
    emit("or #rcx, #rax");
    pop("rdi");
    pop("rcx");
}
/**
 * 如果是数组，则加载地址，其他情况则是取数
 * @param ty
 * @param label
 * @param off
 */
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
/**
 * int broadcast,根据usigned与否拓展至8btyes
 * @param ty
 */
void CodeGen::emit_intcast(const std::shared_ptr<DataStruct::Type> &ty) {
    switch(ty->kind) {
        case DataStruct::TYPE_KIND::KIND_BOOL:
        case DataStruct::TYPE_KIND::KIND_CHAR:
            ty->usig ? emit("movzbq #al, #rax") : emit("movsbq #al, #rax");
            return;
        case DataStruct::TYPE_KIND::KIND_SHORT:
            ty->usig ? emit("movzwq #ax, #rax") : emit("movswq #ax, #rax");
            return;
        case DataStruct::TYPE_KIND::KIND_INT:
            ty->usig ? emit("mov #eax, #eax") : emit("cltq"); //有符号拓展
            return;
        case DataStruct::TYPE_KIND::KIND_LONG:
        case DataStruct::TYPE_KIND::KIND_LLONG:
            return;
    }
}
/**
 * 浮点数转换为整数
 * @param ty
 */
void CodeGen::emit_toint(const std::shared_ptr<DataStruct::Type> &ty) {
    SAVE;
    if (ty->kind == DataStruct::TYPE_KIND::KIND_FLOAT)
        emit("cvttss2si #xmm0, #eax");
    else if (ty->kind == DataStruct::TYPE_KIND::KIND_DOUBLE)
        emit("cvttsd2si #xmm0, #eax");
}
/**
 * load data from memory to register
 * type:
 * (1) array: lea off(%base), %rax
 * (2) float: movss off(%base) %xmm0
 * (3) double,long double: movsd off(%base),%xmm0
 * (4) int, movzbl.. off(%base), %rax
 * 还可能进行bit早错
 * @param ty
 * @param base memory label
 * @param off
 */
void CodeGen::emit_lload(const std::shared_ptr<DataStruct::Type> &ty, const std::string &base, int off) {
    SAVE;
    if (ty->kind == DataStruct::TYPE_KIND::KIND_ARRAY) {
        emit("lea %d(#%s), #rax", off, base);
    } else if (ty->kind == DataStruct::TYPE_KIND::KIND_FLOAT) {
        emit("movss %d(#%s), #xmm0", off, base);
    } else if (ty->kind == DataStruct::TYPE_KIND::KIND_DOUBLE || ty->kind == DataStruct::TYPE_KIND::KIND_LDOUBLE) {
        emit("movsd %d(#%s), #xmm0", off, base);
    } else {
        auto inst = get_load_inst(ty);
        emit("%s %d(#%s), #rax", inst, off, base);
        maybe_emit_bitshift_load(ty);
    }
}
/**
 * 根据rax设置条件码
 * @param ty
 */
void CodeGen::maybe_convert_bool(const std::shared_ptr<DataStruct::Type> &ty) {
    if (ty->kind == DataStruct::TYPE_KIND::KIND_BOOL) {
        emit("test #rax, #rax");
        emit("setne #al");
    }
}

void CodeGen::emit_gsave(const std::string &varname, const std::shared_ptr<DataStruct::Type> &ty, int off) {
    SAVE;
    if (ty->kind == DataStruct::TYPE_KIND::KIND_ARRAY)
        Error::error("type is array");
    maybe_convert_bool(ty);
    auto reg = get_int_reg(ty, 'a');
    auto addr = varname+"+"+std::to_string(off)+"(%%rip)";
    maybe_emit_bitshift_save(ty, addr);
    emit("mov #%s, %s", reg, addr);
}
void CodeGen::emit_lsave(const std::shared_ptr<DataStruct::Type> &ty, int off) {
    SAVE;
    if (ty->kind == DataStruct::TYPE_KIND::KIND_FLOAT) {
        emit("movss #xmm0, %d(#rbp)", off);
    } else if (ty->kind == DataStruct::TYPE_KIND::KIND_DOUBLE) {
        emit("movsd #xmm0, %d(#rbp)", off);
    } else {
        maybe_convert_bool(ty);
        auto reg = get_int_reg(ty, 'a');
        auto addr = std::to_string(off)+"(%%rbp)";
        maybe_emit_bitshift_save(ty, addr);
        emit("mov #%s, %s", reg, addr);
    }
}
void CodeGen::do_emit_assign_deref(const std::shared_ptr<DataStruct::Type> &ty, int off) {
    SAVE;
    emit("mov (#rsp), #rcx");
    auto reg = get_int_reg(ty, 'c');
    if (off)
        emit("mov #%s, %d(#rax)", reg, off);
    else
        emit("mov #%s, (#rax)", reg);
    pop("rax");
}
void CodeGen::emit_assign_deref(const std::shared_ptr<DataStruct::Node> &var) {
    SAVE;
    push("rax");
    emit_expr(var->unop);
    do_emit_assign_deref(var->unop->getTy()->ptr, 0);
}
/**
 * 指针运算
 * emit_expr(left);
 * push("rcx");
 * push("rax");
 * emit_expr(right);
 * @param kind
 * @param left
 * @param right
 */
void CodeGen::emit_pointer_arith(DataStruct::AST_TYPE kind, const std::shared_ptr<DataStruct::Node> &left, const std::shared_ptr<DataStruct::Node> &right) {
    SAVE;
    emit_expr(left);
    push("rcx");
    push("rax");
    emit_expr(right);
    int size = left->getTy()->ptr->size;
    if (size > 1)
        emit("imul $%d, #rax", size);
    emit("mov #rax, #rcx");
    pop("rax");
    switch (kind) {
        case DataStruct::AST_TYPE::PLUS: emit("add #rcx, #rax"); break;
        case DataStruct::AST_TYPE::SUB: emit("sub #rcx, #rax"); break;
        default: Error::error("invalid operator '%d'", static_cast<int>(kind));
    }
    pop("rcx");
}
/**
 * 先以4字节填充，不足4字节的部分再以单字节填充
 * @param start 开始地址
 * @param end 结束地址
 */
void CodeGen::emit_zero_filler(int start, int end) {
    SAVE;
    for (; start <= end - 4; start += 4)
        emit("movl $0, %d(#rbp)", start);
    for (; start < end; start++)
        emit("movb $0, %d(#rbp)", start);
}
/**
 *
 * @param node
 */
void CodeGen::ensure_lvar_init(const std::shared_ptr<DataStruct::Node> &node) {
    SAVE;
    if (node->getKind() != DataStruct::AST_TYPE::AST_LVAR)
        Error::error("node kind is not lvar");
    if (!node->lvarinit.empty())
        emit_decl_init(node->lvarinit, node->loff, node->getTy()->size);
    node->lvarinit.clear();
}
void CodeGen::emit_assign_struct_ref(const std::shared_ptr<DataStruct::Node> &struc,
                            const std::shared_ptr<DataStruct::Type> &field, int off) {
    SAVE;
    switch (struc->getKind()) {
        case DataStruct::AST_TYPE::AST_LVAR:
            ensure_lvar_init(struc);
            emit_lsave(field, struc->loff + field->offset + off);
            break;
        case DataStruct::AST_TYPE::AST_GVAR:
            emit_gsave(struc->glabel, field, field->offset + off);
            break;
        case DataStruct::AST_TYPE::AST_STRUCT_REF:
            emit_assign_struct_ref(struc->struc, field, off + struc->getTy()->offset);
            break;
        case DataStruct::AST_TYPE::AST_DEREF:
            push("rax");
            emit_expr(struc->unop);
            do_emit_assign_deref(field, field->offset + off);
            break;
        default:
            Error::error("internal error: %s", _parser->node2s(struc));
    }
}
/**
 * 加载struct引用，这里要分成如下几种情况，根据struc的kind
 * （1）AST_LVAR
 * ensure_lvar_init(struc);
 * movzsl... struc->loff + field->offset + off(%rbp) %rax
 * 这种情况定义的是lvalue，所以直接从内存中取值
 * （2）AST_GVAR
 * gvalue则是从label+fiels->offset(%rip)取值
 * （3）AST_STRUCT_REF
 * 这种情况说明当前是嵌套的struct
 * emit_load_struct_ref(struc->struc, field, struc->getTy()->offset + off);
 * （4）AST_DEREF  ->
 * 需要先分析出struc->unop,也就是该struct的基址，然后从基址加偏移处取出该值
 * @param struc
 * @param field
 * @param off
 */
void CodeGen::emit_load_struct_ref(const std::shared_ptr<DataStruct::Node> &struc,
                          const std::shared_ptr<DataStruct::Type> &field, int off) {
    SAVE;
    switch (struc->getKind()) {
        case DataStruct::AST_TYPE::AST_LVAR:
            ensure_lvar_init(struc);
            emit_lload(field, "rbp", struc->loff + field->offset + off);
            break;
        case DataStruct::AST_TYPE::AST_GVAR:
            emit_gload(field, struc->glabel, field->offset + off);
            break;
        case DataStruct::AST_TYPE::AST_STRUCT_REF:
            emit_load_struct_ref(struc->struc, field, struc->getTy()->offset + off);
            break;
        case DataStruct::AST_TYPE::AST_DEREF:
            emit_expr(struc->unop);
            emit_lload(field, "rax", field->offset + off);
            break;
        default:
            Error::error("internal error: %s", _parser->node2s(struc));
    }
}
void CodeGen::emit_store(const std::shared_ptr<DataStruct::Node> &var) {
    SAVE;
    switch (var->getKind()) {
        case DataStruct::AST_TYPE::AST_DEREF: emit_assign_deref(var); break;
        case DataStruct::AST_TYPE::AST_STRUCT_REF: emit_assign_struct_ref(var->struc, var->getTy(), 0); break;
        case DataStruct::AST_TYPE::AST_LVAR:
            ensure_lvar_init(var);
            emit_lsave(var->getTy(), var->loff);
            break;
        case DataStruct::AST_TYPE::AST_GVAR: emit_gsave(var->glabel, var->getTy(), 0); break;
        default: Error::error("internal error");
    }
}
/**
 * 按ty类型分类：
 * （1）floattype
 * push %xmm1
 * xorpd %xmm1,%xmm1
 * ucomiss(d) %xmm1,%xmm0
 * setne %al
 * pop %xmm1
 * movzb %al,%eax
 * (2)
 * cmp $0, %rax
 * setne %al
 * movzb %al,%eax
 * @param ty
 */
void CodeGen::emit_to_bool(const std::shared_ptr<DataStruct::Type> &ty) {
    SAVE;
    if (_parser->is_flotype(ty)) {
        push_xmm(1);
        emit("xorpd #xmm1, #xmm1");
        emit("%s #xmm1, #xmm0", (ty->kind == DataStruct::TYPE_KIND::KIND_FLOAT) ? "ucomiss" : "ucomisd");
        emit("setne #al");
        pop_xmm(1);
    } else {
        emit("cmp $0, #rax");
        emit("setne #al");
    }
    emit("movzb #al, #eax");
}
void CodeGen::emit_comp(const std::string &inst, const std::string &usiginst, const std::shared_ptr<DataStruct::Node> &node) {
    SAVE;
    if (_parser->is_flotype(node->left->getTy())) {
        emit_expr(node->left);
        push_xmm(0);
        emit_expr(node->right);
        pop_xmm(1);
        if (node->left->getTy()->kind == DataStruct::TYPE_KIND::KIND_FLOAT)
            emit("ucomiss #xmm0, #xmm1");
        else
            emit("ucomisd #xmm0, #xmm1");
    } else {
        emit_expr(node->left);
        push("rax");
        emit_expr(node->right);
        pop("rcx");
        auto kind = node->left->getTy()->kind;
        if (kind == DataStruct::TYPE_KIND::KIND_LONG || kind == DataStruct::TYPE_KIND::KIND_LLONG)
            emit("cmp #rax, #rcx");
        else
            emit("cmp #eax, #ecx");
    }
    if (_parser->is_flotype(node->left->getTy()) || node->left->getTy()->usig)
        emit("%s #al", usiginst);
    else
        emit("%s #al", inst);
    emit("movzb #al, #eax");
}
/**
 * DIV:  分子存放在RDX：RAX，结果也存放在这两个寄存器里面，注意除法是无符号数和有福好数的时候最高位的拓展
 * @param node
 */
void CodeGen::emit_binop_int_arith(const std::shared_ptr<DataStruct::Node> &node) {
    SAVE;
    std::string op;
    switch (node->getKind()) {
        case DataStruct::AST_TYPE::PLUS: op = "add"; break;
        case DataStruct::AST_TYPE::SUB: op = "sub"; break;
        case DataStruct::AST_TYPE::MUL: op = "imul"; break;
        case DataStruct::AST_TYPE::XOR: op = "xor"; break;
        case DataStruct::AST_TYPE::OP_SAL: op = "sal"; break;
        case DataStruct::AST_TYPE::OP_SAR: op = "sar"; break;
        case DataStruct::AST_TYPE::OP_SHR: op = "shr"; break;
        case DataStruct::AST_TYPE::DIV: case DataStruct::AST_TYPE::LEFT: break;
        default: Error::error("invalid operator '%d'", static_cast<int>(node->getKind()));
    }
    emit_expr(node->left);
    push("rax");
    emit_expr(node->right);
    emit("mov #rax, #rcx");
    pop("rax");
    if (node->getKind() == DataStruct::AST_TYPE::DIV|| node->getKind() == DataStruct::AST_TYPE::LEFT) {
        if (node->getTy()->usig) {
            emit("xor #edx, #edx");
            emit("div #rcx");
        } else {
            emit("cqto");   //符号拓展
            emit("idiv #rcx");
        }
        if (node->getKind() == DataStruct::AST_TYPE::LEFT)
            emit("mov #edx, #eax");     //这么看eax保存商，edx保存余数
    } else if (node->getKind() == DataStruct::AST_TYPE::OP_SAL || node->getKind() == DataStruct::AST_TYPE::OP_SAR
               || node->getKind() == DataStruct::AST_TYPE::OP_SHR) {
        emit("%s #cl, #%s", op, get_int_reg(node->left->getTy(), 'a'));
    } else {
        emit("%s #rcx, #rax", op);
    }
}
void CodeGen::emit_binop_float_arith(const std::shared_ptr<DataStruct::Node> &node) {
    SAVE;
    std::string op;
    bool isdouble = (node->getTy()->kind == DataStruct::TYPE_KIND::KIND_DOUBLE);
    switch (node->getKind()) {
        case DataStruct::AST_TYPE::PLUS: op = (isdouble ? "addsd" : "addss"); break;
        case DataStruct::AST_TYPE::SUB: op = (isdouble ? "subsd" : "subss"); break;
        case DataStruct::AST_TYPE::MUL: op = (isdouble ? "mulsd" : "mulss"); break;
        case DataStruct::AST_TYPE::DIV: op = (isdouble ? "divsd" : "divss"); break;
        default: Error::error("invalid operator '%d'", static_cast<int>(node->getKind()));
    }
    emit_expr(node->left);
    push_xmm(0);
    emit_expr(node->right);
    emit("%s #xmm0, #xmm1", (isdouble ? "movsd" : "movss"));
    pop_xmm(0);
    emit("%s #xmm1, #xmm0", op);
}
/**
 * (1)int->float                    cvtsi2ss #eax, #xmm0
 * (2)int->double                   cvtsi2sd #eax, #xmm0
 * (3)float->double                 cvtps2pd #xmm0, #xmm0
 * (4)double,long double-> float    cvtpd2ps #xmm0, #xmm0
 * (5)bool
 * (6)int->int                      符号拓展或无符号拓展
 * (7)int                           from(float):cvttss2si #xmm0, #eax   (double)cvttsd2si #xmm0, #eax
 * @param to
 * @param from
 */
void CodeGen::emit_load_convert(const std::shared_ptr<DataStruct::Type> &to, const std::shared_ptr<DataStruct::Type> &from) {
    SAVE;
    if (_parser->is_inttype(from) && to->kind == DataStruct::TYPE_KIND::KIND_FLOAT)
        emit("cvtsi2ss #eax, #xmm0");
    else if (_parser->is_inttype(from) && to->kind == DataStruct::TYPE_KIND::KIND_DOUBLE)
        emit("cvtsi2sd #eax, #xmm0");
    else if (from->kind == DataStruct::TYPE_KIND::KIND_FLOAT && to->kind == DataStruct::TYPE_KIND::KIND_DOUBLE)
        emit("cvtps2pd #xmm0, #xmm0");
    else if ((from->kind == DataStruct::TYPE_KIND::KIND_DOUBLE || from->kind == DataStruct::TYPE_KIND::KIND_LDOUBLE)
             && to->kind == DataStruct::TYPE_KIND::KIND_FLOAT)
        emit("cvtpd2ps #xmm0, #xmm0");
    else if (to->kind == DataStruct::TYPE_KIND::KIND_BOOL)
        emit_to_bool(from);
    else if (_parser->is_inttype(from) && _parser->is_inttype(to))
        emit_intcast(from);
    else if (_parser->is_inttype(to))
        emit_toint(from);
}
/**
 * leave
 * ret
 */
void CodeGen::emit_ret() {
    SAVE;
    emit("leave");
    emit("ret");
}
void CodeGen::emit_binop(const std::shared_ptr<DataStruct::Node> &node) {
    SAVE;
    if (node->getTy()->kind == DataStruct::TYPE_KIND::KIND_PTR) {
        emit_pointer_arith(node->getKind(), node->left, node->right);
        return;
    }
    switch (node->getKind()) {
        case DataStruct::AST_TYPE::LOW: emit_comp("setl", "setb", node); return;
        case DataStruct::AST_TYPE::OP_EQ: emit_comp("sete", "sete", node); return;
        case DataStruct::AST_TYPE::OP_LE: emit_comp("setle", "setna", node); return;
        case DataStruct::AST_TYPE::OP_NE: emit_comp("setne", "setne", node); return;
    }
    if (_parser->is_inttype(node->getTy()))
        emit_binop_int_arith(node);
    else if (_parser->is_flotype(node->getTy()))
        emit_binop_float_arith(node);
    else
        Error::error("internal error: %s", _parser->node2s(node));
}
/**
 * 根据node.kind的类型决定把ival，fval，等送到off指定的地址
 * @param node
 * @param totype
 * @param off
 */
void CodeGen::emit_save_literal(const std::shared_ptr<DataStruct::Node> &node,
                       const std::shared_ptr<DataStruct::Type> &totype, int off) {
    switch (totype->kind) {
        case DataStruct::TYPE_KIND::KIND_BOOL:  emit("movb $%d, %d(#rbp)", !!node->ival, off); break;
        case DataStruct::TYPE_KIND::KIND_CHAR:  emit("movb $%d, %d(#rbp)", node->ival, off); break;
        case DataStruct::TYPE_KIND::KIND_SHORT: emit("movw $%d, %d(#rbp)", node->ival, off); break;
        case DataStruct::TYPE_KIND::KIND_INT:   emit("movl $%d, %d(#rbp)", node->ival, off); break;
        case DataStruct::TYPE_KIND::KIND_LONG:
        case DataStruct::TYPE_KIND::KIND_LLONG:
        case DataStruct::TYPE_KIND::KIND_PTR: {
            emit("movl $%lu, %d(#rbp)", ((uint64_t)node->ival) & ((1L << 32) - 1), off);
            emit("movl $%lu, %d(#rbp)", ((uint64_t)node->ival) >> 32, off + 4);
            break;
        }
        case DataStruct::TYPE_KIND::KIND_FLOAT: {
            float fval = node->fval;
            emit("movl $%u, %d(#rbp)", *(uint32_t *)&fval, off);
            break;
        }
        case DataStruct::TYPE_KIND::KIND_DOUBLE:
        case DataStruct::TYPE_KIND::KIND_LDOUBLE: {
            emit("movl $%lu, %d(#rbp)", *(uint64_t *)&node->fval & ((1L << 32) - 1), off);
            emit("movl $%lu, %d(#rbp)", *(uint64_t *)&node->fval >> 32, off + 4);
            break;
        }
        default:
            Error::error("internal error: <%s> <%s> <%d>", _parser->node2s(node), _parser->ty2s(totype), off);
    }
}
/**
 * 取地址操作，只有下面几种node可以取地址
 * （1）AST_LVAR：emit("lea %d(#rbp), #rax", node->loff);
 * （2）AST_GVAR：mit("lea %s(#rip), #rax", node->glabel); 所以全局对象是从label处取地址的
 * （3）AST_DEREF:解引用node，此时该node代表要对起变量unop进行解引用操作，因而需要分析unop
 * （4）AST_STRUCT_REF：这里要结合parser来分析，因而对于该类node而言，他的struc可能是个AST_DEREF node，也有可能就是个STRUCT
 * 对于前者，采用情况（3）处理，对于后者则采用（1）或（2）处理，总之上面两种情况就是取得了基址，还需要一步
 * emit("add $%d, #rax", node->getTy()->offset);
 * 来获得成员变量的地址
 * （5）函数：emit("lea %s(#rip), #rax", node->fname);
 * @param node
 */
void CodeGen::emit_addr(const std::shared_ptr<DataStruct::Node> &node) {
    switch (node->getKind()) {
        case DataStruct::AST_TYPE::AST_LVAR:
            ensure_lvar_init(node);
            emit("lea %d(#rbp), #rax", node->loff);
            break;
        case DataStruct::AST_TYPE::AST_GVAR:
            emit("lea %s(#rip), #rax", node->glabel);
            break;
        case DataStruct::AST_TYPE::AST_DEREF:
            emit_expr(node->unop);
            break;
        case DataStruct::AST_TYPE::AST_STRUCT_REF:
            emit_addr(node->struc);
            emit("add $%d, #rax", node->getTy()->offset);
            break;
        case DataStruct::AST_TYPE::AST_FUNCDESG:
            emit("lea %s(#rip), #rax", node->fname);
            break;
        default:
            Error::error("internal error: %s", _parser->node2s(node));
    }
}
void CodeGen::emit_copy_struct(const std::shared_ptr<DataStruct::Node> &left,
                               const std::shared_ptr<DataStruct::Node> &right) {
    push("rcx");
    push("r11");
    emit_addr(right);
    emit("mov #rax, #rcx");
    emit_addr(left);
    int i = 0;
    for (; i < left->getTy()->size; i += 8) {
        emit("movq %d(#rcx), #r11", i);
        emit("movq #r11, %d(#rax)", i);
    }
    for (; i < left->getTy()->size; i += 4) {
        emit("movl %d(#rcx), #r11", i);
        emit("movl #r11, %d(#rax)", i);
    }
    for (; i < left->getTy()->size; i++) {
        emit("movb %d(#rcx), #r11", i);
        emit("movb #r11, %d(#rax)", i);
    }
    pop("r11");
    pop("rcx");
}

/**
 * 根据我们读取初始化参数时设置的偏移值，把未初始化的部分初始化为0
 * @param inits
 * @param off
 * @param totalsize
 */
void CodeGen::emit_fill_holes(const std::vector<std::shared_ptr<DataStruct::Node>> &inits, int off, int totalsize) {
    // If at least one of the fields in a variable are initialized,
    // unspecified fields has to be initialized with 0.
    auto len = inits.size();
    std::vector<std::shared_ptr<DataStruct::Node>> buf;
    for (auto&e:inits)
        buf.push_back(e);
//    qsort(buf, len, sizeof(Node *), cmpinit);

    int lastend = 0;
    for (auto&node:buf) {
        if (lastend < node->initoff)
            emit_zero_filler(lastend + off, node->initoff + off);
        lastend = node->initoff + node->totype->size;
    }
    emit_zero_filler(lastend + off, totalsize + off);
}
void CodeGen::emit_decl_init(const std::vector<std::shared_ptr<DataStruct::Node>> &inits, int off, int totalsize) {
    emit_fill_holes(inits, off, totalsize);
    for (auto&node:inits) {
        if (node->getKind() != DataStruct::AST_TYPE::AST_INIT)
            Error::error("not init node");
        bool isbitfield = (node->totype->bitsize > 0);
        if (node->initval->getKind() == DataStruct::AST_TYPE::AST_LITERAL && !isbitfield) {
            emit_save_literal(node->initval, node->totype, node->initoff + off);
        } else {
            emit_expr(node->initval);
            emit_lsave(node->totype, node->initoff + off);
        }
    }
}
void CodeGen::emit_pre_inc_dec(const std::shared_ptr<DataStruct::Node> &node, const std::string &op) {
    emit_expr(node->unop);
    emit("%s $%d, #rax", op, node->getTy()->ptr ? node->getTy()->ptr->size : 1);
    emit_store(node->unop);
}
void CodeGen::emit_post_inc_dec(const std::shared_ptr<DataStruct::Node> &node, const std::string &op) {
    SAVE;
    emit_expr(node->unop);
    push("rax");
    emit("%s $%d, #rax", op, node->getTy()->ptr ? node->getTy()->ptr->size : 1);
    emit_store(node->unop);
    pop("rax");
}
/**
 * 统计参数floate参数的数码和其他类参数的树木
 * @param
 */
void CodeGen::set_reg_nums(const std::vector<std::shared_ptr<DataStruct::Node>> &args) {
    numgp = numfp = 0;
    for (auto&arg:args) {
        if (_parser->is_flotype(arg->getTy()))
            numfp++;
        else
            numgp++;
    }
}
/**
 * test %rax,%rax
 * je label
 * @param label
 */
void CodeGen::emit_je(const std::string &label) {
    emit("test #rax, #rax");
    emit("je %s", label);
}
/*
 * 生成汇编  label：
 */
void CodeGen::emit_label(const std::string &label) {
    emit("%s:", label);
}
/**
 * 生成汇编： jmp label
 * @param label
 */
void CodeGen::emit_jmp(const std::string &label) {
    emit("jmp %s", label);
}
/**
 * 整型对象， mov #ival %rax
 * 浮点型对象：
 *     float：格式为 .data
 *                      label:
 *                      .long fval
 *                  .text  (首次的时候会出现这些标签）
 *            然后movss label(rip)  #xmm0
 *     double:和上面基本一样，就是long变成了quad，movss变成了movsd
 * string：和上面基本一样，就是quad变成了.string sval,movsd变成了lea
 * 其他情况则是错误
 * @param node
 */
void CodeGen::emit_literal(const std::shared_ptr<DataStruct::Node> &node) {
    SAVE;
    switch (node->getTy()->kind) {
        case DataStruct::TYPE_KIND::KIND_BOOL:
        case DataStruct::TYPE_KIND::KIND_CHAR:
        case DataStruct::TYPE_KIND::KIND_SHORT:
            emit("mov $%u, #rax", node->ival);
            break;
        case DataStruct::TYPE_KIND::KIND_INT:
            emit("mov $%u, #rax", node->ival);
            break;
        case DataStruct::TYPE_KIND::KIND_LONG:
        case DataStruct::TYPE_KIND::KIND_LLONG: {
            emit("mov $%lu, #rax", node->ival);
            break;
        }
        case DataStruct::TYPE_KIND::KIND_FLOAT: {
            if (!node->flabel.empty()) {
                node->flabel = _parser->make_label();
                float fval = node->fval;
                emit_noindent(".data");
                emit_label(node->flabel);
                emit(".long %d", *(uint32_t *)&fval);
                emit_noindent(".text");
            }
            emit("movss %s(#rip), #xmm0", node->flabel);
            break;
        }
        case DataStruct::TYPE_KIND::KIND_DOUBLE:
        case DataStruct::TYPE_KIND::KIND_LDOUBLE: {
            if (!node->flabel.empty()) {
                node->flabel = _parser->make_label();
                emit_noindent(".data");
                emit_label(node->flabel);
                emit(".quad %lu", *(uint64_t *)&node->fval);
                emit_noindent(".text");
            }
            emit("movsd %s(#rip), #xmm0", node->flabel);
            break;
        }
        case DataStruct::TYPE_KIND::KIND_ARRAY: {
            if (!node->slabel.empty()) {
                node->slabel = _parser->make_label();
                emit_noindent(".data");
                emit_label(node->slabel);
                emit(".string \"%s\"", Utils::quote_cstring_len(node->sval.c_str(), node->getTy()->size - 1));
                emit_noindent(".text");
            }
            emit("lea %s(#rip), #rax", node->slabel);
            break;
        }
        default:
            Error::error("internal error");
    }
}
/**
 * 按照'\r\n',\r,\n 做分隔符分割字符串
 * @param buf 分割的字符串
 * @return 分好的每一段
 */
std::vector<std::string> CodeGen::split(std::string &buf) {
    std::vector<std::string > ref;
    decltype(buf.size()) cur=0;
    for(decltype(buf.size()) i=0,len=buf.size();i<len;++i)
    {
        if (buf[i]=='\r'&&i+1<len&&buf[i+1]=='\n'){
            buf[i]='\0';
            ref.push_back(buf.substr(cur,i-cur));
            cur=i+2;
            ++i;
            continue;
        }
        if (buf[i]=='\r'||buf[i]=='\n'){
            buf[i]='\0';
            ref.push_back(buf.substr(cur,i-cur));
            cur=i+1;
        }
    }
    return ref;
}

/**
 * 读取文件的内容，并按照"escape r escape n","escaoe r","escape n" 三种分隔符分割成段
 * @param file 读取的文件
 * @return 分好的段
 */
std::vector<std::string> CodeGen::read_source_file(const std::string &file) {
    std::ifstream ifstream(file);
    if (!ifstream)
        return std::vector<std::string>();
    struct stat st;
    stat(file.c_str(),&st);
    std::string content(st.st_size+1,'0');
    ifstream.read(&content[0],st.st_size);
    if (ifstream.gcount()!=st.st_size){
        ifstream.close();
        return std::vector<std::string>();
    }
    ifstream.close();
    return split(content);
}
/**
 * 打印file的第line行
 * @param file 读取的文件名
 * @param line 行号
 */
void CodeGen::maybe_print_source_line(const std::string &file, int line) {
    if (!Utils::dumpsource)
        return;
    std::vector<std::string > lines;
    if (source_lines.find(file)==source_lines.end()){
        lines=read_source_file(file);
        if (lines.empty())
            return;
        source_lines[file]=lines;
    }
    int len=0;
    emit_nostack("# %s",lines[line-1]);
}
/**
 * 输出.file .loc
 * @param node
 */
void CodeGen::maybe_print_source_loc(const std::shared_ptr<DataStruct::Node> &node) {
    if (-1==node->getSourceloc().line)
        return;
    auto file = node->getSourceloc().file;
    long fileno = 0;
    if (source_files.find(file)==source_files.end()){
        fileno=source_files.size()+1;
        source_files[file]=fileno;
        emit(".file %d \"%s\"", fileno, Utils::quote_cstring_len(file.c_str(),file.size()));
    }
    auto loc = Utils::format(".loc %d %d 0", fileno, node->getSourceloc().line);
    if (loc != last_loc) {
        emit("%s", loc);
        maybe_print_source_line(file, node->getSourceloc().line);
    }
    last_loc = loc;
}

/**
 * lvalue的处理流程
 * （1)进行初始化处理，如果有初始化node，那么根据逻辑偏移来初始化，未初始化的部分填充为0,
 * （2)字面值对象如果不是位域，那么直接把iva，fval等mov到逻辑偏移处
 * （3)不然则mov数据到rax，期间可能会转化为bool，也可能进行位域处理，然后吧经过处理的结果mov到偏移处
 * 上面处理完以后就可以取数了，这里要根据node 的type进行操作，
 * （1）array:emit("lea %d(#%s), #rax", off, base);
 * (2)KIND_FLOAT:emit("movss %d(#%s), #xmm0", off, base);
 * (3)KIND_DOUBLE,KIND_LDOUBLE:emit("movsd %d(#%s), #xmm0", off, base)
 * (4)int family:
 * auto inst = get_load_inst(ty);
 * emit("%s %d(#%s), #rax", inst, off, base);   (struct或者union也都是在栈中保存的基址，因而我们需要知道基址）
 * maybe_emit_bitshift_load(ty);
 * @param node
 */
void CodeGen::emit_lvar(const std::shared_ptr<DataStruct::Node> &node) {
    SAVE;
    ensure_lvar_init(node);
    emit_lload(node->getTy(), "rbp", node->loff);
}
/**
 * node->type:
 * array:
 * (1) off!=0   lea glabel+off(%rip), %rax
 * (2) off==0   lea glabel(%rip), %rax
 * 其他：
 * (1)movszb,movzbl.. glabel+off(%rip), %rax
 * @param node
 */
void CodeGen::emit_gvar(const std::shared_ptr<DataStruct::Node> node) {
    SAVE;
    emit_gload(node->getTy(), node->glabel, 0);
}
/**
 * gcc的拓展特性，首先取得参数的level，存放于rax中，然后
 *     push %r11
 *     mov level %rax   (level未必是int，因而需要分析一下确定采用的哪个)
 *     mov %rbp  %r11   (r11指向当前堆栈）
 * looplabel:
 *     test %rax,%rax
 *     jz endlabel
 *     mov (%r11) %r11
 *     sub $1 ,%rax
 *     jmp looplabel:
 * endlabel:
 *     mov 8(%r11),%rax    （到这里就取得了rbp的帧地址，也就是祖先函数的返回地址）
 *     pop %r11
 * 最终看下来，这个函数并不是返回祖先函数的入口地址，而是返回祖先函数的调用点地址
 * @param node
 */
void CodeGen::emit_builtin_return_address(const std::shared_ptr<DataStruct::Node> &node) {
    push("r11");
    if (node->args.size() != 1)
        Error::error("args !=1");
    emit_expr(std::make_shared<DataStruct::Node>(node->args.front()));
    auto loop = _parser->make_label();
    auto end = _parser->make_label();
    emit("mov #rbp, #r11");
    emit_label(loop);
    emit("test #rax, #rax");
    emit("jz %s", end);
    emit("mov (#r11), #r11");
    emit("sub $1, #rax");
    emit_jmp(loop);
    emit_label(end);
    emit("mov 8(#r11), #rax");
    pop("r11");
}

/**
 * 当参数是指针类型的时候这个才适用，这个函数看起来是设置传递给rax的参数的类型所使用的寄存器类型
 * 根据node->getTy()->ptr的kind来决定是rax所传递的参数是什么类型的
 * （1）DataStruct::TYPE_KIND::KIND_STRUCT
 * mov $2 %eax  MEMORY
 * （2）floattype
 * mov $1 %eax  SSE
 * （3）other
 * mov $0 %eax   integer
 * @param node
 */
void CodeGen::emit_builtin_reg_class(const std::shared_ptr<DataStruct::Node> &node) {
    auto arg = node->args.front();
    if (arg.getTy()->kind != DataStruct::TYPE_KIND::KIND_PTR)
        Error::error("arg is not ptr");
    auto ty = arg.getTy()->ptr;
    if (ty->kind == DataStruct::TYPE_KIND::KIND_STRUCT)
        emit("mov $2, #eax");
    else if (_parser->is_flotype(ty))
        emit("mov $1, #eax");
    else
        emit("mov $0, #eax");
}

/**
 * va_start，可变参数的处理，参数只能为1
 * push %rcx
 * movl $numgp*8, (%rax)    (首先保存一共有多少个参数，然后下面才是参数）
 * movl $48+numfp*16, 4(%rax)
 * lea -176(%rbp),%rcx   (有可变参数的时候，rbp下端176字节保存的都是参数寄存器（6个通用寄存器，8个sse寄存器）的值）
 * mov %rcx,16(%rax)    （这里只所以是16,是因为函数参数不能只包含一个...，应该至少包含一个参数
 * pop %rcx
 * @param node
 */
void CodeGen::emit_builtin_va_start(const std::shared_ptr<DataStruct::Node> &node) {
    SAVE;
    if (node->args.size() != 1)
        Error::error("emit_builtin_va_start,args size is not 1");
    emit_expr(std::make_shared<DataStruct::Node>(node->args.front()));
    push("rcx");
    emit("movl $%d, (#rax)", numgp * 8);
    emit("movl $%d, 4(#rax)", 48 + numfp * 16);
    emit("lea %d(#rbp), #rcx", -REGAREA_SIZE);
    emit("mov #rcx, 16(#rax)");
    pop("rcx");
}
/**
 *处理__builtin_return_address，__builtin_reg_class，__builtin_va_start这3个内嵌函数
 * @param node
 * @return
 */
bool CodeGen::maybe_emit_builtin(const std::shared_ptr<DataStruct::Node> &node) {
    SAVE;
    if ("__builtin_return_address" == node->fname) {
        emit_builtin_return_address(node);
        return true;
    }
    if ("__builtin_reg_class" == node->fname) {
        emit_builtin_reg_class(node);
        return true;
    }
    if ("__builtin_va_start" == node->fname) {
        emit_builtin_va_start(node);
        return true;
    }
    return false;
}
/**
 * 函数参数分类
 * @param ints 保存类int类型，最多6个，多的放入rest中
 * @param floats 保存floattype参数，最多8个，多个放入rest中
 * @param rest 存放KIND_STRUCT以及上面两类多余的参数
 * @param args 函数参数
 */
void CodeGen::classify_args(std::vector<std::shared_ptr<DataStruct::Node>> &ints,
                   std::vector<std::shared_ptr<DataStruct::Node>> &floats,
                   std::vector<std::shared_ptr<DataStruct::Node>> & rest,
                   const std::vector<DataStruct::Node> & args) {
    SAVE;
    int ireg = 0, xreg = 0;
    int imax = 6, xmax = 8;
    for (auto& v:args) {
        if (v.getTy()->kind == DataStruct::TYPE_KIND::KIND_STRUCT)
            rest.push_back(std::make_shared<DataStruct::Node>(v));
        else if (_parser->is_flotype(v.getTy()))
            (xreg++ < xmax) ? floats.push_back(std::make_shared<DataStruct::Node>(v)) : rest.push_back(std::make_shared<DataStruct::Node>(v));
        else
            (ireg++ < imax) ? ints.push_back(std::make_shared<DataStruct::Node>(v)) : rest.push_back(std::make_shared<DataStruct::Node>(v));
    }
}
/**
 * 保存通用寄存器的内容和SSE寄存器的内容到栈中
 * @param nints 要保存的通用寄存器的数目，不超过6
 * @param nfloats 要保存的SSE寄存器的数目，不超过8
 */
void CodeGen::save_arg_regs(int nints, int nfloats) {
    SAVE;
    if (nints>6)
        Error::error("nints>6");
    if (nfloats>8)
        Error::error("nfloats>8");
    for (int i = 0; i < nints; i++)
        push(REGS[i]);
    for (int i = 1; i < nfloats; i++)
        push_xmm(i);
}
/**
 * 从栈中恢复通用寄存器和SSE寄存器的内容
 * @param nints 要保存的通用寄存器的数目，不超过6
 * @param nfloats 要保存的SSE寄存器的数目，不超过8
 */
void CodeGen::restore_arg_regs(int nints, int nfloats) {
    SAVE;
    for (int i = nfloats - 1; i > 0; i--)
        pop_xmm(i);
    for (int i = nints - 1; i >= 0; i--)
        pop(REGS[i]);
}
/**
 * 设置参数，
 * @param vals
 * @return vals中的参数占用的总字节数
 */
int CodeGen::emit_args(const std::vector<std::shared_ptr<DataStruct::Node>> &vals) {
    SAVE;
    int r = 0;
    for (auto& v:vals) {
        if (v->getTy()->kind == DataStruct::TYPE_KIND::KIND_STRUCT) {
            emit_addr(v);
            r += push_struct(v->getTy()->size);
        } else if (_parser->is_flotype(v->getTy())) {
            emit_expr(v);
            push_xmm(0);
            r += 8;
        } else {
            emit_expr(v);
            push("rax");
            r += 8;
        }
    }
    return r;
}
/**
 * 从栈中恢复通用寄存器的内容
 * @param nints 恢复的通用寄存器的数目
 */
void CodeGen::pop_int_args(int nints) {
    SAVE;
    for (int i = nints - 1; i >= 0; i--)
        pop(REGS[i]);
}
/**
 * 从栈中恢复SSE寄存器的内容
 * @param nfloats 恢复的SSE寄存器的数目
 */
void CodeGen::pop_float_args(int nfloats) {
    SAVE;
    for (int i = nfloats - 1; i >= 0; i--)
        pop_xmm(i);
}
/**
 * 若ty是bool类型，则进行zero拓展
 * movzx #al, #rax
 * @param ty
 */
void CodeGen::maybe_booleanize_retval(const std::shared_ptr<DataStruct::Type> &ty) {
    if (ty->kind == DataStruct::TYPE_KIND::KIND_BOOL) {
        emit("movzx #al, #rax");
    }
}
/**
 * 函数调用，注意函数是否含有可变参数，以及寄存器的调用规范
 * @param node
 */
void CodeGen::emit_func_call(const std::shared_ptr<DataStruct::Node> &node) {
    SAVE;
    int opos = stackpos;
    bool isptr = (node->getKind() == DataStruct::AST_TYPE::AST_FUNCPTR_CALL);
    auto ftype = isptr ? node->fptr->getTy()->ptr : node->ftype;

    std::vector<std::shared_ptr<DataStruct::Node>> ints ;
    std::vector<std::shared_ptr<DataStruct::Node>> floats ;
    std::vector<std::shared_ptr<DataStruct::Node>> rest;
    classify_args(ints, floats, rest, node->args);
    save_arg_regs(ints.size(), floats.size());

    bool padding = stackpos % 16;
    if (padding) {
        emit("sub $8, #rsp");
        stackpos += 8;
    }
    std::reverse(rest.begin(),rest.end());
    int restsize = emit_args(rest);
    if (isptr) {
        emit_expr(node->fptr);
        push("rax");
    }
    emit_args(ints);
    emit_args(floats);
    pop_float_args(floats.size());
    pop_int_args(ints.size());

    if (isptr) pop("r11");
    if (ftype->hasva)
        emit("mov $%u, #eax", floats.size());

    if (isptr)
        emit("call *#r11");
    else
        emit("call %s", node->fname);
    maybe_booleanize_retval(node->getTy());
    if (restsize > 0) {
        emit("add $%d, #rsp", restsize);
        stackpos -= restsize;
    }
    if (padding) {
        emit("add $8, #rsp");
        stackpos -= 8;
    }
    restore_arg_regs(ints.size(),floats.size());
    if (opos!=stackpos)
        Error::error("opos!=stackpos");
}
/**
 * 如果有初始化node，那么会处理该函数，否则直接返回
 * 处理流程：
 *  把未初始化的部分初始化为0,然后用初始化相应的节点去初始化lvalue所在内存处的值
 * @param node
 */
void CodeGen::emit_decl(const std::shared_ptr<DataStruct::Node> &node) {
    SAVE;
    if (node->declinit.empty())
        return;
    emit_decl_init(node->declinit, node->declvar->loff, node->declvar->getTy()->size);
}

void CodeGen::emit_conv(const std::shared_ptr<DataStruct::Node> &node) {
    SAVE;
    emit_expr(node->unop);
    emit_load_convert(node->getTy(), node->unop->getTy());
}

void CodeGen::emit_deref(const std::shared_ptr<DataStruct::Node> &node) {
    SAVE;
    emit_expr(node->unop);
    emit_lload(node->unop->getTy()->ptr, "rax", 0);
    emit_load_convert(node->getTy(), node->unop->getTy()->ptr);
}

void CodeGen::emit_ternary(const std::shared_ptr<DataStruct::Node> &node) {
    SAVE;
    emit_expr(node->cond);
    auto ne = _parser->make_label();
    emit_je(ne);
    if (node->then)
        emit_expr(node->then);
    if (node->els) {
        auto end = _parser->make_label();
        emit_jmp(end);
        emit_label(ne);
        emit_expr(node->els);
        emit_label(end);
    } else {
        emit_label(ne);
    }
}
/**
 * jmp label
 * @param node
 */
void CodeGen::emit_goto(const std::shared_ptr<DataStruct::Node> &node) {
    SAVE;
    if (node->newlabel.empty())
        Error::error("node->newlabel empty");
    emit_jmp(node->newlabel);
}
/**
 * 分析返回值，可能会转化为bool类型
 * leave
 * ret
 * @param node
 */
void CodeGen::emit_return(const std::shared_ptr<DataStruct::Node> &node) {
    SAVE;
    if (node->retval) {
        emit_expr(node->retval);
        maybe_booleanize_retval(node->retval->getTy());
    }
    emit_ret();
}

void CodeGen::emit_compound_stmt(const std::shared_ptr<DataStruct::Node> &node) {
    SAVE;
    for(auto&e:node->stmts)
        emit_expr(e);
}
/**
 * (endlabel=make_label)
 *      emit_expr(node->left)
 *      test %rax,%rax
 *      mov &0, %rax
 *      je endlabel
 *      emit_expr(node->left)
 *      test %rax,%rax
 *      mov &0,%rax
 *      je endlabel
 *      mov $1,label
 * endlabel:
 * @param node
 */
void CodeGen::emit_logand(const std::shared_ptr<DataStruct::Node> &node) {
    SAVE;
    auto end = _parser->make_label();
    emit_expr(node->left);
    emit("test #rax, #rax");
    emit("mov $0, #rax");
    emit("je %s", end);
    emit_expr(node->right);
    emit("test #rax, #rax");
    emit("mov $0, #rax");
    emit("je %s", end);
    emit("mov $1, #rax");
    emit_label(end);
}

void CodeGen::emit_logor(const std::shared_ptr<DataStruct::Node> &node) {
    SAVE;
    auto end = _parser->make_label();
    emit_expr(node->left);
    emit("test #rax, #rax");
    emit("mov $1, #rax");
    emit("jne %s", end);
    emit_expr(node->right);
    emit("test #rax, #rax");
    emit("mov $1, #rax");
    emit("jne %s", end);
    emit("mov $0, #rax");
    emit_label(end);
}

void CodeGen::emit_lognot(const std::shared_ptr<DataStruct::Node> &node) {
    SAVE;
    emit_expr(node->unop);
    emit("cmp $0, #rax");
    emit("sete #al");
    emit("movzb #al, #eax");
}

void CodeGen::emit_bitand(const std::shared_ptr<DataStruct::Node> &node) {
    SAVE;
    emit_expr(node->left);
    push("rax");
    emit_expr(node->right);
    pop("rcx");
    emit("and #rcx, #rax");
}

void CodeGen::emit_bitor(const std::shared_ptr<DataStruct::Node> &node) {
    SAVE;
    emit_expr(node->left);
    push("rax");
    emit_expr(node->right);
    pop("rcx");
    emit("or #rcx, #rax");
}

void CodeGen::emit_bitnot(const std::shared_ptr<DataStruct::Node> &node) {
    SAVE;
    emit_expr(node->left);
    emit("not #rax");
}

void CodeGen::emit_cast(const std::shared_ptr<DataStruct::Node> &node) {
    SAVE;
    emit_expr(node->unop);
    emit_load_convert(node->getTy(), node->unop->getTy());
    return;
}

void CodeGen::emit_comma(const std::shared_ptr<DataStruct::Node> &node) {
    SAVE;
    emit_expr(node->left);
    emit_expr(node->right);
}

void CodeGen::emit_assign(const std::shared_ptr<DataStruct::Node> &node) {
    SAVE;
    if (node->left->getTy()->kind == DataStruct::TYPE_KIND::KIND_STRUCT &&
        node->left->getTy()->size > 8) {
        emit_copy_struct(node->left, node->right);
    } else {
        emit_expr(node->right);
        emit_load_convert(node->getTy(), node->right->getTy());
        emit_store(node->left);
    }
}

void CodeGen::emit_label_addr(const std::shared_ptr<DataStruct::Node> &node) {
    SAVE;
    emit("mov $%s, #rax", node->newlabel);
}

void CodeGen::emit_computed_goto(const std::shared_ptr<DataStruct::Node> &node) {
    SAVE;
    emit_expr(node->unop);
    emit("jmp *#rax");
}

void CodeGen::emit_expr(const std::shared_ptr<DataStruct::Node> &node) {
    SAVE;
    maybe_print_source_loc(node);
    switch (node->getKind()) {
        case DataStruct::AST_TYPE::AST_LITERAL: emit_literal(node); return;
        case DataStruct::AST_TYPE::AST_LVAR:    emit_lvar(node); return;
        case DataStruct::AST_TYPE::AST_GVAR:    emit_gvar(node); return;
        case DataStruct::AST_TYPE::AST_FUNCDESG: emit_addr(node); return;
        case DataStruct::AST_TYPE::AST_FUNCALL:
            if (maybe_emit_builtin(node))
                return;
            // fall through
        case DataStruct::AST_TYPE::AST_FUNCPTR_CALL:
            emit_func_call(node);
            return;
        case DataStruct::AST_TYPE::AST_DECL:    emit_decl(node); return;
        case DataStruct::AST_TYPE::AST_CONV:    emit_conv(node); return;
        case DataStruct::AST_TYPE::AST_ADDR:    emit_addr(node->unop); return;
        case DataStruct::AST_TYPE::AST_DEREF:   emit_deref(node); return;
        case DataStruct::AST_TYPE::AST_IF:
        case DataStruct::AST_TYPE::AST_TERNARY:
            emit_ternary(node);
            return;
        case DataStruct::AST_TYPE::AST_GOTO:    emit_goto(node); return;
        case DataStruct::AST_TYPE::AST_LABEL:
            if (!node->newlabel.empty())
                emit_label(node->newlabel);
            return;
        case DataStruct::AST_TYPE::AST_RETURN:  emit_return(node); return;
        case DataStruct::AST_TYPE::AST_COMPOUND_STMT: emit_compound_stmt(node); return;
        case DataStruct::AST_TYPE::AST_STRUCT_REF:
            emit_load_struct_ref(node->struc, node->getTy(), 0);
            return;
        case DataStruct::AST_TYPE::OP_PRE_INC:   emit_pre_inc_dec(node, "add"); return;
        case DataStruct::AST_TYPE::OP_PRE_DEC:   emit_pre_inc_dec(node, "sub"); return;
        case DataStruct::AST_TYPE::OP_POST_INC:  emit_post_inc_dec(node, "add"); return;
        case DataStruct::AST_TYPE::OP_POST_DEC:  emit_post_inc_dec(node, "sub"); return;
        case DataStruct::AST_TYPE::EXCLAMATION: emit_lognot(node); return;
        case DataStruct::AST_TYPE::AND: emit_bitand(node); return;
        case DataStruct::AST_TYPE::OR: emit_bitor(node); return;
        case DataStruct::AST_TYPE::NEG: emit_bitnot(node); return;
        case DataStruct::AST_TYPE::OP_LOGAND: emit_logand(node); return;
        case DataStruct::AST_TYPE::OP_LOGOR:  emit_logor(node); return;
        case DataStruct::AST_TYPE::OP_CAST:   emit_cast(node); return;
        case DataStruct::AST_TYPE::COMMA: emit_comma(node); return;
        case DataStruct::AST_TYPE::ASSIGN: emit_assign(node); return;
        case DataStruct::AST_TYPE::OP_LABEL_ADDR: emit_label_addr(node); return;
        case DataStruct::AST_TYPE::AST_COMPUTED_GOTO: emit_computed_goto(node); return;
        default:
            emit_binop(node);
    }
}

void CodeGen::emit_zero(int size) {
    SAVE;
    for (; size >= 8; size -= 8) emit(".quad 0");
    for (; size >= 4; size -= 4) emit(".long 0");
    for (; size > 0; size--)     emit(".byte 0");
}
//int diff = node->initoff - off;
//if (diff<0)
//Error::error("diff should greater than 0");
//emit_zero(diff);
void CodeGen::emit_padding(const std::shared_ptr<DataStruct::Node> &node, int off) {
    SAVE;
    int diff = node->initoff - off;
    if (diff<0)
        Error::error("diff should greater than 0");
    emit_zero(diff);
}
/**
 * 初始化.data,根据operand的kind分类
 * （1）AST_LVAR
 *      .data depth+1
 *      label:
 *      do_emit_data(operand->lvarinit, operand->getTy()->size, 0, depth + 1);
 *      .data depth
 *      .quad label
 * (2) AST_GVAR
 *      .quad operand->glabel
 * 其他情况，error
 * @param operand
 * @param depth
 */
void CodeGen::emit_data_addr(const std::shared_ptr<DataStruct::Node> &operand, int depth) {
    switch (operand->getKind()) {
        case DataStruct::AST_TYPE::AST_LVAR: {
            auto label = _parser->make_label();
            emit(".data %d", depth + 1);
            emit_label(label);
            do_emit_data(operand->lvarinit, operand->getTy()->size, 0, depth + 1);
            emit(".data %d", depth);
            emit(".quad %s", label);
            return;
        }
        case DataStruct::AST_TYPE::AST_GVAR:
            emit(".quad %s", operand->glabel);
            return;
        default:
            Error::error("internal error");
    }
}
/**
 *      .data depth+1
 *      label:
 *      .string s
 *      .data depth
 *      .quad label
 *      完全不知道这个depth是干嘛的
 * @param s
 * @param depth
 */
void CodeGen::emit_data_charptr(const std::string &s, int depth) {
    auto label = _parser->make_label();
    emit(".data %d", depth + 1);
    emit_label(label);
    emit(".string \"%s\"", Utils::quote_cstring_len(s.c_str(),s.size()));
    emit(".data %d", depth);
    emit(".quad %s", label);
}

void CodeGen::emit_data_primtype(const std::shared_ptr<DataStruct::Type> &ty,
                        const std::shared_ptr<DataStruct::Node> &val, int depth) {
    switch (ty->kind) {
        case DataStruct::TYPE_KIND::KIND_FLOAT: {
            float f = val->fval;
            emit(".long %d", *(uint32_t *)&f);
            break;
        }
        case DataStruct::TYPE_KIND::KIND_DOUBLE:
            emit(".quad %ld", *(uint64_t *)&val->fval);
            break;
        case DataStruct::TYPE_KIND::KIND_BOOL:
            emit(".byte %d", !!_parser->eval_intexpr(val, nullptr));
            break;
        case DataStruct::TYPE_KIND::KIND_CHAR:
            emit(".byte %d", _parser->eval_intexpr(val, nullptr));
            break;
        case DataStruct::TYPE_KIND::KIND_SHORT:
            emit(".short %d", _parser->eval_intexpr(val, nullptr));
            break;
        case DataStruct::TYPE_KIND::KIND_INT:
            emit(".long %d", _parser->eval_intexpr(val, nullptr));
            break;
        case DataStruct::TYPE_KIND::KIND_LONG:
        case DataStruct::TYPE_KIND::KIND_LLONG:
        case DataStruct::TYPE_KIND::KIND_PTR:
            /**
             * 下面是几种特殊的情况：
             * （1）标签地址   .quad   val->newlabel
             * （2）字符串
             * （3）AST_GVAR   .quad val->glabel
             * （4）取地址
             * 分析val是什么类型的对象
             */
            bool is_char_ptr;
            if (val->getKind() == DataStruct::AST_TYPE::OP_LABEL_ADDR) {
                emit(".quad %s", val->newlabel);         //quad保存label
                break;
            }
            is_char_ptr = (val->unop->getTy()->kind == DataStruct::TYPE_KIND::KIND_ARRAY && val->unop->getTy()->ptr->kind == DataStruct::TYPE_KIND::KIND_CHAR);
            if (is_char_ptr) {
                emit_data_charptr(val->unop->sval, depth);                 //字符串
            } else if (val->getKind() == DataStruct::AST_TYPE::AST_GVAR) {
                emit(".quad %s", val->glabel);                  //全局
            } else {
                std::shared_ptr<DataStruct::Node> base = nullptr;
                int v = _parser->eval_intexpr(val, &base);
                if (base == nullptr) {
                    emit(".quad %u", v);
                    break;
                }
                auto ty = base->getTy();
                if (base->getKind() == DataStruct::AST_TYPE::AST_CONV || base->getKind() == DataStruct::AST_TYPE::AST_ADDR)
                    base = base->unop;
                if (base->getKind() != DataStruct::AST_TYPE::AST_GVAR)
                    Error::error("global variable expected, but got %s", _parser->node2s(base));
                if (!ty->ptr)
                    Error::error("null pointer");
                emit(".quad %s+%u", base->glabel, v * ty->ptr->size);
            }
            break;
        default:
            Error::error("don't know how to handle\n  <%s>\n  <%s>", _parser->ty2s(ty), _parser->node2s(val));
    }
}
/**
 * 分析初始化node
 * （1）位域对象
 * 位域对象是连续的，因而需要分析出所有的连续位域的值，组合成一个data
 * @param inits
 * @param size
 * @param off   这个应该是多个全局对象的偏移问题
 * @param depth  不知道这个是干嘛的
 */
void CodeGen::do_emit_data(const std::vector<std::shared_ptr<DataStruct::Node>> &inits, int size, int off, int depth) {
    SAVE;
    for (int i = 0; i < inits.size() && 0 < size; i++) {
        auto node = inits[i];
        auto v = node->initval;
        emit_padding(node, off);
        if (node->totype->bitsize > 0) {
            if (node->totype->bitoff)
                Error::error("totype.bitoff !=0");
            auto data = _parser->eval_intexpr(v, nullptr);
            auto totype = node->totype;
            for (i++ ; i < inits.size(); i++) {
                node = inits[i];
                if (node->totype->bitsize <= 0) {
                    break;
                }
                v = node->initval;
                totype = node->totype;
                data |= ((((long)1 << totype->bitsize) - 1) & _parser->eval_intexpr(v, nullptr)) << totype->bitoff;
            }
            auto node=std::make_shared<DataStruct::Node>();
            node->totype=totype;node->ival=data;node->setKind(DataStruct::AST_TYPE::AST_LITERAL);
            emit_data_primtype(totype, node, depth);
            off += totype->size;
            size -= totype->size;
            if (i == inits.size())
                break;
        } else {
            off += node->totype->size;
            size -= node->totype->size;
        }
        if (v->getKind() == DataStruct::AST_TYPE::AST_ADDR) {
            emit_data_addr(v->unop, depth);
            continue;
        }
        if (v->getKind() == DataStruct::AST_TYPE::AST_LVAR && !v->lvarinit.empty()) {
            do_emit_data(v->lvarinit, v->getTy()->size, 0, depth);
            continue;
        }
        emit_data_primtype(node->totype, node->initval, depth);
    }
    emit_zero(size);
}
/**
 *     .data depth
 * .global glabel(如果不是static)
 *     glabel:
 *
 * @param v
 * @param off
 * @param depth
 */
void CodeGen::emit_data(const std::shared_ptr<DataStruct::Node> &v, int off, int depth) {
    SAVE;
    emit(".data %d", depth);
    if (!v->declvar->getTy()->isstatic)
        emit_noindent(".global %s", v->declvar->glabel);
    emit_noindent("%s:", v->declvar->glabel);
    do_emit_data(v->declinit, v->declvar->getTy()->size, off, depth);
}
/**
 *      .data
 *  .global glabel(node不是static）
 *      .lcomm glabel, size
 * @param v
 */
void CodeGen::emit_bss(const std::shared_ptr<DataStruct::Node> &v) {
    SAVE;
    emit(".data");
    if (!v->declvar->getTy()->isstatic)
        emit(".global %s", v->declvar->glabel);
    emit(".lcomm %s, %d", v->declvar->glabel, v->declvar->getTy()->size);
}

void CodeGen::emit_global_var(const std::shared_ptr<DataStruct::Node> &v) {
    SAVE;
    if (!v->declinit.empty())
        emit_data(v, 0, 0);
    else
        emit_bss(v);
}
/**
 * 保存rdi，rsi，rdx，rcx，r8,r9,xmm0-xmm7共14个寄存器
 * @return rsp偏移量
 */
int CodeGen::emit_regsave_area() {
    emit("sub $%d, #rsp", REGAREA_SIZE);
    emit("mov #rdi, (#rsp)");  //保存第一参数
    emit("mov #rsi, 8(#rsp)");  //第二参数
    emit("mov #rdx, 16(#rsp)");   //第三参数
    emit("mov #rcx, 24(#rsp)");    //第4参数
    emit("mov #r8, 32(#rsp)");     //第5参数
    emit("mov #r9, 40(#rsp)");     //第6参数
    emit("movaps #xmm0, 48(#rsp)");
    emit("movaps #xmm1, 64(#rsp)");
    emit("movaps #xmm2, 80(#rsp)");
    emit("movaps #xmm3, 96(#rsp)");
    emit("movaps #xmm4, 112(#rsp)");
    emit("movaps #xmm5, 128(#rsp)");
    emit("movaps #xmm6, 144(#rsp)");
    emit("movaps #xmm7, 160(#rsp)");
    return REGAREA_SIZE;
}
/**
 * 这个函数参数比较难看啊，总的来说是这样的：
 * rbp+16的开始处是参数开始的地方，因而首先arg设置为2
 * (1)如果是结构体或者union对象，那么肯定得保存到内存中，因而栈中保存的就是对象的内存基址，需要依次从吧内存中的数据拷贝到
 * rsp为基址的内存当中去
 * (2)如果是浮点类型，那么需要根据浮点数参数的数目来传递参数，如果数目小于8,那么就从xmm寄存器依次读取数据到栈顶，
 * ，否则就从rbp上面的站中读取数据到栈顶
 * (3)对剩下的类型，都是整型了，同样根据整型的数目来判断是直接从寄存器存取数据还是从rbp上面的栈中读取
 * 不过要注意的一点是对bool值对象要做zero拓展，然后读取就行了
 * 读取完参数以后，就要注意设置每个参数在当前地址的逻辑偏移了。
 * @param params
 * @param off
 */
void CodeGen::push_func_params(const std::vector<std::shared_ptr<DataStruct::Node>> params, int off) {
    int ireg = 0;
    int xreg = 0;
    int arg = 2;  //看了半天才明白，这个arg代表的是参数的起始偏移点
    //因为rbp的上面的第一个元素是上个过程的bp，再上一个是返回地址，因而这里初始值是2,代表2个元素
    for(auto&v:params){
        if (v->getTy()->kind == DataStruct::TYPE_KIND::KIND_STRUCT) {
            emit("lea %d(#rbp), #rax", arg * 8);  //rax的值变成了 rbp+arg*2,也就是不解引用
            int size = push_struct(v->getTy()->size);
            off -= size;
            arg += size / 8; //统计struct的偏移
        } else if (_parser->is_flotype(v->getTy())) {
            if (xreg >= 8) {
                emit("mov %d(#rbp), #rax", arg++ * 8);
                push("rax");
            } else {
                push_xmm(xreg++);
            }
            off -= 8;
        } else {
            if (ireg >= 6) {
                if (v->getTy()->kind == DataStruct::TYPE_KIND::KIND_BOOL) {
                    emit("mov %d(#rbp), #al", arg++ * 8);
                    emit("movzb #al, #eax");
                } else {
                    emit("mov %d(#rbp), #rax", arg++ * 8);
                }
                push("rax");
            } else {
                if (v->getTy()->kind == DataStruct::TYPE_KIND::KIND_BOOL)
                    emit("movzb #%s, #%s", SREGS[ireg], MREGS[ireg]);
                push(REGS[ireg++]);
            }
            off -= 8;
        }
        v->loff = off;
    }
}
/**
 * 保存rbp
 * 设置rbp为当前的rsp
 * 参数传送（struct只保存基址，其他的都是保存8个字节，因而off直接偏移params.size()*8)
 * 设置函数内部局部变量的栈，并设置其逻辑地址偏移
 * @param func
 */
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
    std::vector<std::shared_ptr<DataStruct::Node>> tv;
    for(auto &e:func->params)
        tv.push_back(std::make_shared<DataStruct::Node>(e));
    if (func->getTy()->hasva) {
        set_reg_nums(tv);
        off -= emit_regsave_area();
    }
    push_func_params(tv, off);
    off -= func->params.size() * 8;

    int localarea = 0;
    for(auto&v:func->localvars){
        int size = align(v.getTy()->size, 8);
        if(size%8)
            Error::error("%s is not aligned to base 8",_parser->node2s(std::make_shared<DataStruct::Node>(v)));
        off -= size;
        v.loff = off;
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