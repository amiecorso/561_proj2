//
// Created by Michal Young on 9/12/18.
//

#include "ASTNode.h"
#include "staticsemantics.cxx"
#include <typeinfo>

namespace AST {
    // Abstract syntax tree.  ASTNode is abstract base class for all other nodes.

    int Program::initcheck(std::set<std::string>* vars, StaticSemantics* ssc) {
        for (map<string, TypeNode>::iterator iter = ssc->hierarchy.begin(); iter != ssc->hierarchy.end(); ++iter) {
            vars->insert(iter->first); // insert class name as constructor
            TypeNode classnode = iter->second;
            std::map<std::string, MethodTable> methods = classnode.methods;
            for(map<string, MethodTable>::iterator iter = methods.begin(); iter != methods.end(); ++iter) {
                vars->insert(iter->first); // insert method name
            }
        }
        if (classes_.initcheck(vars)) {return 1;}
        if (statements_.initcheck(vars)) {return 1;}
        return 0;
    }
            
    int Program::type_infer(StaticSemantics* ssc, std::map<std::string, std::string>* vt, class_and_method* info) { 
        int returnval = 0;
        if (classes_.type_infer(ssc, vt, info)) { returnval = 1; }
        class_and_method* pgminfo = new class_and_method("__pgm__", "");
        std::map<std::string, std::string>* pgmvt = &(ssc->hierarchy)["__pgm__"].instance_vars;
        if (statements_.type_infer(ssc, pgmvt, pgminfo)) { returnval = 1; }
        return returnval;
    }

    int Typecase::type_infer(StaticSemantics* ssc, std::map<std::string, std::string>* vt, class_and_method* info) {
        int returnval = 0;
        if (expr_.type_infer(ssc, vt, info)) {returnval = 1;}
        if (cases_.type_infer(ssc, vt, info)) {returnval = 1;}
        return returnval;
    }

    int Type_Alternative::type_infer(StaticSemantics* ssc, std::map<std::string, std::string>* vt, class_and_method* info) {
        int returnval = 0;
        if (ident_.type_infer(ssc, vt, info)) {returnval = 1;}
        if (classname_.type_infer(ssc, vt, info)) {returnval = 1;}
        // copy table
        std::map<std::string, std::string>* localvt = new std::map<std::string, std::string>(*vt);
        (*localvt)[ident_.get_var()] = classname_.get_var();
        if (block_.type_infer(ssc, localvt, info)) {returnval = 1;}
        // TODO: do we need to put any changes to the local vars here into the original vt?? to make sure we start
        // where we left off next iteration?
        return returnval;
    }

    int Construct::type_infer(StaticSemantics* ssc, std::map<std::string, std::string>* vt, class_and_method* info) { 
        std::cout << "ENTERING Construct::type_infer" << std::endl;
        // recursive call to type-check actual args
        actuals_.type_infer(ssc, vt, info);
        // verify that the construct call matches signature
        std::map<std::string, TypeNode> hierarchy = ssc->hierarchy;
        std::string methodname = method_.get_var();
        TypeNode classnode = hierarchy[methodname]; // method name same as class name
        MethodTable constructortable = classnode.construct;        
        if (constructortable.formalargtypes.size() != actuals_.elements_.size()) {
            std::cout << "Error (Construct): number of actual args (" << actuals_.elements_.size()
                            << ") does not match method signature (" << constructortable.formalargtypes.size() 
                            << ") for call: " << methodname << "(...)" << std::endl;
            return 0;
        }
        for (int i = 0; i < actuals_.elements_.size(); i++) {
            std::string formaltype = constructortable.formalargtypes[i];
            std::string actualtype = actuals_.elements_[i]->get_type(vt, ssc, info->classname);
            //std::cout << "Formal type: " << formaltype << "||" << "Actual type: " << actualtype << std::endl;
            if (!ssc->is_subtype(actualtype, formaltype)) {
                std::cout << "Error (Construct): actual args do not match method signature for constructor call: "
                                        << methodname << "(...)" << std::endl;
                return 0;
            }
        }
        return 0; 
    }

