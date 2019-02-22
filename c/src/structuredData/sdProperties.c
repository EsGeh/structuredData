#include "sdProperties.h"
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
// propertyList, property, propertySym
//----------------------------------

DECL_LIST(AtomList, AtomEl, t_atom,getbytes,freebytes,freebytes)
DEF_LIST(AtomList, AtomEl, t_atom,getbytes,freebytes,freebytes)

typedef enum e_property_type { PROPTYPE_FLOAT, PROPTYPE_SYMBOL, PROPTYPE_LIST } t_property_type;
typedef enum e_init_type { INITTYPE_NONE, INITTYPE_INTERN, INITTYPE_UPDATE } t_init_type;

typedef struct s_property {
  t_object x_obj;
	t_symbol* name;
	t_property_type type;
	AtomList value;
	t_symbol* rcv_sym;
	t_symbol* send_sym;
	t_init_type init_type;
	t_inlet* fromObjIn_in;
	t_outlet* out;
	t_outlet* redirect_out;
} t_property;

void* propertyList_init(
	t_symbol *s,
	int argc,
	t_atom *argv
);
void* property_init(
	t_symbol *s,
	int argc,
	t_atom *argv
);
void* propertySym_init(
	t_symbol *s,
	int argc,
	t_atom *argv
);

int property_initall(
	t_property* x,
	int argc,
	t_atom *argv
);

void property_exit(
	struct s_property* x
);

void property_on_get(
	t_property* x,
	t_symbol *s,
	int argc,
	t_atom *argv
);

void property_on_set(
	t_property* x,
	t_symbol *s,
	int argc,
	t_atom *argv
);

void property_on_priv_set(
	t_property* x,
	t_symbol *s,
	int argc,
	t_atom *argv
);

void property_on_priv_set_noupdate(
	t_property* x,
	t_symbol *s,
	int argc,
	t_atom *argv
);

void property_on_priv_get(
	t_property* x
);

void property_on_init(
	t_property* x
);

// helper:

void property_set(
	t_property* x,
	int argc,
	t_atom *argv
);

void property_output(
	t_property* x
);

void register_propertyMethods(
	t_class* class
);

t_class* register_propertyList(
	t_symbol* className
)
{
	t_class* class =
		class_new(
			className,
			(t_newmethod )propertyList_init, // constructor
			(t_method )property_exit, // destructor
			sizeof(t_property),
			CLASS_DEFAULT, // graphical repr ?
			// creation arguments:
			A_GIMME,
			0
		);
	register_propertyMethods( class );

	return class;
}

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
	register_propertyMethods( class );

	return class;
}

t_class* register_propertySym(
	t_symbol* className
)
{
	t_class* class =
		class_new(
			className,
			(t_newmethod )propertySym_init, // constructor
			(t_method )property_exit, // destructor
			sizeof(t_property),
			CLASS_DEFAULT, // graphical repr ?
			// creation arguments:
			A_GIMME,
			0
		);
	register_propertyMethods( class );

	return class;
}

void register_propertyMethods(
	t_class* class
)
{
	// "external" interface:
	class_addmethod(
		class,
		(t_method )property_on_get,
		gensym("get"),
		A_GIMME,
		0
	);
	class_addmethod(
		class,
		(t_method )property_on_set,
		gensym("set"),
		A_GIMME,
		0
	);

	// "private" interface:
	class_addmethod(
		class,
		(t_method )property_on_priv_set,
		gensym("priv.set"),
		A_GIMME,
		0
	);
	class_addmethod(
		class,
		(t_method )property_on_priv_set_noupdate,
		gensym("priv.set_noupdate"),
		A_GIMME,
		0
	);
	class_addmethod(
		class,
		(t_method )property_on_priv_get,
		gensym("priv.get"),
		0
	);
	class_addmethod(
		class,
		(t_method )property_on_init,
		gensym("init"),
		0
	);
}

void* propertyList_init(
	t_symbol *s,
	int argc,
	t_atom *argv
)
{
  t_property *x = (t_property *)pd_new(property_class);
	x->type = PROPTYPE_LIST;
	if(
		property_initall(
				x,
				argc,
				argv
		)
	)
	{
		return NULL;
	}

  return (void *)x;
}

void* property_init(
	t_symbol *s,
	int argc,
	t_atom *argv
)
{
  t_property *x = (t_property *)pd_new(property_class);
	x->type = PROPTYPE_FLOAT;
	property_initall(
			x,
			argc,
			argv
	);

  return (void *)x;
}

void* propertySym_init(
	t_symbol *s,
	int argc,
	t_atom *argv
)
{
  t_property *x = (t_property *)pd_new(property_class);
	x->type = PROPTYPE_SYMBOL;
	if(
		property_initall(
				x,
				argc,
				argv
		)
	)
	{
		return NULL;
	}

  return (void *)x;
}

