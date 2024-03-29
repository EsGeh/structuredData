#include "sdEvent.h"

#include "m_pd.h"

#include "Global.h"

#include "Map.h"

static t_class* event_class;
static t_class* unevent_class;
static t_class* eventAddParam_class;
static t_class* eventToProperties_class;

t_class* register_event(
	t_symbol* className
);

t_class* register_unevent(
	t_symbol* className
);

t_class* register_eventAddParam(
	t_symbol* className
);

t_class* register_eventToProperties(
	t_symbol* className
);

void sdEvent_setup()
{
	event_class = register_event( gensym("sdEvent") );
	unevent_class = register_unevent( gensym("sdUnEvent") );
	eventAddParam_class = register_eventAddParam( gensym("sdEventAddParam") );
	eventToProperties_class = register_eventToProperties( gensym("sdEventToProperties") );
}

//----------------------------------
// event
//----------------------------------

#define FREE_DYN_A(p,size) (AtomDynA_exit(p))
#define HASH_SYMBOL(sym) \
	((uintptr_t ) sym)

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpointer-to-int-cast"
#pragma GCC diagnostic ignored "-Wsign-compare"
DECL_MAP(ParamMap,t_symbol*,AtomDynA,getbytes,freebytes,FREE_DYN_A,HASH_SYMBOL,COMPARE_SYMBOLS)
DEF_MAP(ParamMap,t_symbol*,AtomDynA,getbytes,freebytes,FREE_DYN_A,HASH_SYMBOL,COMPARE_SYMBOLS)
#pragma GCC diagnostic pop

typedef struct s_event {
  t_object x_obj;
	t_symbol* type;
	SymBuf args;
	// packs by type:
	ParamMap param_values;
	ParamMap param_other_values;

	// unspecific packs:
	//AtomDynA acc;
	t_inlet* arg_inlets;
	t_outlet* outlet;
} t_event;

void* event_init(
	t_symbol *s,
	int argc,
	t_atom *argv
);
void event_exit(
	struct s_event* x
);

void event_bang(
	t_event* x
);

void event_other_args(
	t_event* x,
	t_symbol *s,
	int argc,
	t_atom *argv
);

void event_set_arg(
	t_event* x,
	t_symbol *s,
	int argc,
	t_atom *argv
);

void event_on_set_arg(
	t_event* x,
	t_symbol *s,
	int argc,
	t_atom *argv
);

void event_on_list(
	t_event* x,
	t_symbol *s,
	int argc,
	t_atom *argv
);

t_class* register_event(
	t_symbol* className
)
{
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcast-function-type"
	t_class* class =
		class_new(
			className,
			(t_newmethod )event_init, // constructor
			(t_method )event_exit, // destructor
			sizeof(t_event),
			CLASS_NOINLET,
			// creation arguments:
			A_GIMME,
			0
		);
#pragma GCC diagnostic pop
	class_sethelpsymbol(
			class,
			gensym("sdEvent")
	);

	class_addbang( class, event_bang );
	class_addlist( class, event_other_args );
	class_addanything(
			class,
			event_on_set_arg
	);

	return class;
}

void* event_init(
	t_symbol* UNUSED(s),
	int argc,
	t_atom *argv
)
{
  t_event *x = (t_event *)pd_new(event_class);

	if(
		argc < 1
	)
	{
		pd_error(x, "not enough arguments. syntax: eventname, param1, param2, ...");
		return NULL;
	}
	x->type = atom_getsymbol( &argv[0] );

	SymBuf_init( & x->args, argc-1);
	ParamMap_init( & x->param_values, 1024 );
	ParamMap_init( & x->param_other_values, 1024 );

	if( argc == 1 )
	{
		inlet_new(
				& x->x_obj,
				&x->x_obj.ob_pd,
				&s_bang,
				&s_bang
		);
	}
	else
	{
		for(unsigned int i=0; i<(unsigned int )argc-1; i++)
		{
			if(
					argv[1+i].a_type != A_SYMBOL
			)
			{
				char buf[256];
				atom_string( & argv[1+i] , buf, 255 );
				pd_error(x, "not a symbol: %s. syntax: name, param1, param2, ...", buf);
				event_exit( x );
				return NULL;
			}
			SymBuf_get_array( &x->args )[i] =
				atom_getsymbol( & argv[1+i] );
			x->arg_inlets = inlet_new(
				& x->x_obj,
				&x->x_obj.ob_pd,
				&s_list,
				atom_getsymbol( & argv[1+i] )
			);
			t_atom zero_a;
			SETFLOAT( & zero_a, 0 );
			event_set_arg(
					x,
					atom_getsymbol( &argv[1+i] ),
					1,
					&zero_a
			);
		}
	}

	// other packs:
	x->arg_inlets = inlet_new(
		& x->x_obj,
		&x->x_obj.ob_pd,
		&s_list,
		&s_list
		//gensym("other")
	);

	// rightmost inlet: set event type
	symbolinlet_new(
		& x->x_obj,
		&x->type
	);
	x->outlet =
		outlet_new( & x->x_obj, &s_list);

  return (void *)x;
}