    int If::type_infer(StaticSemantics* ssc, std::map<std::string, std::string>* vt, class_and_method* info) {
        std::cout << "ENTERING If::type_infer" << std::endl;
        std::string cond_type = cond_.get_type(vt, ssc, info->classname);
        if (cond_type != "Boolean") {
            std::cout << "TypeError (If): Condition does not evaluate to type Boolean (ignoring statements)" << std::endl;
            return 0;
        }
        int returnval = 0;
        if (truepart_.type_infer(ssc, vt, info)) {returnval = 1;}
        if (falsepart_.type_infer(ssc, vt, info)) {returnval = 1;}
        return returnval;
    }

    int Call::type_infer(StaticSemantics* ssc, std::map<std::string, std::string>* vt, class_and_method* info) {
        receiver_.type_infer(ssc, vt, info);
        method_.type_infer(ssc, vt, info); // this does nothing
        actuals_.type_infer(ssc, vt, info);
        std::string receivertype = receiver_.get_type(vt, ssc, info->classname);
        // is this method in the type of the receiver?
        std::map<std::string, TypeNode> hierarchy = ssc->hierarchy;
        std::string methodname = method_.get_var();
        TypeNode recvnode = hierarchy[receivertype];
        std::map<std::string, MethodTable> methods = recvnode.methods;
        if (!methods.count(methodname)) {
            std::cout << "Error (Call): method " << methodname << " is not defined for type " << receivertype << endl;
            return 0;
        }
        MethodTable methodtable = methods[methodname];
        if (methodtable.formalargtypes.size() != actuals_.elements_.size()) {
            std::cout << "Error (Call): number of actual args (" << actuals_.elements_.size()
                            << ") does not match method signature (" << methodtable.formalargtypes.size() 
                            << ") for call: " << receiver_.get_var() << "." << methodname << "(...)" << std::endl;
            return 0;
        }
        for (int i = 0; i < actuals_.elements_.size(); i++) {
            std::string formaltype = methodtable.formalargtypes[i];
            std::string actualtype = actuals_.elements_[i]->get_type(vt, ssc, info->classname);
            std::cout << "Formal type: " << formaltype << "||" << "Actual type: " << actualtype << std::endl;
            if (!ssc->is_subtype(actualtype, formaltype)) {
                std::cout << "Error (Call): actual args do not conform to method signature for call: " << 
                                        receiver_.get_var() << "." << methodname << "(...)" << std::endl;
                return 0;
            }
        }
        return 0;
    }

    int AssignDeclare::type_infer(StaticSemantics* ssc, std::map<std::string, std::string>* vt, class_and_method* info)  {
        std::cout << "ENTERING: AssignDeclare::type_infer" << std::endl;
        int returnval = 0;        
        lexpr_.type_infer(ssc, vt, info);
        rexpr_.type_infer(ssc, vt, info);
        std::string lhs_var = lexpr_.get_var();
        std::string static_type = static_type_.get_var();
        std::string rhs_type = rexpr_.get_type(vt, ssc, info->classname);
        map<std::string, std::string> instancevars = (ssc->hierarchy)[info->classname].instance_vars;
        if (!vt->count(lhs_var)) { // NOT in my table
            if (instancevars.count(lhs_var)) { // in class instance vars
                (*vt)[lhs_var] = instancevars[lhs_var]; // initialize in my table with other type
            }
            else { // NOT in class instance vars either
                (*vt)[lhs_var] = static_type; // gets whatever rhs type is
                returnval = 1; // and something changed
            } // end else
        }
        // if we've made it this far, we can perform LCA on the variable, which is in the table and initialized
        std::string lhs_type = (*vt)[lhs_var];
        std::string lca = ssc->get_LCA(lhs_type, rhs_type);
        if (lhs_type != lca) { // change made only if we assign a new type to this var
            if (!(ssc->is_subtype(lca, static_type))) {
                (*vt)[lhs_var] = "TypeError";
                std::cout << "TypeError (AssignDeclare): RHS type " << lca << " is not subtype of static type " << static_type << std::endl;
                returnval = 0;
            }
            (*vt)[lhs_var] = lca;
            returnval = 1;
        }
        return returnval;
    }

