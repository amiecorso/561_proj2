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
};

class TypeNode {
    public:
        string type;
        string parent;
        vector<string> children;
        vector<string> instance_vars;
        vector<Method> methods;
        Method construct;

        TypeNode() {
            children = vector<string>();
            instance_vars = vector<string>();
            methods = vector<Method>();
            construct = Method();
        }

        TypeNode(string name) {
            type = name;
            children = vector<string>();
            instance_vars = vector<string>();
            methods = vector<Method>();
            construct = Method(name);
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
            hierarchy["Int"] = integer;
            TypeNode str("String");
            hierarchy["String"] = str;
            TypeNode boolean("Boolean");
            hierarchy["Boolean"] = boolean;
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
                    node.methods.push_back(newmethod);
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
                    cout << "IN THE LOOP" << endl;
                    cout << stmt->nodetype << endl;
                    //string nodetype = stmt->get_node_type();
                    //cout << nodetype << endl;
                    // which type of statement do we care about?
                    // push found instance vars onto node.instance_vars (vector of strings)
                }

                hierarchy[classname] = node; // finally, add node to table

                // DEAL WITH SUPERCLASS
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
            }

        }

        TablePointers* check() { // traverse and check AST, returning struct with pointers to tables
            TablePointers* tablePointers = new TablePointers();
            tablePointers->ch = &(this->hierarchy);
            tablePointers->vt = &(this->variabletypes);
            return tablePointers;
        }

        // TODO: method that prints out the class hierarchy table in a useful way
};

