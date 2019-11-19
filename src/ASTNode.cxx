//
// Created by Michal Young on 9/12/18.
//

#include "ASTNode.h"
#include "staticsemantics.cxx"

namespace AST {
    // Abstract syntax tree.  ASTNode is abstract base class for all other nodes.

    // Type checking functions defined here to avoid circular #include situation:

    void Assign::type_infer(StaticSemantics* ssc, std::map<std::string, std::string>* vt, std::string classname)  {
        std::cout << "ENTERING: Assign::type_infer" << std::endl;
        std::string lhs_var = lexpr_.get_var();
        //std::cout << "\tlhs_var = " << lhs_var << std::endl;
        std::string rhs_type = rexpr_.get_type(vt);
        //std::cout << "\trhs_type = " << rhs_type << std::endl;
        map<std::string, std::string> instancevars = (ssc->hierarchy)[classname].instance_vars;
        if (instancevars.count(lhs_var)) { // in class instance vars
            (*vt)[lhs_var] = instancevars[lhs_var]; // initialize in my table with other type
        }
        else { // NOT in class instance vars
            if (!vt->count(lhs_var) || ((*vt)[lhs_var] == "Bottom")) { // NOT in my table either (or it is but has value "Bottom")
                (*vt)[lhs_var] = rhs_type; // gets whatever rhs type is
                ssc->change_made = 1; // and something changed
                return; // and we're done!
            }
        } // end else
        // if we've made it this far, we can perform LCA on the variable, which is in the table and initialized
        std::string lhs_type = (*vt)[lhs_var];
        std::string lca = ssc->get_LCA(lhs_type, rhs_type);
        if (lhs_type != lca) { // change made only if we assign a new type to this var
            (*vt)[lhs_var] = lca;
            //std::cout << "^^^^^^^^^^ lhs type = " << lhs_type << ", LCA = " << lca << std::endl;
            ssc->change_made = 1;
        }
    }
    void Methods::type_infer(StaticSemantics* ssc, map<std::string, std::string>* vt, std::string classname) {
            std::cout << "ENTERING: Methods::type_infer" << std::endl;
            for (Method* method: elements_) {
                std::string methodname = method->name_.get_var();
                TypeNode classentry = ssc->hierarchy[classname];
                MethodTable methodtable = classentry.methods[methodname];
                std::map<std::string, std::string>* methodvars = methodtable.vars;
                method->type_infer(ssc, methodvars, classname);
            }
    }

    void Class::type_infer(StaticSemantics* ssc, std::map<std::string, std::string>* vt, std::string classname) {
            std::cout << "ENTERING CALL TO Class::type_infer" << std::endl;
            std::string classname = name_.get_var();
            std::map<std::string, std::string>* classinstancevars = &(ssc->hierarchy[classname].instance_vars);
            std::map<std::string, std::string>* construct_instvars = ssc->hierarchy[classname].construct.vars;
            constructor_.type_infer(ssc, construct_instvars, classname);
            // update class-level instance vars
            for(map<string, string>::iterator iter = classinstancevars->begin(); iter != classinstancevars->end(); ++iter) {
                (*classinstancevars)[iter->first] = (*construct_instvars)[iter->first];
            }
            //ssc->copy_instance_vars(classname);
            // How do we know correct table for each method in methods??
            // this version of type_infer is going to need the class name... 
            methods_.type_infer(ssc, vt, name_.get_var());
    }

    // JSON representation of all the concrete node types.
    // This might be particularly useful if I want to do some
    // tree manipulation in Python or another language.  We'll
    // do this by emitting into a stream.

    // --- Utility functions used by node-specific json output methods

    /* Indent to a given level */
    void ASTNode::json_indent(std::ostream& out, AST_print_context& ctx) {
        if (ctx.indent_ > 0) {
            out << std::endl;
        }
        for (int i=0; i < ctx.indent_; ++i) {
            out << "    ";
        }
    }

    /* The head element looks like { "kind" : "block", */
    void ASTNode::json_head(std::string node_kind, std::ostream& out, AST_print_context& ctx) {
        json_indent(out, ctx);
        out << "{ \"kind\" : \"" << node_kind << "\"," ;
        ctx.indent();  // one level more for children
        return;
    }

    void ASTNode::json_close(std::ostream& out, AST_print_context& ctx) {
        // json_indent(out, ctx);
        out << "}";
        ctx.dedent();
    }

    void ASTNode::json_child(std::string field, ASTNode& child, std::ostream& out, AST_print_context& ctx, char sep) {
        json_indent(out, ctx);
        out << "\"" << field << "\" : ";
        child.json(out, ctx);
        out << sep;
    }

    void Stub::json(std::ostream& out, AST_print_context& ctx) {
        json_head("Stub", out, ctx);
        json_indent(out, ctx);
        out  << "\"rule\": \"" <<  name_ << "\"";
        json_close(out, ctx);
    }


    void Program::json(std::ostream &out, AST::AST_print_context &ctx) {
        json_head("Program", out, ctx);
        json_child("classes_", classes_, out, ctx);
        json_child("statements_", statements_, out, ctx, ' ');
        json_close(out, ctx);
    }

    void Formal::json(std::ostream &out, AST::AST_print_context &ctx) {
        json_head("Formal", out, ctx);
        json_child("var_", var_, out, ctx);
        json_child("type_", type_, out, ctx, ' ');
        json_close(out, ctx);
    }

