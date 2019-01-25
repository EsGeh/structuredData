#include "sdEvent.h"

#include "m_pd.h"


static t_class* event_class;
static t_class* unevent_class;
static t_class* eventAddParam_class;

t_class* register_event(
	t_symbol* className
);

t_class* register_unevent(
	t_symbol* className
);

t_class* register_eventAddParam(
	t_symbol* className
);

void sdEvent_setup()
{
	event_class = register_event( gensym("sdEvent") );
	unevent_class = register_unevent( gensym("sdUnEvent") );
	eventAddParam_class = register_eventAddParam( gensym("sdEventAddParam") );
}

//----------------------------------
// event
//----------------------------------

typedef struct s_event {
  t_object x_obj;
	t_symbol* type;
	unsigned int paramsCount;
	t_symbol** params;
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

void event_input(
	t_event* x,
	t_symbol *s,
	int argc,
	t_atom *argv
);

t_class* register_event(
	t_symbol* className
)
{
	t_class* class =
		class_new(
			className,
			(t_newmethod )event_init, // constructor
			(t_method )event_exit, // destructor
			sizeof(t_event),
			CLASS_DEFAULT, // graphical repr ?
			// creation arguments:
			A_GIMME,
			0
		);

	class_addlist( class, event_input );

	return class;
}

void* event_init(
	t_symbol *s,
	int argc,
	t_atom *argv
)
{
  t_event *x = (t_event *)pd_new(event_class);

	if(
		argc < 1
	)
	{
		pd_error(x, "not enough arguments. syntax: name, param1, param2, ...");
		return NULL;
	}
	x->type = atom_getsymbol( &argv[0] );
	x->paramsCount = argc-1;
	x->params = getbytes( sizeof( t_symbol* ) * x->paramsCount );
	for(unsigned int i=0; i< argc-1; i++)
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
		x->params[0+i] = atom_getsymbol( & argv[1+i] );
	}

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
	freebytes( x->params, sizeof( t_symbol* ) * x->paramsCount );
}

void event_input(
	t_event* x,
	t_symbol *s,
	int argc,
	t_atom *argv
)
{
	if( argc > x->paramsCount )
	{
		pd_error( x, "too many values in message!: got %i, expected %i", argc, x->paramsCount );
		return;
	}

	unsigned new_count = (argc*3) + 2;
	t_atom* list_ret = getbytes( sizeof( t_atom ) * new_count);
	SETSYMBOL( &list_ret[0], x->type );
	SETFLOAT( & list_ret[1], new_count - 2 );
	unsigned int pos = 2;
	for(unsigned int i=0; i< argc; i++)
	{
		SETSYMBOL( &list_ret[pos+0], x->params[i] );
		SETFLOAT( & list_ret[pos+1], 1 );
		list_ret[pos+2] = argv[i];
		pos += 3;
	}

	outlet_list(
		x->outlet,
		& s_list,
		new_count,
		list_ret
	);

	freebytes( list_ret, sizeof( t_atom ) * new_count );
}

//----------------------------------
// unevent
//----------------------------------

typedef struct s_unevent {
  t_object x_obj;
	unsigned int paramsCount;
	t_symbol** params;
	t_outlet** outlets;
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

// utils:
void unevent_outputAt(
	t_unevent* x,
	t_symbol* s, // output operation
	// what to output
	int argc,
	t_atom *argv
);

t_class* register_unevent(
	t_symbol* className
)
{
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

	class_addlist( class, unevent_input );

	return class;
}

void* unevent_init(
	t_symbol *s,
	int argc,
	t_atom *argv
)
{
  t_unevent *x = (t_unevent *)pd_new(unevent_class);

	x->paramsCount = argc;
	x->params = getbytes( sizeof( t_symbol* ) * x->paramsCount );
	x->outlets = getbytes( sizeof( t_outlet* ) * x->paramsCount );
	for(unsigned int i=0; i< argc; i++)
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
	t_symbol *s,
	int argc,
	t_atom *argv
)
{
	// output the event type:
	outlet_symbol(
		x->typeOutlet,
		atom_getsymbol( & argv[0] )
	);
	// output parameters:
	unsigned int pos = 2;
	while( pos < argc )
	{
		if(
			argc < 2
			|| argv[pos+0].a_type != A_SYMBOL
			|| argv[pos+1].a_type != A_FLOAT
			// || (argc-pos) - 2 >= atom_getint( &argv[1] )
		)
		{
			pd_error(x, "at pos %i: invalid sdPack", pos);
			return;
		}
		t_symbol* type = atom_getsymbol( &argv[pos+0] );
		unsigned int count = atom_getint( &argv[pos+1] );
		unevent_outputAt(
			x,
			type,
			count,
			& argv[pos+2]
		);
		pos += (count + 2) ;
	}
}

void unevent_outputAt(
	t_unevent* x,
	t_symbol* s, // output operation
	// what to output
	int argc,
	t_atom *argv
)
{
	for(unsigned int i=0; i<x->paramsCount; i++)
	{
		t_symbol* currentSym = x->params[i];
		t_outlet* currentOutlet = x->outlets[i];
		if( currentSym == s || currentSym == gensym("all") )
		{
			outlet_list(
				currentOutlet,
				& s_list,
				argc,
				argv
			);
		}
	}
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
	t_symbol *s,
	int argc,
	t_atom *argv
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
	t_symbol *s,
	int argc,
	t_atom *argv
)
{
	if(
		argc < 2
		|| argv[0].a_type != A_SYMBOL
		|| argv[1].a_type != A_FLOAT
		// || (argc-pos) - 2 >= atom_getint( &argv[1] )
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
	for(unsigned int i=0; i < argc; i++)
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
	t_symbol *s,
	int argc,
	t_atom *argv
)
{
	if(
		argc < 2
		|| argv[0].a_type != A_SYMBOL
		|| argv[1].a_type != A_FLOAT
		// || (argc-pos) - 2 >= atom_getint( &argv[1] )
	)
	{
		pd_error(x, "invalid sdPack");
		return;
	}
	//post("input, argc: %i", argc);
	t_atom* ret = getbytes( argc + x->otherPackCount );
	for( unsigned int i=0; i<argc; i++ )
	{
		ret[i] = argv[i];
	}
	for( unsigned int i=0; i < x->otherPackCount; i++ )
		ret[argc+i] = x->otherPack[i];
	SETFLOAT( &ret[1], atom_getint( & ret[1] ) + x->otherPackCount);
	//unsigned int old_size = atom_getint( & ret[1] );
	//SETFLOAT( &ret[1], old_size + x->otherPackCount);

	outlet_list(
		x->outlet,
		& s_list,
		argc + x->otherPackCount,
		ret
	);
}
