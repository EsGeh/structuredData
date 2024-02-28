#include "sdData.h"
#include "LinkedList.h"

#include "m_pd.h"

#ifdef __GNUC__
#  define UNUSED(x) UNUSED_ ## x __attribute__((__unused__))
#else
#  define UNUSED(x) UNUSED_ ## x
#endif


static t_class* data_class;

t_class* register_data(
	t_symbol* className
);

void sdData_setup() {
	data_class = register_data( gensym("sdData") );
}


// utils:

#define DECL_LIST_STD(LIST,ELEMENT,DATA)\
	DECL_LIST(LIST,ELEMENT,DATA,getbytes,freebytes,freebytes)
#define DEF_LIST_STD(LIST,ELEMENT,DATA)\
	DEF_LIST(LIST,ELEMENT,DATA,getbytes,freebytes,freebytes)

DECL_LIST_STD(AtomList, AtomEl, t_atom)
DEF_LIST_STD(AtomList, AtomEl, t_atom);

typedef struct SPack
{
	//unsigned int count;
	t_atom* atoms;
} Pack;


#define DELPACK(pPack,size) \
{\
	freebytes( pPack->atoms, sizeof( t_atom ) * atom_getint( & pPack->atoms[1] ) ); \
	freebytes( pPack, sizeof( Pack ) ); \
}\

DECL_LIST(PackList, PackEl, Pack,getbytes,freebytes,DELPACK)
DEF_LIST(PackList, PackEl, Pack,getbytes,freebytes,DELPACK);

//----------------------------------
// data
//----------------------------------

typedef struct s_data {
  t_object x_obj;
	PackList packs;
	// specific outlets for every operation:
	unsigned int outlet_count;
	t_symbol** outletDescriptions;
	t_outlet** outlets;
} t_data;

void* data_init(
	t_symbol *s,
	int argc,
	t_atom *argv
);
void data_exit(
	t_data* x
);

// manipulate packs:
void data_set(
	t_data *x,
	t_symbol *s,
	int argc,
	t_atom *argv
);
void data_append(
	t_data *x,
	t_symbol *s,
	int argc,
	t_atom *argv
);
void data_prepend(
	t_data *x,
	t_symbol *s,
	int argc,
	t_atom *argv
);
void data_clear(
	t_data *x
);

void data_filterPacksAccept(
	t_data *x,
	t_symbol *s,
	int argc,
	t_atom *argv
);
void data_filterPacksReject(
	t_data *x,
	t_symbol *s,
	int argc,
	t_atom *argv
);

// output packs:
void data_bang(t_data *x);

void data_pop(
	t_data *x
);
void data_popRev(
	t_data *x
);
void data_serialize(
	t_data *x
);
void data_serializeRev(
	t_data *x
);
void data_get(
	t_data *x,
	t_float index
);
void data_get_accept(
	t_data *x,
	t_symbol *s,
	int argc,
	t_atom *argv
);
void data_get_reject(
	t_data *x,
	t_symbol *s,
	int argc,
	t_atom *argv
);
void data_count(
	t_data *x
);

// helper:
void data_outputAt(
	t_data* x,
	t_symbol* s, // output operation
	// what to output
	int argc,
	t_atom *argv
);

t_class* register_data(
	t_symbol* className
)
{
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcast-function-type"
	t_class* class =
		class_new(
			className,
			(t_newmethod )data_init, // constructor
			(t_method )data_exit, // destructor
			sizeof(t_data),
			CLASS_DEFAULT, // graphical repr ?
			// creation arguments:
			A_GIMME,
			0
		);
#pragma GCC diagnostic pop
	class_sethelpsymbol(
			class,
			gensym("sdData")
	);

	class_addbang(class, data_bang);
	class_addlist(class, data_append);
	class_addmethod(
		class,
		(t_method )data_set,
		gensym("set"),
		A_GIMME,
		0
	);
	class_addmethod(
		class,
		(t_method )data_append,
		gensym("append"),
		A_GIMME,
		0
	);
	class_addmethod(
		class,
		(t_method )data_prepend,
		gensym("prepend"),
		A_GIMME,
		0
	);
	class_addmethod(
		class,
		(t_method )data_clear,
		gensym("clear"),
		0
	);
	class_addmethod(
		class,
		(t_method )data_filterPacksAccept,
		gensym("filterAccept"),
		A_GIMME,
		0
	);
	class_addmethod(
		class,
		(t_method )data_filterPacksReject,
		gensym("filterReject"),
		A_GIMME,
		0
	);

	class_addmethod(
		class,
		(t_method )data_pop,
		gensym("pop"),
		0
	);
	class_addmethod(
		class,
		(t_method )data_popRev,
		gensym("popRev"),
		0
	);
	class_addmethod(
		class,
		(t_method )data_serialize,
		gensym("serialize"),
		0
	);
	class_addmethod(
		class,
		(t_method )data_serializeRev,
		gensym("serializeRev"),
		0
	);
	class_addmethod(
		class,
		(t_method )data_get,
		gensym("get"),
		A_FLOAT,
		0
	);
	class_addmethod(
		class,
		(t_method )data_get_accept,
		gensym("getAccept"),
		A_GIMME,
		0
	);
	class_addmethod(
		class,
		(t_method )data_get_reject,
		gensym("getReject"),
		A_GIMME,
		0
	);
	class_addmethod(
		class,
		(t_method )data_count,
		gensym("count"),
		0
	);


	return class;
}

