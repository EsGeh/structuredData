#include "sdObjState.h"
#include "LinkedList.h"

#include "m_pd.h"

#include <string.h>

#define ACCUML_SIZE 256


static t_class* objState_class;
static t_class* property_class;
static t_class* propertySym_class;
static t_class* propertyList_class;

t_class* register_objState(
	t_symbol* className
);

t_class* register_property(
	t_symbol* className
);

t_class* register_propertySym(
	t_symbol* className
);

t_class* register_propertyList(
	t_symbol* className
);

void sdObjState_setup()
{
	objState_class = register_objState( gensym("sdObjState") );
	property_class = register_property( gensym("sdProperty") );
	propertySym_class = register_propertySym( gensym("sdPropertySym") );
	propertyList_class = register_propertyList( gensym("sdPropertyList") );
}

// SymList: a list of t_symbol

#pragma GCC diagnostic ignored "-Wunused-value"
DECL_LIST(SymList, SymEl, t_symbol,getbytes,freebytes,)
DEF_LIST(SymList, SymEl, t_symbol,getbytes,freebytes,);
#pragma GCC diagnostic pop


//----------------------------------
// objState
//----------------------------------

typedef struct s_objState {
  t_object x_obj;
	t_symbol* objName;
	t_symbol* globalIn;
	SymList* outList;
	int accumlPos;
	t_atom* accumlArray;
	t_inlet* fromObjIn_in;
	t_inlet* fromProperties_in;
	t_outlet* toProperties_out;
} t_objState;

void* objState_init(
	t_symbol *s,
	int argc,
	t_atom *argv
);
void objState_exit(
	struct s_objState* x
);

void objState_input(
	t_objState* x,
	t_symbol *s,
	int argc,
	t_atom *argv
);

void objState_fromProps(
	t_objState* x,
	t_symbol *s,
	int argc,
	t_atom *argv
);

// helper:
void objState_flush(
	t_objState* x
);

t_class* register_objState(
	t_symbol* className
)
{
	t_class* class =
		class_new(
			className,
			(t_newmethod )objState_init, // constructor
			(t_method )objState_exit, // destructor
			sizeof(t_objState),
			CLASS_DEFAULT, // graphical repr ?
			// creation arguments:
			A_GIMME,
			0
		);

	class_addlist( class, objState_input );
	class_addmethod(
		class,
		(t_method )objState_fromProps,
		gensym("fromProps"),
		A_GIMME,
		0
	);

	return class;
}

void* objState_init(
	t_symbol *s,
	int argc,
	t_atom *argv
)
{
  t_objState *x = (t_objState *)pd_new(objState_class);

	if( argc < 1 || argc > 2 )
	{
		pd_error( x, "to few parameters. syntax: objName [globalIn]" );
		return NULL;
	}
	else if( argv[0].a_type != A_SYMBOL || argv[1].a_type != A_SYMBOL )
	{
		pd_error( x, "symbol expected. syntax: objName [globalIn]" );
		return NULL;
	}

	if( atom_getsymbol( &argv[0] ) == gensym("") )
	{
		pd_error( x, "invalid objName. syntax: objName [globalIn]");
		return NULL;
	}
	else
	{
		x->objName = atom_getsymbol( &argv[0] );
	}

	if( argc != 2 )
	{
		x->globalIn = gensym("GLOBAL");
	}
	else
	{
		x->globalIn = atom_getsymbol( &argv[1] );
	}

	x->outList = getbytes( sizeof( SymList ) );
	SymListInit( x->outList );

	x->accumlPos = -1;
	x->accumlArray = getbytes( sizeof( t_atom ) * ACCUML_SIZE);
	SETSYMBOL( & x->accumlArray[0], x->objName );

	// receive:
	pd_bind(
		& x->x_obj.ob_pd,
		x->objName
	);
	// receive:
	pd_bind(
		& x->x_obj.ob_pd,
		x->globalIn
	);

	x->fromObjIn_in =
		x->x_obj.ob_inlet;

	x->fromProperties_in =
		inlet_new(
			& x->x_obj,
			& x->x_obj.ob_pd,
			gensym("list"),
			gensym("fromProps")
		);

	x->toProperties_out =
		outlet_new( & x->x_obj, &s_list);

  return (void *)x;
}

void objState_exit(
	t_objState* x
)
{
	// unbind:
	pd_unbind(
		& x->x_obj.ob_pd,
		x->objName
	);
	// unbind:
	pd_unbind(
		& x->x_obj.ob_pd,
		x->globalIn
	);
	freebytes( x->accumlArray, sizeof( t_atom ) * ACCUML_SIZE);
	SymListExit( x->outList );
	freebytes( x->outList, sizeof( SymList ) );
}

int cmp_symbol_ptrs(t_symbol* p1, t_symbol* p2)
{
	return p1 == p2;
}

