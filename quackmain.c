#include <stdio.h>
#include "Builtins.h"
int main(int argc, char **argv) {
obj_Obj reg__0;
struct class_Pt_struct;
struct obj_Pt;
typedef struct obj_Pt* obj_Pt;
typedef struct class_Pt_struct* class_Pt;

typedef struct obj_Pt_struct {
class_Pt clazz;
obj_Pt this;
obj_Int this.x;
obj_Int x;
} * obj_Pt;

struct class_Pt_struct the_class_Pt_struct;

struct class_Pt_struct {
obj_Pt (*constructor) (obj_Int, obj_Int);
obj_String (*STRING) (obj_Obj);
obj_Obj (*PRINT) (obj_Obj);
obj_Boolean (*EQUALS) (obj_Obj, obj_Obj);
};

extern class_Pt the_class_Pt;


obj_Pt new_Pt(obj_Int, obj_Int) {
obj_Pt new_thing = (obj_Pt) malloc(sizeof(struct obj_Pt_struct));
new_thing->clazz = the_class_Pt;
obj_Int reg__1;
obj_Int var_this.x;
obj_Int var_x;
reg__1 = var_x;
var_this.x = reg__1;
}

struct  class_Pt_struct  the_class_Pt_struct = {
new_Pt,
Obj_method_STRING,
Obj_method_PRINT,
Obj_method_EQUALS
};

}
