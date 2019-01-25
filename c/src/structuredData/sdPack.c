#include "sdPack.h"
#include <string.h>

#include "m_pd.h"


static t_class* pack_class;
static t_class* unpack_class;
static t_class* packFromHuman_class;
static t_class* packFilter_class;;
static t_class* packToMessage_class;;
static t_class* packFromMessage_class;;

t_class* register_pack(
	t_symbol* className
);
t_class* register_unpack(
	t_symbol* className
);
t_class* register_packFromHuman(
	t_symbol* className
);
t_class* register_packFilter(
	t_symbol* className
);
t_class* register_packToMessage(
	t_symbol* className
);
t_class* register_packFromMessage(
	t_symbol* className
);

void sdPack_setup()
{
	pack_class = register_pack( gensym("sdPack") );
	unpack_class = register_unpack( gensym("sdUnPack") );
	packFromHuman_class = register_packFromHuman( gensym("sdPackFromHuman") );
	packFilter_class = register_packFilter( gensym("sdPackFilter") );
	packToMessage_class = register_packToMessage( gensym("sdPackToMessage") );
	packFromMessage_class = register_packFromMessage( gensym("sdPackFromMessage") );
}

//----------------------------------
// pack
//----------------------------------

typedef struct s_pack {
  t_object x_obj;
	t_symbol* name;
	t_outlet* outlet;
} t_pack;

void* pack_init(
	t_symbol *s,
	int argc,
	t_atom *argv
);
void pack_exit(
	struct s_pack* x
);

void pack_input(
	t_pack* x,
	t_symbol *s,
	int argc,
	t_atom *argv
);

t_class* register_pack(
	t_symbol* className
)
{
	t_class* class =
		class_new(
			className,
			(t_newmethod )pack_init, // constructor
			(t_method )pack_exit, // destructor
			sizeof(t_pack),
			CLASS_DEFAULT, // graphical repr ?
			// creation arguments:
			A_GIMME,
			0
		);

	class_addlist( class, pack_input );

	return class;
}

void* pack_init(
	t_symbol *s,
	int argc,
	t_atom *argv
)
{
  t_pack *x = (t_pack *)pd_new(pack_class);

	if( argc > 1  )
	{
		pd_error(x, "too many arguments!");
		return NULL;
	}
	if( argc == 1 )
	{
		x->name = atom_getsymbol( &argv[0] );
	}
	else
	{
		x->name = gensym("UNDEFINED");
	}

	symbolinlet_new(
		& x->x_obj,
		&x->name
	);
	x->outlet =
		outlet_new( & x->x_obj, &s_list);

  return (void *)x;
}

void pack_exit(
	t_pack* x
)
{
}

void pack_input(
	t_pack* x,
	t_symbol *s,
	int argc,
	t_atom *argv
)
{
	unsigned new_count = argc + 2;
	t_atom* list_ret = getbytes( sizeof( t_atom ) * new_count);
	SETSYMBOL( &list_ret[0], x->name );
	SETFLOAT( & list_ret[1], argc );
	memcpy( &list_ret[2], argv, argc * sizeof( t_atom ) );

	outlet_list(
		x->outlet,
		& s_list,
		new_count,
		list_ret
	);

	freebytes( list_ret, sizeof( t_atom ) * new_count );
}


//----------------------------------
// unpack
//----------------------------------

typedef struct s_unpack {
  t_object x_obj;
	t_outlet* typeOutlet;
	t_outlet* sizeOutlet;
	t_outlet* contentOutlet;
} t_unpack;

void* unpack_init(
	t_symbol *s,
	int argc,
	t_atom *argv
);
void unpack_exit(
	t_unpack* x
);

void unpack_input(
	t_unpack* x,
	t_symbol *s,
	int argc,
	t_atom *argv
);