    int Assign::type_infer(StaticSemantics* ssc, std::map<std::string, std::string>* vt, class_and_method* info)  {
        std::cout << "ENTERING: Assign::type_infer" << std::endl;
        int returnval = 0;
        lexpr_.type_infer(ssc, vt, info);
        rexpr_.type_infer(ssc, vt, info);
        std::string lhs_var = lexpr_.get_var();
        std::string rhs_type = rexpr_.get_type(vt, ssc, info->classname);
        map<std::string, std::string> instancevars = (ssc->hierarchy)[info->classname].instance_vars;
        if (!vt->count(lhs_var)) { // NOT in my table
            if (instancevars.count(lhs_var)) { // in class instance vars
                (*vt)[lhs_var] = instancevars[lhs_var]; // initialize in my table with other type
            }
            else { // NOT in class instance vars either
                (*vt)[lhs_var] = rhs_type; // gets whatever rhs type is
                returnval = 1; // and something changed
            } // end else
        }
        // if we've made it this far, we can perform LCA on the variable, which is in the table and initialized
        std::string lhs_type = (*vt)[lhs_var];
        std::string lca = ssc->get_LCA(lhs_type, rhs_type);
        if (lhs_type != lca) { // change made only if we assign a new type to this var
            (*vt)[lhs_var] = lca;
            returnval = 1;
        }
        return returnval;
    }

    int Methods::type_infer(StaticSemantics* ssc, map<std::string, std::string>* vt, class_and_method* info) {
        std::cout << "ENTERING: Methods::type_infer" << std::endl;
        int returnval = 0;
        for (Method* method: elements_) {
            std::string methodname = method->name_.get_var();
            TypeNode classentry = ssc->hierarchy[info->classname];
            MethodTable methodtable = classentry.methods[methodname];
            std::map<std::string, std::string>* methodvars = methodtable.vars;
            std::map<std::string, std::string> oldmethodvars = std::map<std::string, std::string>(*methodvars); // copy constructor?
            class_and_method* methodinfo = new class_and_method(info->classname, methodname);
            method->type_infer(ssc, methodvars, methodinfo);
            // WE WILL FIGURE OUT RETURN TYPE LATER
            // Before we remove them, are the class instance vars being assigned conformant types?
            std::map<std::string, std::string> classinstance = classentry.instance_vars;
            for(map<string, string>::iterator iter = methodvars->begin(); iter != methodvars->end(); ++iter) {
                if (classinstance.count(iter->first)) { // if this var is in the class instance table
                    std::string methodtype = iter->second;
                    std::string classtype = classinstance[iter->first];
                    if (!ssc->is_subtype(methodtype, classtype)) {
                        std::cout << "Error (Methods): instance variable " << iter->first << " assigned non-conformant"
                                        << " type in method " << methodname << ". Instance type: " << classtype
                                        << ", Method assigned type: " << methodtype << std::endl;
                    }
                }
            }
            // at this point, we need to REMOVE any class instance variables from the method table:
            std::vector<std::string> erasables = std::vector<std::string>();
            for(map<string, string>::iterator iter = methodvars->begin(); iter != methodvars->end(); ++iter) {
                if (classinstance.count(iter->first)) { // if this var is in the class instance table
                    erasables.push_back(iter->first);
                }
            }
            for (std::string eraseme: erasables) {
                if (methodvars->count(eraseme)) {
                    methodvars->erase(eraseme);// remove it
                }
            }
            // finally, we need to decide if this method actually changed anything!
            if (!ssc->compare_maps(oldmethodvars, *methodvars)) { // maps not equal
                returnval = 1;
            }

        }
        return returnval;
    }