void objState_input(
	t_objState* x,
	t_symbol *s,
	int argc,
	t_atom *argv
)
{
	if(
		(argc < 2)
		|| (argv[0].a_type != A_SYMBOL)
		|| (argv[1].a_type != A_FLOAT)
		|| ((atom_getint( &argv[1] ) + 2) != argc)
	)
	{
		//post("argc: %i, argv[1]: %i", argc, atom_getint( & argv[1] ));
		pd_error(x, "invalid sdPack");
		return;
	}

	// out ( add <dest> )
	// out ( del <dest> )
	// out ( clear )
	if(
		atom_getint( &argv[1] ) >= 1
		&& atom_getsymbol( & argv[0] ) == gensym("out")
	)
	{
		// out ( add <dest> )
		if( atom_getsymbol( & argv[2] ) == gensym("add") )
		{
			if( atom_getint( & argv[1] ) != 2 )
			{
				pd_error(x, "unknown syntax! expected: out ( add <dest> )");
				return;
			}
			t_symbol* pDest = atom_getsymbol( & argv[3] );
			if( ! SymListGetElement( x->outList, pDest, cmp_symbol_ptrs ) )
			{
				char buf[256];
				atom_string( & argv[3], buf, 255 );
				//post( "adding dest: %s", buf );
				SymListAdd( x->outList, pDest );
			}
		}
		// out ( del <dest> )
		else if( atom_getsymbol( & argv[2] ) == gensym("del") )
		{
			if( atom_getint( & argv[1] ) != 2 )
			{
				pd_error(x, "unknown syntax! expected: out ( del <dest> )");
				return;
			}
			t_symbol* pDest = atom_getsymbol( & argv[3] );
			SymEl *pDestEl = SymListGetElement( x->outList, pDest, cmp_symbol_ptrs );
			if( pDestEl )
			{
				SymListDel( x->outList, pDestEl );
			}
			else
			{
				/*
				char buf[256];
				atom_string( & argv[3], buf, 255 );
				post("out ( del <dest> ... ): no element named %s", buf);
				*/
			}
		}
		// out ( clear )
		else if( atom_getsymbol( & argv[2] ) == gensym("clear") )
		{
			if( atom_getint( & argv[1] ) != 1 )
			{
				pd_error(x, "unknown syntax! expected: out ( clear )");
				return;
			}
			SymListClear( x->outList );
		}
		else
		{
			pd_error(x, "unknown syntax!");
			return;
		}
	}
	// get ( <property> )
	// get ( <property> out ( dest1 ... ) )
	// get ( out ( dest1 ... ) )
	else if(
		atom_getint( &argv[1] ) >= 1
		&& atom_getsymbol( & argv[0] ) == gensym("get")
	)
	{
		// get ( <property> )
		if(
			atom_getint( &argv[1] ) == 1
			// && atom_getsymbol( &argv[2] ) != gensym("out")
		)
		{
			outlet_list(
				x->toProperties_out,
				&s_list,
				argc,
				argv
			);
		}
		// get ( <property> out ( dest1 ... ) )
		else if(
			atom_getint( &argv[1] ) >= 3
			&& atom_getsymbol( &argv[2] ) != gensym("out")
			&& atom_getsymbol( &argv[3] ) == gensym("out")
			&& argv[4].a_type == A_FLOAT
		)
		{
			// (temporarily) change outList:
			SymList* old_outList = x->outList;
			x->outList = getbytes( sizeof( SymList ) );
			SymListInit( x->outList );
			for(unsigned int i=0; i< atom_getint( & argv[4] ); i++)
			{
				SymListAdd( x->outList, atom_getsymbol( & argv[5+i] ) );
			}
			// query properties:
			t_atom output[3];
			SETSYMBOL( &output[0], gensym("get") );
			SETFLOAT( &output[1], 1 );
			output[2] = argv[2];
			outlet_list(
				x->toProperties_out,
				&s_list,
				3,
				output
			);
			// change back outList:
			SymListExit( x->outList );
			freebytes( x->outList, sizeof( SymList ) );
			x->outList = old_outList;
		}
		// get ( out ( dest1 ... ) )
		else if(
			atom_getint( &argv[1] ) >= 2
			&& atom_getsymbol( &argv[2] ) == gensym("out")
			&& argv[3].a_type == A_FLOAT
		)
		{
			// set "accuml":
			//post("set accuml");
			x->accumlPos = 2;
			// (temporarily) change outList:
			SymList* old_outList = x->outList;
			x->outList = getbytes( sizeof( SymList ) );
			SymListInit( x->outList );
			for(unsigned int i=0; i< atom_getint( & argv[3] ); i++)
			{
				SymListAdd( x->outList, atom_getsymbol( & argv[4+i] ) );
			}

			// send "get ( )":
			t_atom output[2];
			SETSYMBOL( &output[0], gensym("get") );
			SETFLOAT( &output[1], 0 );
			outlet_list(
				x->toProperties_out,
				&s_list,
				2,
				output
			);
			// append "pseudo property" <out> ( ... ):
			int out_size = SymListGetSize( old_outList );
			SETSYMBOL( & x->accumlArray[ x->accumlPos + 0 ], gensym("out") );
			SETFLOAT( & x->accumlArray[ x->accumlPos + 1 ], out_size);
			LIST_FORALL_BEGIN(SymList,SymEl,t_symbol,old_outList,i,pEl)
				SETSYMBOL( & x->accumlArray[ x->accumlPos + 2 + i ], pEl->pData );
			LIST_FORALL_END(SymList,SymEl,t_symbol,old_outList,i,pEl)
			x->accumlPos += ( 2 + out_size);

			// change back outList:
			objState_flush(x);
			SymListExit( x->outList );
			freebytes( x->outList, sizeof( SymList ) );
			x->outList = old_outList;
			// unset "accuml":
			//post("unset accuml");
			x->accumlPos = -1;
		}
	}
	// set ( <property> ( val1 [...] ) ... )      (...: for list properties)
	//   (you can set several properties at once...)
	else if(
		atom_getsymbol( & argv[0] ) == gensym("set")
	)
	{
		int currentPos = 2;
		while( currentPos < argc )
		{
			int currentSize = atom_getint( &argv[currentPos + 1] );
			if( currentPos + currentSize >= argc )
			{
				pd_error( x, "invalid pack at position %i: not enough parameters", currentPos );
				return;
			}
			// send: set ( <property> <val1> [...] )
			t_atom* toSend = getbytes( sizeof( t_atom ) * (currentSize+2+1) );
			SETSYMBOL( &toSend[0], gensym("set") );
			SETFLOAT( &toSend[1], currentSize+1 );
			toSend[2] = argv[currentPos] ;
			for( int i=0; i < currentSize; i++ )
			{
				toSend[3+i] = argv[currentPos+2+i];
			}
			outlet_list(
				x->toProperties_out,
				&s_list,
				currentSize+2+1,
				toSend
			);
			freebytes( toSend, sizeof( t_atom ) * (currentSize+2+1) );
			currentPos = currentPos + 2 + currentSize ;
		}
	}
	// init ( )
	else if(
		atom_getsymbol( & argv[0] ) == gensym("init")
		&& atom_getint( & argv[1] ) == 0
	)
	{
		outlet_list(
			x->toProperties_out,
			&s_list,
			argc,
			argv
		);
	}
	else
	{
		// redirect any other message:
		outlet_list(
			x->toProperties_out,
			&s_list,
			argc,
			argv
		);
		//pd_error(x, "unknown syntax!");
		return;
	}
}