t_class* register_unpack(
	t_symbol* className
)
{
	t_class* class =
		class_new(
			className,
			(t_newmethod )unpack_init, // constructor
			(t_method )unpack_exit, // destructor
			sizeof(t_unpack),
			CLASS_DEFAULT, // graphical repr ?
			// creation arguments:
			0
		);

	class_addlist( class, unpack_input );

	return class;
}

void* unpack_init(
	t_symbol *s,
	int argc,
	t_atom *argv
)
{
  t_unpack *x = (t_unpack *)pd_new(unpack_class);

	if( argc > 0  )
	{
		pd_error(x, "too many arguments!");
		return NULL;
	}

	x->contentOutlet =
		outlet_new( & x->x_obj, &s_list);
	x->sizeOutlet =
		outlet_new( & x->x_obj, &s_list);
	x->typeOutlet =
		outlet_new( & x->x_obj, &s_list);

  return (void *)x;
}

void unpack_exit(
	t_unpack* x
)
{
}

void unpack_input(
	t_unpack* x,
	t_symbol *s,
	int argc,
	t_atom *argv
)
{
	if(
		argc < 2
		|| argv[0].a_type != A_SYMBOL
		|| argv[1].a_type != A_FLOAT
		|| argc - 2 != atom_getfloat( &argv[1] )
	)
	{
		pd_error(x, "invalid sdPack!");
		return;
	}

	outlet_list(
		x->typeOutlet,
		& s_list,
		1,
		&argv[0]
	);
	outlet_list(
		x->sizeOutlet,
		& s_list,
		1,
		&argv[1]
	);
	outlet_list(
		x->contentOutlet,
		& s_list,
		argc-2,
		&argv[2]
	);
}

//----------------------------------
// packFromHuman
//----------------------------------

typedef struct s_packFromHuman {
  t_object x_obj;
	t_symbol* name;
	t_outlet* outlet;
} t_packFromHuman;

void* packFromHuman_init(
	t_symbol *s,
	int argc,
	t_atom *argv
);
void packFromHuman_exit(
	t_packFromHuman* x
);

void packFromHuman_input(
	t_packFromHuman* x,
	t_symbol *s,
	int argc,
	t_atom *argv
);

t_class* register_packFromHuman(
	t_symbol* className
)
{
	t_class* class =
		class_new(
			className,
			(t_newmethod )packFromHuman_init, // constructor
			(t_method )packFromHuman_exit, // destructor
			sizeof(t_packFromHuman),
			CLASS_DEFAULT, // graphical repr ?
			// creation arguments:
			0
		);

	class_addlist( class, packFromHuman_input );

	return class;
}

void* packFromHuman_init(
	t_symbol *s,
	int argc,
	t_atom *argv
)
{
  t_packFromHuman *x = (t_packFromHuman *)pd_new(packFromHuman_class);

	if( argc > 0  )
	{
		pd_error(x, "too many arguments!");
		return NULL;
	}
	x->outlet =
		outlet_new( & x->x_obj, &s_list);

  return (void *)x;
}

void packFromHuman_exit(
	t_packFromHuman* x
)
{
}

#define GET_PACKNAME(tok) \
{ \
	if(*index >= argc) \
	{ \
		post("at pos %i: expected PackName, got END OF FILE", *index); \
		return 1; \
	} \
	if(	\
		argv[(*index)].a_type != A_SYMBOL \
		|| atom_getsymbol( &argv[(*index)] ) == gensym("(") \
		|| atom_getsymbol( &argv[(*index)] ) == gensym(")") \
	) \
	{ \
		char buf[256]; \
		atom_string( &argv[*index], buf, 255 ); \
		post("at pos %i: expected PackName, got %s", *index, buf); \
		return 1; \
	} \
	tok = &argv[*index]; \
	(*index) ++; \
}

#define GET_ANYSYM(tok) \
{ \
	if(*index >= argc) \
	{ \
		post("at pos %i: expected symbol, got END OF FILE", *index); \
		return 1; \
	} \
	if(	argv[(*index)].a_type != A_SYMBOL ) \
	{ \
		char buf[256]; \
		atom_string( &argv[*index], buf, 255 ); \
		post("at pos %i: expected symbol, got %s", *index, buf); \
		return 1; \
	} \
	tok = &argv[*index]; \
	(*index) ++; \
}

