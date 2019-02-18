#include "sdObjState.h"
#include "LinkedList.h"

#include "m_pd.h"

#include <string.h>

#define ACCUML_SIZE 256


static t_class* property_class;
static t_class* propertySym_class;
static t_class* propertyList_class;

t_class* register_property(
	t_symbol* className
);

t_class* register_propertySym(
	t_symbol* className
);

t_class* register_propertyList(
	t_symbol* className
);

void sdProperties_setup()
{
	property_class = register_property( gensym("sdProperty") );
	propertySym_class = register_propertySym( gensym("sdPropertySym") );
	propertyList_class = register_propertyList( gensym("sdPropertyList") );
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