void data_outputAt(
	t_data* x,
	t_symbol* s, // output operation
	// what to output
	int argc,
	t_atom *argv
)
{
	for(unsigned int i=0; i<x->outlet_count; i++)
	{
		t_symbol* currentSym = x->outletDescriptions[i];
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

void* data_init(
	t_symbol* UNUSED(s),
	int argc,
	t_atom *argv
)
{
  t_data *x = (t_data *)pd_new(data_class);


	PackList_init(& x->packs );

  inlet_new(
		& x->x_obj,
		& x->x_obj.ob_pd,
		gensym("list"),
		gensym("append")
	);
	if( argc == 0 )
	{
		x -> outlet_count = 1;
		x->outletDescriptions = getbytes( sizeof( t_symbol* ) * 1 );
		x->outlets = getbytes( sizeof( t_outlet* ) * 1 );
		x->outletDescriptions[0] = gensym("all");
  	x->outlets[0] = outlet_new( & x->x_obj, &s_list);
	}
	else
	{
		x -> outlet_count = argc;
		x->outletDescriptions = getbytes( sizeof( t_symbol* ) * argc );
		x->outlets = getbytes( sizeof( t_outlet* ) * argc );
		for(unsigned int i=0; i<(unsigned int )argc; i++)
		{
			t_symbol* sym = atom_getsymbol( & argv[i] );
			if(
				sym == gensym("all")
				|| sym == gensym("bang")
				|| sym == gensym("pop")
				|| sym == gensym("popRev")
				|| sym == gensym("serialize")
				|| sym == gensym("serializeRev")
				|| sym == gensym("getAccept")
				|| sym == gensym("getReject")
				|| sym == gensym("get")
				|| sym == gensym("count")
			)
			{
  			x->outletDescriptions[i] = sym;
  			x->outlets[i] = outlet_new( & x->x_obj, &s_list);
			}
			else
			{
				pd_error(x,"syntax: arg1 arg2 ..., argi one of: all, bang, pop, popRev, serialize, serializeRev, get, getAccept, getReject, count");
				data_exit(x);
				return NULL;
			}
		}
	}

  return (void *)x;
}

void data_exit(
	t_data* x
)
{
	PackList_exit( & x->packs );
	freebytes( x->outletDescriptions, sizeof( t_symbol* ) * x->outlet_count );
	freebytes( x->outlets, sizeof( t_outlet* ) * x->outlet_count );
}


////////////////////////////////////////////////////////
// manipulate packs:
////////////////////////////////////////////////////////

void data_set(
	t_data *x,
	t_symbol *s,
	int argc,
	t_atom *argv
)
{
	data_clear( x );
	data_append( x, s, argc, argv );
}

void data_append(
	t_data* x,
	t_symbol* UNUSED(s),
	int argc,
	t_atom *argv
)
{
	unsigned int pos = 0;
	while( pos < (unsigned int )argc )
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
		Pack* newPack = getbytes( sizeof( Pack ) );
		unsigned int count = atom_getint( &argv[pos+1] );
		newPack->atoms = getbytes( sizeof( t_atom ) * (count+2) );
		for(unsigned int i=0; i< count+2; i++)
		{
			newPack->atoms[i] = argv[pos+i];
			/*
			char buf[256];
			atom_string(& argv[pos+i], buf, 255);
			post("%i: %s", pos+i, buf );
			*/
		}
		PackList_append(& x->packs, newPack );
		pos += (count + 2) ;
	}
}

void data_prepend(
	t_data* x,
	t_symbol* UNUSED(s),
	int argc,
	t_atom *argv
)
{
	unsigned int pos = 0;
	while( pos < (unsigned int )argc )
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
		Pack* newPack = getbytes( sizeof( Pack ) );
		unsigned int count = atom_getint( &argv[pos+1] );
		newPack->atoms = getbytes( sizeof( t_atom ) * (count+2) );
		for(unsigned int i=0; i< count+2; i++)
		{
			newPack->atoms[i] = argv[pos+i];
			/*
			char buf[256];
			atom_string(& argv[pos+i], buf, 255);
			post("%i: %s", pos+i, buf );
			*/
		}
		PackList_prepend(& x->packs, newPack );
		pos += (count + 2) ;
	}
}

