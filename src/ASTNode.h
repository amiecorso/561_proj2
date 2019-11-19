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

class StaticSemantics;
class class_and_method {
    public:
        std::string classname;
        std::string methodname;
        class_and_method(std::string classname, std::string method) {
            this->classname = classname;
            this->methodname = method;
        }
        void print( ) {
            //std::cout << "\t\t\t classname: " << this->classname << std::endl;
            //std::cout << "\t\t\t methodname: " << this->methodname << std::endl;
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
        virtual void collect_vars(std::map<std::string, std::string>* vt) {std::cout << "UNIMPLEMENTED COLLECT_VARS" << std::endl;};
        virtual std::string get_var() {std::cout << "UNIMPLEMENTED GET_VAR" << std::endl; return "";};
        virtual std::string get_type(std::map<std::string, std::string>* vt, StaticSemantics* ssc, std::string classname) {
            std::cout << "UNIMPLEMENTED GET_TYPE" << std::endl;
            return "";
        };
        virtual void type_infer(StaticSemantics* ssc, std::map<std::string, std::string>* vt, class_and_method* info) {
            std::cout << "UNIMPLEMENTED type_infer" << std::endl;
        }
        virtual void json(std::ostream& out, AST_print_context& ctx) = 0;  // Json string representation
        std::string str() {
            std::stringstream ss;
            AST_print_context ctx;
            json(ss, ctx);
            return ss.str();
        }
    protected:
        void json_indent(std::ostream& out, AST_print_context& ctx);
        void json_head(std::string node_kind, std::ostream& out, AST_print_context& ctx);
        void json_close(std::ostream& out, AST_print_context& ctx);
        void json_child(std::string field, ASTNode& child, std::ostream& out, AST_print_context& ctx, char sep=',');
    };

    class Stub : public ASTNode {
        std::string name_;
    public:
        explicit Stub(std::string name) : name_{name} {}
        void json(std::ostream& out, AST_print_context& ctx) override;
    };

    /*
     * Abstract base class for nodes that have sequences
     * of children, e.g., block of statements, sequence of
     * classes.  These may be able to share some operations,
     * especially when applying a method to the sequence just
     * means applying the method to each element of the sequence.
     * We need a different kind of sequence depending on type of
     * elements if we want to access elements without casting while
     * still having access to their fields.
     * (Will replace 'Seq')
     */
    template<class Kind>
    class Seq : public ASTNode {
    public:
        std::string kind_;
        std::vector<Kind *> elements_;

        Seq(std::string kind) : kind_{kind}, elements_{std::vector<Kind *>()} {}

        std::string get_var() override {return "";}
        void collect_vars(std::map<std::string, std::string>* vt) override {return;}
        void type_infer(StaticSemantics* ssc, std::map<std::string, std::string>* vt, class_and_method* info) override {
            std::cout << "ENTERING Seq::type_infer" << std::endl;
            info->print();
            for (Kind *el: elements_) {
                el->type_infer(ssc, vt, info);
            }
        };

        void append(Kind *el) { elements_.push_back(el); }

        void json(std::ostream &out, AST_print_context &ctx) override {
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
            std::string text_;
            std::string get_var() override {return text_;}
            void collect_vars(std::map<std::string, std::string>* vt) override {return;}
            std::string get_type(std::map<std::string, std::string>* vt, StaticSemantics* ssc, std::string classname) override {
                //std::cout << "ENTERING: Ident::get_type" << std::endl;
                if (vt->count(text_)) { // Identifier in table
                    return (*vt)[text_];
                }
                else { // not in table!!
                    std::cout << "TypeError: Identifier " << text_ << " uninitialized" << std::endl;
                    return "TypeError"; // error?? 
                }
            }
            explicit Ident(std::string txt) : text_{txt} {}
            void json(std::ostream& out, AST_print_context& ctx) override;
    };

    /* A block is a sequence of statements or expressions.
     * For simplicity we'll just make it a sequence of ASTNode,
     * and leave it to the parser to build valid structures.
     */
    class Block : public Seq<ASTNode> {
    public:
        explicit Block() : Seq("Block") {}
        void type_infer(StaticSemantics* ssc, std::map<std::string, std::string>* vt, class_and_method* info) override {
            std::cout << "ENTERING: Block::type_infer" << std::endl;
            info->print();
            for (ASTNode *stmt: elements_) {
                stmt->type_infer(ssc, vt, info);
            }
        }
     };

