#include "sdList.h"
#include "LinkedList.h"
#include "Global.h"

#include "m_pd.h"


static t_class* list_class;
static t_class* listEqual_class;

t_class* register_list(
	t_symbol* className
);

t_class* register_listEqual(
	t_symbol* className
);

void sdList_setup() {
	list_class = register_list( gensym("sdList") );
	listEqual_class = register_listEqual( gensym("sdListEqual") );
}

//----------------------------------
// list
//----------------------------------


typedef struct s_list {
  t_object x_obj;
	AtomList atoms;
	// specific outlets for every operation:
	unsigned int outlet_count;
	t_symbol** outletDescriptions;
	t_outlet** outlets;
} t_list;

void* list_init(
	t_symbol *s,
	int argc,
	t_atom *argv
);
void list_exit(
	t_list* x
);

// manipulate list:
void list_set(
	t_list *x,
	t_symbol *s,
	int argc,
	t_atom *argv
);
void list_append(
	t_list *x,
	t_symbol *s,
	int argc,
	t_atom *argv
);
void list_prepend(
	t_list *x,
	t_symbol *s,
	int argc,
	t_atom *argv
);
void list_setAdd(
	t_list *x,
	t_symbol *s,
	int argc,
	t_atom *argv
);
void list_del(
	t_list *x,
	t_float index
);
void list_clear(
	t_list *x
);

// output list:
void list_bang(t_list *x);
void list_pop(
	t_list *x
);
void list_popRev(
	t_list *x
);
void list_serialize(
	t_list *x
);
void list_serializeRev(
	t_list *x
);
void list_get(
	t_list *x,
	t_float index
);
void list_count(
	t_list *x
);
void list_find(
	t_list *x,
	t_symbol *s,
	int argc,
	t_atom *argv
);
void list_findAll(
	t_list *x,
	t_symbol *s,
	int argc,
	t_atom *argv
);
void list_equal(
	t_list *x,
	t_symbol *s,
	int argc,
	t_atom *argv
);

// helper:
void list_outputAt(
	t_list* x,
	t_symbol* s, // output operation
	// what to output
	int argc,
	t_atom *argv
);

t_class* register_list(
	t_symbol* className
)
{
	t_class* class =
		class_new(
			className,
			(t_newmethod )list_init, // constructor
			(t_method )list_exit, // destructor
			sizeof(t_list),
			CLASS_DEFAULT, // graphical repr ?
			// creation arguments:
			A_GIMME,
			0
		);
	class_sethelpsymbol(
			class,
			gensym("sdList")
	);

	// manipulate list:
	class_addmethod(
		class,
		(t_method )list_set,
		gensym("set"),
		A_GIMME,
		0
	);
	class_addmethod(
		class,
		(t_method )list_append,
		gensym("append"),
		A_GIMME,
		0
	);
	class_addmethod(
		class,
		(t_method )list_prepend,
		gensym("prepend"),
		A_GIMME,
		0
	);
	class_addmethod(
		class,
		(t_method )list_setAdd,
		gensym("setAdd"),
		A_GIMME,
		0
	);
	class_addmethod(
		class,
		(t_method )list_del,
		gensym("del"),
		A_FLOAT,
		0
	);
	class_addmethod(
		class,
		(t_method )list_clear,
		gensym("clear"),
		0
	);
	// shortcut:
	class_addlist(class, list_append);

	// output list:
	class_addbang(class, list_bang);
	class_addmethod(
		class,
		(t_method )list_pop,
		gensym("pop"),
		0
	);
	class_addmethod(
		class,
		(t_method )list_popRev,
		gensym("popRev"),
		0
	);
	class_addmethod(
		class,
		(t_method )list_serialize,
		gensym("serialize"),
		0
	);
	class_addmethod(
		class,
		(t_method )list_serializeRev,
		gensym("serializeRev"),
		0
	);
	class_addmethod(
		class,
		(t_method )list_get,
		gensym("get"),
		A_FLOAT,
		0
	);
	class_addmethod(
		class,
		(t_method )list_count,
		gensym("count"),
		0
	);
	class_addmethod(
		class,
		(t_method )list_find,
		gensym("find"),
		A_GIMME,
		0
	);
	class_addmethod(
		class,
		(t_method )list_findAll,
		gensym("findAll"),
		A_GIMME,
		0
	);
	class_addmethod(
		class,
		(t_method )list_equal,
		gensym("equal"),
		A_GIMME,
		0
	);

	return class;
}