void data_clear(
	t_data *x
)
{
	PackList_clear( & x->packs );
}

void data_filterPacksAccept(
	t_data *x,
	t_symbol* UNUSED(s),
	int argc,
	t_atom *argv
)
{
	if( ! PackList_is_empty( & x->packs ) )
	{
		PackEl* pEl = PackList_get_first( & x->packs );
		do
		{
			t_atom* packName = & (pEl->pData->atoms[0]);
			/*
			char buf[256];
			atom_string( packName, buf, 255 );
			post( "pack: %s", buf );
			*/
			int accept = 0;
			for( int iAcc = 0; iAcc< argc; iAcc++ )
			{
				if( atom_getsymbol( packName ) == atom_getsymbol( &argv[iAcc] ) )
				{
					accept = 1;
					break;
				}
			}
			PackEl* next = PackList_get_next( & x->packs, pEl );
			if( !accept )
			{
				PackList_del( & x->packs, pEl );
				//post( "rejected!" );
			}
			pEl = next;
		}
		while( pEl != NULL );
	}
}

void data_filterPacksReject(
	t_data *x,
	t_symbol* UNUSED(s),
	int argc,
	t_atom *argv
)
{
	if( ! PackList_is_empty( & x->packs ) )
	{
		PackEl* pEl = PackList_get_first( & x->packs );
		do
		{
			t_atom* packName = & (pEl->pData->atoms[0]);
			/*
			char buf[256];
			atom_string( packName, buf, 255 );
			post( "pack: %s", buf );
			*/
			int accept = 1;
			for( int iAcc = 0; iAcc< argc; iAcc++ )
			{
				if( atom_getsymbol( packName ) == atom_getsymbol( &argv[iAcc] ) )
				{
					accept = 0;
					break;
				}
			}
			PackEl* next = PackList_get_next( & x->packs, pEl );
			if( !accept )
			{
				PackList_del( & x->packs, pEl );
				//post( "rejected!" );
			}
			pEl = next;
		}
		while( pEl != NULL );
	}
}


////////////////////////////////////////////////////////
// output packs:
////////////////////////////////////////////////////////

void data_bang(t_data *x)
{
	unsigned int atomsCount = 0;
	LIST_FORALL_BEGIN(PackList,PackEl,Pack, & x->packs, i, pEl)
		atomsCount +=
			(atom_getint( & pEl->pData->atoms[1] ) + 2);
	LIST_FORALL_END(PackList,PackEl,Pack, & x->packs, i, pEl)
	//post("atomsCount: %i", atomsCount);

	t_atom* atoms_array = getbytes( sizeof( t_atom ) * atomsCount );
	unsigned int insertPos = 0;
	LIST_FORALL_BEGIN(PackList,PackEl,Pack, & x->packs, i, pEl)
		unsigned int countAtomsInPack = atom_getint( & pEl->pData->atoms[1] ) + 2;
		for(unsigned int iAtom=0; iAtom<countAtomsInPack; iAtom++)
		{
			atoms_array[insertPos] = (pEl->pData->atoms)[iAtom];
			insertPos ++;
		}
	LIST_FORALL_END(PackList,PackEl,Pack, & x->packs, i, pEl)
	data_outputAt(
		x,
		gensym("bang"),
		atomsCount,
		atoms_array
	);
	freebytes( atoms_array, sizeof( t_atom ) * atomsCount );
}

void data_pop(
	t_data *x
)
{
	if( !PackList_is_empty( & x -> packs ) )
	{
		PackEl* el = PackList_get_first( & x -> packs );
		unsigned int count = atom_getint( &el->pData->atoms[1] ) + 2;
		data_outputAt(
			x,
			gensym("pop"),
			count,
			el->pData->atoms
		);
		PackList_del( & x -> packs, el );
	}
}