    /* Formal arguments list is a list of
     * identifier: type pairs.
     */
    class Formal : public ASTNode {
        public:
            ASTNode& var_;
            ASTNode& type_;
            explicit Formal(ASTNode& var, ASTNode& type_) :
                var_{var}, type_{type_} {};
            void type_infer(StaticSemantics* ssc, std::map<std::string, std::string>* vt, class_and_method* info) override {
                std::cout << "ENTERING Formal:type_infer" << std::endl;
                std::string var = var_.get_var();
                std::cout << typeid(type_).name() << std::endl;
                std::string type = type_.get_var();
                (*vt)[var] = type;
                // TODO: do we need to check if this thing is already in the table or anything?
            }
            std::string get_var() override {return "";}
            void collect_vars(std::map<std::string, std::string>* vt) override {return;}
            void json(std::ostream& out, AST_print_context&ctx) override;
    };

    class Formals : public Seq<Formal> {
    public:
        explicit Formals() : Seq("Formals") {}
        void type_infer(StaticSemantics* ssc, std::map<std::string, std::string>* vt, class_and_method* info) override {
            std::cout << "ENTERING Formals:type_infer" << std::endl;
            for (ASTNode *formal: elements_) {
                formal->type_infer(ssc, vt, info);
            }
        }
    };

    class Method : public ASTNode {
        public:
            ASTNode& name_;
            Formals& formals_;
            ASTNode& returns_;
            Block& statements_;
            
            explicit Method(ASTNode& name, Formals& formals, ASTNode& returns, Block& statements) :
            name_{name}, formals_{formals}, returns_{returns}, statements_{statements} {}
            void type_infer(StaticSemantics* ssc, std::map<std::string, std::string>* vt, class_and_method* info) override {
                std::cout << "ENTERING Method::type_infer" << std::endl;
                formals_.type_infer(ssc, vt, info);
                statements_.type_infer(ssc, vt, info);
            }
            std::string get_var() override {return "";}
            void collect_vars(std::map<std::string, std::string>* vt) override {return;}
            void json(std::ostream& out, AST_print_context&ctx) override;
    };

    class Methods : public Seq<Method> {
    public:
        explicit Methods() : Seq("Methods") {}
        void type_infer(StaticSemantics* ssc, std::map<std::string, std::string>* vt, class_and_method* info) override;
    };

    /* An assignment has an lvalue (location to be assigned to)
     * and an expression.  We evaluate the expression and place
     * the value_ in the variable.  An assignment may also place a
     * static type constraint on a variable.  This is logically a
     * distinct operation, and could be represented as a separate statement,
     * but it's convenient to keep it in the assignment since our syntax
     * puts it there.
     */

    class Statement : public ASTNode { 
        public:
            std::string get_var() override {return "";}
            void collect_vars(std::map<std::string, std::string>* vt) override {return;}
    };

    class Assign : public Statement {
    protected:  // But inherited by AssignDeclare
        ASTNode &lexpr_;
        ASTNode &rexpr_;
    public:
        void collect_vars(std::map<std::string, std::string>* vt) override {
            std::string var_name = lexpr_.get_var();
            (*vt)[var_name] = "Bottom";
        }
        void type_infer(StaticSemantics* ssc, std::map<std::string, std::string>* vt, class_and_method* info) override;

        explicit Assign(ASTNode &lexpr, ASTNode &rexpr) :
           lexpr_{lexpr}, rexpr_{rexpr} {}
        void json(std::ostream& out, AST_print_context& ctx) override;
    };

    class AssignDeclare : public Assign {
        Ident &static_type_;
    public:
        explicit AssignDeclare(ASTNode &lexpr, ASTNode &rexpr, Ident &static_type) :
            Assign(lexpr, rexpr), static_type_{static_type} {}
        void type_infer(StaticSemantics* ssc, std::map<std::string, std::string>* vt, class_and_method* info) override;
        void json(std::ostream& out, AST_print_context& ctx) override;

    };

    /* A statement could be just an expression ... but
     * we might want to interpose a node here.
     */
    class Expr : public Statement { 
        public:
    };

    /* When an expression is an LExpr, we
     * the LExpr denotes a location, and we
     * need to load it.
     */
    class Load : public Expr {
        LExpr &loc_;
    public:
        Load(LExpr &loc) : loc_{loc} {}
        std::string get_var() override {return loc_.get_var();}
        void collect_vars(std::map<std::string, std::string>* vt) override {return;}
        std::string get_type(std::map<std::string, std::string>* vt, StaticSemantics* ssc, std::string classname) override {
            std::cout << "ENTERING Load::get_type" << std::endl;
            std::string result = loc_.get_type(vt, ssc, classname);
            //std::cout << "\t type of loc_: " << typeid(loc_).name() << result << std::endl;
            //std::cout << "\t result of loc_.get_type: " << result << std::endl;
            return loc_.get_type(vt, ssc, classname);
        }
        void type_infer(StaticSemantics* ssc, std::map<std::string, std::string>* vt, class_and_method* info) override {
             return loc_.type_infer(ssc, vt, info); 
        }
        void json(std::ostream &out, AST_print_context &ctx) override;
    };

