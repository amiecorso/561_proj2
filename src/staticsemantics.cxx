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
    string type;
    string parent;
    vector<string> children;
    vector<instancevar> instance_vars;
    vector<method> methods;
    constructor construct;
};

typedef struct {
    map<string, TypeNode>* ch;
    map<string, string>* vt;
} TablePointers;

class StaticSemantics {
    AST::ASTNode* astroot;
    map<string, TypeNode>* classhierarchy;
    map<string, string>* variabletypes;

    public:
        StaticSemantics() { // default constructor ??
            astroot = NULL;
            classhierarchy = new map<string, TypeNode>();
            variabletypes = new map<string, string>();
        }
        StaticSemantics(AST::ASTNode* root) { // parameterized constructor
            astroot = root;
            classhierarchy = new map<string, TypeNode>();
            variabletypes = new map<string, string>();
        }
        // TODO: create destructor?

        void populateClassHierarchy() { // create class hierarchy


        }

        TablePointers* check() { // traverse and check AST, returning struct with pointers to tables
            TablePointers* tablePointers = new TablePointers();
            tablePointers->ch = this->classhierarchy; // TODO what's wrong with this???
            tablePointers->vt = this->variabletypes;
        }
};

