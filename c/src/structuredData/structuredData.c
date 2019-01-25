#include "structuredData.h"
#include "Global.h"

#include "m_pd.h"


//----------------------------------
// sdAny
//----------------------------------

typedef struct _any {
  t_object x_obj;
	t_atom value;
} t_any;
static t_class* any_class;

void *any_new(
	t_symbol *s,
	int argc,
	t_atom *argv
);
void any_bang(t_any *x);
void any_set(
	t_any *x,
	t_symbol *s,
	int argc,
	t_atom *argv
);

void register_any(
	t_symbol* className
)
{
	t_class* class =
		class_new(
			className,
			(t_newmethod)any_new, // constructor
			0, // destructor
			sizeof(t_any),
			CLASS_DEFAULT, // graphical repr ?
			// creation arguments:
			A_GIMME,
			0
		);
	class_addbang(class, any_bang);
	class_addanything(class, any_set);
	any_class = class;
}

void *any_new(
	t_symbol *s,
	int argc,
	t_atom *argv
)
{
  t_any *x = (t_any *)pd_new(any_class);

	switch(argc)
	{
		case 0:
			SETFLOAT(&(x->value), 0.0);
			break;
		case 1:
			x->value = ( * argv );
			break;
		default:
			pd_error(x,"too many arguments: %u", argc);
	}
  inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_list, gensym("set"));
  outlet_new(&x->x_obj, &s_float);

  return (void *)x;
}

void any_bang(t_any *x)
{
  outlet_list(
		x->x_obj.ob_outlet,
		&s_list,
		1,
		&(x->value)
	);
}

void any_set(
	t_any *x,
	t_symbol *s,
	int argc,
	t_atom *argv
)
{
	if(s == &s_float || s == &s_symbol || s == &s_list)
	{
		x->value = argv[0];
		outlet_list(
			x->x_obj.ob_outlet,
			&s_list,
			1,
			&(x->value)
		);
	}
	else if( s == gensym("set") )
	{
		if( argc != 1 )
		{
			pd_error(x,"format: set <value>");
			return;
		}
		x->value = *argv;
	}
	else
	{
		t_atom sel;
		SETSYMBOL(&sel, s);
		char buf[256];
		atom_string(&sel, buf, 255);
		pd_error(x,"sdAny: no method for %s", buf);
	}
}

//----------------------------------
// sdGate
//----------------------------------

typedef struct _gate {
  t_object x_obj;
	t_float value;
	t_outlet* acceptOut;
	t_outlet* rejectOut;
} t_gate;

static t_class* gate_class;

void *gate_new(
	t_float init
);
void gate_in(
	t_gate *x,
	t_symbol *s,
	int argc,
	t_atom *argv
);

void register_gate(
	t_symbol* className
)
{
	t_class* class =
		class_new(
			className,
			(t_newmethod)gate_new, // constructor
			0, // destructor
			sizeof(t_gate),
			CLASS_DEFAULT, // graphical repr ?
			// creation arguments:
			A_DEFFLOAT,
			0
		);
	class_addanything(class, gate_in);
	gate_class = class;
}

void *gate_new(
	t_float init
)
{
  t_gate *x = (t_gate *)pd_new(gate_class);

	x -> value = init;
	floatinlet_new( & x->x_obj, & x->value);
  x-> acceptOut = outlet_new(&x->x_obj, &s_list);
  x-> rejectOut = outlet_new(&x->x_obj, &s_list);

  return (void *)x;
}

void gate_in(
	t_gate *x,
	t_symbol *s,
	int argc,
	t_atom *argv
)
{
	if( x->value != 0 )
	{
		outlet_anything(
			x->acceptOut,
			s,
			argc,
			argv
		);
	}
	else
	{
		outlet_anything(
			x->rejectOut,
			s,
			argc,
			argv
		);
	}
}

//----------------------------------
// counter
//----------------------------------

typedef struct _counter {
  t_object x_obj;
	t_float value;
	t_float limit;
	t_float autoreset;
	t_outlet* valueOut;
	t_outlet* overflowOut;
} t_counter;

static t_class* counter_class;

void* counter_new(
	t_float initLimit,
	t_float initAutoreset
);
void counter_set(
	t_counter *x,
	t_float newVal
);
void counter_bang(
	t_counter *x
);

void register_counter(
	t_symbol* className
)
{
	t_class* class =
		class_new(
			className,
			(t_newmethod)counter_new, // constructor
			0, // destructor
			sizeof(t_counter),
			CLASS_DEFAULT, // graphical repr ?
			// creation arguments:
			A_DEFFLOAT,
			A_DEFFLOAT,
			A_DEFFLOAT,
			0
		);
	class_addfloat(class, counter_set);
	class_addbang(class, counter_bang);
	counter_class = class;
}

