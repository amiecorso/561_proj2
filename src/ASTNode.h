//
// Started by Michal Young on 9/12/18.
// Finished by Amie Corso Nov 2019
//
#ifndef ASTNODE_H
#define ASTNODE_H

#include <string>
#include <sstream>
#include <vector>
#include <iostream>
#include <typeinfo>
#include <map>
#include <set>
#include "CodegenContext.h"

using namespace std;

class StaticSemantics;

class class_and_method {
    public:
        string classname;
        string methodname;
        class_and_method(string classname, string method) {
            this->classname = classname;
            this->methodname = method;
        }
};

namespace AST {
    // Abstract syntax tree.  ASTNode is abstract base class for all other nodes.
    // Json conversion and pretty-printing can pass around a print context object
    // to keep track of indentation, and possibly other things.
    class AST_print_context {
    public:
        int indent_; // Number of spaces to place on left, after each newline
        AST_print_context() : indent_{0} {};
        void indent() { ++indent_; }
        void dedent() { --indent_; }
    };

    class ASTNode {
    public:
        virtual string genL(Context *con) {cout << "GENL UNIMP" << endl; return "";}
        virtual void genR(Context *con, string targreg) {cout << "GENR UNIMPLLLL" << endl;}
        virtual void genBranch(Context *con, string true_branch, string false_branch) { cout << "GENBRANCH UNIMP" << endl; }
        virtual void collect_vars(map<string, string>* vt) {cout << "UNIMPLEMENTED COLLECT_VARS" << endl;};
        virtual string get_var() {cout << "UNIMPLEMENTED GET_VAR" << endl; return "";};
        virtual int initcheck(set<string>* vars) {cout << "UNIMPLEMENTED initcheck" << endl; return 0;}
        virtual string type_infer(StaticSemantics* ssc, map<string, string>* vt, class_and_method* info) {
            cout << "UNIMPLEMENTED type_infer" << endl;
            return "UNIMP type_infer";
        }
        virtual void json(ostream& out, AST_print_context& ctx) = 0;  // Json string representation
        string str() {
            stringstream ss;
            AST_print_context ctx;
            json(ss, ctx);
            return ss.str();
        }
    protected:
        void json_indent(ostream& out, AST_print_context& ctx);
        void json_head(string node_kind, ostream& out, AST_print_context& ctx);
        void json_close(ostream& out, AST_print_context& ctx);
        void json_child(string field, ASTNode& child, ostream& out, AST_print_context& ctx, char sep=',');
    };

    class Stub : public ASTNode {
        string name_;
    public:
        string type_infer(StaticSemantics* ssc, map<string, string>* vt, class_and_method* info) override {
            cout << "UNIMP TYPEINFER STUB" << endl;
            return 0;
        }
        explicit Stub(string name) : name_{name} {}
        void json(ostream& out, AST_print_context& ctx) override;
    };

    template<class Kind>
    class Seq : public ASTNode {
    public:
        string kind_;
        vector<Kind *> elements_;

        Seq(string kind) : kind_{kind}, elements_{vector<Kind *>()} {}

        void genR(Context *con, string targreg) override {
            for (ASTNode *node: elements_) {
                node->genR(con, targreg);
            }
        }
        string get_var() override {return "";}
        void collect_vars(map<string, string>* vt) override {return;}
        int initcheck(set<string>* vars) override {
            for (ASTNode *node: elements_) {
                if (node->initcheck(vars)) { return 1; } // failure
            }
            return 0;
        }
        string type_infer(StaticSemantics* ssc, map<string, string>* vt, class_and_method* info) override {
            //cout << "ENTERING Seq::type_infer" << endl;
            for (Kind *el: elements_) {el->type_infer(ssc, vt, info);}
            return "Nothing";
        };

        void append(Kind *el) { elements_.push_back(el); }

        void json(ostream &out, AST_print_context &ctx) override {
            json_head(kind_, out, ctx);
            out << "\"elements_\" : [";
            auto sep = "";
            for (Kind *el: elements_) {
                out << sep;
                el->json(out, ctx);
                sep = ", ";
            }
            out << "]";
            json_close(out, ctx);
        }

    };

