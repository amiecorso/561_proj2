// CodegenContext.h
#ifndef AST_CODEGENCONTEXT_H
#define AST_CODEGENCONTEXT_H

#include <ostream>
#include <map>

using namespace std;

class StaticSemantics;
namespace AST {class ASTNode; }

class Context {
    int next_reg_num = 0;
    int next_label_num = 0;
    map<string, string> local_vars;
    ostream &object_code;
public:
    string classname;
    string methodname;
    StaticSemantics* ssc;

    explicit Context(ostream &out, StaticSemantics* ss, string clsname, string methname) : 
        object_code{out}, ssc{ss}, classname{clsname}, methodname{methname} {};

    void emit(string s);

    string alloc_reg(string type);

    void free_reg(string reg);

    string get_local_var(string &ident);
    string get_type(AST::ASTNode& node);
    string new_branch_label(const char* prefix);
    void emit_instance_vars();
    string get_formal_argtypes(string methodname);
    void emit_method_sigs();
};

#endif //AST_CODEGENCONTEXT_H