void objState_fromProps(
	t_objState* x,
	t_symbol *s,
	int argc,
	t_atom *argv
)
{
	if( x->accumlPos < 0)
	{
		//post("fromProps");
		LIST_FORALL_BEGIN(SymList,SymEl,t_symbol,x->outList,i,pEl)
			if( pEl->pData->s_thing )
			{
				//post("sending to %s", pEl->pData->s_name);
				typedmess( pEl->pData->s_thing, &s_list, argc, argv );
			}
		LIST_FORALL_END(SymList,SymEl,t_symbol,x->outList,i,pEl)
	}
	else
	{
		for( unsigned int i=0; i< argc-2; i++ )
		{
			x-> accumlArray[ x->accumlPos + i] = argv[ 2+i ];
		}
		x->accumlPos += argc-2;
	}
}

void objState_flush(
	t_objState* x
)
{
	//post("fromProps");
	SETFLOAT( & x->accumlArray[1], x->accumlPos - 2 );
	LIST_FORALL_BEGIN(SymList,SymEl,t_symbol,x->outList,i,pEl)
		if( pEl->pData->s_thing )
		{
			//post("sending to %s", pEl->pData->s_name);
			typedmess( pEl->pData->s_thing, &s_list, x->accumlPos, x->accumlArray );
		}
	LIST_FORALL_END(SymList,SymEl,t_symbol,x->outList,i,pEl)
}

//----------------------------------
// property
//----------------------------------

typedef struct s_property {
  t_object x_obj;
	t_symbol* name;
	t_float value;
	t_symbol* rcv_sym;
	t_symbol* send_sym;
	t_symbol* init;
	t_inlet* fromObjIn_in;
	t_outlet* out;
	t_outlet* redirect_out;
} t_property;

void* property_init(
	t_symbol *s,
	int argc,
	t_atom *argv
);
void property_exit(
	struct s_property* x
);

