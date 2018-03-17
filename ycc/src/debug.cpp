//
// Created by yk on 18-3-17.
//

#include "../include/debug.h"
#include "../include/code_gen.h"
namespace Utils {

    std::string encoding_prefix(DataStruct::ENCODE enc) {
        switch (enc) {
            case DataStruct::ENCODE::ENC_CHAR16:
                return "u";
            case DataStruct::ENCODE::ENC_CHAR32:
                return "U";
            case DataStruct::ENCODE::ENC_UTF8:
                return "u8";
            case DataStruct::ENCODE::ENC_WCHAR:
                return "L";
        }
        return "";
    }

    std::string tok2s(const DataStruct::Token &tok) {
        if (tok.kind == DataStruct::TOKEN_TYPE::TPLACEHOLDER)
            return "(null)";
        switch (tok.kind) {
            case DataStruct::TOKEN_TYPE::TIDENT:
                return *(tok.sval);
            case DataStruct::TOKEN_TYPE::TKEYWORD:
                switch (tok.id) {
#define op(id, str)         case DataStruct::AST_TYPE::id: return str;
#define keyword(id, str, _) case DataStruct::AST_TYPE::id: return str;

#include "../include/keyword.inc"

#undef keyword
#undef op
                    default:
                        return Lex::get_keywords_string(tok.id);
                }
            case DataStruct::TOKEN_TYPE::TCHAR:
                return Utils::format("%s'%s'",
                                     encoding_prefix(tok.enc),
                                     Utils::quote_char(tok.c));
            case DataStruct::TOKEN_TYPE::TNUMBER:
                return *(tok.sval);
            case DataStruct::TOKEN_TYPE::TSTRING:
                return format("%s\"%s\"",
                              encoding_prefix(tok.enc),
                              Utils::parse_cstring((*(tok.sval)).c_str()));
            case DataStruct::TOKEN_TYPE::TEOF:
                return "(eof)";
            case DataStruct::TOKEN_TYPE::TINVALID:
                return std::string{static_cast<char >(tok.c)};
            case DataStruct::TOKEN_TYPE::TNEWLINE:
                return "(newline)";
            case DataStruct::TOKEN_TYPE::TSPACE:
                return "(space)";
            case DataStruct::TOKEN_TYPE::TMACRO_PARAM:
                return "(macro-param)";
        }
        Error::error("internal error: unknown token kind: %d", static_cast<int>(tok.kind));
    }

//    std::string decorate_int(const std::string &name, const std::shared_ptr<DataStruct::Type> &ty) {
//        auto u = (ty->usig) ? "u" : "";
//        if (ty->bitsize > 0)
//            return format("%s%s:%d:%d", u, name, ty->bitoff, ty->bitoff + ty->bitsize);
//        return u + name;
//    }
//
//    std::string do_ty2s(std::unordered_map<std::shared_ptr<DataStruct::Type>, bool> &&dict,
//                        const std::shared_ptr<DataStruct::Type> &ty) {
//        if (!ty)
//            return "(nil)";
//        switch (ty->kind) {
//            case DataStruct::TYPE_KIND::KIND_VOID:
//                return "void";
//            case DataStruct::TYPE_KIND::KIND_BOOL:
//                return "_Bool";
//            case DataStruct::TYPE_KIND::KIND_CHAR:
//                return decorate_int("char", ty);
//            case DataStruct::TYPE_KIND::KIND_SHORT:
//                return decorate_int("short", ty);
//            case DataStruct::TYPE_KIND::KIND_INT:
//                return decorate_int("int", ty);
//            case DataStruct::TYPE_KIND::KIND_LONG:
//                return decorate_int("long", ty);
//            case DataStruct::TYPE_KIND::KIND_LLONG:
//                return decorate_int("llong", ty);
//            case DataStruct::TYPE_KIND::KIND_FLOAT:
//                return "float";
//            case DataStruct::TYPE_KIND::KIND_DOUBLE:
//                return "double";
//            case DataStruct::TYPE_KIND::KIND_LDOUBLE:
//                return "long double";
//            case DataStruct::TYPE_KIND::KIND_PTR:
//                return format("*%s", do_ty2s(std::move(dict), ty->ptr));
//            case DataStruct::TYPE_KIND::KIND_ARRAY:
//                return format("[%d]%s", ty->len, do_ty2s(std::move(dict), ty->ptr));
//            case DataStruct::TYPE_KIND::KIND_STRUCT: {
//                auto kind = ty->is_struct ? "struct" : "union";
//                if (dict.find(ty) != dict.end())
//                    return format("(%s)", kind);
//                dict[ty] = true;
//                if (!ty->fields.empty()) {
//                    std::string b;
//                    b += "(";
//                    b += kind;
//                    for (auto &ele:ty->fields) {
//                        auto fieldtype = ele.second;
//                        buf_printf(b, " (%s)", do_ty2s(std::move(dict), fieldtype));
//                    }
//                    b += ")";
//                    return b;
//                }
//            }
//            case DataStruct::TYPE_KIND::KIND_FUNC: {
//                std::string b;
//                b += "(";
//                if (!(ty->params.empty())) {
//                    for (int i = 0; i < ty->params.size(); i++) {
//                        if (i > 0)
//                            b += ",";
//                        auto t = ty->params[i];
//                        b += do_ty2s(std::move(dict), t);
//                    }
//                }
//                buf_printf(b, ")=>%s", do_ty2s(std::move(dict), ty->rettype));
//                return b;
//            }
//            default:
//                return format("(Unknown ty: %d)", static_cast<int >(ty->kind));
//        }
//    }
//
//    std::string ty2s(const std::shared_ptr<DataStruct::Type> &ty) {
//        return do_ty2s(std::unordered_map<std::shared_ptr<DataStruct::Type>, bool>(), ty);
//    }

//    void uop_to_string(std::string &b, std::string &&op, const std::shared_ptr<DataStruct::Node> &node) {
//        buf_printf(b, "(%s %s)", op, node2s(node->unop));
//    }
//
//    void binop_to_string(std::string &b, std::string &&op, const std::shared_ptr<DataStruct::Node> &node) {
//        buf_printf(b, "(%s %s %s)", op, node2s(node->left), node2s(node->right));
//    }
//
//    void a2s_declinit(std::string &b, const std::vector<std::shared_ptr<DataStruct::Node>> &initlist) {
//        for (int i = 0; i < initlist.size(); i++) {
//            if (i > 0)
//                b += " ";
//            b += node2s(initlist[i]);
//        }
//    }
//
//    void do_node2s(std::string &b, const std::shared_ptr<DataStruct::Node> &node) {
//        if (!node) {
//            b += "(nil)";
//            return;
//        }
//        switch (node->getKind()) {
//            case DataStruct::AST_TYPE::AST_LITERAL:
//                switch (node->getTy()->kind) {
//                    case DataStruct::TYPE_KIND::KIND_CHAR:
//                        if (node->ival == '\n') b += "'\n'";
//                        else if (node->ival == '\\') b += "'\\\\'";
//                        else if (node->ival == '\0') b += "'\\0'";
//                        else buf_printf(b, "'%c'", node->ival);
//                        break;
//                    case DataStruct::TYPE_KIND::KIND_INT:
//                        b += std::to_string(node->ival);
//                        break;
//                    case DataStruct::TYPE_KIND::KIND_LONG:
//                        b += std::to_string(node->ival) + "L";
//                        break;
//                    case DataStruct::TYPE_KIND::KIND_LLONG:
//                        b += std::to_string(node->ival) + "L";
//                        break;
//                    case DataStruct::TYPE_KIND::KIND_FLOAT:
//                    case DataStruct::TYPE_KIND::KIND_DOUBLE:
//                    case DataStruct::TYPE_KIND::KIND_LDOUBLE:
//                        b += std::to_string(node->fval);
//                        break;
//                    case DataStruct::TYPE_KIND::KIND_ARRAY:
//                        b += format("\"%s\"", node->sval);
//                        break;
//                    default:
//                        Error::error("internal error");
//                }
//                break;
//            case DataStruct::AST_TYPE::AST_LABEL:
//                b += node->label + ":";
//                break;
//            case DataStruct::AST_TYPE::AST_LVAR:
//                buf_printf(b, "lv=%s", node->varname);
//                if (!node->lvarinit.empty()) {
////                    std::vector<std::shared_ptr<DataStruct::Node>> v;
////                    for(auto e:node->lvarinit)
////                        v.push_back(std::make_shared<DataStruct::Node>(e));
//                    b += "(";
//                    a2s_declinit(b, node->lvarinit);
//                    b += ")";
//                }
//                break;
//            case DataStruct::AST_TYPE::AST_GVAR:
//                buf_printf(b, "gv=%s", node->varname);
//                break;
//            case DataStruct::AST_TYPE::AST_FUNCALL:
//            case DataStruct::AST_TYPE::AST_FUNCPTR_CALL: {
//                buf_printf(b, "(%s)%s(", ty2s(node->getTy()),
//                           node->getKind() == DataStruct::AST_TYPE::AST_FUNCALL ? node->fname : node2s(node));
//                for (int i = 0; i < node->args.size(); i++) {
//                    if (i > 0)
//                        b += ",";
//                    b += node2s(std::make_shared<DataStruct::Node>(node->args[i]));
//                }
//                b += ")";
//                break;
//            }
//            case DataStruct::AST_TYPE::AST_FUNCDESG: {
//                buf_printf(b, "(funcdesg %s)", node->fname);
//                break;
//            }
//            case DataStruct::AST_TYPE::AST_FUNC: {
//                buf_printf(b, "(%s)%s(", ty2s(node->getTy()), node->fname);
//                for (int i = 0; i < node->params.size(); i++) {
//                    if (i > 0)
//                        b += ",";
//                    buf_printf(b, "%s %s", ty2s(node->params[i].getTy()),
//                               node2s(std::make_shared<DataStruct::Node>(node->params[i])));
//                }
//                b += ")";
//                do_node2s(b, node->body);
//                break;
//            }
//            case DataStruct::AST_TYPE::AST_GOTO:
//                buf_printf(b, "goto(%s)", node->label);
//                break;
//            case DataStruct::AST_TYPE::AST_DECL:
//                buf_printf(b, "(decl %s %s",
//                           ty2s(node->declvar->getTy()),
//                           node->declvar->varname);
//                if (!node->declinit.empty()) {
//                    b += " ";
//                    a2s_declinit(b, node->declinit);
//                }
//                b += ")";
//                break;
//            case DataStruct::AST_TYPE::AST_INIT:
//                buf_printf(b, "%s@%d", node2s(node->initval), node->initoff, ty2s(node->totype));
//                break;
//            case DataStruct::AST_TYPE::AST_CONV:
//                buf_printf(b, "(conv %s=>%s)", node2s(node->unop), ty2s(node->getTy()));
//                break;
//            case DataStruct::AST_TYPE::AST_IF:
//                buf_printf(b, "(if %s %s",
//                           node2s(node->cond),
//                           node2s(node->then));
//                if (node->els)
//                    b += node2s(node->els);
//                b += ")";
//                break;
//            case DataStruct::AST_TYPE::AST_TERNARY:
//                buf_printf(b, "(? %s %s %s)",
//                           node2s(node->cond),
//                           node2s(node->then),
//                           node2s(node->els));
//                break;
//            case DataStruct::AST_TYPE::AST_RETURN:
//                buf_printf(b, "(return %s)", node2s(node->retval));
//                break;
//            case DataStruct::AST_TYPE::AST_COMPOUND_STMT: {
//                b += "{";
//                for (auto &e:node->stmts) {
//                    do_node2s(b, e);
//                    b += ";";
//                }
//                b += "}";
//                break;
//            }
//            case DataStruct::AST_TYPE::AST_STRUCT_REF:
//                do_node2s(b, node->struc);
//                b += ".";
//                b += node->field;
//                break;
//            case DataStruct::AST_TYPE::AST_ADDR:
//                uop_to_string(b, "addr", node);
//                break;
//            case DataStruct::AST_TYPE::AST_DEREF:
//                uop_to_string(b, "deref", node);
//                break;
//            case DataStruct::AST_TYPE::OP_SAL:
//                binop_to_string(b, "<<", node);
//                break;
//            case DataStruct::AST_TYPE::OP_SAR:
//            case DataStruct::AST_TYPE::OP_SHR:
//                binop_to_string(b, ">>", node);
//                break;
//            case DataStruct::AST_TYPE::OP_GE:
//                binop_to_string(b, ">=", node);
//                break;
//            case DataStruct::AST_TYPE::OP_LE:
//                binop_to_string(b, "<=", node);
//                break;
//            case DataStruct::AST_TYPE::OP_NE:
//                binop_to_string(b, "!=", node);
//                break;
//            case DataStruct::AST_TYPE::OP_PRE_INC:
//                uop_to_string(b, "pre++", node);
//                break;
//            case DataStruct::AST_TYPE::OP_PRE_DEC:
//                uop_to_string(b, "pre--", node);
//                break;
//            case DataStruct::AST_TYPE::OP_POST_INC:
//                uop_to_string(b, "post++", node);
//                break;
//            case DataStruct::AST_TYPE::OP_POST_DEC:
//                uop_to_string(b, "post--", node);
//                break;
//            case DataStruct::AST_TYPE::OP_LOGAND:
//                binop_to_string(b, "and", node);
//                break;
//            case DataStruct::AST_TYPE::OP_LOGOR:
//                binop_to_string(b, "or", node);
//                break;
//            case DataStruct::AST_TYPE::OP_A_ADD:
//                binop_to_string(b, "+=", node);
//                break;
//            case DataStruct::AST_TYPE::OP_A_SUB:
//                binop_to_string(b, "-=", node);
//                break;
//            case DataStruct::AST_TYPE::OP_A_MUL:
//                binop_to_string(b, "*=", node);
//                break;
//            case DataStruct::AST_TYPE::OP_A_DIV:
//                binop_to_string(b, "/=", node);
//                break;
//            case DataStruct::AST_TYPE::OP_A_MOD:
//                binop_to_string(b, "%=", node);
//                break;
//            case DataStruct::AST_TYPE::OP_A_AND:
//                binop_to_string(b, "&=", node);
//                break;
//            case DataStruct::AST_TYPE::OP_A_OR:
//                binop_to_string(b, "|=", node);
//                break;
//            case DataStruct::AST_TYPE::OP_A_XOR:
//                binop_to_string(b, "^=", node);
//                break;
//            case DataStruct::AST_TYPE::OP_A_SAL:
//                binop_to_string(b, "<<=", node);
//                break;
//            case DataStruct::AST_TYPE::OP_A_SAR:
//            case DataStruct::AST_TYPE::OP_A_SHR:
//                binop_to_string(b, ">>=", node);
//                break;
//            case DataStruct::AST_TYPE::EXCLAMATION:
//                uop_to_string(b, "!", node);
//                break;
//            case DataStruct::AST_TYPE::AND:
//                binop_to_string(b, "&", node);
//                break;
//            case DataStruct::AST_TYPE::OR:
//                binop_to_string(b, "|", node);
//                break;
//            case DataStruct::AST_TYPE::OP_CAST: {
//                buf_printf(b, "((%s)=>(%s) %s)",
//                           ty2s(node->unop->getTy()),
//                           ty2s(node->getTy()),
//                           node2s(node->unop));
//                break;
//            }
//            case DataStruct::AST_TYPE::OP_LABEL_ADDR:
//                buf_printf(b, "&&%s", node->label);
//                break;
//            default: {
//                auto left = node2s(node->left);
//                auto right = node2s(node->right);
//                if (node->getKind() == DataStruct::AST_TYPE::OP_EQ)
//                    b += "(== ";
//                else
//                    buf_printf(b, "(%c ", static_cast<char >(node->getKind()));
//                buf_printf(b, "%s %s)", left, right);
//            }
//        }
//    }
//
//    std::string node2s(const std::shared_ptr<DataStruct::Node> &node) {
//        std::string b;
//        do_node2s(b, node);
//        return b;
//    }
}