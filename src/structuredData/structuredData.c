#include "structuredData.h"
#include "Global.h"
#include "sdScriptObj.h"

#include "SymbolTable.h"

//#include "Buffer.h"

#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>

#include "m_pd.h"


static t_class* any_class;
static t_class* gate_class;
static t_class* counter_class;
static t_class* first_class;
static t_class* symbolIsEq_class;
static t_class* isEq_class;
static t_class* filter_class;
static t_class* replace_class;
static t_class* setselector_class;
static t_class* replace_by_name_class;

t_class* register_any(
	t_symbol* className
);
t_class* register_gate(
	t_symbol* className
);
t_class* register_counter(
	t_symbol* className
);
t_class* register_first(
	t_symbol* className
);
t_class* register_symbolIsEq(
	t_symbol* className
);
t_class* register_isEq(
	t_symbol* className
);
t_class* register_filter(
	t_symbol* className
);
t_class* register_replace(
	t_symbol* className
);
t_class* register_setselector(
	t_symbol* className
);
t_class* register_replace_by_name(
	t_symbol* className
);

void structuredDataC_setup()
{
	any_class = register_any( gensym("sdAny") );
	gate_class = register_gate(gensym("sdGate"));
	counter_class = register_counter(gensym("sdCounter"));
	first_class = register_first(gensym("sdFirst"));
	symbolIsEq_class = register_symbolIsEq(gensym("sdSymbolIsEq"));
	isEq_class = register_isEq(gensym("sdIsEq"));
	filter_class = register_filter(gensym("sdFilter"));
	replace_class = register_replace(gensym("sdReplace"));
	setselector_class = register_setselector(gensym("sdSetSelector"));
	replace_by_name_class = register_replace_by_name( gensym("sdVarLookup") );
	sdList_setup();
	sdPack_setup();
	sdData_setup();
	sdEvent_setup();
	sdProperties_setup();
	sdObjState_setup();

	sdScript_setup();
}

//----------------------------------
// sdAny
//----------------------------------

typedef struct _any {
  t_object x_obj;
	t_atom value;
} t_any;

void *any_new(
	t_symbol *s,
	int argc,
	t_atom *argv
);
void any_bang(t_any *x);
void any_set(
	t_any *x,
	t_symbol *s,
	int argc,
	t_atom *argv
);

t_class* register_any(
	t_symbol* className
)
{
	t_class* class =
		class_new(
			className,
			(t_newmethod)any_new, // constructor
			0, // destructor
			sizeof(t_any),
			CLASS_DEFAULT, // graphical repr ?
			// creation arguments:
			A_GIMME,
			0
		);
	class_sethelpsymbol(
			class,
			gensym("sdUtils")
	);
	class_addbang(class, any_bang);
	class_addanything(class, any_set);
	return class;
}

void *any_new(
	t_symbol* UNUSED(s),
	int argc,
	t_atom* argv
)
{
  t_any *x = (t_any *)pd_new(any_class);

	switch(argc)
	{
		case 0:
			SETFLOAT(&(x->value), 0.0);
			break;
		case 1:
			x->value = ( * argv );
			break;
		default:
			pd_error(x,"too many arguments: %u", argc);
	}
  inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_list, gensym("set"));
  outlet_new(&x->x_obj, &s_float);

  return (void *)x;
}

void any_bang(t_any *x)
{
  outlet_list(
		x->x_obj.ob_outlet,
		&s_list,
		1,
		&(x->value)
	);
}

void any_set(
	t_any *x,
	t_symbol *s,
	int argc,
	t_atom *argv
)
{
	if(s == &s_float || s == &s_symbol || s == &s_list)
	{
		x->value = argv[0];
		outlet_list(
			x->x_obj.ob_outlet,
			&s_list,
			1,
			&(x->value)
		);
	}
	else if( s == gensym("set") )
	{
		if( argc != 1 )
		{
			pd_error(x,"format: set <value>");
			return;
		}
		x->value = *argv;
	}
	else
	{
		t_atom sel;
		SETSYMBOL(&sel, s);
		char buf[256];
		atom_string(&sel, buf, 255);
		pd_error(x,"sdAny: no method for %s", buf);
	}
}

