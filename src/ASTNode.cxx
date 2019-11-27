//
// Created by Michal Young on 9/12/18.
//

#include "ASTNode.h"
#include "staticsemantics.cxx"
#include <typeinfo>

using namespace std;

namespace AST {
    // Abstract syntax tree.  ASTNode is abstract base class for all other nodes.

    int Program::initcheck(set<string>* vars, StaticSemantics* ssc) {
        for (map<string, TypeNode>::iterator iter = ssc->hierarchy.begin(); iter != ssc->hierarchy.end(); ++iter) {
            vars->insert(iter->first); // insert class name as constructor
            TypeNode classnode = iter->second;
            map<string, MethodTable> methods = classnode.methods;
            for(map<string, MethodTable>::iterator iter = methods.begin(); iter != methods.end(); ++iter) {
                vars->insert(iter->first); // insert method name
            }
        }
        if (classes_.initcheck(vars)) {return 1;}
        if (statements_.initcheck(vars)) {return 1;}
        return 0;
    }
            
    int Program::type_infer(StaticSemantics* ssc, map<string, string>* vt, class_and_method* info) { 
        int returnval = 0;
        if (classes_.type_infer(ssc, vt, info)) { returnval = 1; }
        class_and_method* pgminfo = new class_and_method("__pgm__", "");
        map<string, string>* pgmvt = &(ssc->hierarchy)["__pgm__"].instance_vars;
        if (statements_.type_infer(ssc, pgmvt, pgminfo)) { returnval = 1; }
        return returnval;
    }

    int Typecase::type_infer(StaticSemantics* ssc, map<string, string>* vt, class_and_method* info) {
        int returnval = 0;
        if (expr_.type_infer(ssc, vt, info)) {returnval = 1;}
        if (cases_.type_infer(ssc, vt, info)) {returnval = 1;}
        return returnval;
    }

    int Type_Alternative::type_infer(StaticSemantics* ssc, map<string, string>* vt, class_and_method* info) {
        int returnval = 0;
        if (ident_.type_infer(ssc, vt, info)) {returnval = 1;}
        if (classname_.type_infer(ssc, vt, info)) {returnval = 1;}
        // copy table
        map<string, string>* localvt = new map<string, string>(*vt);
        (*localvt)[ident_.get_var()] = classname_.get_var();
        if (block_.type_infer(ssc, localvt, info)) {returnval = 1;}
        // TODO: do we need to put any changes to the local vars here into the original vt?? to make sure we start
        // where we left off next iteration?
        return returnval;
    }

    int Construct::type_infer(StaticSemantics* ssc, map<string, string>* vt, class_and_method* info) { 
        cout << "ENTERING Construct::type_infer" << endl;
        // recursive call to type-check actual args
        actuals_.type_infer(ssc, vt, info);
        // verify that the construct call matches signature
        map<string, TypeNode> hierarchy = ssc->hierarchy;
        string methodname = method_.get_var();
        TypeNode classnode = hierarchy[methodname]; // method name same as class name
        MethodTable constructortable = classnode.construct;        
        if (constructortable.formalargtypes.size() != actuals_.elements_.size()) {
            cout << "Error (Construct): number of actual args (" << actuals_.elements_.size()
                            << ") does not match method signature (" << constructortable.formalargtypes.size() 
                            << ") for call: " << methodname << "(...)" << endl;
            return 0;
        }
        for (int i = 0; i < actuals_.elements_.size(); i++) {
            string formaltype = constructortable.formalargtypes[i];
            string actualtype = actuals_.elements_[i]->get_type(vt, ssc, info->classname);
            //cout << "Formal type: " << formaltype << "||" << "Actual type: " << actualtype << endl;
            if (!ssc->is_subtype(actualtype, formaltype)) {
                cout << "Error (Construct): actual args do not match method signature for constructor call: "
                                        << methodname << "(...)" << endl;
                return 0;
            }
        }
        return 0; 
    }