void* list_init(
	t_symbol* UNUSED(s),
	int _argc,
	t_atom *argv
)
{
	unsigned int argc = _argc;
  t_list *x = (t_list *)pd_new(list_class);


	AtomList_init(& x->atoms );

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
		for(unsigned int i=0; i<argc; i++)
		{
			t_symbol* sym = atom_getsymbol( & argv[i] );
			if(
				sym == gensym("all")
				|| sym == gensym("bang")
				|| sym == gensym("pop")
				|| sym == gensym("popRev")
				|| sym == gensym("get")
				|| sym == gensym("count")
				|| sym == gensym("find")
				|| sym == gensym("findAll")
				|| sym == gensym("serialize")
				|| sym == gensym("serializeRev")
				|| sym == gensym("equal")
			)
			{
  			x->outletDescriptions[i] = sym;
  			x->outlets[i] = outlet_new( & x->x_obj, &s_list);
			}
			else
			{
				pd_error(x,"syntax: arg1 arg2 ..., argi one of: all, bang, pop, popRev, serialize, serializeRev, get, count, equal");
				list_exit(x);
				return NULL;
			}
		}
	}

  return (void *)x;
}

void list_exit(
	t_list* x
)
{
	AtomList_exit( & x->atoms );
	freebytes( x->outletDescriptions, sizeof( t_symbol* ) * x->outlet_count );
	freebytes( x->outlets, sizeof( t_outlet* ) * x->outlet_count );
}

// manipulate list:

void list_set(
	t_list *x,
	t_symbol *s,
	int argc,
	t_atom *argv
)
{
	AtomList_clear( & x-> atoms );
	list_append( x, s, argc, argv );
}

void list_append(
	t_list* x,
	t_symbol* UNUSED(s),
	int _argc,
	t_atom *argv
)
{
	unsigned int argc = _argc;
	for( unsigned int i=0; i<argc; i++ )
	{
		t_atom* p = getbytes( sizeof( t_atom ) );
		*p = argv[i];
		AtomList_append( & x->atoms, p);
	}
}

void list_prepend(
	t_list *x,
	t_symbol* UNUSED(s),
	int _argc,
	t_atom *argv
)
{
	unsigned int argc = _argc;
	for( unsigned int i=0; i<argc; i++ )
	{
		t_atom* p = getbytes( sizeof( t_atom ) );
		*p = argv[i];
		AtomList_prepend( & x->atoms, p);
	}
}

void list_setAdd(
	t_list *x,
	t_symbol* UNUSED(s),
	int argc,
	t_atom *argv
)
{
	for( unsigned int i=0; i<(unsigned int )argc; i++ )
	{
		if(
			! AtomList_get_element( & x->atoms, &argv[i], compareAtoms )
		)
		{
			t_atom* p = getbytes( sizeof( t_atom ) );
			*p = argv[i];
			AtomList_append( & x->atoms, p);
		}
	}
}

void list_del(
	t_list *x,
	t_float index
)
{
	unsigned int count = AtomList_get_size( & x->atoms );
	if( index >= 0 )
	{
		unsigned int index_int = (unsigned int )index;
		AtomListEl* pEl = AtomList_get_first( & x->atoms );
		for( unsigned int i=0; i<count; i++ )
		{
			if( i == index_int )
			{
				AtomList_del( & x->atoms, pEl );
				return;
			}
			pEl = AtomList_get_next( & x->atoms, pEl );
		}
	}
	else
	{
		int index_int = count + (int )index;
		AtomListEl* pEl = AtomList_get_last( & x->atoms );
		for( int i=count-1; i>=0; i-- )
		{
			if( i == index_int )
			{
				AtomList_del( & x->atoms, pEl );
				return;
			}
			pEl = AtomList_get_prev( & x->atoms, pEl );
		}
	}
}

void list_clear(
	t_list *x
)
{
	AtomList_clear( & x-> atoms );
}

// output list:
void list_bang(t_list *x)
{
	t_atom* atoms_array;
	unsigned int count = AtomList_get_size( & x->atoms );
	atoms_array =
		getbytes( sizeof( t_atom) * count);
	AtomListEl* pEl = AtomList_get_first( & x->atoms );
	for( unsigned int i=0; i<count; i++ )
	{
		atoms_array[i] = * (pEl->pData) ;
		pEl = AtomList_get_next( & x->atoms, pEl );
	}
	list_outputAt(
		x,
		gensym("bang"),
		count,
		atoms_array
	);
	/*
	outlet_list(
		x-> x_obj.ob_outlet,
		& s_list,
		count,
		atoms_array
	);
	*/
	freebytes( atoms_array, sizeof( t_atom ) * count );
}


