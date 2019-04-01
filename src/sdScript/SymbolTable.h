#ifndef _SYMBOLTABLE_H_
#define _SYMBOLTABLE_H_ 

#include "Global.h"
#include "DynArray.h"
#include "Map.h"

#define VARS_HASH_SIZE 1024
#define SCOPES_HASH_SIZE 1024


#define DEL_VAR(var,size) \
	AtomDynA_exit( var ); \
	freebytes( var, size )

#define HASH_SYMBOL(sym) \
	((unsigned int )sym)

// variable name -> value(s)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpointer-to-int-cast"
DECL_MAP(Scope,t_symbol*,AtomDynA,getbytes,freebytes,DEL_VAR,HASH_SYMBOL)
DEF_MAP(Scope,t_symbol*,AtomDynA,getbytes,freebytes,DEL_VAR,HASH_SYMBOL)
#pragma GCC diagnostic pop

#define DEL_SCOPE(scope,size) \
	Scope_exit( scope ); \
	freebytes( scope, size )

// program name -> Scope
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpointer-to-int-cast"
DECL_MAP(SymbolTable,t_symbol*,Scope,getbytes,freebytes,DEL_SCOPE,HASH_SYMBOL)
DEF_MAP(SymbolTable,t_symbol*,Scope,getbytes,freebytes,DEL_SCOPE,HASH_SYMBOL)
#pragma GCC diagnostic pop

SymbolTable* symtab_init();
void symtab_exit(
	SymbolTable* x
);
Scope* symtab_add_scope(
	SymbolTable* x,
	t_symbol* name
);
Scope* symtab_get_scope(
		SymbolTable* x,
		t_symbol* name
);
void symtab_del_scope(
	SymbolTable* x,
	t_symbol* name
);
AtomDynA* symtab_get_value(
		Scope* scope,
		t_symbol* name
);

#endif 