void* counter_new(
	t_float initLimit,
	t_float initAutoreset
)
{
  t_counter *x = (t_counter *)pd_new(counter_class);

	x -> value = 0;
	x -> limit = initLimit;
	x -> autoreset = initAutoreset;

	floatinlet_new( & x->x_obj, & x->limit);

  x-> valueOut = outlet_new(&x->x_obj, &s_float);
  x-> overflowOut = outlet_new(&x->x_obj, &s_bang);

  return (void *)x;
}

void counter_set(
	t_counter *x,
	t_float newVal
)
{
	x -> value = newVal;
}

void counter_bang(
	t_counter *x
)
{
	if( x -> limit == -1 || x->value < x->limit )
	{
		outlet_float( x->valueOut, x->value );
		x -> value ++;
	}
	else if( x -> autoreset != 0 )
	{
		x -> value = 0;
		if( x -> limit == -1 || x->value < x->limit )
			outlet_float( x->valueOut, x->value );
		else
			outlet_bang( x->overflowOut );
		x -> value ++;
	}
	else
	{
		outlet_bang( x->overflowOut );
	}
}

//----------------------------------
// first
//----------------------------------

typedef struct _first {
  t_object x_obj;
	t_float reject;
	t_outlet* acceptOut;
	t_outlet* rejectOut;
} t_first;

static t_class* first_class;

void* first_new(
);
void first_bang(
	t_first *x
);
void first_reset(
	t_first *x
);

void register_first(
	t_symbol* className
)
{
	t_class* class =
		class_new(
			className,
			(t_newmethod)first_new, // constructor
			0, // destructor
			sizeof(t_first),
			CLASS_DEFAULT, // graphical repr ?
			// creation arguments:
			0
		);
	class_addbang(class, first_bang);
	class_addmethod(
		class,
		(t_method )first_reset,
		gensym("reset"),
		0
	);
	first_class = class;
}

void* first_new(
)
{
  t_first *x = (t_first *)pd_new(first_class);

	x -> reject = 0;

	inlet_new(
		& x->x_obj,
		& x->x_obj.ob_pd,
		gensym("bang"), // accept method
		gensym("reset")  // method to call
	);

  x-> acceptOut = outlet_new(&x->x_obj, &s_anything);
  x-> rejectOut = outlet_new(&x->x_obj, &s_anything);

  return (void *)x;
}

void first_bang(
	t_first *x
)
{
	if( x-> reject == 0 )
	{
		outlet_bang( x->acceptOut );
		x->reject = 1;
	}
	else
	{
		outlet_bang( x->rejectOut );
	}
}

void first_reset(
	t_first *x
)
{
	x->reject = 0;
}

//----------------------------------
// symbolIsEq
//----------------------------------

typedef struct _symbolIsEq {
  t_object x_obj;
	t_symbol* sym2;
} t_symbolIsEq;

static t_class* symbolIsEq_class;

void* symbolIsEq_new(
	t_symbol *s,
	int argc,
	t_atom *argv
);
void symbolIsEq_compare(
	t_symbolIsEq *x,
	t_symbol* sym
);

void register_symbolIsEq(
	t_symbol* className
)
{
	t_class* class =
		class_new(
			className,
			(t_newmethod)symbolIsEq_new, // constructor
			0, // destructor
			sizeof(t_symbolIsEq),
			CLASS_DEFAULT, // graphical repr ?
			// creation arguments:
			A_GIMME,
			0
		);
	class_addsymbol(class, symbolIsEq_compare);
	symbolIsEq_class = class;
}

void* symbolIsEq_new(
	t_symbol *s,
	int argc,
	t_atom *argv
)
{
  t_symbolIsEq *x = (t_symbolIsEq *)pd_new(symbolIsEq_class);

	if( argc == 0 )
		x -> sym2 = gensym("");
	else if( argc == 1 )
		x -> sym2 = atom_getsymbol( &argv[0] );
	else
	{
		pd_error(x,"too many arguments!");
		return NULL;
	}

	symbolinlet_new(
		& x->x_obj,
		& x->sym2
	);
  outlet_new(&x->x_obj, &s_float);

  return (void *)x;
}

void symbolIsEq_compare(
	t_symbolIsEq *x,
	t_symbol* sym
)
{
	//post("hello");
	if( sym == x -> sym2 )
		outlet_float( x->x_obj.ob_outlet, 1.0 );
	else
		outlet_float( x->x_obj.ob_outlet, 0.0 );
}

//----------------------------------
// isEq
//----------------------------------

typedef struct _isEq {
  t_object x_obj;
	t_atom other;
} t_isEq;

static t_class* isEq_class;

void* isEq_new(
	t_symbol *s,
	int argc,
	t_atom *argv
);

void isEq_compare(
	t_isEq *x,
	t_symbol *s,
	int argc,
	t_atom *argv
);

void isEq_setOther(
	t_isEq *x,
	t_symbol *s,
	int argc,
	t_atom *argv
);