void event_exit(
	t_event* x
)
{
	SymBuf_exit( & x->args );
	ParamMap_exit( & x->param_values );
	ParamMap_exit( & x->param_other_values );
}

void event_bang(
	t_event* x
)
{
	DB_PRINT("event_bang");
	AtomDynA acc;
	AtomDynA_init( &acc );
	{
		t_atom type_a;
		SETSYMBOL( & type_a, x->type );
		AtomDynA_append(
				&acc,
				type_a
		);
		t_atom count_a;
		SETFLOAT( &count_a, 0 );
		AtomDynA_append(
				&acc,
				count_a
		);
	}

	// accumulate args:
	for( unsigned int i=0; i< SymBuf_get_size( &x->args ); i++ )
	{
		t_symbol* param_name = SymBuf_get_array( &x->args )[i];
		t_atom param_name_a;
		SETSYMBOL( &param_name_a, param_name );
		AtomDynA* value = ParamMap_get(
				& x->param_values,
				param_name
		);
		char param_name_str[256];
		atom_string( &param_name_a, param_name_str, 255 );
		if( !value )
		{
			pd_error(
					x,
					"value not set: %s",
					param_name_str
			);
			ParamMap_clear( & x->param_other_values );
			AtomDynA_exit( &acc );
			return;
		}
		{
			AtomDynA_append(
					&acc,
					param_name_a
			);
		}
		{
			t_atom count;
			SETFLOAT(
					&count,
					AtomDynA_get_size( value )
			);
			AtomDynA_append(
					&acc,
					count
			);
		}
		for( unsigned int val_ind=0; val_ind< AtomDynA_get_size( value ); val_ind++ )
		{
			AtomDynA_append(
					&acc,
					AtomDynA_get_array( value )[val_ind]
			);
		}
	}
	// other args
	MAP_FORALL_KEYS_BEGIN(ParamMap,t_symbol*,&x->param_other_values,param_name)
		t_atom param_name_a;
		SETSYMBOL( &param_name_a, param_name );
		AtomDynA* value = ParamMap_get(
				& x->param_other_values,
				param_name
		);
		char param_name_str[256];
		atom_string( &param_name_a, param_name_str, 255 );
		if( !value )
		{
			pd_error(
					x,
					"FATAL ERROR with '%s'",
					param_name_str
			);
			ParamMap_clear( & x->param_other_values );
			AtomDynA_exit( &acc );
			return;
		}
		{
			AtomDynA_append(
					&acc,
					param_name_a
			);
		}
		{
			t_atom count;
			SETFLOAT(
					&count,
					AtomDynA_get_size( value )
			);
			AtomDynA_append(
					&acc,
					count
			);
		}
		for( unsigned int val_ind=0; val_ind< AtomDynA_get_size( value ); val_ind++ )
		{
			AtomDynA_append(
					&acc,
					AtomDynA_get_array( value )[val_ind]
			);
		}
	MAP_FORALL_KEYS_END(ParamMap,t_symbol*,&x->param_other_values,param_name)
	SETSYMBOL(
			&AtomDynA_get_array( &acc )[0],
			x->type
	);
	SETFLOAT(
			&AtomDynA_get_array( &acc )[1],
			AtomDynA_get_size( &acc ) - 2
	);
	outlet_list(
			x->outlet,
			& s_list,
			AtomDynA_get_size( &acc ),
			AtomDynA_get_array( &acc )
	);
	ParamMap_clear( & x->param_other_values );
	AtomDynA_exit( & acc );
}

