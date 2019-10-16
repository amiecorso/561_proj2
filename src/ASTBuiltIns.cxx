//
// Created by Michal Young on 10/23/18.
//

#include "ASTBuiltIns.h"

static AST::Class Nothing = AST::Class(
        *new AST::Ident("Nothing"), // Name
        *new AST::Ident("Obj"),     // Superclass
        *new AST::Method(*new AST::Ident("Construct"), *new AST::Formals(),
                *new AST::Ident("Nothing"), *new AST::Block()),  // Constructor
        *new AST::Methods() // Zero methods
        );

static AST::Ident AST_builtins::bi_true = AST::Ident("true");
static AST::Ident AST_builtins::bi_false = AST::Ident("false");

/*
namespace AST_builtins {

    // The built-in classes
    extern AST::Class Obj;
    extern AST::Class Nothing;
    extern AST::Class String;
    extern AST::Class Boolean;
    extern AST::Class Int;

    // The built-in identifiers
    extern AST::Ident bi_none;
    extern AST::Ident bi_true;
    extern AST::Ident bi_false;

}*/
