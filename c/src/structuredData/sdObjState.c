#include "sdObjState.h"
#include "LinkedList.h"

#include "m_pd.h"

#include <string.h>

#define ACCUML_SIZE 256


static t_class* objState_class;

t_class* register_objState(
	t_symbol* className
);

void sdObjState_setup()
{
	objState_class = register_objState( gensym("sdObjState") );
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
