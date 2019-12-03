#include "ASTNode.h"
#include <string>
#include <sstream>
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
        string inheritedfrom;

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
            for(map<string, string>::iterator iter = vars->begin(); iter != vars->end(); ++iter) {
                cout << "\t\t" << iter->first << ":" << iter->second << endl;
            }
            cout << "\t" << "inheritedfrom: " << inheritedfrom << endl;
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
        int resolved;
        vector<string> methodlist;

        TypeNode() {
            instance_vars = map<string, string>();
            methods = map<string, MethodTable>();
            construct = MethodTable();
            resolved = 0;
            methodlist = vector<string>();
        }

        TypeNode(string name) {
            type = name;
            instance_vars = map<string, string>();
            methods = map<string, MethodTable>();
            construct = MethodTable(name);
            construct.returntype = name;
            resolved = 0;
            methodlist = vector<string>();
        }

        void print() {
            cout << "Type: " << type << endl;
            cout << "Parent: " << parent << endl;
            cout << "Instance vars: " << endl;;
            for(map<string, string>::iterator iter = instance_vars.begin(); iter != instance_vars.end(); ++iter) {
                cout << "\t" << iter->first << ":" << iter->second << endl;
            }
            cout << "MethodList: ";
            for (string meth: methodlist) {
                cout << meth << ", ";
            }
            cout << endl;
            cout << "Methods: " << endl;
            for(map<string, MethodTable>::iterator iter = methods.begin(); iter != methods.end(); ++iter) {
                MethodTable method =  iter->second;
                method.print();
            }
            cout << "Constructor: " << endl;
            construct.print();
        }
};

class Edge {
    public:
        vector<string> children;
        int visited;

        Edge() {
            children = vector<string>();
            visited = 0;
        }

        void print() {
            cout << "children: ";
            for (string child: children) {
                cout << child << ", ";
            }
            cout << endl;
            cout << "visited: " << visited << endl;
        }
};

class StaticSemantics {
    public:
        AST::ASTNode* astroot;
        int found_error;
        int changed;
        map<string, TypeNode> hierarchy;
        map<string, Edge*> edges;
        vector<string> sortedclasses;

        StaticSemantics(AST::ASTNode* root) { // parameterized constructor
            astroot = root;
            found_error = 0;
            changed = 1;
            hierarchy = map<string, TypeNode>();
            edges = map<string, Edge*>();
            sortedclasses = vector<string>();

        }
        // TODO: create destructor?

        void toposort() {
            sortedclasses.push_back("Obj");
            for(map<string,TypeNode>::iterator iter = hierarchy.begin(); iter != hierarchy.end(); ++iter) {
                TypeNode *node = &hierarchy[iter->first]; // get node directly from map
                toposort_aux(node);
            }
        }
        void toposort_aux(TypeNode* node) {
            if (!node->resolved) {
                string parent = node->parent;
                TypeNode* pp = &hierarchy[parent];
                toposort_aux(pp);
                sortedclasses.push_back(node->type);
                node->resolved = 1;
            }
        }

        int populateEdges() {
            for(map<string,TypeNode>::iterator iter = hierarchy.begin(); iter != hierarchy.end(); ++iter) {
                TypeNode node = iter->second;
                edges[node.type] = new Edge();
            }
            for(map<string,TypeNode>::iterator iter = hierarchy.begin(); iter != hierarchy.end(); ++iter) {
                TypeNode node = iter->second;
                string parent = node.parent;
                if (iter->first == "Obj") {
                    continue;
                }
                if (!edges.count(node.parent)) {
                    cout << "Error: class " << node.parent << " undefined, but is superclass of " << iter->first << endl;
                    return 0;
                }
                edges[node.parent]->children.push_back(node.type);
            }
            return 1;
        }

        int isCyclic(string root) {
            Edge* rootedge = edges[root];
            for (string child: rootedge->children) {
                Edge* childedge = edges[child];
                if (childedge->visited) { return 1;} // cycle!!
                childedge->visited = 1;
                if (isCyclic(child)) { return 1;}
            }
            return 0;
        }