    /* L_Expr nodes are AST nodes that can be evaluated for location.
     * Most can also be evaluated for value_.  An example of an L_Expr
     * is an identifier, which can appear on the left_ hand or right_ hand
     * side of an assignment.  For example, in x = y, x is evaluated for
     * location and y is evaluated for value_.
     *
     * For now, a location is just a name, because that's what we index
     * the symbol table with.  In a full compiler, locations can be
     * more complex, and typically in code generation we would have
     * LExpr evaluate to an address in a register.
     */
    class LExpr : public ASTNode {
        /* Abstract base class */
    };

    /* Identifiers like x and literals like 42 are the
    * leaves of the AST.  A literal can only be evaluated
    * for value_ (the 'eval' method), but an identifier
    * can also be evaluated for location (when we want to
    * store something in it).
    */
    class Ident : public LExpr {
        public:
            string text_;

            string genL(Context *con) override {
                return con->get_local_var(text_);
            }
            void genR(Context *con, string targreg) override {
                /* The lvalue, i.e., address of memory */
                string loc = con->get_local_var(text_);
                con->emit(targreg + " = " + loc + ";");
            }
            string get_var() override {return text_;}
            void collect_vars(map<string, string>* vt) override {return;}
            string type_infer(StaticSemantics* ssc, map<string, string>* vt, class_and_method* info) override;
            int initcheck(set<string>* vars) override {
                if (!vars->count(text_)) { // not 0 would be 1, indicating failure
                    cout << "INIT ERROR: var " << text_ << " used before initialized" << endl;
                    return 1;
                }
                return 0;
            }
            explicit Ident(string txt) : text_{txt} {}
            void json(ostream& out, AST_print_context& ctx) override;
    };

    class Block : public Seq<ASTNode> {
    public:
        explicit Block() : Seq("Block") {}
     };

    class Formal : public ASTNode {
        public:
            ASTNode& var_;
            ASTNode& type_;
            explicit Formal(ASTNode& var, ASTNode& type_) :
                var_{var}, type_{type_} {};
            string type_infer(StaticSemantics* ssc, map<string, string>* vt, class_and_method* info) override {
                //cout << "ENTERING Formal:type_infer" << endl;
                string var = var_.get_var();
                string type = type_.get_var();
                (*vt)[var] = type;
                return type;
            }
            int initcheck(set<string>* vars) override {
                vars->insert(var_.get_var());
                return 0;
            }
            string get_var() override {return "";}
            void collect_vars(map<string, string>* vt) override {return;}
            void json(ostream& out, AST_print_context&ctx) override;
    };

    class Formals : public Seq<Formal> {
    public:
        explicit Formals() : Seq("Formals") {}
    };

    class Method : public ASTNode {
        public:
            ASTNode& name_;
            Formals& formals_;
            ASTNode& returns_;
            Block& statements_;
            
            void genR(Context *con, string targreg) override;
            explicit Method(ASTNode& name, Formals& formals, ASTNode& returns, Block& statements) :
            name_{name}, formals_{formals}, returns_{returns}, statements_{statements} {}
            string type_infer(StaticSemantics* ssc, map<string, string>* vt, class_and_method* info) override {
                //cout << "ENTERING Method::type_infer for method " << name_.get_var() << endl;
                formals_.type_infer(ssc, vt, info);
                statements_.type_infer(ssc, vt, info);
                return "Nothing";
            }
            int initcheck(set<string>* vars) override {
                if (formals_.initcheck(vars)) { return 1; }
                if (statements_.initcheck(vars)) { return 1; }
                return 0; // success
            }
            string get_var() override {return "";}
            void collect_vars(map<string, string>* vt) override {return;}
            void json(ostream& out, AST_print_context&ctx) override;
    };

    class Methods : public Seq<Method> {
    public:
        explicit Methods() : Seq("Methods") {}
        string type_infer(StaticSemantics* ssc, map<string, string>* vt, class_and_method* info) override;
    };

    class Statement : public ASTNode { 
    };

    class Assign : public Statement {
    protected:  // But inherited by AssignDeclare
        ASTNode &lexpr_;
        ASTNode &rexpr_;
    public:
        void genR(Context *con, string targreg) override {
            string type = con->get_type(lexpr_);
            string reg = con->alloc_reg(type);
            string loc = lexpr_.genL(con);
            rexpr_.genR(con, reg);
            /* Store the value in the location */
            con->emit(loc + " = " + reg + ";");
        }
        void collect_vars(map<string, string>* vt) override {
            string var_name = lexpr_.get_var();
            if (var_name.rfind("this", 0) == 0) {
                (*vt)[var_name] = "Bottom";
            }
        }
        string type_infer(StaticSemantics* ssc, map<string, string>* vt, class_and_method* info) override;
        int initcheck(set<string>* vars) override {
            if (rexpr_.initcheck(vars)) { return 1; }
            vars->insert(lexpr_.get_var());
            return 0;
        }        
        explicit Assign(ASTNode &lexpr, ASTNode &rexpr) :
           lexpr_{lexpr}, rexpr_{rexpr} {}
        void json(ostream& out, AST_print_context& ctx) override;
    };