void property_input(
	t_property* x,
	t_symbol *s,
	int argc,
	t_atom *argv
);

void property_set(
	t_property* x,
	t_float newVal
);

void property_set_noupdate(
	t_property* x,
	t_float newVal
);

// helper:
void property_output(
	t_property* x,
	t_symbol* package_name
);

t_class* register_property(
	t_symbol* className
)
{
	t_class* class =
		class_new(
			className,
			(t_newmethod )property_init, // constructor
			(t_method )property_exit, // destructor
			sizeof(t_property),
			CLASS_DEFAULT, // graphical repr ?
			// creation arguments:
			A_GIMME,
			0
		);

	// "sdPack" interface:
	class_addlist( class, property_input );

	// "internal" messages
	class_addmethod(
		class,
		(t_method )property_set,
		gensym("set"),
		A_FLOAT,
		0
	);
	class_addmethod(
		class,
		(t_method )property_set_noupdate,
		gensym("set_noupdate"),
		A_FLOAT,
		0
	);

	return class;
}

void* property_init(
	t_symbol *s,
	int argc,
	t_atom *argv
)
{
  t_property *x = (t_property *)pd_new(property_class);

	if( argc < 1 || argc > 4 )
	{
		pd_error( x, "wrong number of parameters. syntax: objName [$0 default init?]" );
		return NULL;
	}

	char rcv_str[256];
	if( argc >= 1 )
	{
		x->name = atom_getsymbol( &argv[0] );
	}
	if( argc >= 2 )
	{
		char buf[256];
		atom_string( &argv[1], buf, 255);
		//post(" temp: %s", buf );
		strcpy(
			rcv_str,
			buf
		);
		strcat(
			rcv_str,
			"-"
		);
		strcat(
			rcv_str,
			x->name->s_name
		);
		x->send_sym =
			gensym( rcv_str );

		strcat(
			rcv_str,
			"_rcv"
		);
		//post( "rcv: %s", rcv_str );
		x-> rcv_sym =
			gensym( rcv_str );

		// receive:
		pd_bind(
			& x->x_obj.ob_pd,
			x-> rcv_sym
		);
	}
	else
	{
		x->send_sym =
			NULL;
		x->rcv_sym =
			NULL;
		/*
		strcat(
			rcv_str,
			x->name->s_name
		);
		strcat(
			rcv_str,
			"_rcv"
		);
		*/
	}
	if( argc >= 3 )
	{
		x->value = atom_getfloat( &argv[2] );
	}
	else
	{
		x->value = 0;
	}

	if(
		argc >= 4
		&& argv[3].a_type == A_SYMBOL
	)
	{
		if(
			atom_getsymbol( & argv[3] ) == gensym("intern")
			|| atom_getsymbol( & argv[3] ) == gensym("info")
			|| atom_getsymbol( & argv[3] ) == gensym("update")
		)
		{
			x->init = atom_getsymbol( &argv[3] );
		}
		else
		{
			pd_error( x, "unexpected value for argument 4 ['init?]'. Possible values: intern, info, update " );
			return NULL;
		}
	}
	else if(
		argc >= 4
		&& argv[3].a_type == A_FLOAT
	)
	{
		pd_error( x, "unexpected value for argument 4 ['init?']. Possible values: intern, info, update " );
		return NULL;
	}
	else
	{
		x->init = 0;
	}

	x->fromObjIn_in =
		x->x_obj.ob_inlet;

	x->out =
		outlet_new( & x->x_obj, &s_list);
	x->redirect_out =
		outlet_new( & x->x_obj, &s_list);

  return (void *)x;
}

void property_exit(
	t_property* x
)
{
	if( x->rcv_sym )
	{
		// unbind:
		pd_unbind(
			& x->x_obj.ob_pd,
			x->rcv_sym
		);
	}
}