void register_isEq(
	t_symbol* className
)
{
	t_class* class =
		class_new(
			className,
			(t_newmethod)isEq_new, // constructor
			0, // destructor
			sizeof(t_isEq),
			CLASS_DEFAULT, // graphical repr ?
			// creation arguments:
			A_GIMME,
			0
		);
	class_addlist(class, isEq_compare);
	class_addmethod(
		class,
		(t_method )isEq_setOther,
		gensym("set"),
		A_GIMME,
		0
	);
	isEq_class = class;
}

void* isEq_new(
	t_symbol *s,
	int argc,
	t_atom *argv
)
{
  t_isEq *x = (t_isEq *)pd_new(isEq_class);

	if( argc == 0 )
		SETSYMBOL( & x->other, gensym(""));
	else if( argc == 1 )
		x -> other = argv[0];
	else
	{
		pd_error(x,"too many arguments!");
		return NULL;
	}

	inlet_new(
		& x->x_obj,
		& x->x_obj.ob_pd,
		gensym("list"), // accept method
		gensym("set")  // method to call
	);

  outlet_new(&x->x_obj, &s_float);

  return (void *)x;
}

void isEq_compare(
	t_isEq *x,
	t_symbol *s,
	int argc,
	t_atom *argv
)
{
	if( argc == 0 )
	{
		pd_error(x, "to few parameters!");
		return;
	}
	if( compareAtoms( & x->other, & argv[0] )  )
	{
		outlet_float( x->x_obj.ob_outlet, 1.0 );
	}
	else
		outlet_float( x->x_obj.ob_outlet, 0.0 );
}

void isEq_setOther(
	t_isEq *x,
	t_symbol *s,
	int argc,
	t_atom *argv
)
{
	if( argc == 0 )
	{
		pd_error(x, "to few parameters!");
		return;
	}
	x -> other = argv[0];
}

//----------------------------------
// filter
//----------------------------------

typedef struct _filter {
  t_object x_obj;
	t_atom other;
	t_outlet* acceptOut;
	t_outlet* rejectOut;
} t_filter;

static t_class* filter_class;

void* filter_new(
	t_symbol *s,
	int argc,
	t_atom *argv
);

void filter_input(
	t_filter *x,
	t_symbol *s,
	int argc,
	t_atom *argv
);

void filter_setOther(
	t_filter *x,
	t_symbol *s,
	int argc,
	t_atom *argv
);

void register_filter(
	t_symbol* className
)
{
	t_class* class =
		class_new(
			className,
			(t_newmethod)filter_new, // constructor
			0, // destructor
			sizeof(t_filter),
			CLASS_DEFAULT, // graphical repr ?
			// creation arguments:
			A_GIMME,
			0
		);
	// "methods":
	class_addlist(class, filter_input);
	class_addmethod(
		class,
		(t_method )filter_setOther,
		gensym("set"),
		A_GIMME,
		0
	);
	filter_class = class;
}

void* filter_new(
	t_symbol *s,
	int argc,
	t_atom *argv
)
{
  t_filter *x = (t_filter *)pd_new(filter_class);

	if( argc == 0 )
		SETSYMBOL( & x->other, gensym(""));
	else if( argc == 1 )
		x -> other = argv[0];
	else
	{
		pd_error(x,"too many arguments!");
		return NULL;
	}

	inlet_new(
		& x->x_obj,
		& x->x_obj.ob_pd,
		gensym("list"), // accept method
		gensym("set")  // method to call
	);

  x->acceptOut = outlet_new(&x->x_obj, &s_list);
  x->rejectOut = outlet_new(&x->x_obj, &s_list);

  return (void *)x;
}

void filter_input(
	t_filter *x,
	t_symbol *s,
	int argc,
	t_atom *argv
)
{
	if( argc == 0 )
	{
		pd_error(x, "to few parameters!");
		return;
	}
	if( compareAtoms( & x->other, & argv[0] )  )
	{
		outlet_list(
			x->acceptOut,
			s,
			argc,
			argv
		);
	}
	else
		outlet_list(
			x->rejectOut,
			s,
			argc,
			argv
		);
}

void filter_setOther(
	t_filter *x,
	t_symbol *s,
	int argc,
	t_atom *argv
)
{
	if( argc == 0 )
	{
		pd_error(x, "to few parameters!");
		return;
	}
	x -> other = argv[0];
}

void structuredDataC_setup()
{
	register_any( gensym("sdAny") );
	register_gate(gensym("sdGate"));
	register_counter(gensym("sdCounter"));
	register_first(gensym("sdFirst"));
	register_symbolIsEq(gensym("sdSymbolIsEq"));
	register_isEq(gensym("sdIsEq"));
	register_filter(gensym("sdFilter"));
	sdList_setup();
	sdPack_setup();
	sdData_setup();
	sdEvent_setup();
	sdObjState_setup();
}
