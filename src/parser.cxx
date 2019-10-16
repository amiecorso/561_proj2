//
// The parser driver just glues together a parser object
// and a lexer object.
//

#include "lex.yy.h"
#include "ASTNode.h"
#include "Messages.h"

#include <iostream>
#include <unistd.h>  // getopt is here

class Driver {
    int debug_level = 0;
public:
    explicit Driver(const reflex::Input in) : lexer(in), parser(new yy::parser(lexer, &root)) { root = nullptr; }

    ~Driver() { delete parser; }

    void debug() { debug_level = 1; }

    AST::ASTNode *parse() {
        parser->set_debug_level(debug_level); // 0 = no debugging, 1 = full tracing
        // std::cout << "Running parser\n";
        int result = parser->parse();
        if (result == 0 && report::ok()) {  // 0 == success, 1 == failure
            // std::cout << "Extracting result\n";
            if (root == nullptr) {
                std::cout << "But I got a null result!  How?!\n";
            }
            return root;
        } else {
            // std::cout << "Parse failed, no tree\n";
            return nullptr;
        }
    }

private:
    yy::Lexer lexer;
    yy::parser *parser;
    AST::ASTNode *root;
};

int main(int argc, char **argv) {
    std::string filename;
    char c;
    FILE *f;
    int index;
    int debug; // 0 = no debugging, 1 = full tracing

    while ((c = getopt(argc, argv, "t")) != -1) {
        if (c == 't') {
            std::cerr <<  "Debugging mode\n";
            debug = 1;
        }
    }

    for (index = optind; index < argc; ++index) {
        if( !(f = fopen(argv[index], "r"))) {
            perror(argv[index]);
            exit(1);
        }
        Driver driver(f);
        if (debug) driver.debug();
        AST::ASTNode *root = driver.parse();
        if (root != nullptr) {
            // std::cout << "Parsed!\n";
            AST::AST_print_context context;
            root->json(std::cout, context);
            std::cout << std::endl;
        } else {
            std::cout << "No tree produced." << std::endl;
        }
    }

}