void event_other_args(
	t_event* x,
	t_symbol* UNUSED(s),
	int argc,
	t_atom *argv
)
{
	DB_PRINT("event_other_args");
	unsigned int pos = 0;
	while( pos < (unsigned int )argc )
	{
		if(
			argc < 2
			|| argv[pos+0].a_type != A_SYMBOL
			|| argv[pos+1].a_type != A_FLOAT
			|| pos+2 + atom_getint( &argv[1] ) > argc
		)
		{
			pd_error(x, "at pos %i: invalid sdPack", pos);
			return;
		}
		t_symbol* type = atom_getsymbol( &argv[pos+0] );
		t_int count = atom_getint( &argv[pos+1] );

		AtomDynA* args = getbytes( sizeof( AtomDynA ) );
		AtomDynA_init( args );
		for( int i=0; i<count; i++ )
		{
			AtomDynA_append(
					args,
					argv[pos+2+i]
			);
		}
		// delete value, if already in map:
		{
			AtomDynA* value =
				ParamMap_get( 
					& x->param_other_values,
					type
				);
			if( value )
			{
				ParamMap_delete(
						& x->param_other_values,
						type
				);
			}
		}
		ParamMap_insert(
			& x->param_other_values,
			type,
			args
		);

		pos += (count + 2) ;
	}
}

void event_set_arg(
	t_event* x,
	t_symbol *s,
	int argc,
	t_atom *argv
)
{
	AtomDynA* args = getbytes( sizeof( AtomDynA ) );
	AtomDynA_init( args );
	for( int i=0; i<argc; i++ )
	{
		AtomDynA_append(
				args,
				argv[i]
		);
	}
	// delete value, if already in map:
	{
		AtomDynA* value =
			ParamMap_get( 
				& x->param_other_values,
				s
			);
		if( value )
		{
			ParamMap_delete(
					& x->param_other_values,
					s
			);
		}
	}
	ParamMap_insert(
		& x->param_values,
		s,
		args
	);
}

void event_on_set_arg(
	t_event* x,
	t_symbol *s,
	int argc,
	t_atom *argv
)
{
	DB_PRINT("event_on_set_arg");
	event_set_arg(
			x,
			s,
			argc,
			argv
	);
	if( SymBuf_get_size( &x->args ) > 0 && s == SymBuf_get_array( &x->args )[0] )
		event_bang( x );
}

//----------------------------------
// unevent
//----------------------------------

DECL_DYN_ARRAY(IntArray,t_int, getbytes, freebytes)
DEF_DYN_ARRAY(IntArray,t_int, getbytes, freebytes)

typedef struct s_unevent {
  t_object x_obj;
	unsigned int paramsCount;
	t_symbol** params;
	t_outlet** outlets;
	t_outlet* otherParamsOutlet;
	t_outlet* typeOutlet;
} t_unevent;

void* unevent_init(
	t_symbol *s,
	int argc,
	t_atom *argv
);
void unevent_exit(
	struct s_unevent* x
);

void unevent_input(
	t_unevent* x,
	t_symbol *s,
	int argc,
	t_atom *argv
);

t_class* register_unevent(
	t_symbol* className
)
{
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcast-function-type"
	t_class* class =
		class_new(
			className,
			(t_newmethod )unevent_init, // constructor
			(t_method )unevent_exit, // destructor
			sizeof(t_unevent),
			CLASS_DEFAULT, // graphical repr ?
			// creation arguments:
			A_GIMME,
			0
		);
#pragma GCC diagnostic pop
	class_sethelpsymbol(
			class,
			gensym("sdEvent")
	);


	class_addlist( class, unevent_input );

	return class;
}

void* unevent_init(
	t_symbol* UNUSED(s),
	int argc,
	t_atom *argv
)
{
  t_unevent *x = (t_unevent *)pd_new(unevent_class);

	x->paramsCount = argc;
	x->params = getbytes( sizeof( t_symbol* ) * x->paramsCount );
	x->outlets = getbytes( sizeof( t_outlet* ) * x->paramsCount );
	for(unsigned int i=0; i<(unsigned int )argc; i++)
	{
		if(
				argv[i].a_type != A_SYMBOL
		)
		{
			char buf[256];
			atom_string( & argv[1+i] , buf, 255 );
			pd_error(x, "not a symbol: %s. syntax: param1, param2, ...", buf);
			unevent_exit( x );
			return NULL;
		}
		x->params[i] = atom_getsymbol( & argv[i] );

		x->outlets[i] =
			outlet_new( & x->x_obj, &s_list);
	}

	x->otherParamsOutlet =
		outlet_new( & x->x_obj, &s_symbol);

	x->typeOutlet =
		outlet_new( & x->x_obj, &s_symbol);

  return (void *)x;
}

