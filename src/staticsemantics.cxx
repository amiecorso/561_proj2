#include "ASTNode.h"
#include "Messages.h"

#include <iostream>
#include <unistd.h>  // getopt is here
#include <map>
#include <vector>

using namespace std;

struct method {
    string methodname;
    string returntype;
};

struct constructor {
    string constructorname;
};

struct instancevar {
    string instancename;
    string instancetype;
};

class TypeNode {
    public:
        string type;
        string parent;
        vector<string> children;
        vector<instancevar> instance_vars;
        vector<method> methods;
        constructor construct;

        TypeNode() {
            children = vector<string>();
            instance_vars = vector<instancevar>();
            methods = vector<method>();
        }

        TypeNode(string name) {
            type = name;
            children = vector<string>();
            instance_vars = vector<instancevar>();
            methods = vector<method>();
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
                cout << el->name_.text_ << endl;
            }
            // upon encountering new classes, create relevant class node in table
        }

        TablePointers* check() { // traverse and check AST, returning struct with pointers to tables
            TablePointers* tablePointers = new TablePointers();
            tablePointers->ch = &(this->hierarchy);
            tablePointers->vt = &(this->variabletypes);
            return tablePointers;
        }
};