void data_popRev(
	t_data *x
)
{
	if( !PackList_is_empty( & x -> packs ) )
	{
		PackEl* el = PackList_get_last( & x -> packs );
		unsigned int count = atom_getint( &el->pData->atoms[1] ) + 2;
		data_outputAt(
			x,
			gensym("popRev"),
			count,
			el->pData->atoms
		);
		PackList_del( & x -> packs, el );
	}
}

void data_serialize(
	t_data *x
)
{
	LIST_FORALL_BEGIN(PackList,PackEl,Pack, & x->packs, i, pEl)
		unsigned int countAtomsInPack = atom_getint( & pEl->pData->atoms[1] ) + 2;
		data_outputAt(
			x,
			gensym("serialize"),
			countAtomsInPack,
			pEl->pData->atoms
		);
	LIST_FORALL_END(PackList,PackEl,Pack, & x->packs, i, pEl)
}

void data_serializeRev(
	t_data *x
)
{
	LIST_FORALL_REV_BEGIN(PackList,PackEl,Pack, & x->packs, i, pEl)
		unsigned int countAtomsInPack = atom_getint( & pEl->pData->atoms[1] ) + 2;
		data_outputAt(
			x,
			gensym("serializeRev"),
			countAtomsInPack,
			pEl->pData->atoms
		);
	LIST_FORALL_REV_END(PackList,PackEl,Pack, & x->packs, i, pEl)
}

void data_get(
	t_data *x,
	t_float index
)
{
	LIST_FORALL_BEGIN(PackList,PackEl,Pack, & x->packs, i, pEl)
		unsigned int countAtomsInPack = atom_getint( & pEl->pData->atoms[1] ) + 2;
		if(
				(index >= 0 && i == index)
				||
				(index < 0 && i == (PackList_get_size( & x->packs ) + (t_int )index))
		)
		{
			data_outputAt(
				x,
				gensym("get"),
				countAtomsInPack,
				pEl->pData->atoms
			);
			return;
		}
	LIST_FORALL_END(PackList,PackEl,Pack, & x->packs, i, pEl)
}

void data_get_accept(
	t_data *x,
	t_symbol* UNUSED(s),
	int argc,
	t_atom *argv
)
{
	if( ! PackList_is_empty( & x->packs ) )
	{
		PackEl* pEl = PackList_get_first( & x->packs );
		do
		{
			t_atom* packName = & (pEl->pData->atoms[0]);
			/*
			char buf[256];
			atom_string( packName, buf, 255 );
			post( "pack: %s", buf );
			*/
			int accept = 0;
			for( int iAcc = 0; iAcc< argc; iAcc++ )
			{
				if( atom_getsymbol( packName ) == atom_getsymbol( &argv[iAcc] ) )
				{
					accept = 1;
					break;
				}
			}
			PackEl* next = PackList_get_next( & x->packs, pEl );
			if( accept )
			{
				unsigned int countAtomsInPack = atom_getint( & pEl->pData->atoms[1] ) + 2;
				data_outputAt(
					x,
					gensym("getAccept"),
					countAtomsInPack,
					pEl->pData->atoms
				);
			}
			pEl = next;
		}
		while( pEl != NULL );
	}
}

void data_get_reject(
	t_data *x,
	t_symbol* UNUSED(s),
	int argc,
	t_atom *argv
)
{
	if( ! PackList_is_empty( & x->packs ) )
	{
		PackEl* pEl = PackList_get_first( & x->packs );
		do
		{
			t_atom* packName = & (pEl->pData->atoms[0]);
			/*
			char buf[256];
			atom_string( packName, buf, 255 );
			post( "pack: %s", buf );
			*/
			int accept = 0;
			for( int iAcc = 0; iAcc< argc; iAcc++ )
			{
				if( atom_getsymbol( packName ) == atom_getsymbol( &argv[iAcc] ) )
				{
					accept = 1;
					break;
				}
			}
			PackEl* next = PackList_get_next( & x->packs, pEl );
			if( !accept )
			{
				unsigned int countAtomsInPack = atom_getint( & pEl->pData->atoms[1] ) + 2;
				data_outputAt(
					x,
					gensym("getReject"),
					countAtomsInPack,
					pEl->pData->atoms
				);
			}
			pEl = next;
		}
		while( pEl != NULL );
	}
}

void data_count(
	t_data *x
)
{
	t_atom ret;
	SETFLOAT( &ret, PackList_get_size( &x->packs ) );
	data_outputAt(
		x,
		gensym("count"),
		1,
		&ret
	);
}