void unevent_exit(
	t_unevent* x
)
{
	freebytes( x->outlets, sizeof( t_outlet* ) * x->paramsCount );
	freebytes( x->params, sizeof( t_symbol* ) * x->paramsCount );
}

void unevent_input(
	t_unevent* x,
	t_symbol* UNUSED(s),
	int argc,
	t_atom *argv
)
{
	if(
		argc < 2
		|| argv[0].a_type != A_SYMBOL
		|| argv[1].a_type != A_FLOAT
		|| argc != atom_getint( &argv[1] ) + 2
	)
	{
		pd_error(x, "invalid sdEvent");
		return;
	}
	// output the event type:
	outlet_symbol(
		x->typeOutlet,
		atom_getsymbol( & argv[0] )
	);

	// find indices where to find the values which to unpack...:
	// a mapping from    index(x->params) to   index_of_pack_in_incoming_msg (-1, if not found)
	t_int* param_indices;
	param_indices = getbytes( sizeof( t_int ) * x->paramsCount );
	// array of "other" packs:
	IntArray otherPacks;
	IntArray_init( &otherPacks );

	// init indices:
	for( unsigned int i=0; i< x->paramsCount; i++)
		param_indices[i] = -1;

	{
		unsigned int pos = 2;
		while( pos < (unsigned int )argc )
		{
			if(
				argc < 2
				|| argv[pos+0].a_type != A_SYMBOL
				|| argv[pos+1].a_type != A_FLOAT
				|| pos + 1 + atom_getint( &argv[pos+1] ) >= argc
			)
			{
				pd_error(x, "at pos %i: invalid sdPack", pos);
				return;
			}
			t_symbol* type = atom_getsymbol( &argv[pos+0] );
			unsigned int count = atom_getint( &argv[pos+1] );
			BOOL found = FALSE;
			for( unsigned int i = 0; i < x->paramsCount; i++ )
			{
				if( x->params[i] == type )
				{
					param_indices[i] = pos;
					found = TRUE;
				}
			}
			if( ! found )
			{
				IntArray_append( &otherPacks, pos );
			}
			pos += (count + 2) ;
		}
	}

	// output "other" parameters:
	{
		for(unsigned int i = 0; i<IntArray_get_size( &otherPacks ); i++)
		{
			t_int pos = IntArray_get_array( &otherPacks )[i];
			if(
				argv[pos+0].a_type != A_SYMBOL
				|| argv[pos+1].a_type != A_FLOAT
				|| pos + 1 + atom_getint( &argv[pos+1] ) >= argc
			)
			{
				pd_error(x, "at pos %i: invalid sdPack", (int )pos);
				IntArray_exit( & otherPacks );
				freebytes( param_indices, sizeof( t_int ) * x->paramsCount );
				return;
			}
			unsigned int count = atom_getint( &argv[pos+1] );
			outlet_list(
				x->otherParamsOutlet,
				& s_list,
				count+2,
				& argv[ pos+0 ]
			);
		}
	}
	{
		// output parameters:
		for(int i = x->paramsCount-1; i>= 0; i--)
		{
			t_symbol* currentSym = x->params[i];
			t_outlet* currentOutlet = x->outlets[i];

			// unconditionally dump at an "all" outlet
			if( currentSym == gensym("all") )
			{
				unsigned int pos = 2;
				while( pos < (unsigned int )argc )
				{
					if(
						argv[pos+0].a_type != A_SYMBOL
						|| argv[pos+1].a_type != A_FLOAT
						|| pos + 1 + atom_getint( &argv[pos+1] ) >= argc
					)
					{
						pd_error(x, "at pos %i: invalid sdPack", pos);
						IntArray_exit( & otherPacks );
						freebytes( param_indices, sizeof( t_int ) * x->paramsCount );
						return;
					}
					unsigned int count = atom_getint( &argv[pos+1] );

					outlet_list(
						currentOutlet,
						& s_list,
						atom_getint( & argv[ pos+1 ] ) + 2,
						& argv[ pos+0 ]
					);

					pos += (count + 2) ;
				}
			}
			if( param_indices[i] == -1 )
			{
				// if there is no such value, just output at the otherParamsOutlet:
				/*
				outlet_list(
					x->otherParamsOutlet,
					& s_list,
					min(atom_getint( &argv[pos+1] ), argc-pos ),
					& argv[pos]
				);
				*/
				/*
				char buf[256];
				t_atom current_sym_atom;
				SETSYMBOL( &current_sym_atom, currentSym );
				atom_string( &current_sym_atom, buf, 256 );
				pd_error( x, "no such value in event: %s", buf );
				*/
			}
			else
			{
				outlet_list(
					currentOutlet,
					& s_list,
					atom_getint( & argv[ param_indices[i]+1 ] ),
					& argv[ param_indices[i]+2 ]
				);
			}
		}
	}
	IntArray_exit( & otherPacks );
	freebytes( param_indices, sizeof( t_int ) * x->paramsCount );
}