    void Method::json(std::ostream &out, AST::AST_print_context &ctx) {
        json_head("Method", out, ctx);
        json_child("name_", name_, out, ctx);
        json_child("formals_", formals_, out, ctx);
        json_child("returns_", returns_, out, ctx);
        json_child("statements_", statements_, out, ctx, ' ');
        json_close(out, ctx);
    }

    void Assign::json(std::ostream& out, AST_print_context& ctx) {
        json_head("Assign", out, ctx);
        json_child("lexpr_", lexpr_, out, ctx);
        json_child("rexpr_", rexpr_, out, ctx, ' ');
        json_close(out, ctx);
     }

    void AssignDeclare::json(std::ostream& out, AST_print_context& ctx) {
        json_head("Assign", out, ctx);
        json_child("lexpr_", lexpr_, out, ctx);
        json_child("rexpr_", rexpr_, out, ctx);
        json_child("static_type_", static_type_, out, ctx, ' ');
        json_close(out, ctx);
    }

    void Return::json(std::ostream &out, AST::AST_print_context &ctx) {
        json_head("Return", out, ctx);
        json_child("expr_", expr_, out, ctx, ' ');
        json_close(out, ctx);
    }

    void If::json(std::ostream& out, AST_print_context& ctx) {
        json_head("If", out, ctx);
        json_child("cond_", cond_, out, ctx);
        json_child("truepart_", truepart_, out, ctx);
        json_child("falsepart_", falsepart_, out, ctx, ' ');
        json_close(out, ctx);
    }

    void While::json(std::ostream& out, AST_print_context& ctx) {
        json_head("While", out, ctx);
        json_child("cond_", cond_, out, ctx);
        json_child("body_", body_, out, ctx, ' ');
        json_close(out, ctx);
    }


    void Typecase::json(std::ostream& out, AST_print_context& ctx) {
        json_head("Typecase", out, ctx);
        json_child("expr_", expr_, out, ctx);
        json_child("cases_", cases_, out, ctx, ' ');
        json_close(out, ctx);
    }

    void Type_Alternative::json(std::ostream& out, AST_print_context& ctx) {
        json_head("Type_Alternative", out, ctx);
        json_child("ident_", ident_, out, ctx);
        json_child("classname_", classname_, out, ctx);
        json_child("block_", block_, out, ctx, ' ');
        json_close(out, ctx);
    }

    void Load::json(std::ostream &out, AST::AST_print_context &ctx) {
        json_head("Load", out, ctx);
        json_child("loc_", loc_, out, ctx, ' ');
        json_close(out, ctx);
    }

    void Ident::json(std::ostream& out, AST_print_context& ctx) {
        json_head("Ident", out, ctx);
        out << "\"text_\" : \"" << text_ << "\"";
        json_close(out, ctx);
    }

    void Class::json(std::ostream &out, AST::AST_print_context &ctx) {
        json_head("Class", out, ctx);
        json_child("name_", name_, out, ctx);
        json_child("super_", super_, out, ctx);
        json_child("constructor_", constructor_, out, ctx);
        json_child("methods_", methods_, out, ctx, ' ');
        json_close(out, ctx);
    }

    void Call::json(std::ostream &out, AST::AST_print_context &ctx) {
        json_head("Call", out, ctx);
        json_child("obj_", receiver_, out, ctx);
        json_child("method_", method_, out, ctx);
        json_child("actuals_", actuals_, out, ctx, ' ');
        json_close(out, ctx);
    }

    void Construct::json(std::ostream &out, AST::AST_print_context &ctx) {
        json_head("Construct", out, ctx);
        json_child("method_", method_, out, ctx);
        json_child("actuals_", actuals_, out, ctx, ' ');
        json_close(out, ctx);
    }

    void IntConst::json(std::ostream& out, AST_print_context& ctx) {
        json_head("IntConst", out, ctx);
        out << "\"value_\" : " << value_ ;
        json_close(out, ctx);
    }

    void StrConst::json(std::ostream& out, AST_print_context& ctx) {
        json_head("StrConst", out, ctx);
        out << "\"value_\" : \"" << value_  << "\"";
        json_close(out, ctx);
    }


    void BinOp::json(std::ostream& out, AST_print_context& ctx) {
        json_head(opsym, out, ctx);
        json_child("left_", left_, out, ctx);
        json_child("right_", right_, out, ctx, ' ');
        json_close(out, ctx);
    }


    void Not::json(std::ostream& out, AST_print_context& ctx) {
        json_head("Not", out, ctx);
        json_child("left_", left_, out, ctx, ' ');
        json_close(out, ctx);
    }

    void Dot::json(std::ostream& out, AST_print_context& ctx) {
        json_head("Dot", out, ctx);
        json_child("left_", left_, out, ctx);
        json_child("right_", right_, out, ctx, ' ');
        json_close(out, ctx);
    }


    /* Convenience factory for operations like +, -, *, / */
    Call* Call::binop(std::string opname, Expr& receiver, Expr& arg) {
        Ident* method = new Ident(opname);
        Actuals* actuals = new Actuals();
        actuals->append(&arg);
        return new Call(receiver, *method, *actuals);
    }
}