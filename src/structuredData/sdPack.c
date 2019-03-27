#include "sdPack.h"
#include "LinkedList.h"
#include "Buffer.h"
#include "DynArray.h"
#include "Global.h"


#include <string.h>

#include "m_pd.h"


static t_class* pack_class;
static t_class* unpack_class;
static t_class* packFromHuman_class;
static t_class* packFilter_class;;
static t_class* packToMessage_class;;
static t_class* packFromMessage_class;;
static t_class* packMatch_class;;

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
t_class* register_packMatch(
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
	packMatch_class = register_packMatch( gensym("sdPackMatch") );
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
	class_sethelpsymbol(
			class,
			gensym("sdPack")
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
	class_sethelpsymbol(
			class,
			gensym("sdPack")
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
	class_sethelpsymbol(
			class,
			gensym("sdPack")
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

// recursively parse the input:
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
	unsigned int* index,
	// recursion depth
	unsigned int depth
)
{
	// EOF:
	if( *index >= argc )
		return 0;
	
	//char buf[256];

	while(
		(*index) < argc
	)
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

		// '(' indicates a "pdPack": <packname> ( ... )
		//                                      /\
		//                                      ||
		if(
			(*index) < argc
			&&
			atom_getsymbol( &argv[*index] ) == gensym("(")
		)
		{
			//post("(");

			if( (*index) == 0)
			{
				post("at pos 0: got '(' without preceeding 'packname'", (*index));
				return 1;
			}
			// step back one pos, where the pack begins:
			// '(' indicates a "pdPack": <packname> ( ... )
			//                              /\
			//                              ||
			//                           indexStart
			(*index) -= 1;
			(*outCount) -= 1;

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
			outputAtoms[*outCount] = *packName; (*outCount)++;

			GET_SYM( "(" );
			// size can only be set in the end
			(*outCount)++;
			
			// recursively read the content of the pack:
			int err =
				packFromHuman_fromHuman(
					argc,
					argv,
					outCount,
					outputAtoms,
					index,
					depth+1
				);
			if( err )
				return err;

			GET_SYM( ")" );

			SETFLOAT(
				&outputAtoms[indexStart+1],
				(*outCount) - (indexStart+2)
			);
		}
		// ')' without opening '(':
		else if( (*index) < argc )
		{
			if(
				depth > 0
			)
			{
				// this ')' is supposed to be consumed by the caller
				return 0;
			}
			else
			{
				post("at pos %i: got ')' before opening '('", (*index));
				return 1;
			}
		}
	}
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
			&index,
			0 // recursion depth
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

void packFilter_on_set(
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
	class_sethelpsymbol(
			class,
			gensym("sdPack")
	);

	class_addlist( class, packFilter_input );

	class_addmethod(
			class,
			(t_method )packFilter_on_set,
			gensym("set"),
			A_GIMME,
			0
	);

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

		/*
		symbolinlet_new(
			& x->x_obj,
			& x->acceptedTypes[0]
		);
		*/
	}
	else if( argc == 1 )
	{
		x->acceptedTypesCount = 1;
		x->acceptedTypes = getbytes( sizeof( t_symbol* ) * 1 );
		x->acceptedTypes[0] = atom_getsymbol( &argv[0] );

		/*
		symbolinlet_new(
			& x->x_obj,
			& x->acceptedTypes[0]
		);
		*/
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
	inlet_new(
		& x->x_obj,
		& x->x_obj.ob_pd,
		gensym( "list" ),
		gensym( "set" )
	);
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

void packFilter_on_set(
	t_packFilter* x,
	t_symbol *s,
	int argc,
	t_atom *argv
)
{
	freebytes( x->acceptedTypes, sizeof( t_symbol* ) * x->acceptedTypesCount );
	x->acceptedTypesCount = argc;
	x->acceptedTypes = getbytes( sizeof( t_symbol* ) * argc );
	for( unsigned int i=0; i<argc; i++)
	{
		x->acceptedTypes[i] = atom_getsymbol( &argv[i] );
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
	class_sethelpsymbol(
			class,
			gensym("sdPack")
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
	class_sethelpsymbol(
			class,
			gensym("sdPack")
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

//----------------------------------
// packMatch
//----------------------------------

DECL_LIST(AtomList, AtomEl, t_atom, getbytes, freebytes, freebytes)
DEF_LIST(AtomList, AtomEl, t_atom, getbytes, freebytes, freebytes)

DECL_BUFFER(AtomBuf, t_atom, getbytes, freebytes)
DEF_BUFFER(AtomBuf, t_atom, getbytes, freebytes)

typedef struct
{
	t_symbol* name;
	AtomBuf pattern;
} PatternInfo;

#define FREE_PATTTERN_INFO(x,size) \
{ \
	AtomBuf_exit( & x->pattern ); \
	freebytes( x, size ); \
}

DECL_LIST(PatternList, PatternEl, PatternInfo, getbytes, freebytes, FREE_PATTTERN_INFO)
DEF_LIST(PatternList, PatternEl, PatternInfo, getbytes, freebytes, FREE_PATTTERN_INFO)

/*
DECL_DYN_ARRAY(AtomDynArray, t_atom, getbytes, freebytes)
DEF_DYN_ARRAY(AtomDynArray, t_atom, getbytes, freebytes)

#define FREE_ATOM_BUF(x,size) \
{ \
	AtomDynArray_exit( x ); \
}

DECL_LIST(AtomBufList, AtomBufListEl, AtomDynArray, getbytes, freebytes, FREE_ATOM_BUF)
DEF_LIST(AtomBufList, AtomBufListEl, AtomDynArray, getbytes, freebytes, FREE_ATOM_BUF)
*/

typedef struct {
	int pattern_pos;
	int pattern_size;
} SubPatternInfo;

DECL_LIST(SubPatternsList, SubPatternsListEl, SubPatternInfo, getbytes, freebytes, freebytes)
DEF_LIST(SubPatternsList, SubPatternsListEl, SubPatternInfo, getbytes, freebytes, freebytes)

typedef struct s_packMatch {
  t_object x_obj;
	PatternList patterns;
	t_outlet* outlet;
	t_outlet* outlet_reject;
} t_packMatch;

void* packMatch_init(
	t_symbol *s,
	int argc,
	t_atom *argv
);
void packMatch_exit(
	struct s_packMatch* x
);

void packMatch_patterns_add(
	t_packMatch* x,
	t_symbol *s,
	int argc,
	t_atom *argv
);

void packMatch_patterns_clear(
	t_packMatch* x
);

void packMatch_input(
	t_packMatch* x,
	t_symbol *s,
	int argc,
	t_atom *argv
);

BOOL match(
		t_packMatch* x,
		t_symbol* pattern_name,
		t_atom* pattern,
		int pattern_size,
		int* pattern_pos,
		t_atom* argv,
		int size,
		int* pos
);

BOOL match_rec(
		t_packMatch* x,
		t_symbol* pattern_name,
		t_atom* pattern,
		int pattern_size,
		int* pattern_pos,
		t_atom* argv,
		int size,
		int* pos,
		int rec_depth
);

// DEBUG OUTPUT HELPERS:
void match_db_print(
		t_packMatch* x,
		t_symbol* pattern_name,
		t_atom* pattern,
		int pattern_size,
		int* pattern_pos,
		t_atom* argv,
		int size,
		int* pos,
		int rec_depth,
		char* msg
);
void match_db_print_start(
		t_packMatch* x,
		t_symbol* pattern_name,
		t_atom* pattern,
		int pattern_size,
		int* pattern_pos,
		t_atom* argv,
		int size,
		int* pos,
		int rec_depth
);
void match_error(
		t_packMatch* x,
		t_symbol* pattern_name,
		t_atom* pattern,
		int pattern_size,
		int* pattern_pos,
		t_atom* argv,
		int size,
		int* pos,
		int rec_depth,
		char* msg
);

t_class* register_packMatch(
	t_symbol* className
)
{
	t_class* class =
		class_new(
			className,
			(t_newmethod )packMatch_init, // constructor
			(t_method )packMatch_exit, // destructor
			sizeof(t_packMatch),
			CLASS_DEFAULT, // graphical repr ?
			// creation arguments:
			A_GIMME,
			0
		);
	class_sethelpsymbol(
			class,
			gensym("sdPackMatch")
	);

	class_addlist( class, packMatch_input );

	class_addmethod(
			class,
			(t_method )packMatch_patterns_add,
			gensym("add"),
			A_GIMME,
			0
	);

	class_addmethod(
			class,
			(t_method )packMatch_patterns_clear,
			gensym("clear"),
			A_GIMME,
			0
	);

	return class;
}

void* packMatch_init(
	t_symbol *s,
	int argc,
	t_atom *argv
)
{
  t_packMatch *x = (t_packMatch *)pd_new(packMatch_class);

	PatternList_init( & x-> patterns );

	if( argc > 0  )
	{
		pd_error(x, "too many arguments!");
		return NULL;
	}
	x->outlet =
		outlet_new( & x->x_obj, &s_list);
	x -> outlet_reject =
		outlet_new( & x->x_obj, &s_list);

  return (void *)x;
}

void packMatch_exit(
	t_packMatch* x
)
{
	PatternList_exit( & x-> patterns );
}

void packMatch_input(
	t_packMatch* x,
	t_symbol *s,
	int argc,
	t_atom *argv
)
{
	BOOL match_found = FALSE;
	LIST_FORALL_BEGIN(PatternList, PatternEl, PatternInfo, & x->patterns, i_pattern, patternEl)
		int pattern_pos = 0;
		int pos = 0;
		PatternInfo* patternInfo = patternEl->pData;
		if(
				match(
						x,
						patternInfo->name,
						AtomBuf_get_array( &patternInfo->pattern ),
						AtomBuf_get_size( &patternInfo->pattern ),
						&pattern_pos,
						argv,
						argc,
						&pos
				)
		)
		{
			AtomBuf ret;
			AtomBuf_init( & ret, argc + 2 );
			SETSYMBOL( & AtomBuf_get_array( & ret )[0], patternInfo->name );
			SETFLOAT( & AtomBuf_get_array( & ret )[1], argc );
			memcpy(
				& AtomBuf_get_array( & ret )[2],
				argv,
				argc * sizeof( t_atom )
			);
			match_found = TRUE;
			outlet_list(
				x->outlet,
				& s_list,
				argc + 2,
				AtomBuf_get_array( & ret )
			);
			AtomBuf_exit( & ret );
			//return;
		}
	LIST_FORALL_END(PatternList, PatternEl, PatternInfo, & x->patterns, i_pattern, patternEl)
	if( !match_found )
	outlet_bang(
			x->outlet_reject
	);
}

BOOL match(
		t_packMatch* x,
		t_symbol* pattern_name,
		t_atom* pattern,
		int pattern_size,
		int* pattern_pos,
		t_atom* argv,
		int size,
		int* pos
)
{
	BOOL ret = match_rec(
		x,
		pattern_name,
		pattern,
		pattern_size,
		pattern_pos,
		argv,
		size,
		pos,
		0
	);

	if(
			ret
			&& (*pattern_pos) < pattern_size
	)
	{
		char buf[256];
		t_atom pattern_name_atom;
		SETSYMBOL( & pattern_name_atom, pattern_name );
		atom_string( &pattern_name_atom, buf, 256 );
		DB_PRINT(
				"ERROR in pattern %s at pos %i: unmatched rest!",
				buf,
				(*pos)
		);
		return FALSE;
	}

	return ret;
}

BOOL match_rec(
		t_packMatch* x,
		t_symbol* pattern_name,
		t_atom* pattern,
		int pattern_size,
		int* pattern_pos,
		t_atom* argv,
		int size,
		int* pos,
		int rec_depth
)
{
	match_db_print_start(
			x,
			pattern_name,
			pattern, pattern_size, pattern_pos,
			argv, size, pos,
			rec_depth
	);

	// match NON strict:
	if(
			(*pattern_pos) < pattern_size
			&&
			atom_getsymbol( &pattern[*pattern_pos] ) == gensym("*")
	)
	{
		// eat '*'
		(*pattern_pos) ++;

		if( 
			(*pattern_pos) >= pattern_size
			||
			atom_getsymbol( &pattern[ (*pattern_pos) + 1] ) == gensym(")")
		)
		{
			// ARBITRARY CONTENT:
			while(
				*pos < size
			)
			{
				(*pos) ++;
			}
		}
		else
		{
			// make shure the input contains certain packages

			// 1. read all packages in the pattern:
			SubPatternsList expected_packages;
			SubPatternsList_init ( & expected_packages );
			while(
				(*pattern_pos) < pattern_size
				&&
				atom_getsymbol( &pattern[*pattern_pos] ) != gensym(")")
			)
			{
				SubPatternInfo* subPatternInfo = getbytes( sizeof( SubPatternInfo ) );
				SubPatternsList_append(
						& expected_packages,
						subPatternInfo
				);
				int depth = 0;

				// 'pack' '(' ...
				if(
					(*pattern_pos) + 1 < pattern_size
					&&
					pattern[ (*pattern_pos) + 0 ].a_type == A_SYMBOL
					&&
					atom_getsymbol( &pattern[ (*pattern_pos) + 1 ]) == gensym("(")
				)
				{
					subPatternInfo->pattern_pos =
							(*pattern_pos);
					depth = 1;

					(*pattern_pos) += 2;
					while(
						(*pattern_pos) < pattern_size
						&&
						depth > 0
					)
					{
						if( 
								atom_getsymbol( & pattern[ (*pattern_pos) + 0 ] ) == gensym( "(" )
						)
						{
							depth++;
						}
						else if( 
								atom_getsymbol( & pattern[ (*pattern_pos) + 0 ] ) == gensym( ")" )
						)
						{
							depth--;
						}
						(*pattern_pos) ++;
					}
					if( depth > 0 && *pattern_pos >= pattern_size )
					{
						char buf[256];
						t_atom pattern_name_atom;
						SETSYMBOL( & pattern_name_atom, pattern_name );
						atom_string( &pattern_name_atom, buf, 256 );
						match_error(
								x,
								pattern_name,
								pattern, pattern_size, pattern_pos,
								argv, size, pos,
								rec_depth,
								"invalid pack!"
						);
						SubPatternsList_exit( & expected_packages );
						return FALSE;
					}
				}
				else
				{
					char buf[256];
					t_atom pattern_name_atom;
					SETSYMBOL( & pattern_name_atom, pattern_name );
					atom_string( &pattern_name_atom, buf, 256 );
					match_error(
							x,
							pattern_name,
							pattern, pattern_size, pattern_pos,
							argv, size, pos,
							rec_depth,
							"invalid pack!"
					);
					SubPatternsList_exit( & expected_packages );
					return FALSE;
				}
				subPatternInfo->pattern_size =
					(*pattern_pos);
			}

			// 2. try to find all patterns in the input:
			{
				int matched = 0;
				while(
						*pos < size
				)
				{
					// read the input packet wise:
					if(
							*pos + 1 >= size
							||
							argv[ (*pos) + 0 ].a_type != A_SYMBOL
							||
							argv[ (*pos) + 1].a_type != A_FLOAT
							||
							(*pos) + 1 + atom_getint( & argv[ *pos ] ) >= size
					)
					{
						match_db_print(
								x,
								pattern_name,
								pattern, pattern_size, pattern_pos,
								argv, size, pos,
								rec_depth,
								"FAIL: input: invalid packet"
						);

						SubPatternsList_exit( & expected_packages );
						return FALSE;
					}
					int pack_size = 
							atom_getint( & argv[ *pos ] );
					int pos_temp = (*pos);
					// check if this is an expected packet:
					LIST_FORALL_BEGIN(SubPatternsList, SubPatternsListEl, SubPatternInfo, &expected_packages, i, pEl)
						SubPatternInfo* subPatternInfo = pEl->pData;
						int current_pat_pos = subPatternInfo->pattern_pos;
						pos_temp = (*pos);
						if(
								match_rec(
										x,
										pattern_name,
										pattern,
										subPatternInfo->pattern_size,
										&current_pat_pos,
										argv,
										pos_temp + 2 + pack_size,
										&pos_temp,
										rec_depth+1
								)
						)
						{
							matched ++;
						}
					LIST_FORALL_END(SubPatternsList, SubPatternsListEl, SubPatternInfo, &expected_packages, i, pEl)
					(*pos) += (2 + pack_size);
				}
				if( matched != SubPatternsList_get_size( & expected_packages ) )
				{
					match_db_print(
							x,
							pattern_name,
							pattern, pattern_size, pattern_pos,
							argv, size, pos,
							rec_depth,
							"FAIL: packets missing in input"
					);

					SubPatternsList_exit( & expected_packages );
					return FALSE;
				}
			}
			SubPatternsList_exit( & expected_packages );
		}
	}

	// match strict:
	else
	{

		while(
			*pattern_pos < pattern_size
			&&
			atom_getsymbol( &pattern[*pattern_pos] ) != gensym(")")
			&&
			*pos < size
		)
		{

			// ARBITRARY CONTENT:
			while(
				*pattern_pos < pattern_size
				&&
				atom_getsymbol( &pattern[*pattern_pos] ) != gensym("(")
				&&
				atom_getsymbol( &pattern[*pattern_pos] ) != gensym(")")
				&&
				*pos < size
			)
			{
				if(
						atom_getsymbol( & pattern[*pattern_pos] ) != gensym( "?" )
						&&
						!compareAtoms( & argv[*pos], & pattern[*pattern_pos] ) 
				)
				{
					char buf[256];
					atom_string( & argv[*pos], buf, 256 );
					char buf_pat[256];
					atom_string( & pattern[*pattern_pos], buf_pat, 256 );
					match_db_print(
							x,
							pattern_name,
							pattern, pattern_size, pattern_pos,
							argv, size, pos,
							rec_depth,
							"FAIL: !="
					);
					(*pos) ++;
					(*pattern_pos) ++;
					return FALSE;
				}
				// dbg print
				{
					char buf[256];
					atom_string( & argv[*pos], buf, 256 );
					char buf_pat[256];
					atom_string( & pattern[*pattern_pos], buf_pat, 256 );
					match_db_print(
							x,
							pattern_name,
							pattern, pattern_size, pattern_pos,
							argv, size, pos,
							rec_depth,
							"=="
					);
				}
				(*pos) ++;
				(*pattern_pos) ++;
			}
			// '('
			if(
				*pattern_pos < pattern_size
				&&
				atom_getsymbol( &pattern[*pattern_pos] ) == gensym("(")
			)
			{
				if( argv[*pos].a_type != A_FLOAT )
				{
					match_db_print(
							x,
							pattern_name,
							pattern, pattern_size, pattern_pos,
							argv, size, pos,
							rec_depth,
							"FAIL"
					);

					return FALSE;
				}
				int content_size = 
					atom_getint( & argv[*pos] );

				// eat '(':
				(*pos) ++;
				(*pattern_pos) ++;

				if(
					! match_rec(
							x,
							pattern_name,
							pattern,
							pattern_size,
							pattern_pos,
							argv,
							(*pos) + content_size,
							pos,
							rec_depth+1
					)
				)
				{
					match_db_print(
							x,
							pattern_name,
							pattern, pattern_size, pattern_pos,
							argv, size, pos,
							rec_depth,
							"FAIL: (subpattern fail)"
					);

					return FALSE;
				}
				if( 
					*pattern_pos >= pattern_size
				)
				{
					char buf[256];
					t_atom pattern_name_atom;
					SETSYMBOL( & pattern_name_atom, pattern_name );
					atom_string( &pattern_name_atom, buf, 256 );
					match_db_print(
							x,
							pattern_name,
							pattern, pattern_size, pattern_pos,
							argv, size, pos,
							rec_depth,
							"FAIL: eof"
					);
					return FALSE;
				}
				else if( atom_getsymbol( &pattern[*pattern_pos] ) != gensym(")") )
				{
					char buf_pat[256];
					t_atom pattern_name_atom;
					SETSYMBOL( & pattern_name_atom, pattern_name );
					atom_string( &pattern_name_atom, buf_pat, 256 );
					match_db_print(
							x,
							pattern_name,
							pattern, pattern_size, pattern_pos,
							argv, size, pos,
							rec_depth,
							"FAIL: eof"
					);
					return FALSE;
				}

				// eat ')':
				(*pattern_pos) ++;
			}
		}

		// some input wasn't matched!:
		if( (*pos) < size )
		{
			match_db_print(
					x,
					pattern_name,
					pattern, pattern_size, pattern_pos,
					argv, size, pos,
					rec_depth,
					"FAIL: input rest"
			);
			return FALSE;
		}

		// no match for rest input
		if(
				(*pattern_pos) < pattern_size
				&&
				atom_getsymbol( &pattern[*pattern_pos] ) != gensym(")")
		)
		{
			match_db_print(
					x,
					pattern_name,
					pattern, pattern_size, pattern_pos,
					argv, size, pos,
					rec_depth,
					"FAIL: pattern rest"
			);
			return FALSE;
		}

	}

	match_db_print(
			x,
			pattern_name,
			pattern, pattern_size, pattern_pos,
			argv, size, pos,
			rec_depth,
			"SUCCESS"
	);

	return TRUE;

}

void packMatch_patterns_add(
	t_packMatch* x,
	t_symbol *s,
	int argc,
	t_atom *argv
)
{
	PatternInfo* info = getbytes( sizeof( PatternInfo ) );
	info->name = atom_getsymbol( &argv[0] );
	AtomBuf_init( & info->pattern, argc - 1);
	memcpy(
			AtomBuf_get_array( & info->pattern ),
			& argv[1],
			sizeof( t_atom ) * (argc-1)
	);
	PatternList_append(
			& x->patterns,
			info
	);
}

void packMatch_patterns_clear(
	t_packMatch* x
)
{
	PatternList_clear(
			& x->patterns
	);
}

void match_db_print(
		t_packMatch* x,
		t_symbol* pattern_name,
		t_atom* pattern,
		int pattern_size,
		int* pattern_pos,
		t_atom* argv,
		int size,
		int* pos,
		int rec_depth,
		char* msg
)
{
	char pattern_name_buf[1024];
	{
		t_atom pattern_name_atom;
		SETSYMBOL( & pattern_name_atom, pattern_name );
		atom_string( &pattern_name_atom, pattern_name_buf, 1024 );
	}
	char pattern_current_buf[1024];
	{
		sprintf( pattern_current_buf, "'");
		if( *pattern_pos < pattern_size )
		{
			t_atom* current = & pattern[ *pattern_pos ];
			char current_buf[1024];
			atom_string( current, current_buf, 1024 );
			strcat(
					pattern_current_buf,
					current_buf
			);
		}
		strcat(
				pattern_current_buf,
				"'"
		);
	}
	char input_current_buf[1024];
	{
		sprintf( input_current_buf, "'");
		if( *pos < size )
		{
			t_atom* current = & argv[ *pos ];
			char current_buf[1024];
			atom_string( current, current_buf, 1024 );
			strcat(
					input_current_buf,
					current_buf
			);
		}
		strcat(
				input_current_buf,
				"'"
		);
	}

	char indent_buf[256];
	sprintf( indent_buf, "");
	for(int i=0; i<rec_depth; i++)
	{
		strcat(indent_buf, "-");
	}

	DB_PRINT(
			"%spattern %s:(%i,%i: %s) input (%i,%i: %s): %s",
			indent_buf,
			pattern_name_buf,
			*pattern_pos, pattern_size,
			pattern_current_buf,
			*pos, size,
			input_current_buf,
			msg
	);
}

void match_db_print_start(
		t_packMatch* x,
		t_symbol* pattern_name,
		t_atom* pattern,
		int pattern_size,
		int* pattern_pos,
		t_atom* argv,
		int size,
		int* pos,
		int rec_depth
)
{
	char pattern_name_buf[1024];
	{
		t_atom pattern_name_atom;
		SETSYMBOL( & pattern_name_atom, pattern_name );
		atom_string( &pattern_name_atom, pattern_name_buf, 1024 );
	}
	char pattern_buf[1024];
	{
		sprintf( pattern_buf, "'");
		for(int i=(*pattern_pos); i< pattern_size; i++)
		{
			t_atom* current = & pattern[ i ];
			char current_buf[1024];
			atom_string( current, current_buf, 1024 );
			if( i != 0 )
				strcat(
						pattern_buf,
						" "
				);
			strcat(
					pattern_buf,
					current_buf
			);
		}
		strcat(
				pattern_buf,
				"'"
		);
	}
	char arg_buf[1024];
	{
		sprintf( arg_buf, "'");
		for(int i=(*pos); i< size; i++)
		{
			t_atom* current = & argv[ i ];
			char current_buf[1024];
			atom_string( current, current_buf, 1024 );
			if( i != 0 )
				strcat(
						arg_buf,
						" "
				);
			strcat(
					arg_buf,
					current_buf
			);
		}
		strcat(
				arg_buf,
				"'"
		);
	}
	char indent_buf[256];
	sprintf( indent_buf, "");
	for(int i=0; i<rec_depth; i++)
	{
		strcat(indent_buf, "-");
	}
	DB_PRINT(
			"%spattern %s:(%i,%i: %s) input (%i,%i: %s): start",
			indent_buf,
			pattern_name_buf,
			*pattern_pos,
			pattern_size,
			pattern_buf,
			*pos,
			size,
			arg_buf
	);
}

void match_error(
		t_packMatch* x,
		t_symbol* pattern_name,
		t_atom* pattern,
		int pattern_size,
		int* pattern_pos,
		t_atom* argv,
		int size,
		int* pos,
		int rec_depth,
		char* msg
)
{
	char pattern_name_buf[256];
	{
		t_atom pattern_name_atom;
		SETSYMBOL( & pattern_name_atom, pattern_name );
		atom_string( &pattern_name_atom, pattern_name_buf, 256 );
	}
	char indent_buf[256];
	sprintf( indent_buf, "");
	for(int i=0; i<rec_depth; i++)
	{
		strcat(indent_buf, "-");
	}
	pd_error( x,
			"%spattern %s: ERROR at pos %i: %s",
			indent_buf,
			pattern_name_buf,
			*pattern_pos,
			msg
	);
}
