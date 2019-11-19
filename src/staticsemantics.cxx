#include "ASTNode.h"
#include "Messages.h"

#include <iostream>
#include <map>
#include <vector>
#include <set>

using namespace std;

class MethodTable {
    public:
        string methodname;
        string returntype;
        vector<string> formalargtypes;
        map<string, string>* vars;

        MethodTable () {
            formalargtypes = vector<string>();
            vars = new map<string, string>();
        }

        MethodTable(string name) {
            methodname = name;
            formalargtypes = vector<string>();
            vars = new map<string, string>();
        }

        void print() {
            cout << '\t' << "method name: " << methodname << endl;
            cout << '\t' << "return type: " << returntype << endl;
            cout << '\t' << "formal arg types: ";
            for (string formalarg: formalargtypes) {
                cout << formalarg << ", ";
            }
            cout << endl;
            cout << "\t" << "variables: " << endl;
            for(std::map<string, string>::iterator iter = vars->begin(); iter != vars->end(); ++iter) {
                cout << "\t\t" << iter->first << ":" << iter->second << endl;
            }
            cout << endl;
        }
};

class TypeNode {
    public:
        string type;
        string parent;
        map<string, string> instance_vars;
        map<string, MethodTable> methods;
        MethodTable construct;

        TypeNode() {
            instance_vars = map<string, string>();
            methods = map<string, MethodTable>();
            construct = MethodTable();
        }

        TypeNode(string name) {
            type = name;
            instance_vars = map<string, string>();
            methods = map<string, MethodTable>();
            construct = MethodTable(name);
        }

        void print() {
            cout << "Type: " << type << endl;
            cout << "Parent: " << parent << endl;
            cout << "Instance vars: " << endl;;
            for(map<string, string>::iterator iter = instance_vars.begin(); iter != instance_vars.end(); ++iter) {
                cout << "\t" << iter->first << ":" << iter->second << endl;
            }
            cout << "Methods: " << endl;
            for(map<string, MethodTable>::iterator iter = methods.begin(); iter != methods.end(); ++iter) {
                MethodTable method =  iter->second;
                method.print();
            }
            cout << "Constructor: " << endl;
            construct.print();
        }
};

class StaticSemantics {
    public:
        AST::ASTNode* astroot;
        int found_error;
        int change_made = 1;
        map<string, TypeNode> hierarchy;

        StaticSemantics(AST::ASTNode* root) { // parameterized constructor
            astroot = root;
            found_error = 0;
            hierarchy = map<string, TypeNode>();
        }
        // TODO: create destructor?

        void printClassHierarchy() {
            cout << "=========CLASS HIERARCHY============" << endl;
            for(map<string,TypeNode>::iterator iter = hierarchy.begin(); iter != hierarchy.end(); ++iter) {
                TypeNode node = iter->second;
                node.print();
                cout << "===================================" << endl;
            }
        }

        void populateBuiltins() {
            // Populate built-ins
            TypeNode obj("Obj");
            obj.parent = "TYPE_ERROR";
            hierarchy["Obj"] = obj;

            TypeNode integer("Int");
            integer.parent = "Obj";
            hierarchy["Int"] = integer;
            MethodTable newmethod("PLUS");
            newmethod.returntype = "Int";
            hierarchy["Int"].methods["PLUS"] = newmethod;

            TypeNode str("String");
            str.parent = "Obj";
            hierarchy["String"] = str;

            TypeNode boolean("Boolean");
            boolean.parent = "Obj";
            hierarchy["Boolean"] = boolean;

            TypeNode nothing("Nothing");
            nothing.parent = "Obj";
        }

        void populateClassHierarchy() { // create class hierarchy
            populateBuiltins();

            // Traverse the classes
            AST::Program *root = (AST::Program*) astroot;
            AST::Classes classesnode = root->classes_;
            vector<AST::Class *> classes = classesnode.elements_;
            for (AST::Class *el: classes) {
                string classname = el->name_.text_;
                TypeNode node;
                if (hierarchy.count(classname)) { // if already in table
                    node = hierarchy[classname]; // just fetch that node
                }
                else {
                    node = TypeNode(classname); // otherwise create new node
                }
                node.parent = el->super_.text_; // update superclass

                // constructor (node.construct already exists and is a Method object with name initialized)
                AST::Method *constructor = (AST::Method *) &(el->constructor_);
                AST::Ident *returnt = (AST::Ident*) &(constructor->returns_);
                node.construct.returntype = returnt->text_; // fill out returntype
                AST::Formals* formalsnode = (AST::Formals*) &(constructor->formals_);
                vector<AST::Formal *> formals = formalsnode->elements_;
                for (AST::Formal *formal: formals) {
                        AST::Ident *type = (AST::Ident *) &(formal->type_);
                        node.construct.formalargtypes.push_back(type->text_); // populate formal arg types
                    }

                // instancevars (can be found in the statements_ attribute (AST::Block node) of the constructor AST::Method node)
                AST::Block* blocknode = (AST::Block*) &(constructor->statements_);
                vector<AST::Statement *> *statements = (vector<AST::Statement *> *) &blocknode->elements_;
                vector<AST::Statement *> stmts = *statements;

                for (AST::Statement *stmt: stmts) {
                    stmt->collect_vars(&node.instance_vars);
                } 

                // methods 
                vector<AST::Method *> methods = (el->methods_).elements_;
                for (AST::Method *meth: methods) {
                    AST::Ident* methodname = (AST::Ident*) &(meth->name_);
                    AST::Ident* returntype = (AST::Ident*) &(meth->returns_);
                    MethodTable newmethod(methodname->text_);
                    newmethod.returntype = returntype->text_;
                    AST::Formals* formalsnode = (AST::Formals*) &(meth->formals_);
                    vector<AST::Formal *> formals = formalsnode->elements_;
                    for (AST::Formal *formal: formals) {
                        AST::Ident *type = (AST::Ident *) &(formal->type_);
                        newmethod.formalargtypes.push_back(type->text_);
                    }
                    node.methods[newmethod.methodname] = newmethod;
                }
                hierarchy[classname] = node; // finally, add node to table

            } // end for class in classes
            cout << " *********************** CH BEFORE TYPE CHECKING *****************" << endl;
            printClassHierarchy();

        } // end populateClassHierarchy