//----------------------------------
// sdGate
//----------------------------------

typedef struct _gate {
  t_object x_obj;
	t_float value;
	t_outlet* acceptOut;
	t_outlet* rejectOut;
} t_gate;

void *gate_new(
	t_float init
);
void gate_in(
	t_gate *x,
	t_symbol *s,
	int argc,
	t_atom *argv
);

t_class* register_gate(
	t_symbol* className
)
{
	t_class* class =
		class_new(
			className,
			(t_newmethod)gate_new, // constructor
			0, // destructor
			sizeof(t_gate),
			CLASS_DEFAULT, // graphical repr ?
			// creation arguments:
			A_DEFFLOAT,
			0
		);
	class_sethelpsymbol(
			class,
			gensym("sdUtils")
	);
	class_addanything(class, gate_in);
	return class;
}

void *gate_new(
	t_float init
)
{
  t_gate *x = (t_gate *)pd_new(gate_class);

	x -> value = init;
	floatinlet_new( & x->x_obj, & x->value);
  x-> acceptOut = outlet_new(&x->x_obj, &s_list);
  x-> rejectOut = outlet_new(&x->x_obj, &s_list);

  return (void *)x;
}

void gate_in(
	t_gate *x,
	t_symbol *s,
	int argc,
	t_atom *argv
)
{
	if( x->value != 0 )
	{
		outlet_anything(
			x->acceptOut,
			s,
			argc,
			argv
		);
	}
	else
	{
		outlet_anything(
			x->rejectOut,
			s,
			argc,
			argv
		);
	}
}

//----------------------------------
// counter
//----------------------------------

typedef struct _counter {
  t_object x_obj;
	t_float value;
	t_float limit;
	t_float autoreset;
	t_outlet* valueOut;
	t_outlet* overflowOut;
} t_counter;

void* counter_new(
	t_float initLimit,
	t_float initAutoreset
);
void counter_set(
	t_counter *x,
	t_float newVal
);
void counter_bang(
	t_counter *x
);

t_class* register_counter(
	t_symbol* className
)
{
	t_class* class =
		class_new(
			className,
			(t_newmethod)counter_new, // constructor
			0, // destructor
			sizeof(t_counter),
			CLASS_DEFAULT, // graphical repr ?
			// creation arguments:
			A_DEFFLOAT,
			A_DEFFLOAT,
			A_DEFFLOAT,
			0
		);
	class_sethelpsymbol(
			class,
			gensym("sdUtils")
	);
	class_addfloat(class, counter_set);
	class_addbang(class, counter_bang);
	return class;
}

void* counter_new(
	t_float initLimit,
	t_float initAutoreset
)
{
  t_counter *x = (t_counter *)pd_new(counter_class);

	x -> value = 0;
	x -> limit = initLimit;
	x -> autoreset = initAutoreset;

	floatinlet_new( & x->x_obj, & x->limit);

  x-> valueOut = outlet_new(&x->x_obj, &s_float);
  x-> overflowOut = outlet_new(&x->x_obj, &s_bang);

  return (void *)x;
}

void counter_set(
	t_counter *x,
	t_float newVal
)
{
	x -> value = newVal;
}

void counter_bang(
	t_counter *x
)
{
	if( x -> limit == -1 || x->value < x->limit )
	{
		outlet_float( x->valueOut, x->value );
		x -> value ++;
	}
	else if( x -> autoreset != 0 )
	{
		x -> value = 0;
		if( x -> limit == -1 || x->value < x->limit )
			outlet_float( x->valueOut, x->value );
		else
			outlet_bang( x->overflowOut );
		x -> value ++;
	}
	else
	{
		outlet_bang( x->overflowOut );
	}
}

//----------------------------------
// first
//----------------------------------

typedef struct _first {
  t_object x_obj;
	t_float accept;
	t_outlet* acceptOut;
	t_outlet* rejectOut;
} t_first;

void* first_new();

void first_input(
	t_first *x,
	t_symbol* s,
	int argc,
	t_atom *argv
);

