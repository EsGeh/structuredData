#include "sdObjState.h"
#include "LinkedList.h"
#include "DynArray.h"

#include "m_pd.h"

#include <string.h>


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


DECL_DYN_ARRAY(AtomBuffer,t_atom,getbytes,freebytes)
DEF_DYN_ARRAY(AtomBuffer,t_atom,getbytes,freebytes)


//----------------------------------
// objState
//----------------------------------

typedef enum e_last_method { LAST_METHOD_SET, LAST_METHOD_GET } t_last_method;

typedef struct s_objState {
  t_object x_obj;
	t_symbol* objName;
	SymList groups;
	//t_symbol* globalIn;
	SymList* outList;
	t_last_method last_method;
	BOOL enable_accuml;
	AtomBuffer accumlArray;
	t_inlet* toObj_in;
	t_inlet* fromProperties_in;
	t_inlet* objEvents_in;
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

void objState_sendEvent(
	t_objState* x,
	t_symbol *s,
	int argc,
	t_atom *argv
);

// helper:

void objState_get(
	t_objState* x,
	t_atom* prop,
	int out_count,
	t_atom* out
);


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
	class_sethelpsymbol(
			class,
			gensym("sdObjState")
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

	// send events
	class_addmethod(
		class,
		(t_method )objState_sendEvent,
		gensym("send"),
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

	if( argc < 1 )
	{
		pd_error( x, "to few parameters. syntax: objName [group1] ..." );
		return NULL;
	}
	SymList_init( & x->groups );
	if( atom_getsymbol( &argv[0] ) == gensym("") )
	{
		pd_error( x, "invalid objName. syntax: objName [group1] ...");
		return NULL;
	}
	else
	{
		x->objName = atom_getsymbol( &argv[0] );
	}

	for( int i=1; i< argc; i++ )
	{
		if( atom_getsymbol( & argv[i] ) == gensym("") )
		{
			pd_error( x, "invalid group name at pos %i. syntax: objName [group1] ...", i );
			return NULL;
		}
		SymList_append( & x->groups, atom_getsymbol( & argv[i] ) );
	}

	// init groupIn:
	if( argc == 1 )
	{
		SymList_append( & x->groups, gensym("GLOBAL") );
	}

	x->outList = getbytes( sizeof( SymList ) );
	SymList_init( x->outList );

	x->enable_accuml = 0;
	AtomBuffer_init( & x->accumlArray );
	{
		t_atom obj_name;
		SETSYMBOL( & obj_name, x->objName );
		AtomBuffer_append( & x->accumlArray, obj_name );
	}

	// receive:
	pd_bind(
		& x->x_obj.ob_pd,
		x->objName
	);

	LIST_FORALL_BEGIN(SymList,SymEl,t_symbol,&x->groups,i,pEl)
		// receive group messages:
		pd_bind(
			& x->x_obj.ob_pd,
			pEl->pData
		);
	LIST_FORALL_END(SymList,SymEl,t_symbol,&x->groups,i,pEl)

	x->toObj_in =
		x->x_obj.ob_inlet;

	x->fromProperties_in =
		inlet_new(
			& x->x_obj,
			& x->x_obj.ob_pd,
			&s_list,
			gensym("fromProps")
		);

	x->objEvents_in =
		inlet_new(
			& x->x_obj,
			& x->x_obj.ob_pd,
			&s_list,
			gensym("send")
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

	LIST_FORALL_BEGIN(SymList,SymEl,t_symbol,&x->groups,i,pEl)
		// unbind from groups:
		pd_unbind(
			& x->x_obj.ob_pd,
			pEl->pData
		);
	LIST_FORALL_END(SymList,SymEl,t_symbol,&x->groups,i,pEl)

	AtomBuffer_exit( & x->accumlArray );
	SymList_exit( x->outList );
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
			t_symbol* dest = atom_getsymbol( & argv[3] );
			if( ! SymList_get_element( x->outList, dest, cmp_symbol_ptrs ) )
			{
				SymList_append( x->outList, dest );
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
			SymEl *pDestEl = SymList_get_element( x->outList, pDest, cmp_symbol_ptrs );
			if( pDestEl )
			{
				SymList_del( x->outList, pDestEl );
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
			SymList_clear( x->outList );
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
			&& (argv[2].a_type == A_SYMBOL)
		)
		{
			// send to obj: "get <property>"
			objState_get(
				x,
				&argv[2],
				0,
				NULL
			);
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
			int out_count = atom_getint( & argv[3] );
			t_atom* out = getbytes( sizeof( t_symbol ) * out_count );
			for(unsigned int i=0; i<out_count; i++)
			{
				out[i] = argv[4+i];
			}
			objState_get(
					x,
					NULL,
					out_count,
					out
			);
			freebytes( out, sizeof( t_symbol ) * out_count );

		}
		// get ( <property> out ( dest1 ... ) )
		else if(
			atom_getint( &argv[1] ) >= 3
			&& atom_getsymbol( &argv[2] ) != gensym("out")
			&& atom_getsymbol( &argv[3] ) == gensym("out")
			&& argv[4].a_type == A_FLOAT
		)
		{
			t_atom* prop = &argv[2];
			int out_count = atom_getint( & argv[4] );
			t_atom* out = getbytes( sizeof( t_symbol ) * out_count );
			for(unsigned int i=0; i<out_count; i++)
			{
				out[i] = argv[5+i];
			}
			objState_get(
					x,
					prop,
					out_count,
					out
			);
			freebytes( out, sizeof( t_symbol ) * out_count );
		}
		else
		{
			pd_error(x, "unknown syntax in event starting with 'get'!");
			return;
		}
	}

	// set ( <property> ( val1 [...] ) ... )      (...: for list properties)
	// or
	// set_no_out ( <property> ( val1 [...] ) ... )      (...: for list properties)
	//   (you can set several properties at once...)
	else if(
		atom_getsymbol( & argv[0] ) == gensym("set")
		|| 
		atom_getsymbol( & argv[0] ) == gensym("set_no_out")
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
				atom_getsymbol( & argv[0] ),
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
				t_symbol* dest = atom_getsymbol( & argv[3] );
				if( ! SymList_get_element( x->outList, dest, cmp_symbol_ptrs ) )
				{
					SymList_append( x->outList, dest );
				}
			}

			// out del <dest>
			else if(
				argc == 3
				&& atom_getsymbol( & argv[1] ) == gensym("del")
			)
			{
				t_symbol* pDest = atom_getsymbol( & argv[3] );
				SymEl *pDestEl = SymList_get_element( x->outList, pDest, cmp_symbol_ptrs );
				if( pDestEl )
				{
					SymList_del( x->outList, pDestEl );
				}
			}

			// out clear
			else if(
				argc == 2
				&& atom_getsymbol( & argv[1] ) == gensym("clear")
			)
			{
				SymList_clear( x->outList );
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
				objState_get(
						x,
						& argv[1],
						0,
						NULL
					);
			}

			else if(
				// get out dest1 ...
				argc >= 2
				&& atom_getsymbol( & argv[1] ) == gensym("out")
			)
			{
				int out_count = argc-2;
				objState_get(
						x,
						NULL,
						out_count,
						& argv[2]
				);
			}

			else if(
				// get <property> out dest1 ...
				argc >= 3
				&& (argv[1].a_type == A_SYMBOL)
				&& atom_getsymbol( & argv[2] ) == gensym("out")
			)
			{
				int out_count = argc-3;
				objState_get(
						x,
						& argv[1],
						out_count,
						& argv[3]
				);
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
	// syntax: <prop> <val1> ...

	if( ! x-> enable_accuml )
	{
		//post("fromProps");
		// send to all registered "listeners"( = every obj in x->outlist)

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
			objState_sendEvent(
					x,
					s,
					val_count + 4,
					output_buf
			);
		}
		freebytes( output_buf, sizeof( t_atom ) * (val_count + 4) );
	}
	else
		// accumulate output:
	{
		// append to buffer: <prop> <val_count> <val1> ...
		int val_count = argc-1;
		AtomBuffer_append( & x->accumlArray, argv[0] );
		{
			t_atom val_count_atom;
			SETFLOAT( & val_count_atom, val_count );
			AtomBuffer_append( & x->accumlArray, val_count_atom);
		}
		// <property> ( <val1> ... ) )
		for( unsigned int i=0; i< val_count; i++ )
		{
			AtomBuffer_append( & x-> accumlArray, argv[ 1+i ] );
		}
	}
}

void objState_sendEvent(
	t_objState* x,
	t_symbol *s,
	int argc,
	t_atom *argv
)
{

		LIST_FORALL_BEGIN(SymList,SymEl,t_symbol,x->outList,iOut,pEl)
			if( pEl->pData->s_thing )
			{
				// send event:
				typedmess(
						pEl->pData->s_thing,
						&s_list,
						argc,
						argv
					);
			}
		LIST_FORALL_END(SymList,SymEl,t_symbol,x->outList,iOut,pEl)
		outlet_list(
			x->obj_out,
			&s_list,
			argc,
			argv
		);
}

void objState_get(
	t_objState* x,
	t_atom* prop,
	int out_count,
	t_atom* out
)
{
	if( prop != NULL && out == NULL )
	{
		// send to obj: "get <property>"
		outlet_anything(
			x->toProperties_out,
			gensym("get"),
			1,
			prop
		);
	}
	else if( out != NULL )
	{
		// (temporarily) change outList:
		SymList* old_outList = x->outList;
		x->outList = getbytes( sizeof( SymList ) );
		SymList_init( x->outList );
		for(unsigned int i=0; i< out_count; i++)
		{
			SymList_append( x->outList, atom_getsymbol( & out[i] ) );
		}

		if( prop == NULL )
		{
			//x->accumlPos = 2;
			AtomBuffer_set_size( & x->accumlArray, 2 );
			x->enable_accuml = TRUE;
			// send to obj: "get ( )":
			outlet_anything(
				x->toProperties_out,
				gensym("get"),
				0,
				NULL
			);
			
			// append "pseudo property" <out> ( ... ):
			{
				t_atom temp;
				SETSYMBOL( &temp, gensym("out") );
				AtomBuffer_append( & x->accumlArray, temp );
			}
			{
				int out_size = SymList_get_size( old_outList );
				t_atom temp;
				SETFLOAT( &temp, out_size );
				AtomBuffer_append( & x->accumlArray, temp );
			}
			LIST_FORALL_BEGIN(SymList,SymEl,t_symbol,old_outList,i,pEl)
				t_atom temp;
				SETSYMBOL( &temp, pEl->pData );
				AtomBuffer_append( & x->accumlArray, temp );
			LIST_FORALL_END(SymList,SymEl,t_symbol,old_outList,i,pEl)

			// change back outList:
			objState_flush(x);
			x->enable_accuml = FALSE;
		}
		else
		{
			// send to obj: "get <property>"
			// query properties:
			outlet_anything(
				x->toProperties_out,
				gensym("get"),
				1,
				prop
			);
		}
		SymList_exit( x->outList );
		freebytes( x->outList, sizeof( SymList ) );
		x->outList = old_outList;
		// unset "accuml":
		//post("unset accuml");
	}
}

void objState_flush(
	t_objState* x
)
{
	//post("fromProps");
	SETFLOAT(
			& AtomBuffer_get_array( & x->accumlArray )[1],
			AtomBuffer_get_size( & x->accumlArray ) - 2
	);

	LIST_FORALL_BEGIN(SymList,SymEl,t_symbol,x->outList,i,pEl)
		if( pEl->pData->s_thing )
		{
			//post("sending to %s", pEl->pData->s_name);
			typedmess(
					pEl->pData->s_thing,
					&s_list,
					AtomBuffer_get_size( & x->accumlArray ),
					AtomBuffer_get_array( & x->accumlArray )
			);
		}
	LIST_FORALL_END(SymList,SymEl,t_symbol,x->outList,i,pEl)
}