    /* 'return' statement returns value from method */
    class Return : public Statement {
        ASTNode &expr_;
    public:
        explicit Return(ASTNode& expr) : expr_{expr}  {}
        void type_infer(StaticSemantics* ssc, std::map<std::string, std::string>* vt, class_and_method* info) override;
        void json(std::ostream& out, AST_print_context& ctx) override;
    };

    class If : public Statement {
        ASTNode &cond_; // The boolean expression to be evaluated
        Seq<ASTNode> &truepart_; // Execute this block if the condition is true
        Seq<ASTNode> &falsepart_; // Execute this block if the condition is false
    public:
        explicit If(ASTNode& cond, Seq<ASTNode>& truepart, Seq<ASTNode>& falsepart) :
            cond_{cond}, truepart_{truepart}, falsepart_{falsepart} { };
        void json(std::ostream& out, AST_print_context& ctx) override;
    };

    class While : public Statement {
        ASTNode& cond_;  // Loop while this condition is true
        Seq<ASTNode>&  body_;     // Loop body
    public:
        explicit While(ASTNode& cond, Block& body) :
            cond_{cond}, body_{body} { };
        void type_infer(StaticSemantics* ssc, std::map<std::string, std::string>* vt, class_and_method* info) override {
            std::cout << "ENTERING While::type_infer" << std::endl;
            std::string cond_type = cond_.get_type(vt, ssc, info->classname);
            if (!(cond_type == "Boolean")) {
                std::cout << "TypeError (While): Condition does not evaluate to type Boolean (ignoring statements)" << std::endl;
                return;
            }
            body_.type_infer(ssc, vt, info);
        }
        void json(std::ostream& out, AST_print_context& ctx) override;

    };

    /* A class has a name, a list of arguments, and a body
    * consisting of a block (essentially the constructor)
    * and a list of methods.
    */
    class Class : public ASTNode {
        public:
            Ident& name_;
            Ident& super_;
            ASTNode& constructor_;
            Methods& methods_;

            explicit Class(Ident& name, Ident& super,
                    ASTNode& constructor, Methods& methods) :
                name_{name},  super_{super},
                constructor_{constructor}, methods_{methods} {};
            std::string get_var() override {return "";}
            void collect_vars(std::map<std::string, std::string>* vt) override {return;}
            void type_infer(StaticSemantics* ssc, std::map<std::string, std::string>* vt, class_and_method* info) override;
            void json(std::ostream& out, AST_print_context& ctx) override;
    };

    /* A Quack program begins with a sequence of zero or more
     * class definitions.
     */
    class Classes : public Seq<Class> {
    public:
        explicit Classes() : Seq<Class>("Classes") {}
        void type_infer(StaticSemantics* ssc, std::map<std::string, std::string>* vt, class_and_method* info) override;
    };

    class IntConst : public Expr {
        int value_;
    public:
        explicit IntConst(int v) : value_{v} {}
        std::string get_type(std::map<std::string, std::string>* vt, StaticSemantics* ssc, std::string classname) override {
            return "Int";
        }
        void type_infer(StaticSemantics* ssc, std::map<std::string, std::string>* vt, class_and_method* info) override { return; }
        void json(std::ostream& out, AST_print_context& ctx) override;
    };

    class Type_Alternative : public ASTNode {
        Ident& ident_;
        Ident& classname_;
        Block& block_;
    public:
        explicit Type_Alternative(Ident& ident, Ident& classname, Block& block) :
                ident_{ident}, classname_{classname}, block_{block} {}
        std::string get_var() override {return "";}
        void collect_vars(std::map<std::string, std::string>* vt) override {return;}
        void json(std::ostream& out, AST_print_context& ctx) override;
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
        void json(std::ostream& out, AST_print_context& ctx) override;
    };

    class StrConst : public Expr {
        std::string value_;
    public:
        explicit StrConst(std::string v) : value_{v} {}
        std::string get_type(std::map<std::string, std::string>* vt, StaticSemantics* ssc, std::string classname) override {
            return "String";
        }
        void type_infer(StaticSemantics* ssc, std::map<std::string, std::string>* vt, class_and_method* info) override { return; }
        void json(std::ostream& out, AST_print_context& ctx) override;
    };

    class Actuals : public Seq<Expr> {
    public:
        explicit Actuals() : Seq("Actuals") {}
    };

