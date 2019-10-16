//
// The AST represents the user program.  These imaginary additions are not really part
// of the AST, but for purposes of type checking we treat them as if they were.  They
// represent the built-in classes of Quack, which are implemented in C rather than Quack.
//

#ifndef QUACKPARSER_ASTBUILTINS_H
#define QUACKPARSER_ASTBUILTINS_H

#include <string>
#include "ASTNode.h"


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

}



#endif //QUACKPARSER_ASTBUILTINS_H