void property_input(
	t_property* x,
	t_symbol *s,
	int argc,
	t_atom *argv
)
{
	//post("sdProperty: input");
	if(
		(argc < 2)
		|| (argv[0].a_type != A_SYMBOL)
		|| (argv[1].a_type != A_FLOAT)
		|| ((atom_getint( &argv[1] ) + 2) != argc)
	)
	{
		//post("argc: %i, argv[1]: %i", argc, atom_getint( & argv[1] ));
		pd_error(x, "invalid sdPack");
		return;
	}

	if(
		// get ( <property> )
		atom_getsymbol( & argv[0] ) == gensym( "get" )
		&& atom_getint( &argv[1] ) == 1
		&& atom_getsymbol( &argv[2] ) == x->name

	)
	{
		// output "info ( <property> ( <val> ) )"
		property_output(
			x,
			gensym("info")
		);
	}
	else if(
		// get ( )
		atom_getsymbol( & argv[0] ) == gensym( "get" )
		&& atom_getint( &argv[1] ) == 0
	)
	{
		// output "info ( <property> ( <val> ) )"
		property_output(
			x,
			gensym("info")
		);
		outlet_list(
			x->redirect_out,
			&s_list,
			argc,
			argv
		);
	}
	else if(
		// set ( <property> <val> )
		atom_getsymbol( & argv[0] ) == gensym( "set" )
		&& atom_getint( &argv[1] ) == 2
		&& atom_getsymbol( &argv[2] ) == x->name
	)
	{
		property_set(
			x,
			atom_getfloat( & argv[3] )
		);
	}

	else if(
		// init ( )
		atom_getsymbol( & argv[0] ) == gensym( "init" )
		&& atom_getint( &argv[1] ) == 0
	)
	{
		if( x->init == gensym( "intern" ) )
		{
			t_atom val;
			SETFLOAT( & val, x->value );
			if( x->send_sym && x->send_sym->s_thing )
			{
				typedmess(
					x->send_sym->s_thing,
					&s_float,
					1,
					&val
				);
			}
		}
		else if( x->init == gensym( "info" ) )
		{
			// output "info ( <property> ( <val> ) )"
			property_output(
				x,
				gensym("info")
			);
		}
		else if( x->init == gensym( "update" ) )
		{
			// output "update ( <property> ( <val> ) )"
			property_output(
				x,
				gensym("update")
			);
		}
		outlet_list(
			x->redirect_out,
			&s_list,
			argc,
			argv
		);
	}

	else
	{
		outlet_list(
			x->redirect_out,
			&s_list,
			argc,
			argv
		);
	}
}

void property_set(
	t_property* x,
	t_float newVal
)
{
	//post("sdProperty: set");
	x->value = newVal;

	t_atom val;
	SETFLOAT( & val, newVal );
	if( x->send_sym && x->send_sym->s_thing )
	{
		typedmess(
			x->send_sym->s_thing,
			&s_float,
			1,
			&val
		);
	}
	// output "update ( <property> ( <val> ) )"
	property_output(
		x,
		gensym("update")
	);
}

void property_set_noupdate(
	t_property* x,
	t_float newVal
)
{
	//post("sdProperty: set_noupdate");
	x->value = newVal;
	// output "update ( <property> ( <val> ) )"
	property_output(
		x,
		gensym("update")
	);
}

void property_output(
	t_property* x,
	t_symbol* package_name
)
{
	// output "<package_name> ( <propertySym> ( <val> ) )"
	t_atom outArray[5];
	SETSYMBOL( & outArray[0], package_name );
	SETFLOAT( & outArray[1], 3 );
	SETSYMBOL( & outArray[2], x->name );
	SETFLOAT( & outArray[3], 1 );
	SETFLOAT( & outArray[4], x->value );
	outlet_list(
		x->out,
		&s_list,
		5,
		outArray
	);
}

//----------------------------------
// propertySym
//----------------------------------

typedef struct s_propertySym {
  t_object x_obj;
	t_symbol* name;
	t_symbol* value;
	t_symbol* rcv_sym;
	t_symbol* send_sym;
	t_symbol* init;
	t_inlet* fromObjIn_in;
	t_outlet* out;
	t_outlet* redirect_out;
} t_propertySym;

void* propertySym_init(
	t_symbol *s,
	int argc,
	t_atom *argv
);
void propertySym_exit(
	struct s_propertySym* x
);

void propertySym_input(
	t_propertySym* x,
	t_symbol *s,
	int argc,
	t_atom *argv
);

void propertySym_set(
	t_propertySym* x,
	t_symbol* newVal
);

void propertySym_set_noupdate(
	t_propertySym* x,
	t_symbol* newVal
);

// helper:
void propertySym_output(
	t_propertySym* x,
	t_symbol* package_name
);

t_class* register_propertySym(
	t_symbol* className
)
{
	t_class* class =
		class_new(
			className,
			(t_newmethod )propertySym_init, // constructor
			(t_method )propertySym_exit, // destructor
			sizeof(t_propertySym),
			CLASS_DEFAULT, // graphical repr ?
			// creation arguments:
			A_GIMME,
			0
		);

	// "sdPack" interface
	class_addlist( class, propertySym_input );

	// internal interface
	class_addmethod(
		class,
		(t_method )propertySym_set,
		gensym("set"),
		A_SYMBOL,
		0
	);
	class_addmethod(
		class,
		(t_method )propertySym_set_noupdate,
		gensym("set_noupdate"),
		A_SYMBOL,
		0
	);

	return class;
}