#define GET_SYM(sym) \
{ \
	if(*index >= argc) \
	{ \
		char buf[256]; \
		atom_string( &argv[*index], buf, 255 ); \
		post("at pos %i: expected %s, got END OF FILE", *index, sym); \
		return 1; \
	} \
	if(	atom_getsymbol( &argv[*index] ) != gensym(sym) ) \
	{ \
		char buf[256]; \
		atom_string( &argv[*index], buf, 255 ); \
		post("at pos %i: expected %s, got %s", *index, sym, buf); \
		return 1; \
	} \
	(*index) ++; \
}

int packFromHuman_fromHuman(
	// input:
	int argc,
	t_atom* argv,
	// util:
	//AtomList* stack,
	// output:
	unsigned int* outCount,
	t_atom* outputAtoms,
	// read pointer:
	unsigned int* index
)
{
	// EOF:
	if( *index >= argc )
		return 0;

	t_atom* packName;
	unsigned int indexStart = *outCount;
	/*
	char buf[256];
	atom_string(&argv[*index], buf, 255);
	post("START: %s", buf);
	*/
	GET_PACKNAME( packName );
	/*
	atom_string(&argv[*index], buf, 255);
	post("(: %s", buf);
	*/
	GET_SYM( "(" );

	outputAtoms[*outCount] = *packName; (*outCount)++;
	// size can only be set in the end
	(*outCount)++;

	do
	{
		// ARBITRARY CONTENT:
		while(
			(*index) < argc
			&&
			atom_getsymbol(&argv[*index]) != gensym("(")
			&&
			atom_getsymbol(&argv[*index]) != gensym(")")
		)
		{
			//ACCEPT ANYTHING:
			/*
			atom_string(&argv[*index], buf, 255);
			post("ARBITRARY: %s", buf);
			*/
			outputAtoms[*outCount] = argv[*index]; (*outCount)++;
			(*index)++;
		}
		if(
			(*index) < argc
			&&
			atom_getsymbol( &argv[*index] ) == gensym("(")
		)
		{
			(*index) -= 1;
			(*outCount) -= 1;
			int err =
				packFromHuman_fromHuman(
					argc,
					argv,
					outCount,
					outputAtoms,
					index
				);
			if( err )
				return err;
		}
	}
	while(
		(*index) < argc
		&&
		atom_getsymbol( &argv[*index] ) != gensym(")")
	);

	GET_SYM( ")" );
	SETFLOAT(
		&outputAtoms[indexStart+1],
		(*outCount) - (indexStart+2)
	);
	return 0;
}

void packFromHuman_input(
	t_packFromHuman* x,
	t_symbol *s,
	int argc,
	t_atom *argv
)
{
	t_atom* outputAtoms = getbytes( sizeof( t_atom ) * argc * 3 );
	unsigned int outCount = 0;
	unsigned int index=0;
	if(
		!packFromHuman_fromHuman(
			argc,
			argv,
			&outCount,
			outputAtoms,
			&index
		)
	)
	{
		if( index < argc )
		{
			char buf[256];
			atom_string(&argv[index], buf, 255);
			pd_error(x, "WARNING: at pos %i: found '%s', expected END OF FILE!", index, buf );
		}
		outlet_list(
			x->outlet,
			& s_list,
			outCount,
			outputAtoms
		);
	}

	freebytes( outputAtoms, sizeof( t_atom ) * argc * 3 );
}


//----------------------------------
// packFilter
//----------------------------------

typedef struct s_packFilter {
  t_object x_obj;
	unsigned int acceptedTypesCount;
	t_symbol** acceptedTypes;
	t_outlet* acceptOut;
	t_outlet* rejectOut;
} t_packFilter;

void* packFilter_init(
	t_symbol *s,
	int argc,
	t_atom *argv
);
void packFilter_exit(
	t_packFilter* x
);