    class AssignDeclare : public Assign {
        Ident &static_type_;
    public:
        explicit AssignDeclare(ASTNode &lexpr, ASTNode &rexpr, Ident &static_type) :
            Assign(lexpr, rexpr), static_type_{static_type} {}
        string type_infer(StaticSemantics* ssc, map<string, string>* vt, class_and_method* info) override;
        void json(ostream& out, AST_print_context& ctx) override;

    };

    class Expr : public Statement { 
        public:
    };

    /* When an expression is an LExpr, the LExpr denotes a location, 
        and we need to load it. */
    class Load : public Expr {
        LExpr &loc_;
    public:
        Load(LExpr &loc) : loc_{loc} {}
        void genR(Context *con, string targreg) override {
            string var = get_var();
            string loc = con->get_local_var(var);
            con->emit(targreg + " = " + loc + ";");
        }
        string genL(Context *con) override {
            string var = get_var();
            return con->get_local_var(var);
        }
        string get_var() override {return loc_.get_var();}
        void collect_vars(map<string, string>* vt) override {return;}
        int initcheck(set<string>* vars) override { return 0; }  
        string type_infer(StaticSemantics* ssc, map<string, string>* vt, class_and_method* info) override {
             return loc_.type_infer(ssc, vt, info); 
        }
        void json(ostream &out, AST_print_context &ctx) override;
    };

    class Return : public Statement {
        ASTNode &expr_;
    public:
        explicit Return(ASTNode& expr) : expr_{expr}  {}
        string type_infer(StaticSemantics* ssc, map<string, string>* vt, class_and_method* info) override;
        int initcheck(set<string>* vars) override {
            if (expr_.initcheck(vars)) { return 1; }
            return 0;
        }  
        void json(ostream& out, AST_print_context& ctx) override;
    };

    class If : public Statement {
        ASTNode &cond_; // The boolean expression to be evaluated
        Seq<ASTNode> &truepart_; // Execute this block if the condition is true
        Seq<ASTNode> &falsepart_; // Execute this block if the condition is false
    public:
        void genR(Context* con, string targreg) override {
            std::string thenpart = con->new_branch_label("then");
            std::string elsepart = con->new_branch_label("else");
            std::string endpart = con->new_branch_label("endif");
            cond_.genBranch(con, thenpart, elsepart);
            /* Generate the 'then' part here */
            con->emit(thenpart + ": ;");
            truepart_.genR(con, targreg);
            con->emit(std::string("goto ") + endpart + ";");
            /* Generate the 'else' part here */
            con->emit(elsepart + ": ;");
            falsepart_.genR(con, targreg);
            /* That's all, folks */
            con->emit(endpart + ": ;");
        }

        explicit If(ASTNode& cond, Seq<ASTNode>& truepart, Seq<ASTNode>& falsepart) :
            cond_{cond}, truepart_{truepart}, falsepart_{falsepart} { };
        int initcheck(set<string>* vars) override {
            if (cond_.initcheck(vars)) {return 1;}
            set<string>* trueset = new set<string>(*vars); // copy constructor
            set<string>* falseset = new set<string>(*vars); // copy constructor
            if (truepart_.initcheck(trueset)) {return 1;}
            if (falsepart_.initcheck(falseset)) {return 1;}
            // take set intersection
            for (set<string>::iterator itr = trueset->begin(); itr != trueset->end(); ++itr) {
                if (falseset->count(*itr)) {
                    vars->insert(*itr); // if also in false part, insert to table (duplication ok, it's a set)
                }
            }
            return 0;
        }  
        string type_infer(StaticSemantics* ssc, map<string, string>* vt, class_and_method* info) override;        
        void json(ostream& out, AST_print_context& ctx) override;
    };

    class While : public Statement {
        ASTNode& cond_;  // Loop while this condition is true
        Seq<ASTNode>&  body_;     // Loop body
    public:
        explicit While(ASTNode& cond, Block& body) :
            cond_{cond}, body_{body} { };