    int If::type_infer(StaticSemantics* ssc, map<string, string>* vt, class_and_method* info) {
        cout << "ENTERING If::type_infer" << endl;
        string cond_type = cond_.get_type(vt, ssc, info->classname);
        if (cond_type != "Boolean") {
            cout << "TypeError (If): Condition does not evaluate to type Boolean (ignoring statements)" << endl;
            return 0;
        }
        int returnval = 0;
        if (truepart_.type_infer(ssc, vt, info)) {returnval = 1;}
        if (falsepart_.type_infer(ssc, vt, info)) {returnval = 1;}
        return returnval;
    }

    int Call::type_infer(StaticSemantics* ssc, map<string, string>* vt, class_and_method* info) {
        receiver_.type_infer(ssc, vt, info);
        method_.type_infer(ssc, vt, info); // this does nothing
        actuals_.type_infer(ssc, vt, info);
        string receivertype = receiver_.get_type(vt, ssc, info->classname);
        // is this method in the type of the receiver?
        map<string, TypeNode> hierarchy = ssc->hierarchy;
        string methodname = method_.get_var();
        TypeNode recvnode = hierarchy[receivertype];
        map<string, MethodTable> methods = recvnode.methods;
        if (!methods.count(methodname)) {
            // can't find method in own class' method table!!
            // let's search the parent(s)
            while (1) {
                string classname = recvnode.parent;
                TypeNode parentnode = hierarchy[classname];
                methods = parentnode.methods;
                if (methods.count(methodname)) {break;} // we found it!
                if (classname == "Obj") {
                    cout << "Error (Call): method " << methodname << " is not defined for type " << receivertype << endl;
                    return 0;
                }
            }
        }
        MethodTable methodtable = methods[methodname];
        if (methodtable.formalargtypes.size() != actuals_.elements_.size()) {
            cout << "Error (Call): number of actual args (" << actuals_.elements_.size()
                            << ") does not match method signature (" << methodtable.formalargtypes.size() 
                            << ") for call: " << receiver_.get_var() << "." << methodname << "(...)" << endl;
            return 0;
        }
        for (int i = 0; i < actuals_.elements_.size(); i++) {
            string formaltype = methodtable.formalargtypes[i];
            string actualtype = actuals_.elements_[i]->get_type(vt, ssc, info->classname);
            cout << "Formal type: " << formaltype << "||" << "Actual type: " << actualtype << endl;
            if (!ssc->is_subtype(actualtype, formaltype)) {
                cout << "Error (Call): actual args do not conform to method signature for call: " << 
                                        receiver_.get_var() << "." << methodname << "(...)" << endl;
                return 0;
            }
        }
        return 0;
    }

    int AssignDeclare::type_infer(StaticSemantics* ssc, map<string, string>* vt, class_and_method* info)  {
        cout << "ENTERING: AssignDeclare::type_infer" << endl;
        int returnval = 0;        
        lexpr_.type_infer(ssc, vt, info);
        rexpr_.type_infer(ssc, vt, info);
        string lhs_var = lexpr_.get_var();
        string static_type = static_type_.get_var();
        string rhs_type = rexpr_.get_type(vt, ssc, info->classname);
        map<string, string> instancevars = (ssc->hierarchy)[info->classname].instance_vars;
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
        string lhs_type = (*vt)[lhs_var];
        string lca = ssc->get_LCA(lhs_type, rhs_type);
        if (lhs_type != lca) { // change made only if we assign a new type to this var
            if (!(ssc->is_subtype(lca, static_type))) {
                (*vt)[lhs_var] = "TypeError";
                cout << "TypeError (AssignDeclare): RHS type " << lca << " is not subtype of static type " << static_type << endl;
                returnval = 0;
            }
            (*vt)[lhs_var] = lca;
            returnval = 1;
        }
        return returnval;
    }

    int Assign::type_infer(StaticSemantics* ssc, map<string, string>* vt, class_and_method* info)  {
        cout << "ENTERING: Assign::type_infer" << endl;
        int returnval = 0;
        lexpr_.type_infer(ssc, vt, info);
        rexpr_.type_infer(ssc, vt, info);
        string lhs_var = lexpr_.get_var();
        string rhs_type = rexpr_.get_type(vt, ssc, info->classname);
        map<string, string> instancevars = (ssc->hierarchy)[info->classname].instance_vars;
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
        string lhs_type = (*vt)[lhs_var];
        string lca = ssc->get_LCA(lhs_type, rhs_type);
        if (lhs_type != lca) { // change made only if we assign a new type to this var
            (*vt)[lhs_var] = lca;
            returnval = 1;
        }
        return returnval;
    }

