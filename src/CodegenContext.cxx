// CodegenContext.cxx
#include <ostream>
#include <map>
#include "CodegenContext.h"
#include "staticsemantics.cxx"

using namespace std;

void Context::emit(string s) { object_code << " " << s  << endl; }

/* Getting the name of a "register" (really a local variable in C)
    * has the side effect of emitting a declaration for the variable.
    */
string Context::alloc_reg() {
    int reg_num = next_reg_num++;
    string reg_name = "tmp__" + to_string(reg_num);
    object_code << "int " << reg_name << ";" << endl;
    return reg_name;
}

void Context::free_reg(string reg) {
    this->emit(string("// Free ") + reg);
}

/* Get internal name for a calculator variable.
    * Possible side effect of generating a declaration if
    * the variable has not been mentioned before.  (Later,
    * we should buffer up the program to avoid this.)
    */
string Context::get_local_var(string &ident) {
    if (local_vars.count(ident) == 0) {
        string internal = string("var_") + ident;
        local_vars[ident] = internal;
        // We'll need a declaration in the generated code
        // find type of local var?
        TypeNode classnode = ssc->hierarchy[classname];
        MethodTable methodt = classnode.methods[methodname];
        map<string, string>* vars = methodt.vars;
        string type = (*vars)[ident];
        this->emit(string("obj_") + type + " " + internal + ";");
        return internal;
    }
    return local_vars[ident];
}

/* Get a new, unique branch label.  We use a prefix
    * string just to make the object code a little more
    * readable by indicating what the label was for
    * (e.g., distinguishing the 'else' branch from the 'endif'
    * branch).
    */
string Context::new_branch_label(const char* prefix) {
    return string(prefix) + "_" + to_string(++next_label_num);
}