void list_pop(
	t_list* x
)
{
	if( !AtomList_is_empty( & x -> atoms ) )
	{
		AtomListEl* el = AtomList_get_first( & x -> atoms );
		list_outputAt(
			x,
			gensym("pop"),
			1,
			el->pData
		);
		AtomList_del( & x -> atoms, el );
	}
}

void list_popRev(
	t_list* x
)
{
	if( !AtomList_is_empty( & x -> atoms ) )
	{
		AtomListEl* el = AtomList_get_last( & x -> atoms );
		list_outputAt(
			x,
			gensym("popRev"),
			1,
			el->pData
		);
		AtomList_del( & x -> atoms, el );
	}
}

void list_serialize(
	t_list *x
)
{
	unsigned int count = AtomList_get_size( & x->atoms );
	AtomListEl* pEl = AtomList_get_first( & x->atoms );
	for( unsigned int i=0; i<count; i++ )
	{
		list_outputAt(
			x,
			gensym("serialize"),
			1,
			pEl->pData
		);
		pEl = AtomList_get_next( & x->atoms, pEl );
	}
}

void list_serializeRev(
	t_list *x
)
{
	unsigned int count = AtomList_get_size( & x->atoms );
	AtomListEl* pEl = AtomList_get_last( & x->atoms );
	for( unsigned int i=0; i<count; i++ )
	{
		list_outputAt(
			x,
			gensym("serializeRev"),
			1,
			pEl->pData
		);
		pEl = AtomList_get_prev( & x->atoms, pEl );
	}
}

void list_get(
	t_list *x,
	t_float index
)
{
	unsigned int count = AtomList_get_size( & x->atoms );
	if( index >= 0 )
	{
		unsigned int index_int = (unsigned int )index;
		AtomListEl* pEl = AtomList_get_first( & x->atoms );
		for( unsigned int i=0; i<count; i++ )
		{
			if( i == index_int )
			{
				list_outputAt(
					x,
					gensym("get"),
					1,
					pEl->pData
				);
				return;
			}
			pEl = AtomList_get_next( & x->atoms, pEl );
		}
	}
	else
	{
		int index_int = count + (int )index;
		AtomListEl* pEl = AtomList_get_last( & x->atoms );
		for( int i=count-1; i>=0; i-- )
		{
			if( i == index_int )
			{
				list_outputAt(
					x,
					gensym("get"),
					1,
					pEl->pData
				);
				return;
			}
			pEl = AtomList_get_prev( & x->atoms, pEl );
		}
	}
}

void list_count(
	t_list *x
)
{
	t_atom ret;
	SETFLOAT( & ret, AtomList_get_size( & x->atoms ) );
	list_outputAt(
		x,
		gensym("count"),
		1,
		&ret
	);
	return;
}

void list_find(
	t_list *x,
	t_symbol* UNUSED(s),
	int argc,
	t_atom *argv
)
{
	if( argc == 0 )
	{
		pd_error( x, "find called with no arguments" );
		return;
	}

	t_atom ret;
	AtomListEl* pElStart = AtomList_get_first( & x->atoms );
	for( int i=0; i<(int )AtomList_get_size( & x->atoms ) - argc + 1; i++ )
	{
		AtomListEl* pEl = pElStart;
		int isSame = 1;
		for( int j=0; j<argc; j++ )
		{
			if( !compareAtoms( pEl->pData, & argv[j] ) )
			{
				isSame = 0;
				break;
			}
			pEl = AtomList_get_next( & x->atoms, pEl );
		}
		if( isSame )
		{
			SETFLOAT( &ret, i);
			list_outputAt(
				x,
				gensym("find"),
				1,
				&ret
			);
			return;
		}
		pElStart = AtomList_get_next( & x->atoms, pElStart );
	}

	SETFLOAT( &ret, -1);
	list_outputAt(
		x,
		gensym("find"),
		1,
		&ret
	);
}

void list_findAll(
	t_list *x,
	t_symbol* UNUSED(s),
	int argc,
	t_atom *argv
)
{
	if( argc == 0 )
	{
		pd_error( x, "findAll called with no arguments" );
		return;
	}

	t_atom* ret;
	int ret_size = 0;
	ret = getbytes( sizeof( t_atom ) * AtomList_get_size( & x->atoms ) );


	AtomListEl* pElStart = AtomList_get_first( & x->atoms );
	for( int i=0; i<(int )AtomList_get_size( & x->atoms ) - argc + 1; i++ )
	{
		AtomListEl* pEl = pElStart;
		int isSame = 1;
		for( int j=0; j<argc; j++ )
		{
			if( !compareAtoms( pEl->pData, & argv[j] ) )
			{
				isSame = 0;
				break;
			}
			pEl = AtomList_get_next( & x->atoms, pEl );
		}
		if( isSame )
		{
			SETFLOAT( & ret[ret_size], i );
			ret_size++;
		}
		pElStart = AtomList_get_next( & x->atoms, pElStart );
	}

	list_outputAt(
		x,
		gensym("findAll"),
		ret_size,
		ret
	);

	freebytes( ret, sizeof( t_atom ) * AtomList_get_size( & x->atoms ) );
}

