//
// A code generation context carries information that we need to maintain
// while walking the AST to generate code.  A context is passed along from
// from node to node as the tree is walked.
//
// For this small calculator example (with the rather impractical
// approach of evaluating some simple expressions by translating
// them to C code and then compiling and running the C code) we
// need an output stream and a table of variable values.
//

#ifndef AST_CODEGENCONTEXT_H
#define AST_CODEGENCONTEXT_H

#include <ostream>
#include <map>

using namespace std;

class Context {
    // In place of registers, we'll use local integer variables.
    // Declarations are tricky if we reuse variable names, so we'll
    // just create as many temporaries as we need.
    int next_reg_num = 0;
    int next_label_num = 0;
    map<string, string> local_vars;
    ostream &object_code;
public:
    explicit Context(ostream &out) : object_code{out} {};
    void emit(string s) { object_code << " " << s  << endl; }

    /* Getting the name of a "register" (really a local variable in C)
     * has the side effect of emitting a declaration for the variable.
     */
    string alloc_reg() {
        int reg_num = next_reg_num++;
        string reg_name = "tmp__" + to_string(reg_num);
        object_code << "int " << reg_name << ";" << endl;
        return reg_name;
    }

    void free_reg(string reg) {
        // We don't have real registers, so there is nothing to free.
        this->emit(string("// Free ") + reg);
    }

    /* Get internal name for a calculator variable.
     * Possible side effect of generating a declaration if
     * the variable has not been mentioned before.  (Later,
     * we should buffer up the program to avoid this.)
     */
    string get_local_var(string &ident) {
        if (local_vars.count(ident) == 0) {
            string internal = string("calc_var_") + ident;
            local_vars[ident] = internal;
            // We'll need a declaration in the generated code
            this->emit(string("int ") + internal + "; // Source variable " + ident);
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
    string new_branch_label(const char* prefix) {
        return string(prefix) + "_" + to_string(++next_label_num);
    }

};


#endif //AST_CODEGENCONTEXT_H