t_class* register_first(
	t_symbol* className
)
{
	t_class* class =
		class_new(
			className,
			(t_newmethod)first_new, // constructor
			0, // destructor
			sizeof(t_first),
			CLASS_DEFAULT, // graphical repr ?
			// creation arguments:
			0
		);
	class_sethelpsymbol(
			class,
			gensym("sdUtils")
	);
	class_addlist(class, first_input);
	return class;
}

void* first_new(
)
{
  t_first *x = (t_first *)pd_new(first_class);

	x -> accept = 1;

	floatinlet_new(
		& x->x_obj,
		& x->accept
	);

  x-> acceptOut = outlet_new(&x->x_obj, &s_anything);
  x-> rejectOut = outlet_new(&x->x_obj, &s_anything);

  return (void *)x;
}

void first_input(
	t_first *x,
	t_symbol *s,
	int argc,
	t_atom *argv
)
{
	if( x-> accept != 0 )
	{
		outlet_list(
			x->acceptOut,
			s,
			argc,
			argv
		);
		x->accept = 0;
	}
	else
	{
		outlet_list(
			x->rejectOut,
			s,
			argc,
			argv
		);
	}
}

//----------------------------------
// symbolIsEq
//----------------------------------

typedef struct _symbolIsEq {
  t_object x_obj;
	t_symbol* sym2;
} t_symbolIsEq;

void* symbolIsEq_new(
	t_symbol *s,
	int argc,
	t_atom *argv
);
void symbolIsEq_compare(
	t_symbolIsEq *x,
	t_symbol* sym
);

t_class* register_symbolIsEq(
	t_symbol* className
)
{
	t_class* class =
		class_new(
			className,
			(t_newmethod)symbolIsEq_new, // constructor
			0, // destructor
			sizeof(t_symbolIsEq),
			CLASS_DEFAULT, // graphical repr ?
			// creation arguments:
			A_GIMME,
			0
		);
	class_sethelpsymbol(
			class,
			gensym("sdUtils")
	);
	class_addsymbol(class, symbolIsEq_compare);
	return class;
}

void* symbolIsEq_new(
	t_symbol* UNUSED(s),
	int argc,
	t_atom* argv
)
{
  t_symbolIsEq *x = (t_symbolIsEq *)pd_new(symbolIsEq_class);

	if( argc == 0 )
		x -> sym2 = gensym("");
	else if( argc == 1 )
		x -> sym2 = atom_getsymbol( &argv[0] );
	else
	{
		pd_error(x,"too many arguments!");
		return NULL;
	}

	symbolinlet_new(
		& x->x_obj,
		& x->sym2
	);
  outlet_new(&x->x_obj, &s_float);

  return (void *)x;
}

void symbolIsEq_compare(
	t_symbolIsEq* x,
	t_symbol* sym
)
{
	//post("hello");
	if( sym == x -> sym2 )
		outlet_float( x->x_obj.ob_outlet, 1.0 );
	else
		outlet_float( x->x_obj.ob_outlet, 0.0 );
}

//----------------------------------
// isEq
//----------------------------------

typedef struct _isEq {
  t_object x_obj;
	t_atom other;
} t_isEq;

void* isEq_new(
	t_symbol *s,
	int argc,
	t_atom *argv
);

void isEq_compare(
	t_isEq *x,
	t_symbol *s,
	int argc,
	t_atom *argv
);

void isEq_setOther(
	t_isEq *x,
	t_symbol *s,
	int argc,
	t_atom *argv
);

t_class* register_isEq(
	t_symbol* className
)
{
	t_class* class =
		class_new(
			className,
			(t_newmethod)isEq_new, // constructor
			0, // destructor
			sizeof(t_isEq),
			CLASS_DEFAULT, // graphical repr ?
			// creation arguments:
			A_GIMME,
			0
		);
	class_sethelpsymbol(
			class,
			gensym("sdUtils")
	);
	class_addlist(class, isEq_compare);
	class_addmethod(
		class,
		(t_method )isEq_setOther,
		gensym("set"),
		A_GIMME,
		0
	);
	return class;
}