int property_initall(
	t_property* x,
	int argc,
	t_atom *argv
)
{

	if( x->type == PROPTYPE_LIST)
	{
		if( argc < 1 || argc > 2 )
		{
			pd_error( x, "wrong number of parameters. syntax: <prop_name> [$0]" );
			return 1;
		}
	}
	else if( x->type == PROPTYPE_FLOAT || x->type == PROPTYPE_SYMBOL )
	{
		if( argc < 1 || argc > 4 )
		{
			pd_error( x, "wrong number of parameters. syntax: <prop_name> [$0 default init?]" );
			return 1;
		}
	}
	x->name = atom_getsymbol( & argv[0] );

	char rcv_str[256];
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

	// type checking:
	if(
	 x->type == PROPTYPE_SYMBOL && argc >= 3 && argv[2].a_type != A_SYMBOL
	)
	{
		char buf[256];
		t_atom name;
		SETSYMBOL( & name, x->name );
		atom_string( & name, buf, 255 );
		pd_error( x, "sdPropertySym %s: wrong type for default value. syntax: '<prop_name> [$0 default]'", buf );
		return 1;
	}
	if(
	 x->type == PROPTYPE_FLOAT && argc >= 3 && argv[2].a_type != A_FLOAT
	)
	{
		char buf[256];
		t_atom name;
		SETSYMBOL( & name, x->name );
		atom_string( & name, buf, 255 );
		pd_error( x, "sdProperty %s: wrong type for default value. syntax: '<prop_name> [$0 default]'", buf );
		return 1;
	}

	if( x->type == PROPTYPE_FLOAT || x->type == PROPTYPE_SYMBOL)
	{
		t_atom* init_val = getbytes( sizeof( t_atom ) );
		if( argc >= 3 )
		{
			*init_val = argv[2];
		}
		else
		{
			if( x->type == PROPTYPE_FLOAT )
				SETFLOAT( init_val, 0 );
			if( x->type == PROPTYPE_SYMBOL )
				SETSYMBOL( init_val, gensym("") );
		}
		AtomListAdd( & x->value, init_val );
	}
	if( argc >= 4 )
	{
		if( atom_getsymbol( &argv[3] ) == gensym("intern") )
		{
			x->init_type = INITTYPE_INTERN;
		}
		else if( atom_getsymbol( &argv[3] ) == gensym("update") )
		{
			x->init_type = INITTYPE_UPDATE;
		}
		else
		{
			char buf[256];
			t_atom name;
			SETSYMBOL( & name, x->name );
			atom_string( & name, buf, 255 );
			pd_error( x, "sdProperty %s: wrong type for init value. syntax: '<prop_name> [$0 default init]', init one of: 'intern', 'update'", buf );
			return 1;
		}
	}
	else
	{
		x->init_type = INITTYPE_NONE;
	}

	x->fromObjIn_in =
		x->x_obj.ob_inlet;

	x->out =
		outlet_new( & x->x_obj, &s_list);
	x->redirect_out =
		outlet_new( & x->x_obj, &s_list);

	return 0;
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
	AtomListExit( & x->value );
}

