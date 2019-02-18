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

/* symbols are handled by pd
 * they should never to be released manually:
 */
#pragma GCC diagnostic ignored "-Wunused-value"
DECL_LIST(SymList, SymEl, t_symbol,getbytes,freebytes,)
DEF_LIST(SymList, SymEl, t_symbol,getbytes,freebytes,);
#pragma GCC diagnostic pop


//----------------------------------
// objState
//----------------------------------

typedef enum e_last_method { LAST_METHOD_SET, LAST_METHOD_GET } t_last_method;

typedef struct s_objState {
  t_object x_obj;
	t_symbol* objName;
	t_symbol* globalIn;
	SymList* outList;
	t_last_method last_method;
	int accumlPos;
	t_atom* accumlArray;
	t_inlet* fromObjIn_in;
	t_inlet* fromProperties_in;
	t_outlet* events_to_obj;
	t_outlet* toProperties_out;
	t_outlet* obj_out;
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

void objState_rawinput(
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

	// handle events
	class_addlist( class, objState_input );

	// handle "raw" messages:
	class_addmethod(
		class,
		(t_method )objState_rawinput,
		gensym("raw"),
		A_GIMME,
		0
	);

	// from properties:
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

	// receive GLOBAL:
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
			&s_list,
			gensym("fromProps")
		);

	x->events_to_obj =
		outlet_new( & x->x_obj, &s_list);
	x->toProperties_out =
		outlet_new( & x->x_obj, &s_list);
	x->obj_out =
		outlet_new( & x->x_obj, &s_list);

  return (void *)x;
}