void* isEq_new(
	t_symbol* UNUSED(s),
	int argc,
	t_atom *argv
)
{
  t_isEq *x = (t_isEq *)pd_new(isEq_class);

	if( argc == 0 )
		SETSYMBOL( & x->other, gensym(""));
	else if( argc == 1 )
		x -> other = argv[0];
	else
	{
		pd_error(x,"too many arguments!");
		return NULL;
	}

	inlet_new(
		& x->x_obj,
		& x->x_obj.ob_pd,
		gensym("list"), // accept method
		gensym("set")  // method to call
	);

  outlet_new(&x->x_obj, &s_float);

  return (void *)x;
}

void isEq_compare(
	t_isEq* x,
	t_symbol* UNUSED(s),
	int argc,
	t_atom* argv
)
{
	if( argc == 0 )
	{
		pd_error(x, "to few parameters!");
		return;
	}
	if( compareAtoms( & x->other, & argv[0] )  )
	{
		outlet_float( x->x_obj.ob_outlet, 1.0 );
	}
	else
		outlet_float( x->x_obj.ob_outlet, 0.0 );
}

void isEq_setOther(
	t_isEq* x,
	t_symbol* UNUSED(s),
	int argc,
	t_atom* argv
)
{
	if( argc == 0 )
	{
		pd_error(x, "to few parameters!");
		return;
	}
	x -> other = argv[0];
}

//----------------------------------
// filter
//----------------------------------

typedef struct _filter {
  t_object x_obj;
	AtomBuf args;
	t_outlet* acceptOut;
	t_outlet* rejectOut;
} t_filter;

void* filter_init(
	t_symbol *s,
	int argc,
	t_atom *argv
);

void filter_exit(
	t_filter* x
);

void filter_input(
	t_filter *x,
	t_symbol *s,
	int argc,
	t_atom *argv
);

void filter_set_args(
	t_filter *x,
	t_symbol *s,
	int argc,
	t_atom *argv
);

t_class* register_filter(
	t_symbol* className
)
{
	t_class* class =
		class_new(
			className,
			(t_newmethod )filter_init, // constructor
			(t_method )filter_exit, // destructor
			sizeof(t_filter),
			CLASS_DEFAULT, // graphical repr ?
			// creation arguments:
			A_GIMME,
			0
		);
	class_sethelpsymbol(
			class,
			gensym("sdUtils")
	);
	// "methods":
	class_addlist(class, filter_input);
	class_addmethod(
		class,
		(t_method )filter_set_args,
		gensym("set"),
		A_GIMME,
		0
	);
	return class;
}

void* filter_init(
	t_symbol *s,
	int argc,
	t_atom *argv
)
{
  t_filter *x = (t_filter *)pd_new(filter_class);
	AtomBuf_init( & x->args, 0 );

	filter_set_args( x, s, argc, argv );

	inlet_new(
		& x->x_obj,
		& x->x_obj.ob_pd,
		gensym("list"), // accept method
		gensym("set")  // method to call
	);

  x->acceptOut = outlet_new(&x->x_obj, &s_list);
  x->rejectOut = outlet_new(&x->x_obj, &s_list);

  return (void *)x;
}

void filter_exit(
	t_filter* x
)
{
	AtomBuf_exit( &x->args );
}

void filter_input(
	t_filter *x,
	t_symbol *s,
	int argc,
	t_atom *argv
)
{
	if( argc == 0 )
	{
		pd_error(x, "to few parameters!");
		return;
	}
	BOOL matched = FALSE;
	for(unsigned int i=0; i<AtomBuf_get_size( &x->args ); i++ )
	{
		t_atom* current = & AtomBuf_get_array( &x->args )[i];
		if( compareAtoms( current, & argv[0] ) )
		{
			outlet_list(
				x->acceptOut,
				s,
				argc,
				argv
			);
			matched = TRUE;
		}
	}
	if( !matched )
	{
		outlet_list(
			x->rejectOut,
			s,
			argc,
			argv
		);
	}
}

void filter_set_args(
	t_filter* x,
	t_symbol* UNUSED(s),
	int argc,
	t_atom* argv
)
{
	if( argc == 0 )
	{
		AtomBuf_resize( & x->args, 1 );
		SETSYMBOL( &AtomBuf_get_array( &x->args )[0], gensym(""));
	}
	else
	{
		AtomBuf_resize( & x->args, argc );
		memcpy(
				AtomBuf_get_array( &x->args ),
				argv,
				sizeof( t_atom ) * argc
		);
	}
}

