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
        map<string, string> vars;
        // TODO: a method also needs to know the class-scope instance variables
            // do we put these in vars, or keep a separate table?  maybe put in vars??

        MethodTable () {
            formalargtypes = vector<string>();
            vars = map<string, string>();
        }

        MethodTable(string name) {
            methodname = name;
            formalargtypes = vector<string>();
            vars = map<string, string>();
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
            for(std::map<string, string>::iterator iter = vars.begin(); iter != vars.end(); ++iter) {
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
            for(std::map<string, string>::iterator iter = instance_vars.begin(); iter != instance_vars.end(); ++iter) {
                cout << "\t" << iter->first << ":" << iter->second << endl;
            }
            cout << "Methods: " << endl;
            for(std::map<string, MethodTable>::iterator iter = methods.begin(); iter != methods.end(); ++iter) {
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

        void populateClassHierarchy() { // create class hierarchy
            // Populate built-ins
            TypeNode obj("Obj");
            obj.parent = "TYPE_ERROR";
            hierarchy["Obj"] = obj;
            TypeNode integer("Int");
            integer.parent = "Obj";
            hierarchy["Int"] = integer;
            TypeNode str("String");
            str.parent = "Obj";
            hierarchy["String"] = str;
            TypeNode boolean("Boolean");
            boolean.parent = "Obj";
            hierarchy["Boolean"] = boolean;
            TypeNode nothing("Nothing");
            nothing.parent = "Obj";

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

                hierarchy[classname] = node; // finally, add node to table

            } // end for class in classes
            printClassHierarchy();

        } // end populateClassHierarchy

        // TODO: make sure class hierarchy is ACYCLIC

        void copy_instance_vars(string classname) { // copy class instance vars into method tables
            cout << "ENTERING ssc::copy_instance_vars" << endl;
            TypeNode classtable = hierarchy[classname];
            map<string, string> instancevars = classtable.instance_vars;
            map<string, MethodTable> methods = classtable.methods;
            for(map<string, MethodTable>::iterator miter = methods.begin(); miter != methods.end(); ++miter) {
                map<string, string> methodvars = (miter->second).vars;
                for(map<string, string>::iterator viter = instancevars.begin(); viter != instancevars.end(); ++viter) {
                    methodvars[viter->first] = viter->second;
                } // end for each instance var
            } // end for each method
        }

        string get_LCA(string type1, string type2) {
            cout << "ENTERING: ssc::get_LCA" << type1 << type2 << flush;
            cout << "type1: ";
            cout << type1 << endl;
            cout << "type2: ";
            cout << type2 << endl;
            set<string> type1_path = set<string>();
            string type = type1;
            while (1) {
                type1_path.insert(type);
                if (type == "Obj") { break; }
                type = hierarchy[type].parent;
            }
            cout << "ssc::get_LCA past first while loop" << endl;

            type = type2;
            while (1) {
                if (type1_path.count(type)) {
                    return type;
                }
                type = hierarchy[type].parent;
            }
        }

        map<string, TypeNode>* typeCheck() {
            // Traverse the classes
            // for each class
                // type check its constructor (instance_vars / local vars.. two tables?!)
                // copy instance vars into method tables
                // for each METHOD:
                    // type check the method
            AST::Program *root = (AST::Program*) astroot;
            int changed = 1;
            while (changed) {
                changed = 0; // eventually make this call to type_check on root node
            }
            AST::Classes classesnode = root->classes_;
            vector<AST::Class *> classes = classesnode.elements_;
            for (AST::Class *cls: classes) {
                cout << "IN SSC::typeCheck FOR" << endl;
                cls->type_check(this, nullptr);
            } // end for

            return &this->hierarchy;
        } // end typeCheck

};