void property_on_get(
	t_property* x,
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
		property_output(
			x
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

void property_on_set(
	t_property* x,
	t_symbol *s,
	int argc,
	t_atom *argv
)
{
	// set <property> <val1> <val2> ...

	if(
		argc >= 1
		&& atom_getsymbol( &argv[0] ) == x->name
	)
	{

		if(
		 x->type == PROPTYPE_SYMBOL
		)
		{
			if( argc != 2 || (argc == 2 && argv[1].a_type != A_SYMBOL ) )
			{
				char name_buf[256];
				t_atom name;
				SETSYMBOL( & name, x->name );
				atom_string( & name, name_buf, 255 );
				pd_error( x, "error in sdPropertySym %s: type error! expected 'set <prop_name> <symbol>'", name_buf );
				return;
			}
		}
		else if(
		 x->type == PROPTYPE_FLOAT
		)
		{
			if( argc != 2 || (argc == 2 && argv[1].a_type != A_FLOAT ) )
			{
				char name_buf[256];
				t_atom name;
				SETSYMBOL( & name, x->name );
				atom_string( & name, name_buf, 255 );
				pd_error( x, "error in sdProperty %s: type error! expected 'set <prop_name> <float>'", name_buf );
				return;
			}
		}
		else if(
		 x->type == PROPTYPE_LIST
		)
		{
			if( argc == 0 )
			{
				char name_buf[256];
				t_atom name;
				SETSYMBOL( & name, x->name );
				atom_string( & name, name_buf, 255 );
				pd_error( x, "error in sdPropertyList %s: type error! expected 'set <prop_name> <val1> ...'", name_buf );
				return;
			}
		}

		property_set(
				x,
				argc-1,
				& argv[1]
		);
		if( x->send_sym && x->send_sym->s_thing )
		{
			typedmess(
				x->send_sym->s_thing,
				&s_list,
				argc-1,
				& argv[1]
			);
		}
		property_output(
			x
		);
	}
	else
	{
		// redirect to other properties:
		outlet_anything(
			x->redirect_out,
			s,
			argc,
			argv
		);
	}
}

void property_on_priv_set(
	t_property* x,
	t_symbol *s,
	int argc,
	t_atom *argv
)
{
	if(
		x->type == PROPTYPE_SYMBOL && (argc != 1 || argv[0].a_type != A_SYMBOL)
	)
	{
		char name_buf[256];
		t_atom name;
		SETSYMBOL( & name, x->name );
		atom_string( & name, name_buf, 255 );

		char sel_buf[256];
		t_atom selector;
		SETSYMBOL( & selector, s );
		atom_string( & selector, sel_buf, 255 );

		pd_error( x, "error in sdProperty %s: type error! expected '%s <symbol>'", name_buf, sel_buf );
		return;
	}
	if(
		x->type == PROPTYPE_FLOAT && (argc != 1 || argv[0].a_type != A_FLOAT)
	)
	{
		char name_buf[256];
		t_atom name;
		SETSYMBOL( & name, x->name );
		atom_string( & name, name_buf, 255 );

		char sel_buf[256];
		t_atom selector;
		SETSYMBOL( & selector, s );
		atom_string( & selector, sel_buf, 255 );

		pd_error( x, "error in sdProperty %s: type error! expected '%s <float>'", name_buf, sel_buf);
		return;
	}
	property_set(
			x,
			argc,
			argv
	);
	if( x->send_sym && x->send_sym->s_thing )
	{
		typedmess(
			x->send_sym->s_thing,
			&s_list,
			argc,
			argv
		);
	}
	property_output(
		x
	);
}

void property_on_priv_set_noupdate(
	t_property* x,
	t_symbol *s,
	int argc,
	t_atom *argv
)
{
	if(
		x->type == PROPTYPE_SYMBOL && (argc != 1 || argv[0].a_type != A_SYMBOL)
	)
	{
		char name_buf[256];
		t_atom name;
		SETSYMBOL( & name, x->name );
		atom_string( & name, name_buf, 255 );

		char sel_buf[256];
		t_atom selector;
		SETSYMBOL( & selector, s );
		atom_string( & selector, sel_buf, 255 );

		pd_error( x, "error in sdProperty %s: type error! expected '%s <symbol>'", name_buf, sel_buf );
		return;
	}
	if(
		x->type == PROPTYPE_FLOAT && (argc != 1 || argv[0].a_type != A_FLOAT)
	)
	{
		char name_buf[256];
		t_atom name;
		SETSYMBOL( & name, x->name );
		atom_string( & name, name_buf, 255 );

		char sel_buf[256];
		t_atom selector;
		SETSYMBOL( & selector, s );
		atom_string( & selector, sel_buf, 255 );

		pd_error( x, "error in sdProperty %s: type error! expected '%s <float>'", name_buf, sel_buf);
		return;
	}
	property_set(
			x,
			argc,
			argv
	);
	property_output(
		x
	);
}

void property_on_priv_get(
	t_property* x
)
{
	if( x->send_sym && x->send_sym->s_thing )
	{
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
		typedmess(
			x->send_sym->s_thing,
			&s_list,
			value_count,
			outArray
		);
		freebytes( outArray, sizeof( t_atom ) * value_count );
	}
	outlet_anything(
		x->redirect_out,
		gensym("priv.get"),
		0,
		NULL
	);
}

void property_on_init(
	t_property* x
)
{
	if( x->init_type == INITTYPE_INTERN || x->init_type == INITTYPE_UPDATE )
	{
		if( x->send_sym && x->send_sym->s_thing )
		{
			int value_count = AtomListGetSize( &x->value );
			t_atom* outArray = getbytes( sizeof( t_atom ) * value_count );
			LIST_FORALL_BEGIN(AtomList,AtomEl,t_atom,&x->value,i,pEl)
				outArray[i] = *(pEl->pData);
			LIST_FORALL_END(AtomList,AtomEl,t_atom,&x->value,i,pEl)
			typedmess(
				x->send_sym->s_thing,
				&s_list,
				value_count,
				outArray
			);
			freebytes( outArray, sizeof( t_atom ) * value_count );
		}
	}
	if( x->init_type == INITTYPE_UPDATE )
	{
		property_output(
			x
		);
	}
	outlet_anything(
		x->redirect_out,
		gensym("init"),
		0,
		NULL
	);
}

void property_set(
	t_property* x,
	int argc,
	t_atom *argv
)
{
	int new_value_count = argc;
	AtomListClear( & x->value );
	for(int i=0; i < new_value_count; i++)
	{
		t_atom* newVal = getbytes( sizeof( t_atom ) );
		*newVal = argv[i];
		AtomListAdd( & x->value, newVal );
	}
}

void property_output(
	t_property* x
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