    /* Constructors are different from other method calls. They
      * are static (not looked up in the vtable), have no receiver
      * object, and have their own type-checking rules.
      */
    class Construct : public Expr {
        Ident&  method_;           /* Method name is same as class name */
        Actuals& actuals_;    /* Actual arguments to constructor */
    public:
        explicit Construct(Ident& method, Actuals& actuals) :
                method_{method}, actuals_{actuals} {}
        std::string get_type(std::map<std::string, std::string>* vt, StaticSemantics* ssc, std::string classname) override {
            std::cout << "ENTERING Construct::get_type" << std::endl;
            return method_.get_var();
        }
        void type_infer(StaticSemantics* ssc, std::map<std::string, std::string>* vt, class_and_method* info) override { 
            std::cout << "ENTERING Construct::type_infer" << std::endl;
            // TODO: actuals match signature?
            return; 
        }
        void json(std::ostream& out, AST_print_context& ctx) override;
    };

    /* Method calls are central to type checking and code
     * generation ... and for us, the operators +, -, etc
     * are method calls to specially named methods.
     */
    class Call : public Expr {
        Expr& receiver_;        /* Expression computing the receiver object */
        Ident& method_;         /* Identifier of the method */
        Actuals& actuals_;     /* List of actual arguments */
    public:
        explicit Call(Expr& receiver, Ident& method, Actuals& actuals) :
                receiver_{receiver}, method_{method}, actuals_{actuals} {};
        // Convenience factory for the special case of a method
        // created for a binary operator (+, -, etc).
        static Call* binop(std::string opname, Expr& receiver, Expr& arg);
        std::string get_type(std::map<std::string, std::string>* vt, StaticSemantics* ssc, std::string classname) override;
        void json(std::ostream& out, AST_print_context& ctx) override;
    };

    // Virtual base class for binary operations.
    // Does NOT include +, -, *, /, etc, which
    // are method calls.
    // Does include And, Or, Dot, ...
   class BinOp : public Expr {
    protected:
        std::string opsym;
        ASTNode &left_;
        ASTNode &right_;
        BinOp(std::string sym, ASTNode &l, ASTNode &r) :
                opsym{sym}, left_{l}, right_{r} {};
    public:
        void json(std::ostream& out, AST_print_context& ctx) override;
    };

   class And : public BinOp {
   public:
       explicit And(ASTNode& left, ASTNode& right) :
          BinOp("And", left, right) {}
        std::string get_type(std::map<std::string, std::string>* vt, StaticSemantics* ssc, std::string classname) override {
            return "Boolean";
        }
   };

    class Or : public BinOp {
    public:
        explicit Or(ASTNode& left, ASTNode& right) :
                BinOp("Or", left, right) {}
        std::string get_type(std::map<std::string, std::string>* vt, StaticSemantics* ssc, std::string classname) override {
            return "Boolean";
        }
    };

    class Not : public Expr {
        ASTNode& left_;
    public:
        explicit Not(ASTNode& left ):
            left_{left}  {}
        std::string get_type(std::map<std::string, std::string>* vt, StaticSemantics* ssc, std::string classname) override {
            return "Boolean";
        }
        void json(std::ostream& out, AST_print_context& ctx) override;
    };

    /* Can a field de-reference (expr . IDENT) be a binary
     * operation?  It can be evaluated to a location (l_exp),
     * whereas an operation like * and + cannot, but maybe that's
     * ok.  We'll tentatively group it with Binop and consider
     * changing it later if we need to make the distinction.
     */
    class Dot : public LExpr {
        Expr& left_;
        Ident& right_;
    public:
        std::string get_var() override {return left_.get_var() + "." + right_.get_var();}
        void collect_vars(std::map<std::string, std::string>* vt) override { return; }
        std::string get_type(std::map<std::string, std::string>* vt, StaticSemantics* ssc, std::string classname) override;
        void type_infer(StaticSemantics* ssc, std::map<std::string, std::string>* vt, class_and_method* info) override { 
            left_.type_infer(ssc, vt, info); 
            right_.type_infer(ssc, vt, info);
        }

        explicit Dot (Expr& left, Ident& right) :
           left_{left},  right_{right} {}
        void json(std::ostream& out, AST_print_context& ctx) override;
    };

    /* A program has a set of classes (in any order) and a block of
     * statements.
     */
    class Program : public ASTNode {
    public:
        Classes& classes_;
        Block& statements_;
        explicit Program(Classes& classes, Block& statements) :
                classes_{classes}, statements_{statements} {}
        std::string get_var() override {return "";}
        void collect_vars(std::map<std::string, std::string>* vt) override {return;}
        void json(std::ostream& out, AST_print_context& ctx) override;
    };
}
#endif //ASTNODE_H