        void printClassHierarchy() {
            cout << "=========CLASS HIERARCHY============" << endl;
            for(map<string,TypeNode>::iterator iter = hierarchy.begin(); iter != hierarchy.end(); ++iter) {
                TypeNode node = iter->second;
                node.print();
                cout << "===================================" << endl;
            }
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
                    newmethod.inheritedfrom = classname;
                    node.methods[newmethod.methodname] = newmethod;
                }
                hierarchy[classname] = node; // finally, add node to table

            } // end for class in classes
        } // end populateClassHierarchy

        int search_vector(vector<string>* vec, string target) {
            for (string s: *vec) {
                if (s == target) { return 1; }
            }
            return 0;
        }
        void inherit_methods() {
            for (string classname: sortedclasses) {
                if (classname == "Obj") {continue;}
                if (classname == "__pgm__") {continue;}
                TypeNode *classnode = &hierarchy[classname];
                map<string, MethodTable> *classmethods = &classnode->methods;
                string parent = classnode->parent;
                TypeNode *parentnode = &hierarchy[parent];
                for (string meth: parentnode->methodlist) {
                    classnode->methodlist.push_back(meth);
                }
                for (map<string, MethodTable>::iterator iter = classmethods->begin(); iter != classmethods->end(); ++iter) {
                    if (!search_vector(&classnode->methodlist, iter->first)) {
                        classnode->methodlist.push_back(iter->first);
                    }
                }
                for (string s: classnode->methodlist) {
                    if (!classmethods->count(s)) {
                        MethodTable parentmethod = parentnode->methods[s];
                        MethodTable mt = MethodTable(parentmethod);
                        (*classmethods)[s] = mt;
                    }
                }
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

        int compare_maps(map<string, string> map1, map<string, string>map2) {
            // same types, proceed to compare maps here
            if(map1.size() != map2.size())
                return 0;  // differing sizes, they are not the same
            typename map<string,string>::const_iterator i, j;
            for(i = map1.begin(), j = map2.begin(); i != map1.end(); ++i, ++j)
            {
                if(*i != *j)
                return 0;
            }
            return 1;
        }

        vector<string> split(string strToSplit, char delimeter)
        {
            stringstream ss(strToSplit);
            string item;
            vector<string> splittedStrings;
            while (getline(ss, item, delimeter))
            {
            splittedStrings.push_back(item);
            }
            return splittedStrings;
        }

        map<string, TypeNode>* typeCheck() {
            AST::Program *root = (AST::Program*) astroot;
            while (changed) {
                changed = 0;
                root->type_infer(this, nullptr, nullptr); 
            }
            return &this->hierarchy;
        } // end typeCheck

        void populateBuiltins() {
            // pseudo-class for program: __pgm__
            TypeNode program("__pgm__");
            program.parent = "Obj";
            hierarchy["__pgm__"] = program;

            TypeNode obj("Obj");
            obj.parent = "TYPE_ERROR";
            MethodTable objprint("PRINT");
            objprint.returntype = "Obj";
            objprint.formalargtypes.push_back("Obj");
            objprint.inheritedfrom = "Obj";
            obj.methods["PRINT"] = objprint;
            MethodTable objstring("STRING");
            objstring.returntype = "String";
            objstring.formalargtypes.push_back("Obj");
            objstring.inheritedfrom = "Obj";
            obj.methods["STRING"] = objstring;
            MethodTable objequals("EQUALS");
            objequals.returntype = "Boolean";
            objequals.formalargtypes.push_back("Obj");
            objequals.formalargtypes.push_back("Obj");
            objequals.inheritedfrom = "Obj";
            obj.methods["EQUALS"] = objequals;
            obj.resolved = 1;
            obj.methodlist.push_back("STRING");
            obj.methodlist.push_back("PRINT");
            obj.methodlist.push_back("EQUALS");
            hierarchy["Obj"] = obj;

            TypeNode integer("Int");
            integer.parent = "Obj";
            hierarchy["Int"] = integer;
            MethodTable intplus("PLUS");
            intplus.returntype = "Int";
            intplus.inheritedfrom = "Int";
            intplus.formalargtypes.push_back("Int");
            hierarchy["Int"].methods["PLUS"] = intplus;
            MethodTable intgreater(">");
            intgreater.returntype = "Boolean";
            intgreater.inheritedfrom = "Int";
            intgreater.formalargtypes.push_back("Int");
            hierarchy["Int"].methods[">"] = intgreater;

            TypeNode str("String");
            str.parent = "Obj";
            hierarchy["String"] = str;

            TypeNode boolean("Boolean");
            boolean.parent = "Obj";
            hierarchy["Boolean"] = boolean;

            TypeNode nothing("Nothing");
            nothing.parent = "Obj";
        }

        void* checkAST() { // top-level
            populateClassHierarchy();
            if (!populateEdges()) {
                return nullptr;
            }
            
            if (isCyclic("Obj")) {
                cout << "GRAPH CYCLE DETECTED" << endl;
                return nullptr;
            }
            else {
                cout << "GRAPH ACYCLIC" << endl;
            }
            toposort();
            inherit_methods();
            printClassHierarchy();
            AST::Program *root = (AST::Program*) astroot;
            set<string> *vars = new set<string>;
            if (root->initcheck(vars, this)) { 
                cout << "INITIALIZATION ERRORS" << endl;
                return nullptr;
            }
            typeCheck();
            printClassHierarchy();
            return &hierarchy;
        }
}; 

