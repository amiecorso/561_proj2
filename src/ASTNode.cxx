//
// Created by Michal Young on 9/12/18.
//

#include "ASTNode.h"
#include "staticsemantics.cxx"
#include <typeinfo>

namespace AST {
    // Abstract syntax tree.  ASTNode is abstract base class for all other nodes.

    // Type checking functions defined here to avoid circular #include situation:

    void AssignDeclare::type_infer(StaticSemantics* ssc, std::map<std::string, std::string>* vt, class_and_method* info)  {
        std::cout << "ENTERING: AssignDeclare::type_infer" << std::endl;
            info->print();
        std::string lhs_var = lexpr_.get_var();
        std::string static_type = static_type_.get_var();
        //std::cout << "\tlhs_var = " << lhs_var << std::endl;
        std::string rhs_type = rexpr_.get_type(vt, ssc, info->classname);
        //std::cout << "\trhs_type = " << rhs_type << std::endl;
        map<std::string, std::string> instancevars = (ssc->hierarchy)[info->classname].instance_vars;
        if (!vt->count(lhs_var)) { // NOT in my table
            if (instancevars.count(lhs_var)) { // in class instance vars
                (*vt)[lhs_var] = instancevars[lhs_var]; // initialize in my table with other type
            }
            else { // NOT in class instance vars either
                (*vt)[lhs_var] = static_type; // gets whatever rhs type is
                ssc->change_made = 1; // and something changed
                // not done yet... we've initialized w/static type but still need to perform type inference
            } // end else
        }
        // if we've made it this far, we can perform LCA on the variable, which is in the table and initialized
        std::string lhs_type = (*vt)[lhs_var];
        std::string lca = ssc->get_LCA(lhs_type, rhs_type);
        if (lhs_type != lca) { // change made only if we assign a new type to this var
            if (!(ssc->is_subtype(lca, static_type))) {
                (*vt)[lhs_var] = "TypeError";
                std::cout << "TypeError (AssignDeclare): RHS type " << lca << " is not subtype of static type " << static_type << std::endl;
                return;
            }
            (*vt)[lhs_var] = lca;
            std::cout << "^^^^^^^^^^ADADADAD lhs type = " << lhs_type << ", LCA = " << lca << std::endl;
            ssc->change_made = 1;
        }
    }

    void Assign::type_infer(StaticSemantics* ssc, std::map<std::string, std::string>* vt, class_and_method* info)  {
        std::cout << "ENTERING: Assign::type_infer" << std::endl;
            info->print();
        std::string lhs_var = lexpr_.get_var();
        //std::cout << "\tlhs_var = " << lhs_var << std::endl;
        std::string rhs_type = rexpr_.get_type(vt, ssc, info->classname);
        //std::cout << "\trhs_type = " << rhs_type << std::endl;
        map<std::string, std::string> instancevars = (ssc->hierarchy)[info->classname].instance_vars;
        if (!vt->count(lhs_var)) { // NOT in my table
            if (instancevars.count(lhs_var)) { // in class instance vars
                (*vt)[lhs_var] = instancevars[lhs_var]; // initialize in my table with other type
            }
            else { // NOT in class instance vars either
                (*vt)[lhs_var] = rhs_type; // gets whatever rhs type is
                ssc->change_made = 1; // and something changed
                return; // and we're done!
            } // end else
        }
        // if we've made it this far, we can perform LCA on the variable, which is in the table and initialized
        std::string lhs_type = (*vt)[lhs_var];
        std::string lca = ssc->get_LCA(lhs_type, rhs_type);
        if (lhs_type != lca) { // change made only if we assign a new type to this var
            (*vt)[lhs_var] = lca;
            std::cout << "^^^^^^^^^^ lhs type = " << lhs_type << ", LCA = " << lca << std::endl;
            ssc->change_made = 1;
        }
    }

    void Methods::type_infer(StaticSemantics* ssc, map<std::string, std::string>* vt, class_and_method* info) {
        std::cout << "ENTERING: Methods::type_infer" << std::endl;
            info->print();
        for (Method* method: elements_) {
            std::string methodname = method->name_.get_var();
            TypeNode classentry = ssc->hierarchy[info->classname];
            MethodTable methodtable = classentry.methods[methodname];
            std::map<std::string, std::string>* methodvars = methodtable.vars;
            class_and_method* methodinfo = new class_and_method(info->classname, methodname);
            method->type_infer(ssc, methodvars, methodinfo);
        }
    }