void objState_exit(
	t_objState* x
)
{

	pd_unbind(
		& x->x_obj.ob_pd,
		x->objName
	);

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
		atom_getsymbol( & argv[0] ) == gensym("get")
	)
	{
		x->last_method = LAST_METHOD_GET;

		// get ( <property> )
		if(
			atom_getint( &argv[1] ) == 1
			// && atom_getsymbol( &argv[2] ) != gensym("out")
		)
		{
			// send to obj: "get <property>"
			outlet_anything(
				x->toProperties_out,
				gensym("get"),
				1,
				& argv[2]
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
			// send to obj: "get <property>"
			// query properties:
			outlet_anything(
				x->toProperties_out,
				gensym("get"),
				1,
				& argv[2]
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

			// send to obj: "get ( )":
			outlet_anything(
				x->toProperties_out,
				gensym("get"),
				0,
				NULL
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
		else
		{
			pd_error(x, "unknown syntax in event starting with 'get'!");
			return;
		}
	}

	// set ( <property> ( val1 [...] ) ... )      (...: for list properties)
	//   (you can set several properties at once...)
	else if(
		atom_getsymbol( & argv[0] ) == gensym("set")
	)
	{
		x->last_method = LAST_METHOD_SET;
		int currentPos = 2;
		while( currentPos < argc )
		{
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
				x->toProperties_out,
				gensym("set"),
				currentSize+1,
				toSend
			);
			freebytes( toSend, sizeof( t_atom ) * (currentSize+1) );
			currentPos = currentPos + 2 + currentSize ;
		}
	}
	else
	{
		outlet_anything(
			x->events_to_obj,
			s,
			argc,
			argv
		);
	}

	/*
	// init ( )
	else if(
		atom_getsymbol( & argv[0] ) == gensym("init")
		&& atom_getint( & argv[1] ) == 0
	)
	{
		outlet_anything(
			x->toProperties_out,
			gensym("init"),
			0,
			NULL
		);
	}
	*/

}

void objState_rawinput(
	t_objState* x,
	t_symbol *s,
	int argc,
	t_atom *argv
)
{

	if( argc >= 1 )
	{
		if(
			atom_getsymbol( & argv[0] ) == gensym("out")
		)
		{

			// out add <dest>
			if(
				argc == 3
				&& atom_getsymbol( & argv[1] ) == gensym("add")
			)
			{
				t_symbol* pDest = atom_getsymbol( & argv[3] );
				if( ! SymListGetElement( x->outList, pDest, cmp_symbol_ptrs ) )
				{
					char buf[256];
					atom_string( & argv[3], buf, 255 );
					//post( "adding dest: %s", buf );
					SymListAdd( x->outList, pDest );
				}
			}

			// out del <dest>
			else if(
				argc == 3
				&& atom_getsymbol( & argv[1] ) == gensym("del")
			)
			{
				t_symbol* pDest = atom_getsymbol( & argv[3] );
				SymEl *pDestEl = SymListGetElement( x->outList, pDest, cmp_symbol_ptrs );
				if( pDestEl )
				{
					SymListDel( x->outList, pDestEl );
				}
			}

			// out clear
			else if(
				argc == 2
				&& atom_getsymbol( & argv[1] ) == gensym("clear")
			)
			{
				SymListClear( x->outList );
			}
			else
			{
				pd_error(
					x,
					"wrong syntax for message starting with 'out'"
				);
			}

		}
		if(
			atom_getsymbol( & argv[0] ) == gensym("get")
		)
		{
			x->last_method = LAST_METHOD_GET;

			if(
				// get <property>
				argc == 2
				&& (argv[1].a_type == A_SYMBOL)
			)
			{
				// redirect:
				t_symbol* msg_selector = atom_getsymbol( & argv[0] );
				outlet_anything(
					x->toProperties_out,
					msg_selector,
					argc-1,
					& argv[1]
				);
			}

			else if(
				// get out dest1 ...
				argc >= 2
				&& atom_getsymbol( & argv[1] ) == gensym("out")
			)
			{
				int out_count = argc-2;
				// (temporarily) change outList:
				SymList* old_outList = x->outList;
				x->outList = getbytes( sizeof( SymList ) );
				SymListInit( x->outList );
				for(unsigned int i=0; i< out_count; i++)
				{
					SymListAdd( x->outList, atom_getsymbol( & argv[2+i] ) );
				}
				// query properties:
				outlet_anything(
					x->toProperties_out,
					gensym("get"),
					0,
					NULL
				);
				// change back outList:
				SymListExit( x->outList );
				freebytes( x->outList, sizeof( SymList ) );
				x->outList = old_outList;
			}

			else if(
				// get <property> out dest1 ...
				argc >= 3
				&& (argv[1].a_type == A_SYMBOL)
				&& atom_getsymbol( & argv[2] ) == gensym("out")
			)
			{
				int out_count = argc-3;
				// (temporarily) change outList:
				SymList* old_outList = x->outList;
				x->outList = getbytes( sizeof( SymList ) );
				SymListInit( x->outList );
				for(unsigned int i=0; i< out_count; i++)
				{
					SymListAdd( x->outList, atom_getsymbol( & argv[3+i] ) );
				}
				// query properties:
				outlet_anything(
					x->toProperties_out,
					gensym("get"),
					1,
					& argv[1]
				);
				// change back outList:
				SymListExit( x->outList );
				freebytes( x->outList, sizeof( SymList ) );
				x->outList = old_outList;
			}

			else
			{
				pd_error(
					x,
					"wrong syntax for message starting with 'get'"
				);
			}
		}
		else
		{
			x->last_method = LAST_METHOD_SET;
			// redirect:
			t_symbol* msg_selector = atom_getsymbol( & argv[0] );
			outlet_anything(
				x->toProperties_out,
				msg_selector,
				argc-1,
				& argv[1]
			);
		}
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
		// send to all registered "listeners"( = every obj in x->outlist)
		// <prop> <val1> ...

		// send as event: <package_name> ( <prop> ( <val1> ...  ) )
		int val_count = argc-1;
		t_atom* output_buf = getbytes( sizeof( t_atom ) * (val_count + 4) );
		{
			switch( x->last_method )
			{
				case LAST_METHOD_GET:
					SETSYMBOL( & output_buf[0], gensym("info") );
					break;
				case LAST_METHOD_SET:
					SETSYMBOL( & output_buf[0], gensym("update") );
					break;
			}
			SETFLOAT( & output_buf[1], val_count + 2);
			output_buf[2] = argv[0];
			SETFLOAT( & output_buf[3], val_count );
			for( int i=0; i<val_count; i++ )
			{
				output_buf[4+i] = argv[1+i];
			}

			LIST_FORALL_BEGIN(SymList,SymEl,t_symbol,x->outList,iOut,pEl)
				if( pEl->pData->s_thing )
				{
					// send event:
					typedmess(
							pEl->pData->s_thing,
							&s_list,
							val_count + 4,
							output_buf
						);
				}
			LIST_FORALL_END(SymList,SymEl,t_symbol,x->outList,iOut,pEl)
			outlet_list(
				x->obj_out,
				&s_list,
				val_count + 4,
				output_buf
			);
		}
		freebytes( output_buf, sizeof( t_atom ) * (val_count + 4) );
	}
	else
		// accumulate output:
	{
		t_atom* accuml = x->accumlArray;
		int val_count = argc-1;

		// <property>
		accuml[ x->accumlPos ] = argv[0];
		x->accumlPos ++;

		// <property> (
		SETFLOAT( & accuml[ (x->accumlPos) ], val_count );
		x->accumlPos ++;

		// <property> ( <val1> ... ) )
		for( unsigned int i=0; i< val_count; i++ )
		{
			x-> accumlArray[ x->accumlPos + i] = argv[ 1+i ];
			x->accumlPos ++;
		}
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
	//t_symbol* init;
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

void property_get(
	t_property* x,
	t_symbol *s,
	int argc,
	t_atom *argv
);

void property_set(
	t_property* x,
	t_symbol *s,
	int argc,
	t_atom *argv
);

/*
void property_initbang(
	t_property* x
);
*/

void property_priv_set(
	t_property* x,
	t_float newVal
);

void property_priv_set_noupdate(
	t_property* x,
	t_float newVal
);

// helper:
void property_output(
	t_property* x
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

	// "external" interface:
	class_addmethod(
		class,
		(t_method )property_get,
		gensym("get"),
		A_GIMME,
		0
	);
	class_addmethod(
		class,
		(t_method )property_set,
		gensym("set"),
		A_GIMME,
		0
	);
	/*
	class_addmethod(
		class,
		(t_method )property_initbang,
		gensym("init"),
		0
	);
	*/

	// "priv" messages
	class_addmethod(
		class,
		(t_method )property_priv_set,
		gensym("priv.set"),
		A_FLOAT,
		0
	);
	class_addmethod(
		class,
		(t_method )property_priv_set_noupdate,
		gensym("priv.set_noupdate"),
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

	/*
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
			x->init =
				atom_getsymbol( &argv[3] );
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
	*/

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

void property_get(
	t_property* x,
	t_symbol *s,
	int argc,
	t_atom *argv
)
{

	if(
		// get <property>
		argc == 1
		&& atom_getsymbol( &argv[0] ) == x->name
	)
	{
		// output "<val>"
		property_output(
			x
			//gensym("info")
		);
	}

	else if(
		// get
		argc == 0
	)
	{
		// output "info ( <property> ( <val> ) )"
		property_output(
			x
			//gensym("info")
		);
		outlet_anything(
			x->redirect_out,
			s,
			argc,
			argv
		);
	}

	else
	{
		outlet_anything(
			x->redirect_out,
			s,
			argc,
			argv
		);
	}

}

void property_set(
	t_property* x,
	t_symbol *s,
	int argc,
	t_atom *argv
)
{

	if(
		// set <property> <val>
		argc == 2
		&& atom_getsymbol( &argv[0] ) == x->name
		&& (argv[1].a_type == A_FLOAT)
	)
	{
		property_priv_set(
			x,
			atom_getfloat( & argv[1] )
		);
	}

	else
	{
		outlet_anything(
			x->redirect_out,
			s,
			argc,
			argv
		);
	}


}

/*
void property_initbang(
	t_property* x
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
			x
			//gensym("info")
		);
	}

	else if( x->init == gensym( "update" ) )
	{
		// output "update ( <property> ( <val> ) )"
		property_output(
			x
			//gensym("update")
		);
	}

}
*/

void property_priv_set(
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
		x
		//gensym("update")
	);
}

void property_priv_set_noupdate(
	t_property* x,
	t_float newVal
)
{
	//post("sdProperty: set_noupdate");
	x->value = newVal;
	// output "update ( <property> ( <val> ) )"
	property_output(
		x
		//gensym("update")
	);
}

void property_output(
	t_property* x
)
{
	// output "<property> <val>"
	t_atom outArray[1];
	SETFLOAT( & outArray[0], x->value );
	outlet_anything(
		x->out,
		x->name,
		1,
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
	//t_symbol* init;
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

void propertySym_get(
	t_propertySym* x,
	t_symbol *s,
	int argc,
	t_atom *argv
);

void propertySym_set(
	t_propertySym* x,
	t_symbol *s,
	int argc,
	t_atom *argv
);

/*
void property_initbang(
	t_property* x
);
*/

void propertySym_priv_set(
	t_propertySym* x,
	t_symbol* newVal
);

void propertySym_priv_set_noupdate(
	t_propertySym* x,
	t_symbol* newVal
);

// helper:
void propertySym_output(
	t_propertySym* x
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

	// "external" interface:
	class_addmethod(
		class,
		(t_method )propertySym_get,
		gensym("get"),
		A_GIMME,
		0
	);
	class_addmethod(
		class,
		(t_method )propertySym_set,
		gensym("set"),
		A_GIMME,
		0
	);
	/*
	class_addmethod(
		class,
		(t_method )propertySym_initbang,
		gensym("init"),
		0
	);
	*/

	// "internal" messages
	class_addmethod(
		class,
		(t_method )propertySym_priv_set,
		gensym("priv.set"),
		A_SYMBOL,
		0
	);
	class_addmethod(
		class,
		(t_method )propertySym_priv_set_noupdate,
		gensym("priv.set_noupdate"),
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

	/*
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
	*/

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

void propertySym_get(
	t_propertySym* x,
	t_symbol *s,
	int argc,
	t_atom *argv
)
{

	if(
		// get <property>
		argc == 1
		&& atom_getsymbol( &argv[0] ) == x->name

	)
	{
		// output "info ( <propertySym> ( <val> ) )"
		propertySym_output(
			x
			//gensym("info")
		);
	}

	else if(
		// get
		argc == 0
	)
	{
		// output "info ( <property> ( <val> ) )"
		propertySym_output(
			x
			//gensym("info")
		);
		outlet_anything(
			x->redirect_out,
			s,
			argc,
			argv
		);
	}

	else
	{
		outlet_anything(
			x->redirect_out,
			s,
			argc,
			argv
		);
	}

}

void propertySym_set(
	t_propertySym* x,
	t_symbol *s,
	int argc,
	t_atom *argv
)
{

	if(
		// set <propertySym> <val>
		argc == 2
		&& atom_getsymbol( &argv[0] ) == x->name
		&& (argv[1].a_type == A_SYMBOL)
	)
	{
		propertySym_priv_set(
			x,
			atom_getsymbol( & argv[1] )
		);
	}

	else
	{
		outlet_anything(
			x->redirect_out,
			s,
			argc,
			argv
		);
	}

}

/*
void propertySym_initbang(
	t_property* x
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

}
*/

void propertySym_priv_set(
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
		x
		//gensym("update")
	);
}

void propertySym_priv_set_noupdate(
	t_propertySym* x,
	t_symbol* newVal
)
{
	//post("sdPropertySym: set_noupdate");
	x->value = newVal;
	propertySym_output(
		x
		//gensym("update")
	);
}

void propertySym_output(
	t_propertySym* x
)
{
	// output "<propertySym> <val> "
	t_atom outArray[1];
	SETSYMBOL( & outArray[0], x->value );
	outlet_anything(
		x->out,
		x->name,
		1,
		outArray
	);
}

//----------------------------------
// propertyList
//----------------------------------

DECL_LIST(AtomList, AtomEl, t_atom,getbytes,freebytes,freebytes)
DEF_LIST(AtomList, AtomEl, t_atom,getbytes,freebytes,freebytes)

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

void propertyList_get(
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

void propertyList_priv_set(
	t_propertyList* x,
	t_symbol *s,
	int argc,
	t_atom *argv
);

void propertyList_priv_set_noupdate(
	t_propertyList* x,
	t_symbol *s,
	int argc,
	t_atom *argv
);

// helper:
void propertyList_output(
	t_propertyList* x
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

	// "external" interface:
	class_addmethod(
		class,
		(t_method )propertyList_get,
		gensym("get"),
		A_GIMME,
		0
	);
	class_addmethod(
		class,
		(t_method )propertyList_set,
		gensym("set"),
		A_GIMME,
		0
	);

	// "private" interface:
	class_addmethod(
		class,
		(t_method )propertyList_priv_set,
		gensym("priv.set"),
		A_GIMME,
		0
	);
	class_addmethod(
		class,
		(t_method )propertyList_priv_set_noupdate,
		gensym("priv.set_noupdate"),
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

void propertyList_get(
	t_propertyList* x,
	t_symbol *s,
	int argc,
	t_atom *argv
)
{

	if(
		// get <property>
		(
		argc == 1
		&& atom_getsymbol( &argv[0] ) == x->name
		)
	||
		// get
		argc == 0
	)
	{
		// output "info ( <property> ( <val> ... ) )"
		propertyList_output(
			x
			//gensym( "info" )
		);
		// get:
		if(
			argc == 0
		)
		{
			outlet_anything(
				x->redirect_out,
				s,
				argc,
				argv
			);
		}
	}

	else
	{
		outlet_anything(
			x->redirect_out,
			s,
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

	if(
		// set <property> <val1> <val2> ...
		argc >= 1
		&& atom_getsymbol( &argv[0] ) == x->name
	)
	{
		propertyList_priv_set(
			x,
			&s_list,
			argc - 1,
			& argv[1]
		);
	}
	else
	{
		outlet_anything(
			x->redirect_out,
			s,
			argc,
			argv
		);
	}
}

void propertyList_priv_set(
	t_propertyList* x,
	t_symbol *s,
	int argc,
	t_atom *argv
)
{
	//post("sdPropertyList: set");
	int new_value_count = argc;
	AtomListClear( & x->value );
	for(int i=0; i < new_value_count; i++)
	{
		t_atom* newVal = getbytes( sizeof( t_atom ) );
		*newVal = argv[i];
		AtomListAdd( & x->value, newVal );
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
		x
		//gensym( "update" )
	);
}

void propertyList_priv_set_noupdate(
	t_propertyList* x,
	t_symbol *s,
	int argc,
	t_atom *argv
)
{
	//post("sdPropertyList: set_noupdate");
	int new_value_count = argc;
	AtomListClear( & x->value );
	for(int i=0; i < new_value_count; i++)
	{
		t_atom* newVal = getbytes( sizeof( t_atom ) );
		*newVal = argv[i];
		AtomListAdd( & x->value, newVal );
	}

	// output "update ( <propertyList> ( <val> ... ) )"
	propertyList_output(
		x
		//gensym( "update" )
	);
}

void propertyList_output(
	t_propertyList* x
)
{
	// output "<property> <val> ..."
	int value_count = AtomListGetSize( &x->value );
	t_atom* outArray = getbytes( sizeof( t_atom ) * value_count );
	LIST_FORALL_BEGIN(AtomList,AtomEl,t_atom,&x->value,i,pEl)
		outArray[i] = *(pEl->pData);
	LIST_FORALL_END(AtomList,AtomEl,t_atom,&x->value,i,pEl)
	outlet_anything(
		x->out,
		x->name,
		value_count,
		outArray
	);
	freebytes( outArray, sizeof( t_atom ) * value_count );
}