void packFilter_input(
	t_packFilter* x,
	t_symbol *s,
	int argc,
	t_atom *argv
);

t_class* register_packFilter(
	t_symbol* className
)
{
	t_class* class =
		class_new(
			className,
			(t_newmethod )packFilter_init, // constructor
			(t_method )packFilter_exit, // destructor
			sizeof(t_packFilter),
			CLASS_DEFAULT, // graphical repr ?
			// creation arguments:
			A_GIMME,
			0
		);

	class_addlist( class, packFilter_input );

	return class;
}

void* packFilter_init(
	t_symbol *s,
	int argc,
	t_atom *argv
)
{
  t_packFilter *x = (t_packFilter *)pd_new(packFilter_class);


	if( argc == 0  )
	{
		x->acceptedTypesCount = 1;
		x->acceptedTypes = getbytes( sizeof( t_symbol* ) * 1 );
		x->acceptedTypes[0] = gensym("");

		symbolinlet_new(
			& x->x_obj,
			& x->acceptedTypes[0]
		);
	}
	if( argc == 1 )
	{
		x->acceptedTypesCount = 1;
		x->acceptedTypes = getbytes( sizeof( t_symbol* ) * 1 );
		x->acceptedTypes[0] = atom_getsymbol( &argv[0] );

		symbolinlet_new(
			& x->x_obj,
			& x->acceptedTypes[0]
		);
	}
	else
	{
		x->acceptedTypesCount = argc;
		x->acceptedTypes = getbytes( sizeof( t_symbol* ) * argc );
		for( unsigned int i=0; i<argc; i++)
		{
			x->acceptedTypes[i] = atom_getsymbol( &argv[i] );
		}
	}
	x->acceptOut =
		outlet_new( & x->x_obj, &s_list);
	x->rejectOut =
		outlet_new( & x->x_obj, &s_list);

  return (void *)x;
}

void packFilter_exit(
	t_packFilter* x
)
{
	freebytes( x->acceptedTypes, sizeof( t_symbol* ) * x->acceptedTypesCount );
}

void packFilter_input(
	t_packFilter* x,
	t_symbol *s,
	int argc,
	t_atom *argv
)
{
	if(
		argc < 2
		|| argv[0].a_type != A_SYMBOL
		|| argv[1].a_type != A_FLOAT
		|| argc - 2 != atom_getfloat( &argv[1] )
	)
	{
		pd_error(x, "invalid sdPack!");
		return;
	}
	int accept = 0;
	for( unsigned int i=0; i < x->acceptedTypesCount; i++)
	{
		if( atom_getsymbol( &argv[0] ) == x->acceptedTypes[i] )
		{
			accept = 1;
			break;
		}
	}
	if( accept )
	{
		outlet_list(
			x->acceptOut,
			& s_list,
			argc,
			argv
		);
	}
	else
	{
		outlet_list(
			x->rejectOut,
			& s_list,
			argc,
			argv
		);
	}
}

//----------------------------------
// packToMessage_class
//----------------------------------

typedef struct s_packToMessage {
  t_object x_obj;
	t_outlet* outlet;
} t_packToMessage;

void* packToMessage_init(
	t_symbol *s,
	int argc,
	t_atom *argv
);
void packToMessage_exit(
	struct s_packToMessage* x
);

void packToMessage_input(
	t_packToMessage* x,
	t_symbol *s,
	int argc,
	t_atom *argv
);

t_class* register_packToMessage(
	t_symbol* className
)
{
	t_class* class =
		class_new(
			className,
			(t_newmethod )packToMessage_init, // constructor
			(t_method )packToMessage_exit, // destructor
			sizeof(t_packToMessage),
			CLASS_DEFAULT, // graphical repr ?
			// creation arguments:
			A_GIMME,
			0
		);

	class_addlist( class, packToMessage_input );

	return class;
}

