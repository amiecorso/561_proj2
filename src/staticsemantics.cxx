#include "ASTNode.h"
#include "Messages.h"

#include <iostream>
//#include <unistd.h>
#include <map>
#include <vector>

using namespace std;

class Method {
    public:
        string methodname;
        string returntype;
        vector<string> formalargtypes;
        // TODO: a method also needs to know the class-scope instance variables

        Method () {
            formalargtypes = vector<string>();
        }

        Method(string name) {
            methodname = name;
            formalargtypes = vector<string>();
        }

        void print() {
            cout << '\t' << "method name: " << methodname << endl;
            cout << '\t' << "return type: " << returntype << endl;
            cout << '\t' << "formal arg types: ";
            for (string formalarg: formalargtypes) {
                cout << formalarg << ", ";
            }
            cout << endl << endl;
        }
};

class TypeNode {
    public:
        string type;
        string parent;
        vector<string> children;
        vector<string> instance_vars;
        map<string, Method> methods;
        Method construct;

        TypeNode() {
            children = vector<string>();
            instance_vars = vector<string>();
            methods = map<string, Method>();
            construct = Method();
        }

        TypeNode(string name) {
            type = name;
            children = vector<string>();
            instance_vars = vector<string>();
            methods = map<string, Method>();
            construct = Method(name);
        }

        void print() {
            cout << "Type: " << type << endl;
            cout << "Parent: " << parent << endl;
            cout << "Children: ";
            for (string child: children) {
                cout << child << ", ";
            }
            cout << endl << "Instance vars: ";
            for (string var: instance_vars) {
                cout << var << ", ";
            }
            cout << endl << "Methods: " << endl;
            for(std::map<string, Method>::iterator iter = methods.begin(); iter != methods.end(); ++iter) {
                Method method =  iter->second;
                method.print();
            }
            cout << "Constructor: " << endl;
            construct.print();
        }
};

struct TablePointers{
    map<string, TypeNode>* ch;
    map<string, string>* vt;
};

class StaticSemantics {
    AST::ASTNode* astroot;
    int found_error;
    map<string, TypeNode> hierarchy;
    map<string, string> variabletypes;

    public:
        StaticSemantics(AST::ASTNode* root) { // parameterized constructor
            astroot = root;
            found_error = 0;
            hierarchy = map<string, TypeNode>();
            variabletypes = map<string, string>();
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
            // create obj node?
            TypeNode obj("Obj");
            obj.parent = "TYPE_ERROR";
            obj.children.push_back("Int");
            obj.children.push_back("String");
            obj.children.push_back("Boolean");
            obj.children.push_back("Nothing");
            hierarchy["Obj"] = obj; // insert into class hierarchy map

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
            // traverse the tree
            AST::Program *root = (AST::Program*) astroot;
            AST::Classes classesnode = root->classes_;
            vector<AST::Class *> classes = classesnode.elements_;
            for (AST::Class *el: classes) {
                //cout << el->name_.text_ << endl;
                string classname = el->name_.text_;
                TypeNode node;
                if (hierarchy.count(classname)) { // if already in table
                    node = hierarchy[classname]; // just fetch that node
                }
                else {
                    node = TypeNode(classname); // otherwise create new node
                }
                node.parent = el->super_.text_; // update superclass
                // children (nothing to do here)
                // methods
                vector<AST::Method *> methods = (el->methods_).elements_;
                for (AST::Method *meth: methods) {
                    AST::Ident* methodname = (AST::Ident*) &(meth->name_);
                    AST::Ident* returntype = (AST::Ident*) &(meth->returns_);
                    Method newmethod(methodname->text_);
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

                // DEAL WITH SUPERCLASS (DON'T ACTUALLY NEED TO KNOW CHILDREN!!)
                string superclass = el->super_.text_;
                if (hierarchy.count(superclass)) { // superclass already in table
                    hierarchy[superclass].children.push_back(classname); // just update its subclasses
                }
                else { // superclass not already in table
                    // create node and put in table
                    TypeNode superclassnode(superclass);
                    superclassnode.children.push_back(classname);
                    hierarchy[superclass] = superclassnode;
                }
            } // end for class in classes

            printClassHierarchy();

        } // end populateClassHierarchy

        TablePointers* check() { // traverse and check AST, returning struct with pointers to tables
            TablePointers* tablePointers = new TablePointers();
            tablePointers->ch = &(this->hierarchy);
            tablePointers->vt = &(this->variabletypes);
            return tablePointers;
        }
};