    int Classes::type_infer(StaticSemantics* ssc, std::map<std::string, std::string>* vt, class_and_method* info) {
        int returnval = 0;
        for (AST::Class *cls: elements_) {
            class_and_method* info = new class_and_method(cls->name_.get_var(), "");
            if (cls->type_infer(ssc, vt, info)) {
                returnval = 1;
            }
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
        return returnval;
    }

    int Class::type_infer(StaticSemantics* ssc, std::map<std::string, std::string>* vt, class_and_method* info) {
            int returnval = 0;
            std::cout << "ENTERING Class::type_infer" << std::endl;
            info->print();
            std::map<std::string, std::string>* classinstancevars = &(ssc->hierarchy[info->classname].instance_vars);
            std::map<std::string, std::string>* construct_instvars = ssc->hierarchy[info->classname].construct.vars;
            if (constructor_.type_infer(ssc, construct_instvars, info)) {
                returnval = 1;
            }
            // update class-level instance vars
            for(map<string, string>::iterator iter = classinstancevars->begin(); iter != classinstancevars->end(); ++iter) {
                if (iter->first.rfind("this", 0) == 0) {
                    (*classinstancevars)[iter->first] = (*construct_instvars)[iter->first];
                }   
            }
            info = new class_and_method(name_.get_var(), "");
            if (methods_.type_infer(ssc, vt, info)) {
                returnval = 1;
            }
            return returnval;
    }

    int Return::type_infer(StaticSemantics* ssc, std::map<std::string, std::string>* vt, class_and_method* info) {
        std::cout << "ENTERING Return::type_infer" << std::endl;
        std::string methodname = info->methodname;
        TypeNode classnode = ssc->hierarchy[info->classname];
        MethodTable methodtable = classnode.methods[methodname];
        std::string methodreturntype = methodtable.returntype;
        std::string thisreturntype = expr_.get_type(vt, ssc, info->classname);
        if (!ssc->is_subtype(thisreturntype, methodreturntype)) {
            std::cout << "TypeError (Return): type of return expr " << thisreturntype << " is not subtype of method return type " << methodreturntype << std::endl;
        }
        return 0;
    }
    std::string Construct::get_type(std::map<std::string, std::string>* vt, StaticSemantics* ssc, std::string classname) {
        std::cout << "ENTERING Construct::get_type" << std::endl;
        std::string methodname = method_.get_var();
       // verify that the construct call matches signature
        std::map<std::string, TypeNode> hierarchy = ssc->hierarchy;
        TypeNode classnode = hierarchy[methodname]; // method name same as class name
        MethodTable constructortable = classnode.construct;        
        if (constructortable.formalargtypes.size() != actuals_.elements_.size()) {
            std::cout << "Error (Construct): number of actual args (" << actuals_.elements_.size()
                            << ") does not match method signature (" << constructortable.formalargtypes.size() 
                            << ") for call: " << methodname << "(...)" << std::endl;
            return "TypeError";
        }
        for (int i = 0; i < actuals_.elements_.size(); i++) {
            std::string formaltype = constructortable.formalargtypes[i];
            std::string actualtype = actuals_.elements_[i]->get_type(vt, ssc, classname);
            if (!ssc->is_subtype(actualtype, formaltype)) {
                std::cout << "Error (Construct): actual args do not conform to method signature for constructor call: "
                                        << methodname << "(...)" << std::endl;
                return "TypeError";
            }
        }
        return methodname;
    }


    std::string Call::get_type(std::map<std::string, std::string>* vt, StaticSemantics* ssc, std::string classname) {
            std::cout << "ENTERING Call::get_type" << std::endl;
            std::string classtype = receiver_.get_type(vt, ssc, classname);
            std::string methodname = method_.get_var();
            std::map<std::string, TypeNode> hierarchy = ssc->hierarchy;
            TypeNode classnode = hierarchy[classtype];
            if (!(classnode.methods.count(methodname))) { 
                std::cout << "Error (Call): method " << methodname << "does not exist in class " <<
                            classtype << std::endl;
                return "TypeError";
            }
            MethodTable methodnode = classnode.methods[methodname];
            // Check if the signature of the method matches the formal args here
            if (methodnode.formalargtypes.size() != actuals_.elements_.size()) {
                std::cout << "Error (Call): number of actual args (" << actuals_.elements_.size()
                                << ") does not match method signature (" << methodnode.formalargtypes.size() 
                                << ") for call: " << receiver_.get_var() << "." << methodname << "(...)" << std::endl;
                return "TypeError";
            }
            for (int i = 0; i < actuals_.elements_.size(); i++) {
                std::string formaltype = methodnode.formalargtypes[i];
                std::string actualtype = actuals_.elements_[i]->get_type(vt, ssc, classname);
                std::cout << "Formal type: " << formaltype << "||" << "Actual type: " << actualtype << std::endl;
                if (!ssc->is_subtype(actualtype, formaltype)) {
                    std::cout << "Error (Call): actual args do not conform to method signature for call: " << 
                                            receiver_.get_var() << "." << methodname << "(...)" << std::endl;
                    return "TypeError";
                }
            }
            return methodnode.returntype;
    }

    std::string Dot::get_type(std::map<std::string, std::string>* vt, StaticSemantics* ssc, std::string classname) {
        std::cout << "ENTERING Dot::get_type" << std::endl;
        std::string lhs_id = get_var();
        std::map<std::string, TypeNode> hierarchy = ssc->hierarchy;
        TypeNode classnode = hierarchy[classname];
        std::map<std::string, std::string> instancevars = classnode.instance_vars;
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