//----------------------------------
// eventAddParam
//----------------------------------

typedef struct s_eventAddParam {
  t_object x_obj;
	t_inlet* inlet2;
	t_outlet* outlet;
	unsigned int otherPackCount;
	t_atom* otherPack;
} t_eventAddParam;

void* eventAddParam_init(
	t_symbol *s,
	int argc,
	t_atom *argv
);

void eventAddParam_exit(
	struct s_eventAddParam* x
);

void eventAddParam_set(
	t_eventAddParam* x,
	t_symbol *s,
	int argc,
	t_atom *argv
);

void eventAddParam_input(
	t_eventAddParam* x,
	t_symbol *s,
	int argc,
	t_atom *argv
);

t_class* register_eventAddParam(
	t_symbol* className
)
{
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcast-function-type"
	t_class* class =
		class_new(
			className,
			(t_newmethod )eventAddParam_init, // constructor
			(t_method )eventAddParam_exit, // destructor
			sizeof(t_eventAddParam),
			CLASS_DEFAULT, // graphical repr ?
			// creation arguments:
			0
		);
#pragma GCC diagnostic pop
	class_sethelpsymbol(
			class,
			gensym("sdEvent")
	);

	class_addlist( class, eventAddParam_input );
	class_addmethod(
		class,
		(t_method )eventAddParam_set,
		gensym("set"),
		A_GIMME,
		0
	);

	return class;
}

void* eventAddParam_init(
	t_symbol* UNUSED(s),
	int UNUSED(argc),
	t_atom* UNUSED(argv)
)
{
  t_eventAddParam *x = (t_eventAddParam *)pd_new(eventAddParam_class);

	x->otherPackCount = 0;
	x->otherPack = NULL;
	//getbytes( sizeof( t_atom ) * x->otherPackCount );

	x-> inlet2 =
		inlet_new(
			& x->x_obj,
			& x->x_obj.ob_pd,
			gensym("list"),
			gensym("set")
		);
	x->outlet =
		outlet_new( & x->x_obj, &s_list);

  return (void *)x;
}

void eventAddParam_exit(
	t_eventAddParam* x
)
{
	//if( x->otherPack )
		freebytes( x->otherPack, sizeof( t_atom ) * x->otherPackCount );
}

void eventAddParam_set(
	t_eventAddParam* x,
	t_symbol* UNUSED(s),
	int argc,
	t_atom *argv
)
{
	if(
		argc < 2
		|| argv[0].a_type != A_SYMBOL
		|| argv[1].a_type != A_FLOAT
		|| argc != atom_getint( &argv[1] ) + 2
	)
	{
		pd_error(x, "invalid sdPack");
		return;
	}
	//post("set");
	//if( x->otherPack )
		freebytes( x->otherPack, sizeof( t_atom ) * x->otherPackCount );
	x->otherPackCount = argc;
	//if( argc > 0)
		x->otherPack = getbytes( sizeof( t_atom ) * argc );
		x->otherPackCount = argc;
	for(unsigned int i=0; i < (unsigned int )argc; i++)
	{
		x->otherPack[i] = argv[i];
		/*
		char buf[256];
		atom_string( & argv[i], buf, 255 );
		post("setting arg: %s", buf);
		*/
	}
}

