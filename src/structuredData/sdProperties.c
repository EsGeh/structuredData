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
	t_atom range[4]; // name, min, max, step
	// t_float min, max;
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
void property_on_get_range(
	t_property* x,
	t_symbol *s,
	int argc,
	t_atom *argv
);
void property_on_get_with_range(
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

void property_on_set_in_range(
	t_property* x,
	t_symbol *s,
	int argc,
	t_atom *argv
);

void property_on_set_min(
	t_property* x,
	t_symbol *s,
	int argc,
	t_atom *argv
);

void property_on_set_max(
	t_property* x,
	t_symbol *s,
	int argc,
	t_atom *argv
);

void property_on_set_step(
	t_property* x,
	t_symbol *s,
	int argc,
	t_atom *argv
);

void property_on_set_no_out(
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
void property_output_range(
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
	class_sethelpsymbol(
			class,
			gensym("sdObjState")
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
	class_sethelpsymbol(
			class,
			gensym("sdObjState")
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
	class_sethelpsymbol(
			class,
			gensym("sdObjState")
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
		(t_method )property_on_get_range,
		gensym("get_range"),
		A_GIMME,
		0
	);
	class_addmethod(
		class,
		(t_method )property_on_get_with_range,
		gensym("get_with_range"),
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
	class_addmethod(
		class,
		(t_method )property_on_set_in_range,
		gensym("set_in_range"),
		A_GIMME,
		0
	);
	class_addmethod(
		class,
		(t_method )property_on_set_min,
		gensym("set_min"),
		A_GIMME,
		0
	);
	class_addmethod(
		class,
		(t_method )property_on_set_max,
		gensym("set_max"),
		A_GIMME,
		0
	);
	class_addmethod(
		class,
		(t_method )property_on_set_step,
		gensym("set_step"),
		A_GIMME,
		0
	);

	class_addmethod(
		class,
		(t_method )property_on_set_no_out,
		gensym("set_no_out"),
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
	else if( x->type == PROPTYPE_FLOAT )
	{
		if( argc < 1 || argc > 7 )
		{
			pd_error( x, "wrong number of parameters. syntax: <prop_name> [$0 default init? min max step]" );
			return 1;
		}
	}
	else if( x->type == PROPTYPE_SYMBOL )
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
	AtomList_init( & x->value );

	// type checking:
	/* the following code would lead to problems...:
	 * if $1 or similar is used as argv[2], pd sets its type
	 * to float if it is empty. This should not lead to errors.
	*/
	/*
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
	*/
	if(
	 x->type == PROPTYPE_FLOAT && argc >= 3 && argv[2].a_type != A_FLOAT
	)
	{
		char buf[256];
		t_atom name;
		SETSYMBOL( & name, x->name );
		atom_string( & name, buf, 255 );
		pd_error( x, "sdProperty %s: wrong type for default value. syntax: '<prop_name> [$0 default init? min max]'", buf );
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
		AtomList_append( & x->value, init_val );
	}
	x->init_type = INITTYPE_NONE;
	if( argc >= 4 )
	{
		if( atom_getsymbol( &argv[3] ) == gensym("noinit") )
		{
			x->init_type = INITTYPE_NONE;
		}
		else if( atom_getsymbol( &argv[3] ) == gensym("intern") )
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
			pd_error( x, "sdProperty %s: wrong type for init value. syntax: '<prop_name> [$0 default init]', init one of: 'noinit', 'intern', 'update'", buf );
			return 1;
		}
	}
	SETSYMBOL( & x->range[0], x->name );
	SETFLOAT( & x->range[1], 0 );
	SETFLOAT( & x->range[2], 1000 );
	SETFLOAT( & x->range[3], 0 ); // 0 means float, (= no steps)
	if( argc >= 5 )
	{
		SETFLOAT( & x->range[1], atom_getfloat( &argv[4] ) );
	}
	if( argc >= 6 )
	{
		SETFLOAT( & x->range[2], atom_getfloat( &argv[5] ) );
	}
	if( argc >= 7 )
	{
		SETFLOAT( & x->range[3], atom_getfloat( &argv[6] ) );
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
	AtomList_exit( & x->value );
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

void property_on_get_range(
	t_property* x,
	t_symbol *s,
	int argc,
	t_atom *argv
)
{
	if(
		// get <property>
		(
		x->type == PROPTYPE_FLOAT
		&&
		argc == 1
		&& atom_getsymbol( &argv[0] ) == x->name
		)
	||
		// get
		(
		x->type == PROPTYPE_FLOAT
		&&
		argc == 0
		)
	)
	{
		property_output_range(
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

void property_on_get_with_range(
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
		// output property value:
		property_output(
			x
		);
		// output property range info:
		
		if( x->type == PROPTYPE_FLOAT )
		{
			property_output_range(
				x
			);
		}
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

void property_on_set_in_range(
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
		 x->type != PROPTYPE_FLOAT
		)
		{
				char name_buf[256];
				t_atom name;
				SETSYMBOL( & name, x->name );
				atom_string( & name, name_buf, 255 );
				pd_error( x, "error in sdPropertySym %s: set_in_range only valid for number properties", name_buf );
				return;
		}
		if( argc != 2 || (argc == 2 && argv[1].a_type != A_FLOAT ) )
		{
			char name_buf[256];
			t_atom name;
			SETSYMBOL( & name, x->name );
			atom_string( & name, name_buf, 255 );
			pd_error( x, "error in sdProperty %s: type error! expected 'set_in_range <prop_name> <float>'", name_buf );
			return;
		}

		t_float min, max, step;
		min = atom_getfloat( &x->range[1] );
		max = atom_getfloat( &x->range[2] );
		step = atom_getfloat( &x->range[3] );
		t_float new_val = atom_getfloat( & argv[1] ) * (max - min);
		// rasterize by step:
		if( step != 0 )
		{
			new_val /= step;
			new_val = (int )new_val;
			new_val *= step;
		}
		new_val += min;
		// if the value didn't change, do nothing!:
		t_float old_val;
		old_val = atom_getfloat( AtomList_get_first( &x->value )->pData );
		if(
				old_val == new_val
		)
		{
			return;
		}
		t_atom new_val_atom;
		SETFLOAT( &new_val_atom, new_val );
		property_set(
				x,
				1,
				& new_val_atom
		);
		if( x->send_sym && x->send_sym->s_thing )
		{
			AtomEl* first = AtomList_get_first( & x->value );
			typedmess(
				x->send_sym->s_thing,
				&s_list,
				argc-1,
				first->pData
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

void property_on_set_min(
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
		 x->type != PROPTYPE_FLOAT
		)
		{
				char name_buf[256];
				t_atom name;
				SETSYMBOL( & name, x->name );
				atom_string( & name, name_buf, 255 );
				pd_error( x, "error in sdPropertySym %s: set_min only valid for number properties", name_buf );
				return;
		}
		if( argc != 2 || (argc == 2 && argv[1].a_type != A_FLOAT ) )
		{
			char name_buf[256];
			t_atom name;
			SETSYMBOL( & name, x->name );
			atom_string( & name, name_buf, 255 );
			pd_error( x, "error in sdProperty %s: type error! expected 'set_min <prop_name> <float>'", name_buf );
			return;
		}
		SETFLOAT( & x->range[1], atom_getfloat( &argv[1] ) );
		if( x->send_sym && x->send_sym->s_thing )
		{
			AtomEl* first = AtomList_get_first( & x->value );
			typedmess(
				x->send_sym->s_thing,
				&s_list,
				argc-1,
				first->pData
			);
		}
		property_output_range(
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

void property_on_set_max(
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
		 x->type != PROPTYPE_FLOAT
		)
		{
				char name_buf[256];
				t_atom name;
				SETSYMBOL( & name, x->name );
				atom_string( & name, name_buf, 255 );
				pd_error( x, "error in sdPropertySym %s: set_max only valid for number properties", name_buf );
				return;
		}
		if( argc != 2 || (argc == 2 && argv[1].a_type != A_FLOAT ) )
		{
			char name_buf[256];
			t_atom name;
			SETSYMBOL( & name, x->name );
			atom_string( & name, name_buf, 255 );
			pd_error( x, "error in sdProperty %s: type error! expected 'set_max <prop_name> <float>'", name_buf );
			return;
		}
		SETFLOAT( & x->range[2], atom_getfloat( &argv[1] ) );
		if( x->send_sym && x->send_sym->s_thing )
		{
			AtomEl* first = AtomList_get_first( & x->value );
			typedmess(
				x->send_sym->s_thing,
				&s_list,
				argc-1,
				first->pData
			);
		}
		property_output_range(
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

void property_on_set_step(
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
		 x->type != PROPTYPE_FLOAT
		)
		{
				char name_buf[256];
				t_atom name;
				SETSYMBOL( & name, x->name );
				atom_string( & name, name_buf, 255 );
				pd_error( x, "error in sdPropertySym %s: set_step only valid for number properties", name_buf );
				return;
		}
		if( argc != 2 || (argc == 2 && argv[1].a_type != A_FLOAT ) )
		{
			char name_buf[256];
			t_atom name;
			SETSYMBOL( & name, x->name );
			atom_string( & name, name_buf, 255 );
			pd_error( x, "error in sdProperty %s: type error! expected 'set_step <prop_name> <float>'", name_buf );
			return;
		}
		SETFLOAT( & x->range[3], atom_getfloat( &argv[1] ) );
		if( x->send_sym && x->send_sym->s_thing )
		{
			AtomEl* first = AtomList_get_first( & x->value );
			typedmess(
				x->send_sym->s_thing,
				&s_list,
				argc-1,
				first->pData
			);
		}
		property_output_range(
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

void property_on_set_no_out(
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
		int value_count = AtomList_get_size( &x->value );
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
			int value_count = AtomList_get_size( &x->value );
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
	AtomList_clear( & x->value );
	for(int i=0; i < new_value_count; i++)
	{
		t_atom* newVal = getbytes( sizeof( t_atom ) );
		*newVal = argv[i];
		AtomList_append( & x->value, newVal );
	}
}

void property_output(
	t_property* x
)
{
	// output "<property> <val> ..."
	int value_count = AtomList_get_size( &x->value );
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

void property_output_range(
	t_property* x
)
{
	// output "range_info <property> min max"
		outlet_anything(
		x->out,
		gensym("range_info"),
		4,
		x->range
	);
}