        void genR(Context* con, string targreg) override {
            string checkpart = con->new_branch_label("check_cond");
            string looppart = con->new_branch_label("loop");
            string endpart = con->new_branch_label("endwhile");
            con->emit(checkpart + ": ;");
            cond_.genBranch(con, looppart, endpart);
            con->emit(looppart + ": ;");
            body_.genR(con, targreg);
            con->emit("goto " + checkpart + ";");
            con->emit(endpart + ": ;");
        }

        string type_infer(StaticSemantics* ssc, map<string, string>* vt, class_and_method* info) override {
            //cout << "ENTERING While::type_infer" << endl;
            string cond_type = cond_.type_infer(ssc, vt, info);
            if (cond_type != "Boolean") {
                cout << "TypeError (While): Condition does not evaluate to type Boolean (ignoring statements)" << endl;
            }
            body_.type_infer(ssc, vt, info);
            return "Nothing";
        }
        int initcheck(set<string>* vars) override {
            if (cond_.initcheck(vars)) {return 1;}
            set<string>* bodyset = new set<string>(*vars); // copy constructor
            if (body_.initcheck(bodyset)) {return 1;}
            return 0;
        }  
        void json(ostream& out, AST_print_context& ctx) override;

    };

    class Class : public ASTNode {
        public:
            Ident& name_;
            Ident& super_;
            ASTNode& constructor_;
            Methods& methods_;

            void genR(Context *con, string targreg) override {
                string classname = name_.get_var();
                con->emit("struct class_" + classname + "_struct;");
                con->emit("struct obj_" + classname + ";");
                con->emit("typedef struct obj_" + classname + "* obj_" + classname + ";");
                con->emit("typedef struct class_" + classname + "_struct* class_" + classname + ";");
                con->emit("");
                con->emit("typedef struct obj_" + classname + "_struct {");
                con->emit("class_" + classname + " clazz;");
                con->emit_instance_vars();
                con->emit("} * obj_" + classname + ";");
                con->emit("");
                con->emit("struct class_" + classname + "_struct the_class_" + classname + "_struct;");
                con->emit("");
                con->emit("struct class_" + classname + "_struct {");
                // constructor
                con->emit("obj_" + classname + " (*constructor) (" + con->get_formal_argtypes("constructor") + ");");
                con->emit_method_sigs(); // rest of the methods
                con->emit("};\n");
                con->emit("extern class_" + classname + " the_class_" + classname + ";");
                // now populate constructor
                con->emit("");
                Context * construct_con = new Context(*con);
                construct_con->methodname = "constructor";
                constructor_.genR(construct_con, targreg);
                methods_.genR(con, targreg);
                con->emit_the_class_struct();
            }

            explicit Class(Ident& name, Ident& super,
                    ASTNode& constructor, Methods& methods) :
                name_{name},  super_{super},
                constructor_{constructor}, methods_{methods} {};
            string get_var() override {return "";}
            void collect_vars(map<string, string>* vt) override {return;}
            string type_infer(StaticSemantics* ssc, map<string, string>* vt, class_and_method* info) override;
            int initcheck(set<string>* vars) override {
                if (constructor_.initcheck(vars)) {return 1;}
                if (methods_.initcheck(vars)) {return 1;}
                return 0;
            }  
            void json(ostream& out, AST_print_context& ctx) override;
    };

    class Classes : public Seq<Class> {
    public:
        void genR(Context *con, string targreg) override {
            for (Class *cls: elements_) {
                string classname = cls->name_.get_var();
                Context classcon = Context(*con); // copy constructor
                classcon.classname = classname;
                cls->genR(&classcon, targreg);
            }
        }
        explicit Classes() : Seq<Class>("Classes") {}
        string type_infer(StaticSemantics* ssc, map<string, string>* vt, class_and_method* info) override;
    };

    class IntConst : public Expr {
        int value_;
    public:
        void genR(Context *con, string targreg) override {
            con->emit(targreg + " = int_literal(" + to_string(value_) + ");");
        }
        explicit IntConst(int v) : value_{v} {}
        string type_infer(StaticSemantics* ssc, map<string, string>* vt, class_and_method* info) override { return "Int"; }
        int initcheck(set<string>* vars) override {return 0;};
        void json(ostream& out, AST_print_context& ctx) override;
    };