    int Methods::type_infer(StaticSemantics* ssc, map<string, string>* vt, class_and_method* info) {
        cout << "ENTERING: Methods::type_infer" << endl;
        int returnval = 0;
        for (Method* method: elements_) {
            string methodname = method->name_.get_var();
            TypeNode classentry = ssc->hierarchy[info->classname];
            MethodTable methodtable = classentry.methods[methodname];
            map<string, string>* methodvars = methodtable.vars;
            map<string, string> oldmethodvars = map<string, string>(*methodvars); // copy constructor?
            class_and_method* methodinfo = new class_and_method(info->classname, methodname);
            method->type_infer(ssc, methodvars, methodinfo);
            // WE WILL FIGURE OUT RETURN TYPE LATER
            // Before we remove them, are the class instance vars being assigned conformant types?
            map<string, string> classinstance = classentry.instance_vars;
            for(map<string, string>::iterator iter = methodvars->begin(); iter != methodvars->end(); ++iter) {
                if (classinstance.count(iter->first)) { // if this var is in the class instance table
                    string methodtype = iter->second;
                    string classtype = classinstance[iter->first];
                    if (!ssc->is_subtype(methodtype, classtype)) {
                        cout << "Error (Methods): instance variable " << iter->first << " assigned non-conformant"
                                        << " type in method " << methodname << ". Instance type: " << classtype
                                        << ", Method assigned type: " << methodtype << endl;
                    }
                }
            }
            // at this point, we need to REMOVE any class instance variables from the method table:
            vector<string> erasables = vector<string>();
            for(map<string, string>::iterator iter = methodvars->begin(); iter != methodvars->end(); ++iter) {
                if (classinstance.count(iter->first)) { // if this var is in the class instance table
                    erasables.push_back(iter->first);
                }
            }
            for (string eraseme: erasables) {
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

    int Classes::type_infer(StaticSemantics* ssc, map<string, string>* vt, class_and_method* info) {
        int returnval = 0;
        for (AST::Class *cls: elements_) {
            class_and_method* info = new class_and_method(cls->name_.get_var(), "");
            if (cls->type_infer(ssc, vt, info)) {
                returnval = 1;
            }
        }
        // CHECK WHETHER SUBCLASSES HAVE ALL SUPERCLASS INSTANCE VARIABLES
        map<string, TypeNode> hierarchy = ssc->hierarchy;
        for (AST::Class *cls: elements_) {
            string classname  = cls->name_.get_var();
            TypeNode classnode = hierarchy[classname];
            string parentname = classnode.parent;
            TypeNode parentnode = hierarchy[parentname];
            map<string, string> class_iv = classnode.instance_vars;
            map<string, string> parent_iv = parentnode.instance_vars;
            for(map<string, string>::iterator iter = parent_iv.begin(); iter != parent_iv.end(); ++iter) {
                string var_name = iter->first;
                if (!class_iv.count(var_name)) {
                    cout << "Error: class " << classname << " missing parent instance var " << var_name << endl;
                }
            }
        }
        return returnval;
    }

    int Class::type_infer(StaticSemantics* ssc, map<string, string>* vt, class_and_method* info) {
            int returnval = 0;
            cout << "ENTERING Class::type_infer" << endl;
            info->print();
            map<string, string>* classinstancevars = &(ssc->hierarchy[info->classname].instance_vars);
            map<string, string>* construct_instvars = ssc->hierarchy[info->classname].construct.vars;
            if (constructor_.type_infer(ssc, construct_instvars, info)) {
                returnval = 1;
            }
            // update class-level instance vars
            for(map<string, string>::iterator iter = classinstancevars->begin(); iter != classinstancevars->end(); ++iter) {
                if (iter->first.rfind("this", 0) == 0) {
                    (*classinstancevars)[iter->first] = (*construct_instvars)[iter->first];
                    vector<string> splitthis = ssc->split(iter->first, '.');
                    if (splitthis.size() == 2) {
                        (*classinstancevars)[splitthis[1]] = (*construct_instvars)[iter->first];
                    }
                }   
            }
            (*classinstancevars)["this"] = name_.get_var(); // put a this in there!

            info = new class_and_method(name_.get_var(), "");
            if (methods_.type_infer(ssc, vt, info)) {
                returnval = 1;
            }
            return returnval;
    }

    int Return::type_infer(StaticSemantics* ssc, map<string, string>* vt, class_and_method* info) {
        cout << "ENTERING Return::type_infer" << endl;
        string methodname = info->methodname;
        TypeNode classnode = ssc->hierarchy[info->classname];
        MethodTable methodtable = classnode.methods[methodname];
        string methodreturntype = methodtable.returntype;
        string thisreturntype = expr_.get_type(vt, ssc, info->classname);
        if (!ssc->is_subtype(thisreturntype, methodreturntype)) {
            cout << "TypeError (Return): type of return expr " << thisreturntype << " is not subtype of method return type " << methodreturntype << endl;
        }
        return 0;
    }
    string Construct::get_type(map<string, string>* vt, StaticSemantics* ssc, string classname) {
        cout << "ENTERING Construct::get_type" << endl;
        string methodname = method_.get_var();
       // verify that the construct call matches signature
        map<string, TypeNode> hierarchy = ssc->hierarchy;
        TypeNode classnode = hierarchy[methodname]; // method name same as class name
        MethodTable constructortable = classnode.construct;        
        if (constructortable.formalargtypes.size() != actuals_.elements_.size()) {
            cout << "Error (Construct): number of actual args (" << actuals_.elements_.size()
                            << ") does not match method signature (" << constructortable.formalargtypes.size() 
                            << ") for call: " << methodname << "(...)" << endl;
            return "TypeError";
        }
        for (int i = 0; i < actuals_.elements_.size(); i++) {
            string formaltype = constructortable.formalargtypes[i];
            string actualtype = actuals_.elements_[i]->get_type(vt, ssc, classname);
            if (!ssc->is_subtype(actualtype, formaltype)) {
                cout << "Error (Construct): actual args do not conform to method signature for constructor call: "
                                        << methodname << "(...)" << endl;
                return "TypeError";
            }
        }
        return methodname;
    }


    string Call::get_type(map<string, string>* vt, StaticSemantics* ssc, string classname) {
            cout << "ENTERING Call::get_type" << endl;
            string classtype = receiver_.get_type(vt, ssc, classname);
            string methodname = method_.get_var();
            map<string, TypeNode> hierarchy = ssc->hierarchy;
            TypeNode classnode = hierarchy[classtype];
            if (!(classnode.methods.count(methodname))) { 
                cout << "Error (Call): method " << methodname << "does not exist in class " <<
                            classtype << endl;
                return "TypeError";
            }
            MethodTable methodnode = classnode.methods[methodname];
            // Check if the signature of the method matches the formal args here
            if (methodnode.formalargtypes.size() != actuals_.elements_.size()) {
                cout << "Error (Call): number of actual args (" << actuals_.elements_.size()
                                << ") does not match method signature (" << methodnode.formalargtypes.size() 
                                << ") for call: " << receiver_.get_var() << "." << methodname << "(...)" << endl;
                return "TypeError";
            }
            for (int i = 0; i < actuals_.elements_.size(); i++) {
                string formaltype = methodnode.formalargtypes[i];
                string actualtype = actuals_.elements_[i]->get_type(vt, ssc, classname);
                cout << "Formal type: " << formaltype << "||" << "Actual type: " << actualtype << endl;
                if (!ssc->is_subtype(actualtype, formaltype)) {
                    cout << "Error (Call): actual args do not conform to method signature for call: " << 
                                            receiver_.get_var() << "." << methodname << "(...)" << endl;
                    return "TypeError";
                }
            }
            return methodnode.returntype;
    }

    string Ident::get_type(map<string, string>* vt, StaticSemantics* ssc, string classname) {
        //cout << "ENTERING: Ident::get_type" << endl;
        if (text_ == "this") {
            TypeNode classnode = ssc->hierarchy[classname];
            map<string, string> instancevars = classnode.instance_vars;
            if (instancevars.count(text_)) {return instancevars[text_];}
            else { return "TypeErrorthissss";}
        }
        if (vt->count(text_)) { // Identifier in table
            return (*vt)[text_];
        }
        else { // not in table!!
            cout << "TypeError: Identifier " << text_ << " uninitialized" << endl;
            return "TypeError"; // error?? 
        }
    }

    string Dot::get_type(map<string, string>* vt, StaticSemantics* ssc, string classname) {
        cout << "ENTERING Dot::get_type" << endl;
        /*
        string lhs_id = left_.get_var();
        map<string, TypeNode> hierarchy = ssc->hierarchy;
        TypeNode classnode = hierarchy[classname];
        map<string, string> instancevars = classnode.instance_vars;
        if (instancevars.count(lhs_id)) {
            return instancevars[lhs_id];
        }
        return "TypeError";
        */
        string rhs_id = right_.get_var();
        string lhs_id = left_.get_var();
        string lhs_type = left_.get_type(vt, ssc, classname);
        cout << "-------  rhs_id: " << rhs_id << endl;
        cout << "-------  lhs_id: " << lhs_id << endl;
        cout << "-------  lhs_type: " << lhs_type << endl;
        cout << "--------  class name: " << classname << endl;
        map<string, TypeNode> hierarchy = ssc->hierarchy;
        TypeNode classnode = hierarchy[lhs_type];
        map<string, string> instancevars = classnode.instance_vars;
        if (instancevars.count(rhs_id)) {
            return instancevars[rhs_id];
        }
        return "TypeError";

    }
    // JSON representation of all the concrete node types.
    // This might be particularly useful if I want to do some
    // tree manipulation in Python or another language.  We'll
    // do this by emitting into a stream.

    // --- Utility functions used by node-specific json output methods

    /* Indent to a given level */
    void ASTNode::json_indent(ostream& out, AST_print_context& ctx) {
        if (ctx.indent_ > 0) {
            out << endl;
        }
        for (int i=0; i < ctx.indent_; ++i) {
            out << "    ";
        }
    }

    /* The head element looks like { "kind" : "block", */
    void ASTNode::json_head(string node_kind, ostream& out, AST_print_context& ctx) {
        json_indent(out, ctx);
        out << "{ \"kind\" : \"" << node_kind << "\"," ;
        ctx.indent();  // one level more for children
        return;
    }

    void ASTNode::json_close(ostream& out, AST_print_context& ctx) {
        // json_indent(out, ctx);
        out << "}";
        ctx.dedent();
    }

    void ASTNode::json_child(string field, ASTNode& child, ostream& out, AST_print_context& ctx, char sep) {
        json_indent(out, ctx);
        out << "\"" << field << "\" : ";
        child.json(out, ctx);
        out << sep;
    }

    void Stub::json(ostream& out, AST_print_context& ctx) {
        json_head("Stub", out, ctx);
        json_indent(out, ctx);
        out  << "\"rule\": \"" <<  name_ << "\"";
        json_close(out, ctx);
    }


    void Program::json(ostream &out, AST::AST_print_context &ctx) {
        json_head("Program", out, ctx);
        json_child("classes_", classes_, out, ctx);
        json_child("statements_", statements_, out, ctx, ' ');
        json_close(out, ctx);
    }

    void Formal::json(ostream &out, AST::AST_print_context &ctx) {
        json_head("Formal", out, ctx);
        json_child("var_", var_, out, ctx);
        json_child("type_", type_, out, ctx, ' ');
        json_close(out, ctx);
    }

    void Method::json(ostream &out, AST::AST_print_context &ctx) {
        json_head("Method", out, ctx);
        json_child("name_", name_, out, ctx);
        json_child("formals_", formals_, out, ctx);
        json_child("returns_", returns_, out, ctx);
        json_child("statements_", statements_, out, ctx, ' ');
        json_close(out, ctx);
    }

    void Assign::json(ostream& out, AST_print_context& ctx) {
        json_head("Assign", out, ctx);
        json_child("lexpr_", lexpr_, out, ctx);
        json_child("rexpr_", rexpr_, out, ctx, ' ');
        json_close(out, ctx);
     }

    void AssignDeclare::json(ostream& out, AST_print_context& ctx) {
        json_head("Assign", out, ctx);
        json_child("lexpr_", lexpr_, out, ctx);
        json_child("rexpr_", rexpr_, out, ctx);
        json_child("static_type_", static_type_, out, ctx, ' ');
        json_close(out, ctx);
    }

    void Return::json(ostream &out, AST::AST_print_context &ctx) {
        json_head("Return", out, ctx);
        json_child("expr_", expr_, out, ctx, ' ');
        json_close(out, ctx);
    }

    void If::json(ostream& out, AST_print_context& ctx) {
        json_head("If", out, ctx);
        json_child("cond_", cond_, out, ctx);
        json_child("truepart_", truepart_, out, ctx);
        json_child("falsepart_", falsepart_, out, ctx, ' ');
        json_close(out, ctx);
    }

    void While::json(ostream& out, AST_print_context& ctx) {
        json_head("While", out, ctx);
        json_child("cond_", cond_, out, ctx);
        json_child("body_", body_, out, ctx, ' ');
        json_close(out, ctx);
    }


    void Typecase::json(ostream& out, AST_print_context& ctx) {
        json_head("Typecase", out, ctx);
        json_child("expr_", expr_, out, ctx);
        json_child("cases_", cases_, out, ctx, ' ');
        json_close(out, ctx);
    }

    void Type_Alternative::json(ostream& out, AST_print_context& ctx) {
        json_head("Type_Alternative", out, ctx);
        json_child("ident_", ident_, out, ctx);
        json_child("classname_", classname_, out, ctx);
        json_child("block_", block_, out, ctx, ' ');
        json_close(out, ctx);
    }

    void Load::json(ostream &out, AST::AST_print_context &ctx) {
        json_head("Load", out, ctx);
        json_child("loc_", loc_, out, ctx, ' ');
        json_close(out, ctx);
    }

    void Ident::json(ostream& out, AST_print_context& ctx) {
        json_head("Ident", out, ctx);
        out << "\"text_\" : \"" << text_ << "\"";
        json_close(out, ctx);
    }

    void Class::json(ostream &out, AST::AST_print_context &ctx) {
        json_head("Class", out, ctx);
        json_child("name_", name_, out, ctx);
        json_child("super_", super_, out, ctx);
        json_child("constructor_", constructor_, out, ctx);
        json_child("methods_", methods_, out, ctx, ' ');
        json_close(out, ctx);
    }

    void Call::json(ostream &out, AST::AST_print_context &ctx) {
        json_head("Call", out, ctx);
        json_child("obj_", receiver_, out, ctx);
        json_child("method_", method_, out, ctx);
        json_child("actuals_", actuals_, out, ctx, ' ');
        json_close(out, ctx);
    }

    void Construct::json(ostream &out, AST::AST_print_context &ctx) {
        json_head("Construct", out, ctx);
        json_child("method_", method_, out, ctx);
        json_child("actuals_", actuals_, out, ctx, ' ');
        json_close(out, ctx);
    }

    void IntConst::json(ostream& out, AST_print_context& ctx) {
        json_head("IntConst", out, ctx);
        out << "\"value_\" : " << value_ ;
        json_close(out, ctx);
    }

    void StrConst::json(ostream& out, AST_print_context& ctx) {
        json_head("StrConst", out, ctx);
        out << "\"value_\" : \"" << value_  << "\"";
        json_close(out, ctx);
    }


    void BinOp::json(ostream& out, AST_print_context& ctx) {
        json_head(opsym, out, ctx);
        json_child("left_", left_, out, ctx);
        json_child("right_", right_, out, ctx, ' ');
        json_close(out, ctx);
    }


    void Not::json(ostream& out, AST_print_context& ctx) {
        json_head("Not", out, ctx);
        json_child("left_", left_, out, ctx, ' ');
        json_close(out, ctx);
    }

    void Dot::json(ostream& out, AST_print_context& ctx) {
        json_head("Dot", out, ctx);
        json_child("left_", left_, out, ctx);
        json_child("right_", right_, out, ctx, ' ');
        json_close(out, ctx);
    }


    /* Convenience factory for operations like +, -, *, / */
    Call* Call::binop(string opname, Expr& receiver, Expr& arg) {
        Ident* method = new Ident(opname);
        Actuals* actuals = new Actuals();
        actuals->append(&arg);
        return new Call(receiver, *method, *actuals);
    }
}