//----------------------------------
// replace
//----------------------------------

typedef struct _replace {
  t_object x_obj;
	t_atom* argv;
	int argc;
	t_outlet* outlet;
} t_replace;

void* replace_init(
	t_symbol *s,
	int argc,
	t_atom *argv
);

void replace_exit(
	t_replace *x
);

void replace_input(
	t_replace *x,
	t_symbol *s,
	int argc,
	t_atom *argv
);

t_class* register_replace(
	t_symbol* className
)
{
	t_class* class =
		class_new(
			className,
			(t_newmethod )replace_init, // constructor
			(t_method )replace_exit, // destructor
			sizeof(t_replace),
			CLASS_DEFAULT, // graphical repr ?
			// creation arguments:
			A_GIMME,
			0
		);
	class_sethelpsymbol(
			class,
			gensym("sdUtils")
	);
	// "methods":
	class_addanything(class, replace_input);
	return class;
}

void* replace_init(
	t_symbol* UNUSED(s),
	int argc,
	t_atom* argv
)
{
  t_replace *x = (t_replace *)pd_new(replace_class);

	x->argc = argc;
	if( argc )
	{
		x->argv = getbytes( sizeof( t_atom ) * argc );
		for(int i=0; i<argc; i++)
		{
			x->argv[i] = argv[i];
		}
	}

  x->outlet = outlet_new(&x->x_obj, &s_list);

  return (void *)x;
}

void replace_exit(
	t_replace *x
)
{
	if( x->argc )
	{
		freebytes( x->argv, sizeof( t_atom ) * x->argc );
	}
}

void replace_input(
	t_replace *x,
	t_symbol *s,
	int argc,
	t_atom *argv
)
{
	for( int i=0; i<argc; i++ )
	{
		t_atom* current = & argv[i];

		// parse the symbol and replace %<num> by the corresponding creation arg...:
		char current_str[256];
		atom_string( current, current_str, 255 );
		int current_string_length = strlen( current_str );
		int ret_str_size = 0;
		char ret_str[256];
		// post( "parsing %s", current_str );
		if( current->a_type != A_SYMBOL )
			continue;

		// loop through all '%'
		char* current_pos = strchr( current_str, '%' );
		if( current_pos )
		{
			ret_str_size = current_pos-current_str ;
			strncpy( ret_str, current_str, ret_str_size);
		}
		else
		{
			ret_str_size = current_string_length;
			strncpy( ret_str, current_str, 255 );
		}
		while( current_pos )
		{
			if( (current_pos+1 - current_str) >= current_string_length )
			{
				pd_error( x, "number expected after '%%' at pos %i, but end of string found", (int )(current_pos-current_str) );
			}
			int argIndex = atoi( current_pos+1 );
			int arg_len = strspn( current_pos+1, "0123456789" );
			if( arg_len == 0 )
			{
				pd_error( x, "number expected after '%%' at pos %i", (int )(current_pos-current_str) );
				return;
			}
			else if( argIndex >= x->argc )
			{
				pd_error( x, "no argument for '%%%i'", argIndex );
				return;
			}

			char replacement_str[256];
			atom_string( & x->argv[argIndex], replacement_str, 255 );
			int replacement_len = strlen( replacement_str );
			// append replacement_str:
			strncpy( ret_str+ret_str_size, replacement_str, replacement_len );
			ret_str_size += replacement_len;

			// move to next '%'

			char* next_pos = strchr( current_pos+1, '%' );
			// append rest of current_str until next '%'
			if( next_pos )
			{
				int chars_to_copy = next_pos-current_pos-1-arg_len;
				strncpy( ret_str+ret_str_size, current_pos+1+arg_len, chars_to_copy);
				ret_str_size += chars_to_copy;
			}
			else
			{
				int chars_to_copy = (current_str + current_string_length) - current_pos-1-arg_len;
				strncpy( ret_str+ret_str_size, current_pos+1+arg_len, chars_to_copy );
				ret_str_size += chars_to_copy;
			}
			current_pos = next_pos;
		}
		ret_str[ret_str_size] = '\0';
		//post( "current_str: %s, ret_str: %s", current_str, ret_str );
		SETSYMBOL( current, gensym( ret_str ) );
	}
	outlet_anything(
		x->outlet,
		s,
		argc,
		argv
	);
}