    void Classes::type_infer(StaticSemantics* ssc, std::map<std::string, std::string>* vt, class_and_method* info) {
        for (AST::Class *cls: elements_) {
            class_and_method* info = new class_and_method(cls->name_.get_var(), "");
            cls->type_infer(ssc, vt, info);
        }
        // CHECK WHETHER SUBCLASSES HAVE ALL SUPERCLASS INSTANCE VARIABLES
        std::map<std::string, TypeNode> hierarchy = ssc->hierarchy;
        for (AST::Class *cls: elements_) {
            std::string classname  = cls->name_.get_var();
            TypeNode classnode = hierarchy[classname];
            std::string parentname = classnode.parent;
            TypeNode parentnode = hierarchy[parentname];
            std::map<string, string> class_iv = classnode.instance_vars;
            std::map<string, string> parent_iv = parentnode.instance_vars;
            for(map<string, string>::iterator iter = parent_iv.begin(); iter != parent_iv.end(); ++iter) {
                std::string var_name = iter->first;
                if (!class_iv.count(var_name)) {
                    std::cout << "Error: class " << classname << " missing parent instance var " << var_name << std::endl;
                }
            }
        }
    }

    void Class::type_infer(StaticSemantics* ssc, std::map<std::string, std::string>* vt, class_and_method* info) {
            std::cout << "ENTERING Class::type_infer" << std::endl;
            info->print();
            std::map<std::string, std::string>* classinstancevars = &(ssc->hierarchy[info->classname].instance_vars);
            std::map<std::string, std::string>* construct_instvars = ssc->hierarchy[info->classname].construct.vars;
            constructor_.type_infer(ssc, construct_instvars, info);
            // update class-level instance vars
            for(map<string, string>::iterator iter = classinstancevars->begin(); iter != classinstancevars->end(); ++iter) {
                (*classinstancevars)[iter->first] = (*construct_instvars)[iter->first];
            }
            //ssc->copy_instance_vars(classname);
            // How do we know correct table for each method in methods??
            // this version of type_infer is going to need the class name... 
            info = new class_and_method(name_.get_var(), "");
            methods_.type_infer(ssc, vt, info);
    }

    void Return::type_infer(StaticSemantics* ssc, std::map<std::string, std::string>* vt, class_and_method* info) {
        std::cout << "ENTERING Return::type_infer" << std::endl;
        std::string methodname = info->methodname;
        TypeNode classnode = ssc->hierarchy[info->classname];
        MethodTable methodtable = classnode.methods[methodname];
        std::string methodreturntype = methodtable.returntype;
        std::string thisreturntype = expr_.get_type(vt, ssc, info->classname);
        if (!ssc->is_subtype(thisreturntype, methodreturntype)) {
            std::cout << "TypeError (Return): type of return expr " << thisreturntype << " is not subtype of method return type " << methodreturntype << std::endl;
        }

    }

    std::string Call::get_type(std::map<std::string, std::string>* vt, StaticSemantics* ssc, std::string classname) {
            std::cout << "ENTERING Call::get_type" << std::endl;
            std::string classtype = receiver_.get_type(vt, ssc, classname);
            std::string methodname = method_.get_var();
            std::cout << "\t Call::get_type receiver_ is type: " << typeid(receiver_).name() << std::endl;
            std::cout << "\t Call::get_type result of receiver_.get_type: " << classtype << std::endl;
            std::cout << "\t Call::get_type method_ is called: " << methodname << std::endl;
            std::map<std::string, TypeNode> hierarchy = ssc->hierarchy;
            if (!hierarchy.count(classtype)) { return "TypeError1";}
            TypeNode classnode = hierarchy[classtype];
            //std::cout << "\t and here's the TypeNode" << std::endl;
            //classnode.print();
            if (!(classnode.methods.count(methodname))) { return "TypeError2";}
            MethodTable methodnode = classnode.methods[methodname];
            // TODO: check if the signature of the method matches the formal args here?
            return methodnode.returntype;
    }

    std::string Dot::get_type(std::map<std::string, std::string>* vt, StaticSemantics* ssc, std::string classname) {
        std::cout << "ENTERING Dot::get_type" << std::endl;
        std::cout << "value of classname: " << classname << endl;
        std::string lhs_id = get_var();
        std::cout << "value of get_var: " << lhs_id << endl;
        if (vt->count(get_var())) {
            //return (*vt)[get_var()];
        }
        std::map<std::string, TypeNode> hierarchy = ssc->hierarchy;
        TypeNode classnode = hierarchy[classname];
        std::map<std::string, std::string> instancevars = classnode.instance_vars;
        /*
        std::cout << "PRINTING INSTANCE VARS" << endl;
        for(std::map<string, string>::iterator iter = instancevars.begin(); iter != instancevars.end(); ++iter) {
            cout << "\t\t" << iter->first << ":" << iter->second << endl;
        }
        */
        if (instancevars.count(get_var())) {
            return instancevars[get_var()];
        }
        return "TypeError";
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