void* propertySym_init(
	t_symbol *s,
	int argc,
	t_atom *argv
)
{
  t_propertySym *x = (t_propertySym *)pd_new(propertySym_class);

	if( argc < 1 || argc > 4 )
	{
		pd_error( x, "wrong number of parameters. syntax: objName [$0 default init?]" );
		return NULL;
	}

	char rcv_str[256];
	if( argc >= 1 )
	{
		x->name = atom_getsymbol( &argv[0] );
	}
	if( argc >= 2 )
	{
		char buf[256];
		atom_string( &argv[1], buf, 255);
		//post(" temp: %s", buf );
		strcpy(
			rcv_str,
			buf
		);
		strcat(
			rcv_str,
			"-"
		);
		strcat(
			rcv_str,
			x->name->s_name
		);
		x->send_sym =
			gensym( rcv_str );

		strcat(
			rcv_str,
			"_rcv"
		);
		//post( "rcv: %s", rcv_str );
		x-> rcv_sym =
			gensym( rcv_str );

		// receive:
		pd_bind(
			& x->x_obj.ob_pd,
			x-> rcv_sym
		);
	}
	else
	{
		x->send_sym =
			NULL;
		x-> rcv_sym =
			NULL;
		/*
		strcat(
			rcv_str,
			x->name->s_name
		);
		strcat(
			rcv_str,
			"_rcv"
		);
		*/
	}
	if( argc >= 3 )
	{
		x->value = atom_getsymbol( &argv[2] );
	}
	else
	{
		x->value = gensym("");
	}

	if(
		argc >= 4
		&& argv[3].a_type == A_SYMBOL
	)
	{
		if(
			atom_getsymbol( & argv[3] ) == gensym("intern")
			|| atom_getsymbol( & argv[3] ) == gensym("info")
			|| atom_getsymbol( & argv[3] ) == gensym("update")
		)
		{
			x->init = atom_getsymbol( &argv[3] );
		}
		else
		{
			pd_error( x, "unexpected value for argument 4 'init?'. Possible values: intern, info, update " );
			return NULL;
		}
	}
	else if(
		argc >= 4
		&& argv[3].a_type == A_FLOAT
	)
	{
		pd_error( x, "unexpected value for argument 4 ['init?']. Possible values: intern, info, update " );
		return NULL;
	}
	else
	{
		x->init = 0;
	}

	x->fromObjIn_in =
		x->x_obj.ob_inlet;

	x->out =
		outlet_new( & x->x_obj, &s_list);
	x->redirect_out =
		outlet_new( & x->x_obj, &s_list);

  return (void *)x;
}

void propertySym_exit(
	t_propertySym* x
)
{
	if( x->rcv_sym )
	{
		// unbind:
		pd_unbind(
			& x->x_obj.ob_pd,
			x->rcv_sym
		);
	}
}

void propertySym_input(
	t_propertySym* x,
	t_symbol *s,
	int argc,
	t_atom *argv
)
{
	//post("sdPropertySym: input");
	if(
		(argc < 2)
		|| (argv[0].a_type != A_SYMBOL)
		|| (argv[1].a_type != A_FLOAT)
		|| ((atom_getint( &argv[1] ) + 2) != argc)
	)
	{
		//post("argc: %i, argv[1]: %i", argc, atom_getint( & argv[1] ));
		pd_error(x, "invalid sdPack");
		return;
	}

	if(
		// get ( <property> )
		atom_getsymbol( & argv[0] ) == gensym( "get" )
		&& atom_getint( &argv[1] ) == 1
		&& atom_getsymbol( &argv[2] ) == x->name

	)
	{
		// output "info ( <propertySym> ( <val> ) )"
		propertySym_output(
			x,
			gensym("info")
		);
	}
	else if(
		// get ( )
		atom_getsymbol( & argv[0] ) == gensym( "get" )
		&& atom_getint( &argv[1] ) == 0
	)
	{
		// output "info ( <property> ( <val> ) )"
		propertySym_output(
			x,
			gensym("info")
		);
		outlet_list(
			x->redirect_out,
			&s_list,
			argc,
			argv
		);
	}
	else if(
		// set ( <propertySym> <val> )
		atom_getsymbol( & argv[0] ) == gensym( "set" )
		&& atom_getint( &argv[1] ) == 2
		&& atom_getsymbol( &argv[2] ) == x->name
	)
	{
		propertySym_set(
			x,
			atom_getsymbol( & argv[3] )
		);
	}

	else if(
		// init ( )
		atom_getsymbol( & argv[0] ) == gensym( "init" )
		&& atom_getint( &argv[1] ) == 0
	)
	{
		if( x->init == gensym( "intern" ) )
		{
			t_atom val;
			SETSYMBOL( & val, x->value );
			if( x->send_sym && x->send_sym->s_thing )
			{
				typedmess(
					x->send_sym->s_thing,
					&s_symbol,
					1,
					&val
				);
			}
		}
		else if( x->init == gensym( "info" ) )
		{
			// output "info ( <property> ( <val> ) )"
			propertySym_output(
				x,
				gensym("info")
			);
		}
		else if( x->init == gensym( "update" ) )
		{
			// output "update ( <property> ( <val> ) )"
			propertySym_output(
				x,
				gensym("update")
			);
		}
		outlet_list(
			x->redirect_out,
			&s_list,
			argc,
			argv
		);
	}

	else
	{
		outlet_list(
			x->redirect_out,
			&s_list,
			argc,
			argv
		);
	}
}

