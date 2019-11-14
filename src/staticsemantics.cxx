#include "ASTNode.h"
#include "Messages.h"

#include <iostream>
#include <unistd.h>  // getopt is here
#include <map>

typedef struct {
    std::map<std::string, ClassNode>* ch;
    std::map<std::string, std::string>* vt;
} TablePointers;


class ClassNode {
    std::string name;
    std::string parent;
    std::string child;
};

class StaticSemantics {
    AST::ASTNode* astroot;
    std::map<std::string, ClassNode>* classhierarchy;
    std::map<std::string, std::string>* variabletypes;

    public:
        StaticSemantics() { // default constructor ??
            astroot = NULL;
            classhierarchy = new std::map<std::string, ClassNode>();
            variabletypes = new std::map<std::string, std::string>();
        }
        StaticSemantics(AST::ASTNode* root) { // parameterized constructor
            astroot = root;
            classhierarchy = new std::map<std::string, ClassNode>();
            variabletypes = new std::map<std::string, std::string>();
        }
        // TODO: create destructor?

        void populateClassHierarchy() { // create class hierarchy


        }

        TablePointers* check() { // traverse and check AST
            TablePointers* tablePointers = new TablePointers();
            tablePointers->ch = this->classhierarchy;
            tablePointers->vt = this->variabletypes;
        }
};

