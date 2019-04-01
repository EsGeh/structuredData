#include "SymbolTable.h"

#include <math.h>



SymbolTable* symtab_init()
{
	SymbolTable* x = getbytes( sizeof( SymbolTable ) );
	SymbolTable_init( x, SCOPES_HASH_SIZE );
	return x;
}

void symtab_exit(
	SymbolTable* x
)
{
	SymbolTable_exit( x );
	freebytes( x, sizeof( SymbolTable ) );
}

Scope* symtab_add_scope(
	SymbolTable* x,
	t_symbol* name
)
{
	Scope* scope = getbytes( sizeof( Scope ) );
	Scope_init(
			scope,
			VARS_HASH_SIZE
	);
	SymbolTable_insert(
			x,
			name,
			scope
	);
	return scope;
}

Scope* symtab_get_scope(
		SymbolTable* x,
		t_symbol* name
)
{
	return SymbolTable_get( x, name );
}

void symtab_del_scope(
	SymbolTable* x,
	t_symbol* name
)
{
	SymbolTable_delete(
			x,
			name
	);
}

AtomDynA* symtab_get_value(
		Scope* scope,
		t_symbol* name
)
{
	return Scope_get(
			scope,
			name
	);
}