void propertySym_set(
	t_propertySym* x,
	t_symbol* newVal
)
{
	//post("sdPropertySym: set");
	x->value = newVal;

	t_atom val;
	SETSYMBOL( & val, newVal );
	if( x->send_sym && x->send_sym->s_thing )
	{
		typedmess(
			x->send_sym->s_thing,
			&s_symbol,
			1,
			&val
		);
	}
	// output "update ( <property> ( <val> ) )"
	propertySym_output(
		x,
		gensym("update")
	);
}

void propertySym_set_noupdate(
	t_propertySym* x,
	t_symbol* newVal
)
{
	//post("sdPropertySym: set_noupdate");
	x->value = newVal;
	propertySym_output(
		x,
		gensym("update")
	);
}

void propertySym_output(
	t_propertySym* x,
	t_symbol* package_name
)
{
	// output "<package_name> ( <propertySym> ( <val> ) )"
	t_atom outArray[5];
	SETSYMBOL( & outArray[0], package_name );
	SETFLOAT( & outArray[1], 3 );
	SETSYMBOL( & outArray[2], x->name );
	SETFLOAT( & outArray[3], 1 );
	SETSYMBOL( & outArray[4], x->value );
	outlet_list(
		x->out,
		&s_list,
		5,
		outArray
	);
}

//----------------------------------
// propertyList
//----------------------------------

#pragma GCC diagnostic ignored "-Wunused-value"
DECL_LIST(AtomList, AtomEl, t_atom,getbytes,freebytes,)
DEF_LIST(AtomList, AtomEl, t_atom,getbytes,freebytes,)
#pragma GCC diagnostic pop

typedef struct s_propertyList {
  t_object x_obj;
	t_symbol* name;
	//t_symbol* dollarNull;
	AtomList value;
	t_symbol* rcv_sym;
	t_symbol* send_sym;
	t_inlet* fromObjIn_in;
	t_outlet* out;
	t_outlet* redirect_out;
} t_propertyList;

void* propertyList_init(
	t_symbol *s,
	int argc,
	t_atom *argv
);
void propertyList_exit(
	struct s_propertyList* x
);

void propertyList_input(
	t_propertyList* x,
	t_symbol *s,
	int argc,
	t_atom *argv
);

void propertyList_set(
	t_propertyList* x,
	t_symbol *s,
	int argc,
	t_atom *argv
);

void propertyList_set_noupdate(
	t_propertyList* x,
	t_symbol *s,
	int argc,
	t_atom *argv
);

void propertyList_output(
	t_propertyList* x,
	t_symbol* package_name
);

t_class* register_propertyList(
	t_symbol* className
)
{
	t_class* class =
		class_new(
			className,
			(t_newmethod )propertyList_init, // constructor
			(t_method )propertyList_exit, // destructor
			sizeof(t_propertyList),
			CLASS_DEFAULT, // graphical repr ?
			// creation arguments:
			A_GIMME,
			0
		);

	class_addlist( class, propertyList_input );
	class_addmethod(
		class,
		(t_method )propertyList_set,
		gensym("set"),
		A_GIMME,
		0
	);
	class_addmethod(
		class,
		(t_method )propertyList_set_noupdate,
		gensym("set_noupdate"),
		A_GIMME,
		0
	);

	return class;
}

void* propertyList_init(
	t_symbol *s,
	int argc,
	t_atom *argv
)
{
  t_propertyList *x = (t_propertyList *)pd_new(propertyList_class);

	if( argc < 1 || argc > 2 )
	{
		pd_error( x, "wrong number of parameters. syntax: objName [$0]" );
		return NULL;
	}

	char rcv_str[256];
	if( argc >= 1 )
	{
		x->name = atom_getsymbol( &argv[0] );
	}
	if( argc >= 2 )
	{
		char buf[256];
		atom_string( &argv[1], buf, 255);
		//post(" temp: %s", buf );
		strcpy(
			rcv_str,
			buf
		);
		strcat(
			rcv_str,
			"-"
		);
		strcat(
			rcv_str,
			x->name->s_name
		);
		x->send_sym =
			gensym( rcv_str );

		strcat(
			rcv_str,
			"_rcv"
		);
		//post( "rcv: %s", rcv_str );
		x-> rcv_sym =
			gensym( rcv_str );

		// receive:
		pd_bind(
			& x->x_obj.ob_pd,
			x-> rcv_sym
		);
	}
	else
	{
		x->send_sym =
			NULL;
		x-> rcv_sym =
			NULL;
	}
	AtomListInit( & x->value );

	x->fromObjIn_in =
		x->x_obj.ob_inlet;

	x->out =
		outlet_new( & x->x_obj, &s_list);
	x->redirect_out =
		outlet_new( & x->x_obj, &s_list);

  return (void *)x;
}

