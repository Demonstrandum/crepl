#include "defaults.h"
#include "parse.h"
#include "execute.h"

char *display_nil(void);
char *display_lambda(Lambda *);
char *display_numbernode(NumberNode _);
char *display_parampos(ParamPos _);
char *display_datatype(DataType );
char *display_parsetree(const ParseNode *);
char *display_datavalue(const DataValue *);