        // TODO: make sure class hierarchy is ACYCLIC

        void copy_instance_vars(string classname) { // copy class instance vars into method tables
            cout << "ENTERING ssc::copy_instance_vars WTFFFFFFFFF" << endl;
            TypeNode classtable = hierarchy[classname];
            //cout << "\tgot classtable:" << endl;
            //classtable.print();
            map<string, string> instancevars = classtable.instance_vars;
            cout << "\t got instancevars: " << endl;
            for(map<string, string>::iterator iviter = instancevars.begin(); iviter != instancevars.end(); ++iviter) {
                cout << iviter->first << " : " << iviter->second << endl;
            }
            map<string, MethodTable> methods = classtable.methods;
            for(map<string, MethodTable>::iterator miter = methods.begin(); miter != methods.end(); ++miter) {
                MethodTable mt = miter->second;
                cout << "\t got methodtable:" << endl;
                mt.print();
                map<string, string>* methodvars = mt.vars;
                //(miter->second).vars = map<string, string>(instancevars);
                //cout << "\tassigning: " << endl;
                for(map<string, string>::iterator iviter = instancevars.begin(); iviter != instancevars.end(); ++iviter) {
                    //cout << iviter->first << " : " << iviter->second << endl;
                    (*methodvars)[iviter->first] = iviter->second;
                }
                cout << "\t after assignment: ---------" << endl;
                mt.print();
            }
        }

        int is_subtype(string sub, string super) {
            // return 1 if sub is substype of super, 0 otherwise
            set<string> sub_path_to_root = set<string>();
            string type = sub;
            if (!hierarchy.count(type)) { // subtype not in class hierarchy
                cout << "ERROR: type " << type << " not in class hierarchy" << endl;
                return 0;
            }
            while (1) {
                sub_path_to_root.insert(type);
                if (type == "Obj") { break; }
                type = hierarchy[type].parent;
            }
            if (sub_path_to_root.count(super)) {
                return 1;
            }
            return 0;
        }

        string get_LCA(string type1, string type2) {
            cout << "ENTERING: ssc::get_LCA"  << endl;
            cout << "\t type1: " << type1 << endl;
            cout << "\t type2: " << type2 << endl;
            // TODO: this section is garbage and shouldn't be necessary when rest is working
            if (type1 == "Bottom") { return type2;}
            if (type2 == "Bottom") { return type1;}
            if (type1 == "TypeError") { return type2;}
            if (type2 == "TypeError") { return type1;}
            if (!hierarchy.count(type1)) { // if we have a type that is NOT in the table...
                return type1; // for now we're just going to call it that type
            }
            if (!hierarchy.count(type2)) { // if we have a type that is NOT in the table...
                return type2; // for now we're just going to call it that type
            }
            set<string> type1_path = set<string>();
            string type = type1;
            while (1) {
                type1_path.insert(type);
                if (type == "Obj") { break; }
                type = hierarchy[type].parent;
            }
            type = type2;
            while (1) {
                if (type1_path.count(type)) {
                    return type;
                }
                type = hierarchy[type].parent;
            }
        }

        map<string, TypeNode>* typeCheck() {
            AST::Program *root = (AST::Program*) astroot;
            AST::Classes classesnode = root->classes_;
            vector<AST::Class *> classes = classesnode.elements_;
            while (change_made) {
                change_made = 0; 
                // TODO eventually make this call to type_infer on root node
                for (AST::Class *cls: classes) {
                    cout << "IN SSC::typeCheck FOR" << endl;
                    cls->type_infer(this, nullptr, cls->name_.get_var());
                }
            }
            return &this->hierarchy;
        } // end typeCheck
}; 