void eventAddParam_input(
	t_eventAddParam* x,
	t_symbol* UNUSED(s),
	int argc,
	t_atom *argv
)
{
	if(
		argc < 2
		|| argv[0].a_type != A_SYMBOL
		|| argv[1].a_type != A_FLOAT
		|| argc != atom_getint( &argv[1] ) + 2
	)
	{
		pd_error(x, "invalid sdPack");
		return;
	}
	//post("input, argc: %i", argc);
	t_int new_size = argc + x->otherPackCount;
	t_atom* ret = getbytes( sizeof( t_atom ) * new_size );
	for( unsigned int i=0; i<(unsigned int )argc; i++ )
	{
		ret[i] = argv[i];
	}
	for( unsigned int i=0; i < x->otherPackCount; i++ )
	{
		ret[argc+i] = x->otherPack[i];
	}
	SETFLOAT( &ret[1], atom_getint( & ret[1] ) + x->otherPackCount);

	outlet_list(
		x->outlet,
		& s_list,
		new_size,
		ret
	);
	freebytes( ret, sizeof( t_atom ) * new_size );
}

//----------------------------------
// eventToProperties
//----------------------------------

typedef struct s_eventToProperties {
  t_object x_obj;
	t_outlet* outletSelector;
	t_outlet* outletToProperties;
} t_eventToProperties;

void* eventToProperties_init();

void eventToProperties_input(
	t_eventToProperties* x,
	t_symbol* UNUSED(s),
	int argc,
	t_atom *argv
);

t_class* register_eventToProperties(
	t_symbol* className
)
{
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcast-function-type"
	t_class* class =
		class_new(
			className,
			(t_newmethod )eventToProperties_init, // constructor
			NULL, // destructor
			sizeof(t_eventToProperties),
			CLASS_DEFAULT, // graphical repr ?
			// creation arguments:
			0
		);
#pragma GCC diagnostic pop
	class_sethelpsymbol(
			class,
			gensym("sdEvent")
	);

	class_addlist( class, eventToProperties_input );

	return class;
}

void* eventToProperties_init()
{
	t_eventToProperties *x = (t_eventToProperties *)pd_new(eventToProperties_class);

	x->outletSelector =
		outlet_new( & x->x_obj, &s_list);

	x->outletToProperties =
		outlet_new( & x->x_obj, &s_list);

  return (void *)x;
}

void eventToProperties_input(
	t_eventToProperties* x,
	t_symbol* UNUSED(s),
	int argc,
	t_atom *argv
)
{
	if(
		argc < 2
		|| argv[0].a_type != A_SYMBOL
		|| argv[1].a_type != A_FLOAT
		|| atom_getint( & argv[1] ) != (argc - 2 )
	)
	{
		pd_error(x, "invalid sdEvent");
		return;
	}
	// output parameters:
	int currentPos = 2;
	while( currentPos < argc )
	{
		if(
			argc < 2
			|| argv[currentPos+0].a_type != A_SYMBOL
			|| argv[currentPos+1].a_type != A_FLOAT
			// || (argc-pos) - 2 >= atom_getint( &argv[1] )
		)
		{
			pd_error(x, "at pos %i: invalid sdPack", currentPos);
			return;
		}
		//t_symbol* currentType = atom_getsymbol( &argv[currentPos+0] );
		int currentSize = atom_getint( &argv[currentPos + 1] );
		if( currentPos + currentSize >= argc )
		{
			pd_error( x, "invalid pack at position %i: not enough parameters", currentPos );
			return;
		}

		// send to obj: set <property> <val1> [...]
		t_atom* toSend = getbytes( sizeof( t_atom ) * (currentSize+1) );
		toSend[0] = argv[currentPos] ;
		for( int i=0; i < currentSize; i++ )
		{
			toSend[1+i] = argv[currentPos+2+i];
		}
		outlet_anything(
			x->outletToProperties,
			gensym("set"),
			currentSize+1,
			toSend
		);
		freebytes( toSend, sizeof( t_atom ) * (currentSize+1) );
		currentPos = currentPos + 2 + currentSize ;

		/*
		// send: set ( <property> <val1> ... )
		t_atom* to_properties = getbytes( sizeof( t_atom ) * (3+count) );
		SETSYMBOL( &to_properties[0], gensym("set") );
		SETFLOAT( &to_properties[1], count+1 );
		SETSYMBOL( &to_properties[2], type );
		for( int i=0; i<count; i++ )
		{
			to_properties[3+i] = argv[pos+2+i];
		}
		outlet_list(
			x->outletToProperties,
			& s_list,
			3+count,
			to_properties
		);
		freebytes( to_properties, sizeof( t_atom ) * (3+count) );
		pos += (count + 2) ;
		*/
	}

	// output the event type:
	outlet_anything(
		x->outletSelector,
		atom_getsymbol( & argv[0] ),
		0,
		NULL
	);
}