void propertyList_exit(
	t_propertyList* x
)
{
	if( x->rcv_sym )
	{
		// unbind:
		pd_unbind(
			& x->x_obj.ob_pd,
			x->rcv_sym
		);
	}
	AtomListExit( & x->value );
}

void propertyList_input(
	t_propertyList* x,
	t_symbol *s,
	int argc,
	t_atom *argv
)
{
	//post("sdPropertyList: input");
	if(
		(argc < 2)
		|| (argv[0].a_type != A_SYMBOL)
		|| (argv[1].a_type != A_FLOAT)
		|| ((atom_getint( &argv[1] ) + 2) != argc)
	)
	{
		//post("argc: %i, argv[1]: %i", argc, atom_getint( & argv[1] ));
		pd_error(x, "invalid sdPack");
		return;
	}

	if(
		// get ( <property> )
		(
		atom_getsymbol( & argv[0] ) == gensym( "get" )
		&& atom_getint( &argv[1] ) == 1
		&& atom_getsymbol( &argv[2] ) == x->name
		)
	||
		// get ( )
		(
		atom_getsymbol( & argv[0] ) == gensym( "get" )
		&& atom_getint( &argv[1] ) == 0
		)
	)
	{
		// output "info ( <property> ( <val> ... ) )"
		propertyList_output(
			x,
			gensym( "info" )
		);
		// get ( ):
		if(
			atom_getsymbol( & argv[0] ) == gensym( "get" )
			&& atom_getint( &argv[1] ) == 0
		)
		{
			outlet_list(
				x->redirect_out,
				&s_list,
				argc,
				argv
			);
		}
	}
	else if(
		// set ( <property> <val1> <val2> )
		atom_getsymbol( & argv[0] ) == gensym( "set" )
		&& atom_getsymbol( &argv[2] ) == x->name
	)
	{
		propertyList_set(
			x,
			NULL,
			atom_getint( &argv[1] ) - 1,
			& argv[3]
		);
	}
	else
	{
		outlet_list(
			x->redirect_out,
			&s_list,
			argc,
			argv
		);
	}
}

void propertyList_set(
	t_propertyList* x,
	t_symbol *s,
	int argc,
	t_atom *argv
)
{
	//post("sdPropertyList: set");
	int new_value_count = argc;
	t_atom* val;
	AtomListClear( & x->value );
	for(int i=0; i < new_value_count; i++)
	{
		val = &argv[i];
		AtomListAdd( & x->value, val );
	}

	if( x->send_sym && x->send_sym->s_thing )
	{
		typedmess(
			x->send_sym->s_thing,
			&s_list,
			new_value_count,
			argv
		);
	}
	// output "update ( <propertyList> ( <val> ... ) )"
	propertyList_output(
		x,
		gensym( "update" )
	);
}

void propertyList_set_noupdate(
	t_propertyList* x,
	t_symbol *s,
	int argc,
	t_atom *argv
)
{
	//post("sdPropertyList: set_noupdate");
	int new_value_count = argc;
	t_atom* val;
	AtomListClear( & x->value );
	for(int i=0; i < new_value_count; i++)
	{
		val = &argv[i];
		AtomListAdd( & x->value, val );
	}

	// output "update ( <propertyList> ( <val> ... ) )"
	propertyList_output(
		x,
		gensym( "update" )
	);
}

void propertyList_output(
	t_propertyList* x,
	t_symbol* package_name
)
{
	// output "<package_name> ( <property> ( <val> ... ) )"
	int value_count = AtomListGetSize( &x->value );
	t_atom* outArray = getbytes( sizeof( t_atom ) * ( 2 + 2 + value_count ) );
	SETSYMBOL( & outArray[0], package_name );
	SETFLOAT( & outArray[1], 2 + value_count );
	SETSYMBOL( & outArray[2], x->name );
	SETFLOAT( & outArray[3], value_count );
	LIST_FORALL_BEGIN(AtomList,AtomEl,t_atom,&x->value,i,pEl)
		outArray[4+i] = *(pEl->pData);
	LIST_FORALL_END(AtomList,AtomEl,t_atom,&x->value,i,pEl)
	outlet_list(
		x->out,
		&s_list,
		2 + 2 + value_count,
		outArray
	);
	freebytes( outArray, sizeof( t_atom ) * ( 2 + 2 + value_count ) );
}
