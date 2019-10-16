# 
# Quack parser
#    (build on this for the rest of the compiler)
# 

REFLEX_INCLUDE = /usr/local/include/reflex
REFLEX = reflex --bison-cc --bison-locations --header-file
BISON = bison
CC = g++ -std=c++11
BIN = ../bin
PRODUCT = $(BIN)/parser

top: $(PRODUCT)

##----------------------
#  Scanner
#     generate with REflex from quack.lxx
#     add --debug if you need to see each token as it is matched
##---------------------

lex.yy.cxx lex.yy.h:  quack.lxx
	$(REFLEX) -o lex.yy.cxx quack.lxx

## ----------------------------
# Parser
#     generate with bison from quack.yxx
#
# Note bison options:
#   --defines  :  Produce 'defines' for tokens (to use in lexer)
#   --debug  (same as -t):  Enable tracing parser actions; does not
#        activate unless you trigger it with -t when running the parser
#   --token-table:  Create a table of token names  (yeah, I can ditch my Python script!)
#        creating array yytname[], with YYNNTOKENS being 1 + highest token number.
#        This can be searched to print token names in debugging.
#   -g:  Produce a graph of the automaton ... maybe don't need this?
#   --report:  Produce a (textual) report on the automaton (useful for debugging)

quack.tab.cxx quack.tab.hxx location.hh position.hh stack.hh: quack.yxx
	$(BISON) --defines --debug --token-table --report=all $<

%.o: %.cxx
	$(CC) -c $^

scanner: scanner.o lex.yy.o
	$(CC) -o scanner $^

parser.o: quack.tab.hxx lex.yy.h

$(BIN)/parser: parser.o quack.tab.o lex.yy.o ASTNode.o Messages.o
	$(CC) $^ -o $(BIN)/parser -L /usr/local/lib  -lreflex

## General recipes

clean:
	rm -f *.o *.gch
	# Products of REflex
	rm -f lex.yy.cxx lex.yy.h position.hh stack.hh location.hh
	# Products of bison
	rm -f quack.tab.* quack.output
	rm -f ${PRODUCT}
