#include "sdScriptObj.h"

#include "sdScript.h"

#include "m_pd.h"

#define PROGRAMS_HASH_SIZE 1024


static t_class* script_class;
t_class* register_script_class( t_symbol* class_name );

void sdScript_setup()
{
	script_class = register_script_class( gensym("sdScript") );
}

typedef struct _script_obj {
	t_object obj;

	t_outlet* pOutlet;

	ScriptData script_data;
	ProgramMap programs;
	Scope* global_scope;

	// clock needed if a program "pauses" itself:
	t_clock *clock;

} t_script_obj;


// constructor:
void* t_script_obj_init(
	t_symbol* name,
	int argc,
	t_atom* argv
);
// destructor:
void t_script_obj_exit(
	t_script_obj* this
);

void script_obj_on_set_program(
		t_script_obj* this,
		t_symbol* s,
		int argc,
		t_atom* argv
);
void script_obj_on_set_bang(
		t_script_obj* this,
		t_symbol* s,
		int argc,
		t_atom* argv
);
void script_obj_on_del_program(
		t_script_obj* this,
		t_symbol* s
);
void script_obj_on_set_var(
	t_script_obj* this,
	t_symbol* s,
	int argc,
	t_atom* argv
);
void script_obj_on_exec(
	t_script_obj* this,
	t_symbol* prog_name,
	int argc,
	t_atom* argv
);

// executes the current code:
void script_obj_continue(
	t_script_obj* this
);

//*********************************************
// implementation
//*********************************************


t_class* register_script_class( t_symbol* class_name )
{
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcast-function-type"
	t_class* class =
		class_new(
			class_name, 		// name
			(t_newmethod )t_script_obj_init, 	// newmethod
			(t_method )t_script_obj_exit, 	// freemethod
			sizeof(t_script_obj), 		// size
			CLASS_DEFAULT, 			// object type
			A_GIMME,			// list of atoms following the selector...
			0
		);
#pragma GCC diagnostic pop

	class_addmethod(
		class,
		(t_method )script_obj_on_set_program,
		gensym("prog_set"),
		A_GIMME,
		0
	);
	class_addmethod(
		class,
		(t_method )script_obj_on_set_bang,
		gensym("prog_set_bang"),
		A_GIMME,
		0
	);
	class_addmethod(
		class,
		(t_method )script_obj_on_del_program,
		gensym("prog_del"),
		A_SYMBOL,
		0
	);
	class_addanything(
		class,
		(t_method )script_obj_on_exec
	);
	class_addmethod(
		class,
		(t_method )script_obj_on_set_var,
		gensym( "set" ),
		A_GIMME,
		0
	);

	return class;
}

void* t_script_obj_init(
	t_symbol* UNUSED(name),
	int argc,
	t_atom* argv
)
{
	DB_PRINT("creating sgScript object...");
	t_script_obj* x = (t_script_obj* )pd_new( script_class );

	x -> pOutlet = outlet_new( & x -> obj, &s_list);
	inlet_new(
		& x -> obj,
		& x -> obj . ob_pd,
		gensym("list"),
		gensym("prog_set_bang")
	);

	ProgramMap_init( & x->programs, PROGRAMS_HASH_SIZE );
	x->clock = clock_new( 
			x,
			(t_method )script_obj_continue
	);

	Script_init(
			& x->script_data,
			& x-> programs,
			x -> pOutlet,
			x -> clock
	);


	x->global_scope = getbytes( sizeof( Scope ) );
	Scope_init( x->global_scope, VARS_HASH_SIZE);
	ScopeList_prepend(
		Script_get_global_scopes( & x -> script_data ),
		x->global_scope
	);

	// create global variables:
	for( int i=0; i<argc; i++)
	{
		AtomDynA* new_var = getbytes( sizeof( AtomDynA ) );
		AtomDynA_init( new_var );
		t_atom new_atom;
		SETFLOAT( & new_atom, 0 );
		AtomDynA_append( new_var, new_atom );
		Scope_insert(
				x->global_scope,
				atom_getsymbol( & argv[i] ),
				new_var
		);
	}

	return x;
}

void t_script_obj_exit(
	t_script_obj* x
)
{
	if( x->clock)
		clock_free( x -> clock );

	ProgramMap_exit( & x->programs );
	Script_exit(
			& x->script_data
	);
	Scope_exit( x->global_scope );
	freebytes( x->global_scope, sizeof( Scope ) );
}

void script_obj_on_set_program(
	t_script_obj* this,
	t_symbol* UNUSED(s),
	int argc,
	t_atom* argv
)
{
	DB_PRINT("script_obj_on_set_program");
	if( argc < 1 )
	{
		pd_error(
				this,
				"sdScript: wrong syntax!"
		);
	}

	Script_stop( &this->script_data );

	t_symbol* prog_name = atom_getsymbol(
		& argv[0]
	);
	int prog_size = (argc-1);

	AtomBuf* new_program = getbytes( sizeof( AtomBuf ) );
	AtomBuf_init( new_program, prog_size);

	memcpy(
		AtomBuf_get_array( new_program ),
		& argv[1],
		sizeof( t_atom ) * prog_size
	);

	ProgramMap_insert(
		& this -> programs,
		prog_name,
		new_program
	);
}

void script_obj_on_set_bang(
		t_script_obj* this,
		t_symbol* UNUSED(s),
		int argc,
		t_atom* argv
)
{
	DB_PRINT("script_obj_on_set_bang");

	Script_stop( &this->script_data );

	t_symbol* prog_name = gensym( "bang" );
	int prog_size = argc;

	AtomBuf* new_program = getbytes( sizeof( AtomBuf ) );
	AtomBuf_init( new_program, prog_size);
	
	memcpy(
		AtomBuf_get_array( new_program ),
		& argv[0],
		sizeof( t_atom ) * prog_size
	);

	ProgramMap_insert(
		& this -> programs,
		prog_name,
		new_program
	);
}

void script_obj_on_del_program(
	t_script_obj* this,
	t_symbol* prog_name
)
{
	ProgramMap_delete(
		& this -> programs,
		prog_name
	);
}

void script_obj_on_set_var(
	t_script_obj* this,
	t_symbol* UNUSED(s),
	int argc,
	t_atom* argv
)
{
	DB_PRINT("script_obj_on_set_var");
	if( argc < 1 )
	{
		pd_error( this, "wrong syntax: expected 'set <global_var> val1 ...'" );
		return;
	}

	t_symbol* var_name = atom_getsymbol( & argv[0] );
	AtomDynA* value = symtab_get_var(
			NULL,
			Script_get_global_scopes( & this -> script_data ),
			var_name
	);
	if( !value )
	{
		char buf[1024];
		atom_string( & argv[0], buf, 1024 );
		pd_error( this, "variable not found: '%s'", buf);
		return;
	}
	AtomDynA_set_size(
			value,
			argc-1
	);
	memcpy(
			AtomDynA_get_array( value ),
			& argv[1],
			sizeof( t_atom ) * (argc-1)
	);
}

void script_obj_on_exec(
	t_script_obj* this,
	t_symbol* prog_name,
	int argc,
	t_atom* UNUSED(argv)
)
{
	if(
			argc != 0
	)
	{
		pd_error( this, "sdScript: wrong syntax!"  );
		return;
	}

	Script_exec(
			& this->script_data,
			prog_name
	);
}

// executes the current code:
void script_obj_continue(
	t_script_obj* this
)
{
	Script_continue( & this -> script_data );
}
