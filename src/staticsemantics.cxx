#include "ASTNode.h"
#include "Messages.h"

#include <iostream>
#include <unistd.h>  // getopt is here
#include <map>
#include <vector>

using namespace std;

class Method {
    public:
        string methodname;
        string returntype;
        vector<string> formalargtypes;

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
            //construct = Method(); <-- needs an arg
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
                if (hierarchy.count(classname)) { // already have a node for this
                    TypeNode node = hierarchy[classname];
                    node.parent = el->super_.text_; // update superclass
                    // children (nothing to do here)
                    // methods
                    vector<AST::Method *> methods = (el->methods_).elements_;
                    for (AST::Method *el: methods) {
                        AST::Ident* methodname = (AST::Ident*) &(el->name_);
                        AST::Ident* returntype = (AST::Ident*) &(el->returns_);
                        Method newmethod(methodname->text_);
                        newmethod.returntype = returntype->text_;
                        // TODO: we might need more info in our method struct!
                            // formal args? (signature)
                        node.methods.push_back(newmethod);
                    }
                    // TODO: wait, what is a constructor again?  an AST::Statements? or an AST::Method??
                        // how did michal do it?
                        // gonna need some formal args just like we need in Method
                    // should a constructor just be represented with a method struct in my TypeNode??
                        // it is basically the same thing
                    AST::Block* statements = (AST::Block*) &(el->constructor_);
                    //vector<AST::Statement *> constructor = statements->elements_;
                    // instancevars (packed in constructor)
                    // constructor construct
                }
                else {  // don't already have a node for this
                    TypeNode newtype(classname); // create new node
                    // populate attributes TODO: this is going to be same as above, split out
                    hierarchy[classname] = newtype; // add to table
                }
                // DEAL WITH SUPERCLASS
                string superclass = el->super_.text_;
                if (hierarchy.count(superclass)) { // superclass already in table
                    hierarchy[superclass].children.push_back(classname); // just update its subclasses
                }
                else { // superclass not already in table
                    // create node
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