    class Type_Alternative : public ASTNode {
        Ident& ident_;
        Ident& classname_;
        Block& block_;
    public:
        explicit Type_Alternative(Ident& ident, Ident& classname, Block& block) :
                ident_{ident}, classname_{classname}, block_{block} {}
        string get_var() override {return "";}
        void collect_vars(map<string, string>* vt) override {return;}
        string type_infer(StaticSemantics* ssc, map<string, string>* vt, class_and_method* info) override;
        void json(ostream& out, AST_print_context& ctx) override;
    };

    class Type_Alternatives : public Seq<Type_Alternative> {
    public:
        explicit Type_Alternatives() : Seq("Type_Alternatives") {}

    };

    class Typecase : public Statement {
        Expr& expr_; // An expression we want to downcast to a more specific class
        Type_Alternatives& cases_;    // A case for each potential type
    public:
        explicit Typecase(Expr& expr, Type_Alternatives& cases) :
                expr_{expr}, cases_{cases} {};
        string type_infer(StaticSemantics* ssc, map<string, string>* vt, class_and_method* info) override;
        void json(ostream& out, AST_print_context& ctx) override;
    };

    class StrConst : public Expr {
        string value_;
    public:
        void genR(Context *con, string targreg) override {
            con->emit(targreg + " = str_literal(" + value_ + ");");
        }
        explicit StrConst(string v) : value_{v} {}
        string type_infer(StaticSemantics* ssc, map<string, string>* vt, class_and_method* info) override { return "String"; }
        int initcheck(set<string>* vars) override {return 0;};
        void json(ostream& out, AST_print_context& ctx) override;
    };

    class Actuals : public Seq<Expr> {
    public:
        explicit Actuals() : Seq("Actuals") {}
        string genL(Context *con) override {
            vector<string> actualregs = vector<string>();
            for (ASTNode *actual: elements_) {
                string type = con->get_type(*actual);
                string reg = con->alloc_reg(type);
                actualregs.push_back(reg);
                actual->genR(con, reg);
            }
            string actuals = "";
            for (string reg: actualregs) {
                actuals += reg;
                actuals += ", ";
            }
            int strlen = actuals.length();
            actuals = actuals.erase(strlen - 2, 2); // erase the final ", "
            return actuals;
        }
    };

    class Construct : public Expr {
        Ident&  method_;           /* Method name is same as class name */
        Actuals& actuals_;    /* Actual arguments to constructor */
    public:
        explicit Construct(Ident& method, Actuals& actuals) :
                method_{method}, actuals_{actuals} {}
        int initcheck(set<string>* vars) override {
            if (method_.initcheck(vars)) { return 1;}
            if (actuals_.initcheck(vars)) { return 1;}
            return 0;
        }
        string type_infer(StaticSemantics* ssc, map<string, string>* vt, class_and_method* info) override;
        void json(ostream& out, AST_print_context& ctx) override;
    };

    class Call : public Expr {
        Expr& receiver_;        /* Expression computing the receiver object */
        Ident& method_;         /* Identifier of the method */
        Actuals& actuals_;     /* List of actual arguments */
    public:
        void genBranch(Context *con, string true_branch, string false_branch) override {
            // At present, we don't have 'and' and 'or'
            string mytype = con->get_type(*this);
            string reg = con->alloc_reg(mytype);
            genR(con, reg);
            con->emit(string("if (") + reg + ") goto " + true_branch + ";");
            con->emit(string("goto ") + false_branch + ";");
            con->free_reg(reg);
        }
        void genR(Context *con, string targreg) override {
            //obj_Int x_sum = this_x->clazz->PLUS(this_x, other_x); 
            // names of actual arguments?
                // what if actual arguments are themselves expressions?
            string methodname = method_.get_var();
            string recvtype = con->get_type(receiver_);
            string recvreg = con->alloc_reg(recvtype);
            receiver_.genR(con, recvreg);
            string actuals = actuals_.genL(con);
            con->emit(targreg + " = " + recvreg + "->clazz->" + methodname + "(" + recvreg + ", " + actuals + ");");
        }

        explicit Call(Expr& receiver, Ident& method, Actuals& actuals) :
                receiver_{receiver}, method_{method}, actuals_{actuals} {};
        string type_infer(StaticSemantics* ssc, map<string, string>* vt, class_and_method* info) override;
        int initcheck(set<string>* vars) override {
            if (receiver_.initcheck(vars)) { return 1;}
            if (method_.initcheck(vars)) { return 1;}
            if (actuals_.initcheck(vars)) { return 1;}
            return 0;
        }
        // Convenience factory for the special case of a method
        // created for a binary operator (+, -, etc).
        static Call* binop(string opname, Expr& receiver, Expr& arg);
        void json(ostream& out, AST_print_context& ctx) override;
    };