void list_equal(
	t_list *x,
	t_symbol* UNUSED(s),
	int argc,
	t_atom *argv
)
{
	t_atom ret;
	if (argc != (int )AtomList_get_size( & x->atoms ) )
	{
		SETFLOAT( &ret, 0);
		list_outputAt(
			x,
			gensym("equal"),
			1,
			&ret
		);
		return;
	}
	LIST_FORALL_BEGIN(AtomList,AtomListEl,t_atom,& x->atoms,i,pEl)
		SETFLOAT( &ret, 0);
		if( ! compareAtoms( pEl->pData, &argv[i] ) )
		{
			list_outputAt(
				x,
				gensym("equal"),
				1,
				&ret
			);
		}
	LIST_FORALL_END(AtomList,AtomListEl,t_atom,& x->atoms,i,pEl)

	SETFLOAT( &ret, 1);
	list_outputAt(
		x,
		gensym("equal"),
		1,
		&ret
	);
}

// helper:
void list_outputAt(
	t_list* x,
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

//----------------------------------
// listEqual
//----------------------------------

typedef struct s_listEqual {
  t_object x_obj;
	AtomList atoms;
	t_outlet* outlet;
} t_listEqual;

void* listEqual_init(
	t_symbol *s,
	int argc,
	t_atom *argv
);
void listEqual_exit(
	t_listEqual* x
);

void listEqual_set(
	t_listEqual *x,
	t_symbol *s,
	int argc,
	t_atom *argv
);

void listEqual_exec(
	t_listEqual *x,
	t_symbol *s,
	int argc,
	t_atom *argv
);

t_class* register_listEqual(
	t_symbol* className
)
{
	t_class* class =
		class_new(
			className,
			(t_newmethod )listEqual_init, // constructor
			(t_method )listEqual_exit, // destructor
			sizeof(t_listEqual),
			CLASS_DEFAULT, // graphical repr ?
			// creation arguments:
			A_GIMME,
			0
		);
	class_sethelpsymbol(
			class,
			gensym("sdList")
	);

	// manipulate list:
	class_addmethod(
		class,
		(t_method )listEqual_set,
		gensym("set"),
		A_GIMME,
		0
	);

	class_addlist(
		class,
		(t_method )listEqual_exec
	);

	return class;
}

void* listEqual_init(
	t_symbol *s,
	int argc,
	t_atom *argv
)
{
  t_listEqual *x = (t_listEqual *)pd_new(listEqual_class);


	AtomList_init(& x->atoms );

  inlet_new(
		& x->x_obj,
		& x->x_obj.ob_pd,
		gensym("list"),
		gensym("set")
	);
	x->outlet = outlet_new( & x->x_obj, &s_list);

	// set list from creation args:
	listEqual_set(
		x,
		s,
		argc,
		argv
	);

  return (void *)x;
}

void listEqual_exit(
	t_listEqual* x
)
{
	AtomList_exit( & x->atoms );
}

void listEqual_set(
	t_listEqual *x,
	t_symbol* UNUSED(s),
	int argc,
	t_atom *argv
)
{
	AtomList_clear( & x-> atoms );
	for( unsigned int i=0; i<(unsigned int )argc; i++ )
	{
		t_atom* p = getbytes( sizeof( t_atom ) );
		*p = argv[i];
		AtomList_append( & x->atoms, p);
	}
}

void listEqual_exec(
	t_listEqual *x,
	t_symbol* UNUSED(s),
	int argc,
	t_atom *argv
)
{
	if (argc != (int )AtomList_get_size( & x->atoms ) )
	{
		outlet_float(
			x->outlet,
			0
		);
		return;
	}
	LIST_FORALL_BEGIN(AtomList,AtomListEl,t_atom,& x->atoms,i,pEl)
		if( ! compareAtoms( pEl->pData, &argv[i] ) )
		{
			outlet_float(
				x->outlet,
				0
			);
			return;
		}
	LIST_FORALL_END(AtomList,AtomListEl,t_atom,& x->atoms,i,pEl)
	outlet_float(
		x->outlet,
		1
	);

}