//----------------------------------
// setselector
//----------------------------------

typedef struct _setselector {
  t_object x_obj;
	t_symbol* selector;
	t_outlet* outlet;
} t_setselector;

void* setselector_init(
	t_symbol *arg
);

void setselector_input(
	t_setselector *x,
	t_symbol *s,
	int argc,
	t_atom *argv
);

t_class* register_setselector(
	t_symbol* className
)
{
	t_class* class =
		class_new(
			className,
			(t_newmethod )setselector_init, // constructor
			0, // destructor
			sizeof(t_setselector),
			CLASS_DEFAULT, // graphical repr ?
			// creation arguments:
			A_SYMBOL,
			0
		);
	class_sethelpsymbol(
			class,
			gensym("sdUtils")
	);
	// "methods":
	class_addanything(class, setselector_input);
	post( "register_setselector" );
	return class;
}

void* setselector_init(
	t_symbol* arg
)
{
  t_setselector *x = (t_setselector *)pd_new(setselector_class);

	x->selector = arg;
  x->outlet = outlet_new(&x->x_obj, &s_list);

  return (void *)x;
}

void setselector_input(
	t_setselector* x,
	t_symbol* UNUSED(s),
	int argc,
	t_atom* argv
)
{
	outlet_anything(
		x->outlet,
		x->selector,
		argc,
		argv
	);
}

//----------------------------------
// replace_by_name
//----------------------------------

typedef struct _replace_by_name {
  t_object x_obj;
	Scope scope;
	t_outlet* outlet;
} t_replace_by_name;

void* replace_by_name_init(
	t_symbol *s,
	int argc,
	t_atom *argv
);

void replace_by_name_exit(
	t_replace_by_name *x
);

void replace_by_name_input(
	t_replace_by_name *x,
	t_symbol *s,
	int argc,
	t_atom *argv
);

void replace_by_name_on_set_var(
	t_replace_by_name* this,
	t_symbol* s,
	int argc,
	t_atom* argv
);

void replace_by_name_on_clear(
	t_replace_by_name* this,
	t_symbol* s
);

t_class* register_replace_by_name(
	t_symbol* className
)
{
	t_class* class =
		class_new(
			className,
			(t_newmethod )replace_by_name_init, // constructor
			(t_method )replace_by_name_exit, // destructor
			sizeof(t_replace_by_name),
			CLASS_DEFAULT, // graphical repr ?
			// creation arguments:
			A_GIMME,
			0
		);
	class_sethelpsymbol(
			class,
			gensym("sdUtils")
	);
	// "methods":
	class_addmethod(
			class,
			(t_method )replace_by_name_on_set_var,
			gensym("set"),
			A_GIMME,
			0
	);
	class_addmethod(
			class,
			(t_method )replace_by_name_on_clear,
			gensym("clear"),
			0
	);
	class_addlist(class, replace_by_name_input);
	return class;
}

void* replace_by_name_init(
	t_symbol* UNUSED(s),
	int argc,
	t_atom* argv
)
{
	for( int i=0; i<argc; i++ )
	{
		t_atom* current = & argv[i];
		if( current->a_type != A_SYMBOL )
		{
			pd_error( NULL, "wrong syntax: expected 'varname1 ...'" );
			return NULL;
		}
	}

  t_replace_by_name *x = (t_replace_by_name *)pd_new(replace_by_name_class);
	Scope_init( & x->scope, 1024 );
	for( int i=0; i<argc; i++ )
	{
		t_symbol* var_name =
			atom_getsymbol(
				& argv[i]
			);
		DB_PRINT("adding variable");
		AtomDynA* value = getbytes( sizeof( AtomDynA ) );
		AtomDynA_init( value );
		Scope_insert(
			& x->scope,
			var_name,
			value
		);
		AtomDynA_set_size(
				value,
				1
		);
		SETFLOAT( & AtomDynA_get_array( value )[0], 0 );
	}

  x->outlet = outlet_new(&x->x_obj, &s_list);

  return (void *)x;
}

