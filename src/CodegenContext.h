// CodegenContext.h
#ifndef AST_CODEGENCONTEXT_H
#define AST_CODEGENCONTEXT_H

#include <ostream>
#include <map>

using namespace std;

class StaticSemantics;

class Context {
    int next_reg_num = 0;
    int next_label_num = 0;
    map<string, string> local_vars;
    ostream &object_code;
    string classname;
    string methodname;
    StaticSemantics* ssc;
public:
    explicit Context(ostream &out, StaticSemantics* ss, string clsname, string methname) : 
        object_code{out}, ssc{ss}, classname{clsname}, methodname{methname} {};

    void emit(string s);

    string alloc_reg();

    void free_reg(string reg);

    string get_local_var(string &ident);

    string new_branch_label(const char* prefix);
};

#endif //AST_CODEGENCONTEXT_H