void* packToMessage_init(
	t_symbol *s,
	int argc,
	t_atom *argv
)
{
  t_packToMessage *x = (t_packToMessage *)pd_new(packToMessage_class);

	if( argc > 0 )
	{
		pd_error(x, "too many arguments!");
		return NULL;
	}
	x->outlet =
		outlet_new( & x->x_obj, &s_list);

  return (void *)x;
}

void packToMessage_exit(
	t_packToMessage* x
)
{
}

void packToMessage_input(
	t_packToMessage* x,
	t_symbol *s,
	int argc,
	t_atom *argv
)
{
	//argv[1] = argv[0];
	t_symbol* type = atom_getsymbol( &argv[0] );

	outlet_anything(
		x->outlet,
		type,
		argc-2,
		&argv[2]
	);
}

//----------------------------------
// packFromMessage_class
//----------------------------------

typedef struct s_packFromMessage {
  t_object x_obj;
	t_outlet* outlet;
} t_packFromMessage;

void* packFromMessage_init(
	t_symbol *s,
	int argc,
	t_atom *argv
);
void packFromMessage_exit(
	struct s_packFromMessage* x
);

void packFromMessage_inputList(
	t_packFromMessage* x,
	t_symbol *s,
	int argc,
	t_atom *argv
);

void packFromMessage_inputAny(
	t_packFromMessage* x,
	t_symbol *s,
	int argc,
	t_atom *argv
);

t_class* register_packFromMessage(
	t_symbol* className
)
{
	t_class* class =
		class_new(
			className,
			(t_newmethod )packFromMessage_init, // constructor
			(t_method )packFromMessage_exit, // destructor
			sizeof(t_packFromMessage),
			CLASS_DEFAULT, // graphical repr ?
			// creation arguments:
			A_GIMME,
			0
		);

	class_addlist( class, packFromMessage_inputList );
	class_addanything( class, packFromMessage_inputAny );

	return class;
}

void* packFromMessage_init(
	t_symbol *s,
	int argc,
	t_atom *argv
)
{
  t_packFromMessage *x = (t_packFromMessage *)pd_new(packFromMessage_class);

	if( argc > 0 )
	{
		pd_error(x, "too many arguments!");
		return NULL;
	}
	x->outlet =
		outlet_new( & x->x_obj, &s_list);

  return (void *)x;
}

void packFromMessage_exit(
	t_packFromMessage* x
)
{
}

void packFromMessage_inputList(
	t_packFromMessage* x,
	t_symbol *s,
	int argc,
	t_atom *argv
)
{
	if(
		argc < 1
		|| argv[0].a_type != A_SYMBOL
	)
	{
		pd_error(x, "message doesn't start with a symbol!");
		return;
	}
	//post("list");
	t_atom* ret = getbytes( sizeof( t_atom ) * (argc+1) );
	//memcpy(&ret[2], &argv[1], sizeof( t_atom ) * argc-1 );
	for(unsigned int i=0; i< argc-1; i++)
	{
		ret[i+2] = argv[i+1];
	}
	ret[0] = argv[0];
	SETFLOAT( &ret[1], argc-1 );

	//t_symbol* type = atom_getsymbol( &argv[0] );

	outlet_list(
		x->outlet,
		& s_list,
		argc+1,
		ret
	);
	freebytes( ret, sizeof( t_atom ) * (argc+1) );
}

void packFromMessage_inputAny(
	t_packFromMessage* x,
	t_symbol *s,
	int argc,
	t_atom *argv
)
{
	t_atom* ret = getbytes( sizeof( t_atom ) * (argc+2) );
	for(unsigned int i=0; i< argc; i++)
	{
		ret[i+2] = argv[i];
	}
	SETSYMBOL( &ret[0], s );
	SETFLOAT( &ret[1], argc );

	//t_symbol* type = atom_getsymbol( &argv[0] );

	outlet_list(
		x->outlet,
		& s_list,
		argc+2,
		ret
	);
	freebytes( ret, sizeof( t_atom ) * (argc+2) );
}