   class BinOp : public Expr {
    protected:
        string opsym;
        ASTNode &left_;
        ASTNode &right_;
        BinOp(string sym, ASTNode &l, ASTNode &r) :
                opsym{sym}, left_{l}, right_{r} {};
    public:
        void json(ostream& out, AST_print_context& ctx) override;
    };

   class And : public BinOp {
   public:
       explicit And(ASTNode& left, ASTNode& right) :
          BinOp("And", left, right) {}
        string type_infer(StaticSemantics* ssc, map<string, string>* vt, class_and_method* info) override {
            string left_type = left_.type_infer(ssc, vt, info);
            string right_type = right_.type_infer(ssc, vt, info);
            if (left_type != "Boolean" || right_type != "Boolean") {return "And:TypeError";}
            return "Boolean";
        }
        int initcheck(set<string>* vars) override {
            if (left_.initcheck(vars)) { return 1;}
            if (right_.initcheck(vars)) { return 1;}
            return 0;
        }
   };

    class Or : public BinOp {
    public:
        explicit Or(ASTNode& left, ASTNode& right) :
                BinOp("Or", left, right) {}
        string type_infer(StaticSemantics* ssc, map<string, string>* vt, class_and_method* info) override {
            string left_type = left_.type_infer(ssc, vt, info);
            string right_type = right_.type_infer(ssc, vt, info);
            if (left_type != "Boolean" || right_type != "Boolean") {return "Or:TypeError";}
            return "Boolean";
        }
        int initcheck(set<string>* vars) override {
            if (left_.initcheck(vars)) { return 1;}
            if (right_.initcheck(vars)) { return 1;}
            return 0;
        }
    };

    class Not : public Expr {
        ASTNode& left_;
    public:
        explicit Not(ASTNode& left ):
            left_{left}  {}
        string type_infer(StaticSemantics* ssc, map<string, string>* vt, class_and_method* info) override {
            string left_type = left_.type_infer(ssc, vt, info);
            if (left_type != "Boolean") {return "Not:TypeError";}
            return "Boolean";
        }
        int initcheck(set<string>* vars) override {
            if (left_.initcheck(vars)) { return 1;}
            return 0;
        }
        void json(ostream& out, AST_print_context& ctx) override;
    };

    class Dot : public LExpr {
        Expr& left_;
        Ident& right_;
    public:
        void genR(Context *con, string targreg) override {
            string var = get_var();
            string loc = con->get_local_var(var);
            con->emit(targreg + " = " + loc + ";");
        }
        string genL(Context *con) override {
            string var = get_var();
            return con->get_local_var(var);
        }
        string get_var() override {return left_.get_var() + "." + right_.get_var();}
        void collect_vars(map<string, string>* vt) override { return; }
        string type_infer(StaticSemantics* ssc, map<string, string>* vt, class_and_method* info) override;
        int initcheck(set<string>* vars) override {
            if (!vars->count(get_var())) { // not 0 would be 1, indicating failure
                cout << "INIT ERROR: var " << get_var() << " used before initialized" << endl;
                return 1;
            } 
            return 0;
        }
        explicit Dot (Expr& left, Ident& right) :
           left_{left},  right_{right} {}
        void json(ostream& out, AST_print_context& ctx) override;
    };

    class Program : public ASTNode {
    public:
        Classes& classes_;
        Block& statements_;

        void genR(Context *con, string targreg) override {
            classes_.genR(con, targreg);
            Context classcon = Context(*con); // copy constructor
            classcon.classname = "__pgm__";
            classcon.methodname = "__pgm__";
            statements_.genR(&classcon, targreg);
        }
        explicit Program(Classes& classes, Block& statements) :
                classes_{classes}, statements_{statements} {}
        string type_infer(StaticSemantics* ssc, map<string, string>* vt, class_and_method* info) override;
        string get_var() override {return "";}
        int initcheck(set<string>* vars, StaticSemantics* ssc);
        void collect_vars(map<string, string>* vt) override {return;}
        void json(ostream& out, AST_print_context& ctx) override;
    };
}
#endif //ASTNODE_H