void replace_by_name_exit(
	t_replace_by_name *x
)
{
	Scope_exit( & x->scope );
}

void replace_by_name_on_set_var(
	t_replace_by_name* this,
	t_symbol* UNUSED(s),
	int argc,
	t_atom* argv
)
{
	DB_PRINT("replace_by_name_on_set_var");
	if( argc < 1 )
	{
		pd_error( this, "wrong syntax: expected 'set <global_var> val1 ...'" );
		return;
	}

	t_symbol* var_name = atom_getsymbol( & argv[0] );
	AtomDynA* value =
		Scope_get(
				& this->scope,
				var_name
		);
	if( !value ) {
		DB_PRINT("adding variable");
		value = getbytes( sizeof( AtomDynA ) );
		AtomDynA_init( value );
		Scope_insert(
			& this->scope,
			var_name,
			value
		);
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

void replace_by_name_on_clear(
	t_replace_by_name* this,
	t_symbol* UNUSED(s)
)
{
	DB_PRINT("replace_by_name_on_set_var");
	Scope_clear(
			& this->scope
	);
}

typedef struct _repl_info {
	char* pos;
	int len;
	char* arg_start;
	int arg_len;
	AtomDynA* value;
} repl_info;

DECL_DYN_ARRAY(ReplInfoA,repl_info,getbytes,freebytes)
DEF_DYN_ARRAY(ReplInfoA,repl_info,getbytes,freebytes)

void replace_by_name_parse_str(
	t_replace_by_name *x,
	const char* src,
	ReplInfoA* repl
)
{
	int src_len = strlen( src );
	// loop through all '$var' and '$(var)'
	char* pos = strchr( src, '$' );
	while( pos )
	{
		DB_PRINT("pos-src: %i", pos-src );
		if( (pos+1) - src >= src_len )
		{
			pd_error( x, "at pos %i: end of string after $", (int )(pos-src) );
			return;
		}
		if( *(pos+1) != '(' && !isalpha( *(pos+1) ) )
		{
			pd_error( x, "at pos %i: expected letter or '(' after $", (int )((pos+1)-src) );
			return;
		}
		repl_info info = {
			.pos = pos,
			.len = 0,
			.arg_start = pos,
			.arg_len = 0
		};

		pos += 1;
		if( *pos == '(' )
		{
			pos+= 1;
			info.arg_start = pos;
			info.arg_len = strchr( pos, ')' ) - pos;
			info.len = info.arg_len + 3 ;
			if( info.arg_len == 0 )
			{
				pd_error( x, "at pos %i: no ')' found after '('", (int )(pos-src) );
				return;
			}
		}
		else if( isalpha( *pos ) )
		{
			info.arg_start = pos;
			char* last = pos;
			while( (last+1)-src < src_len && isalnum( *(last+1) ) )
			{
				last++;
			}
			info.arg_len = (last - pos) + 1;
			info.len = info.arg_len+1;
		}
		DB_PRINT("pos: %i, len: %i, arg_start: %i, arg_len: %i",
				info.pos-src,
				info.len,
				info.arg_start-src,
				info.arg_len
		);
		char var_name[256];
		strncpy( var_name, info.arg_start, info.arg_len );
		var_name[info.arg_len] = '\0';
		DB_PRINT("var_name: %s", var_name);

		info.value = Scope_get(
				& x->scope,
				gensym( var_name )
		);
		if( !info.value )
		{
			pd_error(
					x,
					"value not found '%s'",
					var_name
			);
			return;
		}
		
		ReplInfoA_append(
				repl,
				info
		);
		pos = 
			strchr( info.pos+info.len, '$' );
	}
}

void replace_by_name_replace(
	t_replace_by_name *x,
	AtomDynA* ret,
	char* src
)
{
	DB_PRINT( "parsing '%s'...", src );
	ReplInfoA parse_info;
	ReplInfoA_init( &parse_info );
	replace_by_name_parse_str(
			x,
			src,
			&parse_info
	);
	DB_PRINT(
			"%i variables",
			ReplInfoA_get_size( &parse_info )
	);
	IntBuf index;
	IntBuf index_sizes;
	int var_count =
		ReplInfoA_get_size( &parse_info );
	IntBuf_init(
			&index,
			var_count
	);
	IntBuf_init(
			&index_sizes,
			var_count
	);
	// init:
	for(int index_ind=0; index_ind<var_count; index_ind++)
	{
		repl_info* info =
			& ReplInfoA_get_array( &parse_info )[index_ind];

		// all indices = 0;
		IntBuf_get_array(
				&index
		)[index_ind] = 0;

		// init all index sizes
		IntBuf_get_array(
				&index_sizes
		)[index_ind] =
			AtomDynA_get_size(
				info->value
			);
	}

	BOOL done = FALSE;

	// enumerate all combinations of var replacements:
	while( !done )
	{
		// 
		char ret_str[256];
		strcpy( ret_str, "" );
		char* pos = src;
		for(int index_ind=0; index_ind<var_count; index_ind++)
		{
			repl_info* info =
				& ReplInfoA_get_array( &parse_info )[index_ind];
			// copy str until next var:
			strncat(
					ret_str,
					pos,
					info->pos - pos
			);
			// copy var:
			{
				t_atom* val =
					& AtomDynA_get_array( info->value )[
						IntBuf_get_array( &index )[index_ind]
					];
				char temp[256];
				atom_string( val, temp, 255 );
				
				strcat(
						ret_str,
						temp
				);
				pos = info->pos + info->len;
			}
		}
		// copy the last bit:
		strcat(
				ret_str,
				pos
		);
		DB_PRINT("str: %s", ret_str );
		t_atom atom_ret;
		// try to convert as float:
		float f;
		char* end_ptr;
		errno = 0;
		f = strtof( ret_str, &end_ptr );
		if( errno == 0 && end_ptr == &ret_str[strlen(ret_str)])
		//if( FALSE )
		{
			SETFLOAT( &atom_ret, f );
		}
		else
		{
			SETSYMBOL( &atom_ret, gensym(ret_str) );
		}
		errno = 0;
		AtomDynA_append(
				ret,
				atom_ret
		);

		// inc indices:
		if( var_count == 0 )
			done = TRUE;
		for(int index_ind=0; index_ind<var_count; index_ind++)
		{
			// index can be increased:
			if(
					IntBuf_get_array(&index)[index_ind] + 1
					<
					IntBuf_get_array(&index_sizes)[index_ind]
			)
			{
				IntBuf_get_array(&index)[index_ind] += 1;
				break;
			}
			else if( index_ind+1 < var_count )
			{
				IntBuf_get_array(&index)[index_ind] = 0;
			}
			// highest index cannot be increased: we are done!
			else {
				done = TRUE;
			}
		}
	}
	IntBuf_exit( &index );
	IntBuf_exit( &index_sizes );
	ReplInfoA_exit( &parse_info );
}

void replace_by_name_input(
	t_replace_by_name *x,
	t_symbol *s,
	int argc,
	t_atom *argv
)
{
	AtomDynA ret;
	AtomDynA_init( & ret );
	for( int i=0; i<argc; i++ )
	{
		t_atom* current = & argv[i];
		switch( current->a_type )
		{
			case A_SYMBOL:
			{
				//DB_PRINT( "symbol" );
				//char dst[256];
				char src[256];
				AtomDynA atoms;
				AtomDynA_init( &atoms );
				atom_string( current, src, 255 );
				replace_by_name_replace(
						x,
						&atoms,
						src
				);
				for( unsigned int j=0; j<AtomDynA_get_size( &atoms ); j++ )
				{
					AtomDynA_append(
							&ret,
							AtomDynA_get_array( &atoms )[j]
					);
				}
				AtomDynA_exit( &atoms );
			}
			break;
			default:
				//DB_PRINT( "other" );
				AtomDynA_append(
						&ret,
						*current
				);
			break;
		}
	}
	//DB_PRINT( "size: %i", AtomDynA_get_size( &ret ) );
	outlet_anything(
		x->outlet,
		s,
		AtomDynA_get_size( &ret ),
		AtomDynA_get_array( &ret )
	);
	AtomDynA_exit( & ret );
}
