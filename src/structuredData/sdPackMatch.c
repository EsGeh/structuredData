#include "sdPackMatch.h"
#include "Global.h"
#include "Map.h"

#include "sdScript.h"


#include <string.h>

#include "m_pd.h"

#define BOUNDDATA_SIZE 1024

#define CHARBUF_SIZE 1024

static t_class* packMatch_class;;

t_class* register_packMatch(
	t_symbol* className
);

void sdPackMatch_setup()
{
	packMatch_class = register_packMatch( gensym("sdPackMatch") );
}

//----------------------------------
// packMatch
//----------------------------------

typedef struct
{
	t_symbol* name;
	AtomBuf pattern;
	BOOL has_program;
	AtomBuf program;
} PatternInfo;

#define FREE_PATTTERN_INFO(x,size) \
{ \
	AtomBuf_exit( & x->pattern ); \
	AtomBuf_exit( & x->program ); \
	freebytes( x, size ); \
}

DECL_LIST(PatternList, PatternEl, PatternInfo, getbytes, freebytes, FREE_PATTTERN_INFO)
DEF_LIST(PatternList, PatternEl, PatternInfo, getbytes, freebytes, FREE_PATTTERN_INFO)

// the actual pd object:

typedef struct s_packMatch {
  t_object x_obj;
	PatternList patterns;
	t_outlet* outlet;
	t_outlet* outlet_reject;
} t_packMatch;

void* packMatch_init(
	t_symbol *s,
	int argc,
	t_atom *argv
);
void packMatch_exit(
	struct s_packMatch* x
);

void packMatch_patterns_add(
	t_packMatch* x,
	t_symbol *s,
	int argc,
	t_atom *argv
);

void packMatch_patterns_add_script(
	t_packMatch* x,
	t_symbol *s,
	int argc,
	t_atom *argv
);

void packMatch_patterns_clear(
	t_packMatch* x
);

void packMatch_input(
	t_packMatch* x,
	t_symbol *s,
	int argc,
	t_atom *argv
);

// internal utility types:

// needed for "pack mode"
typedef struct {
	int pattern_pos;
	int pattern_size;
	CharBuf bind_sym;
} SubPatternInfo;

#define FREE_SUBPATTERN_INFO( x, size) \
{ \
	CharBuf_exit( & x-> bind_sym ); \
	freebytes( x, size ); \
}

DECL_LIST(SubPatternsList, SubPatternsListEl, SubPatternInfo, getbytes, freebytes, FREE_SUBPATTERN_INFO)
DEF_LIST(SubPatternsList, SubPatternsListEl, SubPatternInfo, getbytes, freebytes, FREE_SUBPATTERN_INFO)

#define DEL_ATOM_DYNA( x , size ) \
{ \
	AtomDynA_exit( x ); \
	freebytes( x, size ); \
}

// #define HASH_SYMBOL( x ) ((unsigned int ) x)

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpointer-to-int-cast"
DECL_MAP(BoundData,t_symbol*, AtomDynA,getbytes, freebytes, DEL_ATOM_DYNA, HASH_SYMBOL, COMPARE_SYMBOLS)
DEF_MAP(BoundData,t_symbol*, AtomDynA,getbytes, freebytes, DEL_ATOM_DYNA, HASH_SYMBOL, COMPARE_SYMBOLS)
#pragma GCC diagnostic pop


// all the variables needed during "runtime":
typedef struct s_RuntimeInfo {
	t_packMatch* x;
	t_symbol* pattern_name;

	t_atom* pattern;
	int pattern_size;
	int pattern_pos;

	t_atom* input;
	int input_size;
	int input_pos;

	// binding utilities:
	int bind_start_level;
	// if bind sym, the buffer to take the name to bind to ("" otherwise)
	CharBuf bind_sym;

	BoundData bind_ret;
	
	int rec_depth;
} RuntimeInfo;


BOOL match(
		RuntimeInfo* rt
);

void bind_ret_to_atom_list(
		BoundData* x,
		AtomDynA* ret
);

void bind_ret_to_scope(
		BoundData* x,
		Scope* scope
);

// actually try to match the input with the pattern
BOOL match_rec(
		RuntimeInfo* rt
);

// lower level:

// in arbitrary mode, we accept every sequence of atoms
BOOL arbitrary_mode(
		RuntimeInfo* rt
);

// in strict mode, we require the input to match exactly to the pattern.
// stuff between " ( ... ) " is checked recursively
BOOL strict_mode(
		RuntimeInfo* rt
);

// in pack mode, we require the input to contain certain packages
// ( in any order ):
BOOL pack_mode(
		RuntimeInfo* rt
);

// pack mode utilities:

BOOL pack_mode_read_pattern(
		RuntimeInfo* rt,
		SubPatternsList* expected_packages
);

BOOL pack_mode_match_input(
		RuntimeInfo* rt,
		SubPatternsList* expected_packages,
		char* bind_arbitrary_sym  // if != "": bind non required packs to this symboL
);

void pack_mode_merge_bound_symbols(
		RuntimeInfo* rt,
		BoundData* new_bound_symbols
);

// ************************
// lexer:
// ************************

typedef enum e_pattern_atom_type {
	FLOAT,
	SYMBOL,
	LEFT_PARENT, // (
	RIGHT_PARENT, // )
	ANY_SYM, // ?
	ANY_SYM_BIND, // ?<bind>
	START_BIND, // <bind>#[
	END_BIND, // #]
	STAR, // *
	END // eof
} t_pattern_atom_type;

// of which kind is the current pattern token?
t_pattern_atom_type lexer_pattern_peek(
		RuntimeInfo* rt,
		int ahead // 0: current, 1,2,3 ...: look ahead
);

// ignorantly consume next pattern token of a specific kind
BOOL lexer_pattern_next_tok(
		RuntimeInfo* rt,
		t_pattern_atom_type expected
);

// ignorantly consume next pattern token
BOOL lexer_pattern_next_tok_any(
		RuntimeInfo* rt
);

// try to read next input symbol of a specific kind (raise an error otherwise)
// take into account the current pattern,
// maybe advance pattern position too.
BOOL lexer_match_next(
		RuntimeInfo* rt,
		BOOL anything,
		t_pattern_atom_type expected_symbol
);

// read next input symbol, taking into account the current pattern,
// also advance through the pattern
BOOL lexer_match_next_any(
		RuntimeInfo* rt,
		BOOL anything // "* mode" - don't consider pattern
);

// ignorantly advance input
BOOL lexer_input_next_tok(
		RuntimeInfo* rt,
		BOOL bind // consider binding
);

// if input == "<bind>#[ ..." match "<bind>#["
// and start binding input to "<bind>"
BOOL lexer_match_start_bind(
		RuntimeInfo* rt
);

// if input == "#] ...", match "#]"
// and stop binding input
BOOL lexer_match_end_bind(
		RuntimeInfo* rt
);

// lower level:

// start binding input to some symbol
BOOL lexer_set_start_bind(
		RuntimeInfo* rt,
		char* bind_sym // bind to this symbol
);

// stop binding input
BOOL lexer_set_end_bind(
		RuntimeInfo* rt
);

// ignorantly append to bind return list
BOOL lexer_append_to_bind(
		RuntimeInfo* rt,
		t_atom* a
);

// utility mainly for debug output:
void pattern_atom_type_to_str(
		t_pattern_atom_type type,
		char* buf,
		int buf_size
);

t_class* register_packMatch(
	t_symbol* className
)
{
	t_class* class =
		class_new(
			className,
			(t_newmethod )packMatch_init, // constructor
			(t_method )packMatch_exit, // destructor
			sizeof(t_packMatch),
			CLASS_DEFAULT, // graphical repr ?
			// creation arguments:
			A_GIMME,
			0
		);
	class_sethelpsymbol(
			class,
			gensym("sdPackMatch")
	);

	class_addlist( class, packMatch_input );

	class_addmethod(
			class,
			(t_method )packMatch_patterns_add,
			gensym("add"),
			A_GIMME,
			0
	);

	class_addmethod(
			class,
			(t_method )packMatch_patterns_add_script,
			gensym("add_script"),
			A_GIMME,
			0
	);

	class_addmethod(
			class,
			(t_method )packMatch_patterns_clear,
			gensym("clear"),
			A_GIMME,
			0
	);

	return class;
}

void* packMatch_init(
	t_symbol *s,
	int argc,
	t_atom *argv
)
{
  t_packMatch *x = (t_packMatch *)pd_new(packMatch_class);

	PatternList_init( & x-> patterns );

	if( argc > 0  )
	{
		pd_error(x, "too many arguments!");
		return NULL;
	}
	x->outlet =
		outlet_new( & x->x_obj, &s_list);
	x -> outlet_reject =
		outlet_new( & x->x_obj, &s_list);

  return (void *)x;
}

void packMatch_exit(
	t_packMatch* x
)
{
	PatternList_exit( & x-> patterns );
}

void packMatch_patterns_add(
	t_packMatch* x,
	t_symbol *s,
	int argc,
	t_atom *argv
)
{
	PatternInfo* info = getbytes( sizeof( PatternInfo ) );
	info->name = atom_getsymbol( &argv[0] );
	info->has_program = FALSE;
	AtomBuf_init( & info->pattern, argc - 1);
	memcpy(
			AtomBuf_get_array( & info->pattern ),
			& argv[1],
			sizeof( t_atom ) * (argc-1)
	);
	PatternList_append(
			& x->patterns,
			info
	);
}

void packMatch_patterns_add_script(
	t_packMatch* x,
	t_symbol *s,
	int argc,
	t_atom *argv
)
{
	// syntax: <pattern_name> ( ...pattern... ) ( ...script... )
	int pattern_pos = -1;
	int pattern_size = -1;
	int script_pos = -1;
	int script_size = -1;
	{
		int pos = 1;
		// read pattern:
		if(
				atom_getsymbol( &argv[1] ) != gensym("(")
				&& pos < argc
		)
		{
			pd_error( x, "invalid input: at pos %i expected '('", pos);
			return;
		}
		pos ++;
		pattern_pos = pos;
		int depth = 1;
		while(
				depth >=1
				&& pos < argc
		)
		{
			if( atom_getsymbol( &argv[pos] ) == gensym("(") )
				depth ++;
			else if( atom_getsymbol( &argv[pos] ) == gensym(")") )
				depth --;
			pos ++;
		}
		if( depth != 0 )
		{
			pd_error( x, "missing ')' after '('" );
			return;
		}
		pattern_size = pos - 2 - pattern_pos + 1;

		// read script
		if(
				atom_getsymbol( &argv[1] ) != gensym("(")
				&& pos < argc
		)
		{
			pd_error( x, "invalid input: at pos %i expected '('", pos);
			return;
		}
		pos ++;
		script_pos = pos;
		depth = 1;
		while(
				depth >=1
				&& pos < argc
		)
		{
			if( atom_getsymbol( &argv[pos] ) == gensym("(") )
				depth ++;
			else if( atom_getsymbol( &argv[pos] ) == gensym(")") )
				depth --;
			pos ++;
		}
		if( depth != 0 )
		{
			pd_error( x, "missing ')' after '('" );
			return;
		}
		script_size = pos - 2 - script_pos + 1;
	}

	DB_PRINT( "pattern: (%i,%i), script: (%i,%i)",
			pattern_pos, pattern_size,
			script_pos, script_size
	);
	PatternInfo* info = getbytes( sizeof( PatternInfo ) );
	info->name = atom_getsymbol( &argv[0] );
	AtomBuf_init( & info->pattern, pattern_size);
	AtomBuf_init( & info->program, script_size);
	memcpy(
			AtomBuf_get_array( & info->pattern ),
			& argv[pattern_pos],
			sizeof( t_atom ) * pattern_size
	);
	memcpy(
			AtomBuf_get_array( & info->program ),
			& argv[script_pos],
			sizeof( t_atom ) * (script_size)
	);
	info->has_program = TRUE;
	PatternList_append(
			& x->patterns,
			info
	);
}

void packMatch_patterns_clear(
	t_packMatch* x
)
{
	PatternList_clear(
			& x->patterns
	);
}

void packMatch_input(
	t_packMatch* x,
	t_symbol *s,
	int argc,
	t_atom *argv
)
{
	DB_PRINT( "packMatch_input" );
	BOOL match_found = FALSE;
	LIST_FORALL_BEGIN(PatternList, PatternEl, PatternInfo, & x->patterns, i_pattern, patternEl)
		PatternInfo* patternInfo = patternEl->pData;

		RuntimeInfo rt = {
			.x = x,
			.pattern_name = patternInfo->name,
			.pattern = AtomBuf_get_array( &patternInfo->pattern ),
			.pattern_size = AtomBuf_get_size( &patternInfo->pattern ),
			.pattern_pos = 0,
			.input = argv,
			.input_size = argc,
			.input_pos = 0,
			.bind_start_level = -1,
			.rec_depth = 0
		};
		CharBuf_init( & rt.bind_sym, 1024 );
		strcpy( CharBuf_get_array( & rt.bind_sym ), "" );
		BoundData_init( & rt.bind_ret, BOUNDDATA_SIZE);
		if(
				match(
					&rt
				)
		)
		{
			if( ! patternInfo -> has_program )
			{
				AtomDynA bind_ret_list;
				AtomDynA_init( & bind_ret_list );
				{
					t_atom atom;
					SETSYMBOL( & atom, rt.pattern_name );
					AtomDynA_append(
							& bind_ret_list,
							atom
					);
				}
				{
					t_atom atom;
					SETSYMBOL( & atom, gensym("size") );
					AtomDynA_append(
							& bind_ret_list,
							atom
					);
				}

				bind_ret_to_atom_list(
					& rt.bind_ret,
					& bind_ret_list
				);

				SETFLOAT(
						& AtomDynA_get_array( & bind_ret_list )[1],
						AtomDynA_get_size( & bind_ret_list ) - 2
				);

				outlet_list(
					x->outlet,
					& s_list,
					AtomDynA_get_size( &bind_ret_list ),
					AtomDynA_get_array( &bind_ret_list )
				);
				AtomDynA_exit( & bind_ret_list );
			}
			else
			{
				ProgramMap programs;
				ProgramMap_init( &programs, 1);
				AtomBuf* bang_prog = getbytes( sizeof( AtomBuf ) );
				AtomBuf_init( bang_prog,
						AtomBuf_get_size( & patternInfo->program )
				);
				memcpy(
					AtomBuf_get_array( bang_prog ),
					AtomBuf_get_array( & patternInfo -> program ),
					sizeof( t_atom ) * AtomBuf_get_size( & patternInfo -> program )
				);
				ProgramMap_insert(
						&programs,
						gensym("bang"),
						bang_prog
				);
				ScriptData script;
				Script_init( &script,
					& programs,
					x->outlet,
					NULL
				);

				bind_ret_to_scope(
					& rt.bind_ret,
					Script_get_global_scope( &script )
				);
				{
					Script_exec( & script, gensym("bang") );
				}
				Script_exit( & script );
				ProgramMap_exit( &programs );
			}
			match_found = TRUE;
		}
		CharBuf_exit( & rt.bind_sym );
		BoundData_exit( & rt.bind_ret );
	LIST_FORALL_END(PatternList, PatternEl, PatternInfo, & x->patterns, i_pattern, patternEl)
	if( !match_found )
	outlet_bang(
			x->outlet_reject
	);
	DB_PRINT( "packMatch_input: done" );
}

#define match_db_print( \
		rt, \
		msg, ... \
) \
{ \
	char pattern_name_buf[CHARBUF_SIZE]; \
	{ \
		t_atom pattern_name_atom; \
		SETSYMBOL( & pattern_name_atom, rt->pattern_name ); \
		atom_string( &pattern_name_atom, pattern_name_buf, CHARBUF_SIZE ); \
	} \
	char pattern_buf[CHARBUF_SIZE]; \
	{ \
		sprintf( pattern_buf, "'"); \
		for(int i=rt->pattern_pos; i< rt->pattern_size; i++) \
		{ \
			t_atom* current = & rt->pattern[ i ]; \
			char current_buf[CHARBUF_SIZE]; \
			atom_string( current, current_buf, CHARBUF_SIZE ); \
			if( i != 0 ) \
				strcat( \
						pattern_buf, \
						" " \
				); \
			strcat( \
					pattern_buf, \
					current_buf \
			); \
		} \
		strcat( \
				pattern_buf, \
				"'" \
		); \
	} \
	char input_buf[CHARBUF_SIZE]; \
	{ \
		sprintf( input_buf, "'"); \
		for(int i=rt->input_pos; i< rt->input_size; i++) \
		{ \
			t_atom* current = & rt->input[ i ]; \
			char current_buf[CHARBUF_SIZE]; \
			atom_string( current, current_buf, CHARBUF_SIZE ); \
			if( i != 0 ) \
				strcat( \
						input_buf, \
						" " \
				); \
			strcat( \
					input_buf, \
					current_buf \
			); \
		} \
		strcat( \
				input_buf, \
				"'" \
		); \
	} \
 \
	char indent_buf[256]; \
	indent_buf[0] ='\0'; \
	for(int i=0; i<rt->rec_depth; i++) \
	{ \
		strcat(indent_buf, "-"); \
	} \
	char msg_filled[CHARBUF_SIZE]; \
	sprintf( msg_filled, msg, ## __VA_ARGS__ ); \
 \
	DB_PRINT( \
			"%s%s, input (%i,%i): %s, pattern (%i,%i): %s: %s", \
			indent_buf, \
			pattern_name_buf, \
			rt->input_pos, rt->input_size, input_buf, \
			rt->pattern_pos, rt->pattern_size, pattern_buf, \
			msg_filled \
	); \
}

#define match_error( \
		rt, \
		msg, ... \
) \
{ \
	char pattern_name_buf[CHARBUF_SIZE]; \
	{ \
		t_atom pattern_name_atom; \
		SETSYMBOL( & pattern_name_atom, rt->pattern_name ); \
		atom_string( &pattern_name_atom, pattern_name_buf, CHARBUF_SIZE ); \
	} \
	char pattern_buf[CHARBUF_SIZE]; \
	{ \
		sprintf( pattern_buf, "'"); \
		for(int i=rt->pattern_pos; i< rt->pattern_size; i++) \
		{ \
			t_atom* current = & rt->pattern[ i ]; \
			char current_buf[CHARBUF_SIZE]; \
			atom_string( current, current_buf, CHARBUF_SIZE ); \
			if( i != 0 ) \
				strcat( \
						pattern_buf, \
						" " \
				); \
			strcat( \
					pattern_buf, \
					current_buf \
			); \
		} \
		strcat( \
				pattern_buf, \
				"'" \
		); \
	} \
	char input_buf[CHARBUF_SIZE]; \
	{ \
		sprintf( input_buf, "'"); \
		for(int i=rt->input_pos; i< rt->input_size; i++) \
		{ \
			t_atom* current = & rt->input[ i ]; \
			char current_buf[CHARBUF_SIZE]; \
			atom_string( current, current_buf, CHARBUF_SIZE ); \
			if( i != 0 ) \
				strcat( \
						input_buf, \
						" " \
				); \
			strcat( \
					input_buf, \
					current_buf \
			); \
		} \
		strcat( \
				input_buf, \
				"'" \
		); \
	} \
 \
	char indent_buf[256]; \
	indent_buf[0] ='\0'; \
	for(int i=0; i<rt->rec_depth; i++) \
	{ \
		strcat(indent_buf, "-"); \
	} \
	char msg_filled[4000]; \
	sprintf( msg_filled, msg, ## __VA_ARGS__ ); \
 \
	pd_error( \
			rt -> x, \
			"%s%s, input (%i,%i): %s, pattern (%i,%i): %s: ERROR: %s", \
			indent_buf, \
			pattern_name_buf, \
			rt->input_pos, rt->input_size, input_buf, \
			rt->pattern_pos, rt->pattern_size, pattern_buf, \
			msg_filled \
	); \
}

BOOL match(
		RuntimeInfo* rt
)
{
	BOOL ret = match_rec(
		rt
	);

	if(
			ret
			&& rt->pattern_pos < rt->pattern_size
	)
	{
		char buf[256];
		t_atom pattern_name_atom;
		SETSYMBOL( & pattern_name_atom, rt->pattern_name );
		atom_string( &pattern_name_atom, buf, 256 );
		DB_PRINT(
				"ERROR in pattern %s at pos %i: unmatched rest!",
				buf,
				rt->input_pos
		);
		return FALSE;
	}
	return ret;
}

void bind_ret_to_atom_list(
		BoundData* x,
		AtomDynA* ret
)
{
	MAP_FORALL_KEYS_BEGIN(BoundData,t_symbol*,x,sym)
		AtomDynA* content = BoundData_get( x, sym );
		{
			t_atom atom;
			SETSYMBOL( & atom, sym );
			AtomDynA_append(
					ret,
					atom
			);
		}
		{
			t_atom atom;
			SETFLOAT( & atom, AtomDynA_get_size( content ) );
			AtomDynA_append(
					ret,
					atom
			);
		}
		for(int i=0; i< AtomDynA_get_size( content ); i++)
		{
			AtomDynA_append(
					ret,
					AtomDynA_get_array( content )[i]
			);
		}
	MAP_FORALL_KEYS_END(BoundData,t_symbol*,x,sym)
}

void bind_ret_to_scope(
		BoundData* x,
		Scope* scope
)
{
	MAP_FORALL_KEYS_BEGIN(BoundData,t_symbol*,x,sym)
		AtomDynA* content = BoundData_get( x, sym );
		AtomDynA* var = getbytes( sizeof ( AtomDynA ) );
		AtomDynA_init( var );
		AtomDynA_set_size( var, AtomDynA_get_size( content ) );
		memcpy(
			AtomDynA_get_array( var ),
			AtomDynA_get_array( content ),
			sizeof( t_atom ) * AtomDynA_get_size( content )
		);
		Scope_insert(
				scope,
				sym,
				var
		);
	MAP_FORALL_KEYS_END(BoundData,t_symbol*,x,sym)
}

// actually try to match the input with the pattern
BOOL match_rec(
		RuntimeInfo* rt
)
{
	match_db_print(
			rt,
			"start"
	);

	// match NON strict "* mode"
	if(
			// "* ..."
			lexer_pattern_peek( rt, 0) == STAR
			|| 
			// "<bind>#[ * ..."
			( lexer_pattern_peek( rt, 0) == START_BIND && lexer_pattern_peek( rt, 1) == STAR )
	)
	{

		// if '*' was the last symbol in the pattern:
		if(
			// "* eof" || "* )" || ...
			lexer_pattern_peek( rt, 0) == STAR
				&& (
					(lexer_pattern_peek( rt, 1) == END || lexer_pattern_peek( rt, 1) == RIGHT_PARENT)
					||
					( lexer_pattern_peek( rt, 1) == START_BIND || lexer_pattern_peek( rt, 1) == END_BIND )
						&& (lexer_pattern_peek( rt, 2) == END || lexer_pattern_peek( rt, 2) == RIGHT_PARENT)
				)
			||
			// "? * eof" || ? "* )"  || ...
			lexer_pattern_peek( rt, 1) == STAR
				&& (
					(lexer_pattern_peek( rt, 2) == END || lexer_pattern_peek( rt, 2) == RIGHT_PARENT)
					||
					( lexer_pattern_peek( rt, 2) == START_BIND || lexer_pattern_peek( rt, 2) == END_BIND )
						&& (lexer_pattern_peek( rt, 3) == END || lexer_pattern_peek( rt, 3) == RIGHT_PARENT)
				)
		)
		{
			match_db_print(
					rt,
					"* arbitrary mode!"
			);
			return arbitrary_mode( rt );
		}
		else
		{
			match_db_print(
					rt,
					"* pack mode!"
			);
			return pack_mode( rt );
		}
	}
	// match strict:
	else
	{
		return strict_mode( rt );
	}

	match_db_print(
			rt,
			"SUCCESS"
	);

	return TRUE;

}

// lower level:

// in arbitrary mode, we accept every sequence of atoms
BOOL arbitrary_mode(
		RuntimeInfo* rt
)
{
	// eat '<bind>#['
	if(
		!lexer_match_start_bind( rt )
	)
		return FALSE;

	// now we know the input begins with '*' ...

	// eat '*'
	if(
			!lexer_pattern_next_tok(
				rt,
				STAR
			)
	)
		return FALSE;

	// ARBITRARY CONTENT:
	while(
		rt->input_pos < rt->input_size
	)
	{
		if(
				!lexer_match_next_any(
					rt,
					TRUE // match anything
				)
		)
			return FALSE;
	}
	if( rt->bind_start_level == rt->rec_depth )
	{
		if(
				lexer_pattern_peek( rt, 0 ) != END_BIND
		)
		{
			match_error(
					rt,
					"#] expected!"
			);
			return FALSE;
		}
	}
	if( !lexer_match_end_bind( rt ) ) return FALSE;
	return TRUE;
}


// in strict mode, we require the input to match exactly to the pattern.
// stuff between " ( ... ) " is checked recursively
BOOL strict_mode(
		RuntimeInfo* rt
)
{
	while(
			lexer_pattern_peek( rt, 0) != END
			&& lexer_pattern_peek( rt, 0) != RIGHT_PARENT
			&& rt->input_pos < rt->input_size
	)
	{
		// ARBITRARY CONTENT:
		while(
			lexer_pattern_peek( rt, 0) != END
			&&
			lexer_pattern_peek( rt, 0) != LEFT_PARENT
			&&
			lexer_pattern_peek( rt, 0) != RIGHT_PARENT
			&&
			rt->input_pos < rt->input_size
		)
		{
			lexer_match_start_bind( rt );
			if(
					!lexer_match_next_any(
						rt,
						FALSE // ignore strictness?
					)
			)
				return FALSE;
			lexer_match_end_bind( rt );
		}
		// '(' .. ')'
		if(
			lexer_pattern_peek( rt, 0) == LEFT_PARENT
		)
		{
			if(
					rt->input_pos >= rt->input_size
					||
					rt->input[rt->input_pos].a_type != A_FLOAT
			)
			{
				match_db_print(
						rt,
						"input: expected 'float'"
				);
				return FALSE;
			}
			int content_size =
				atom_getint( & rt->input[rt->input_pos] );

			// eat '(':
			if(
					!lexer_match_next(
						rt,
						FALSE,
						LEFT_PARENT
					)
			)
				return FALSE;

			int old_input_size = rt->input_size;
			rt->rec_depth ++;
			rt-> input_size = rt->input_pos + content_size;
			if(
				! match_rec(
					rt
				)
			)
			{
				rt->rec_depth --;
				match_db_print(
						rt,
						"FAIL: (subpattern fail)"
				);
				return FALSE;
			}
			rt->rec_depth --;
			rt->input_size = old_input_size;
				
			// eat ')':
			if(
					!lexer_match_next(
						rt,
						FALSE, // forget strictness?
						RIGHT_PARENT
					)
			)
				return FALSE;
		}
		// end of '(' .. ')'
		if( !lexer_match_end_bind( rt ) )
			return FALSE;
	}

	// some input wasn't matched!:
	if( rt->input_pos < rt->input_size )
	{
		match_db_print(
				rt,
				"FAIL: input rest"
		);
		return FALSE;
	}

	// no match for rest input
	if(
			lexer_pattern_peek( rt, 0 ) != END
			&&
			lexer_pattern_peek( rt, 0 ) != RIGHT_PARENT
	)
	{
		match_db_print(
				rt,
				"FAIL: pattern rest"
		);
		return FALSE;
	}
	return TRUE;
}

// in pack mode, we require the input to contain certain packages
// ( in any order ):
BOOL pack_mode(
		RuntimeInfo* rt
)
{
	// we know '<bind>#[ * ...' or '* ...'
	
	// if the '*' is between #[ ... #], we set this
	// variable to the name to bind to ("" otherwise)
	CharBuf bind_arbitrary_sym;
	CharBuf_init( & bind_arbitrary_sym, CHARBUF_SIZE);
	strcpy(
			CharBuf_get_array( & bind_arbitrary_sym ),
			""
	);

	// #[ ...
	if( lexer_pattern_peek( rt, 0 ) == START_BIND )
	{
		if( !lexer_match_start_bind( rt ) )
		{
			CharBuf_exit( & bind_arbitrary_sym );
			return FALSE;
		}
		strcpy(
			CharBuf_get_array( & bind_arbitrary_sym ),
			CharBuf_get_array( & rt->bind_sym )
		);
	}
	// '* ...'
	if( !lexer_pattern_next_tok_any( rt ) )
	{
		CharBuf_exit( & bind_arbitrary_sym );
		return FALSE;
	}
	// '#] ...'
	if( lexer_pattern_peek( rt, 0 ) == END_BIND )
	{
		if( !lexer_match_end_bind( rt ) )
		{
			CharBuf_exit( & bind_arbitrary_sym );
			return FALSE;
		}
	}

	// 1. read all packages in the pattern:
	SubPatternsList expected_packages;
	SubPatternsList_init ( & expected_packages );
	if( !pack_mode_read_pattern( rt, & expected_packages ) )
	{
		CharBuf_exit( & bind_arbitrary_sym );
		SubPatternsList_exit( & expected_packages );
		return FALSE;
	}
	match_db_print(
			rt,
			"done reading pattern"
	);
	
	// debug print:
	SubPatternsListEl* pEl = SubPatternsList_get_first( &expected_packages );
	while( pEl != NULL )
	{
		SubPatternInfo* subPatternInfo = pEl->pData;
		match_db_print(
				rt,
				"pack: pos: %i, size: %i, bind: %s",
				subPatternInfo->pattern_pos,
				subPatternInfo->pattern_size,
				CharBuf_get_array( & subPatternInfo->bind_sym )
		);
		pEl = SubPatternsList_get_next(
				& expected_packages,
				pEl
		);
	}

	// 2. try to find all patterns in the input:
	if( !pack_mode_match_input( rt, & expected_packages, CharBuf_get_array( & bind_arbitrary_sym ) ) )
	{
		CharBuf_exit( & bind_arbitrary_sym );
		SubPatternsList_exit( & expected_packages );
		return FALSE;
	}

	CharBuf_exit( & bind_arbitrary_sym );
	SubPatternsList_exit( & expected_packages );
	return TRUE;
}

BOOL pack_mode_read_pattern(
		RuntimeInfo* rt,
		SubPatternsList* expected_packages
)
{
	while(
			lexer_pattern_peek( rt, 0) != END
			&&
			lexer_pattern_peek( rt, 0) != RIGHT_PARENT
	)
	{
		if( !lexer_match_start_bind( rt ) ) return FALSE;
		match_db_print(
				rt,
				"parsing next pack, sym: %s",
				CharBuf_get_array( & rt->bind_sym )
		);

		SubPatternInfo* subPatternInfo = getbytes( sizeof( SubPatternInfo ) );
		subPatternInfo->pattern_pos =
				rt->pattern_pos;
		CharBuf_init( &subPatternInfo->bind_sym, CHARBUF_SIZE );
		strcpy(
				CharBuf_get_array( &subPatternInfo->bind_sym ) ,
				"" 
		);

		SubPatternsList_append(
				expected_packages,
				subPatternInfo
		);
		int depth = 0;
		if( strcmp( CharBuf_get_array( & rt->bind_sym ), "") )
		{
			strcpy(
					CharBuf_get_array( &subPatternInfo->bind_sym ) ,
					CharBuf_get_array( &rt->bind_sym )
			);
		}

		// <symbol> (
		if( 
			!lexer_pattern_next_tok(
				rt,
				SYMBOL
			)
		)
			return FALSE;
		if( 
			!lexer_pattern_next_tok(
				rt,
				LEFT_PARENT
			)
		)
			return FALSE;

		depth = 1;
		while(
			lexer_pattern_peek( rt, 0) != END
			&&
			depth > 0
		)
		{
			if(
				lexer_pattern_peek( rt, 0) == LEFT_PARENT
			)
			{
				depth++;
			}
			else if( 
				lexer_pattern_peek( rt, 0) == RIGHT_PARENT
			)
			{
				depth--;
			}
			if(
				!lexer_pattern_next_tok_any( rt )
			)
				return FALSE;
		}
		// missing corresponding ')':
		if(
			depth > 0
			&& lexer_pattern_peek( rt, 0) == END
		)
		{
			match_error(
					rt,
					"invalid pack: missing ')'"
			);
			return FALSE;
		}

		subPatternInfo->pattern_size =
			rt->pattern_pos;

		// #] ...
		if(
				lexer_pattern_peek( rt, 0 ) == END_BIND
		)
		{
			if( !lexer_match_end_bind( rt ) ) return FALSE;
		}

	}
	return TRUE;
}

BOOL pack_mode_match_input(
		RuntimeInfo* rt,
		SubPatternsList* expected_packages,
		char* bind_arbitrary_sym  // if != "": bind non required packs to this symbol
)
{
	while(
			rt->input_pos < rt->input_size
	)
	{
		// read the input packet wise:
		if(
				rt->input_pos + 1 >= rt->input_size
				||
				rt->input[ rt->input_pos + 0 ].a_type != A_SYMBOL
				||
				rt->input[ rt->input_pos + 1].a_type != A_FLOAT
				||
				rt->input_pos + 1 + atom_getint( & rt->input[ rt->input_pos ] ) >= rt->input_size
		)
		{
			match_db_print(
					rt,
					"FAIL: input: invalid packet"
			);
			return FALSE;
		}
		match_db_print(
				rt,
				"parsing input pack..."
		);

		int pack_size = 
				atom_getint( & rt->input[ rt->input_pos + 1 ] );

		// try to find a sub pattern in "expected_packages" which
		// the current pack matches to
		SubPatternsListEl* pEl = SubPatternsList_get_first( expected_packages );
		BOOL matched = FALSE;
		while( pEl != NULL )
		{

			// use temporary RuntimeInfo:
			RuntimeInfo rt_temp;
			memcpy( &rt_temp, rt, sizeof( RuntimeInfo ) );
			rt_temp.rec_depth ++;
			CharBuf_init( & rt_temp.bind_sym, CHARBUF_SIZE);
			strcpy( CharBuf_get_array( & rt_temp.bind_sym ), "" );
			rt_temp.bind_start_level = -1;
			BoundData_init( & rt_temp.bind_ret, BOUNDDATA_SIZE );

			SubPatternInfo* subPatternInfo = pEl->pData;
			rt_temp.pattern_pos = subPatternInfo->pattern_pos;
			rt_temp.pattern_size = subPatternInfo->pattern_size;
			rt_temp.input_size = rt->input_pos + 2 + pack_size;
			if(
					strcmp( CharBuf_get_array( & subPatternInfo->bind_sym ), "" )
			)
			{
				if(
						! lexer_set_start_bind(
							& rt_temp,
							CharBuf_get_array( & subPatternInfo->bind_sym )
						)
				)
				{
					CharBuf_exit( & rt_temp.bind_sym );
					BoundData_exit( & rt_temp.bind_ret );
					return FALSE;
				}
			}
			if(
					match_rec(
						&rt_temp
					)
			)
			{
				if( ! lexer_set_end_bind( & rt_temp ) )
				{
					CharBuf_exit( & rt_temp.bind_sym );
					BoundData_exit( & rt_temp.bind_ret );
					return FALSE;
				}
				pack_mode_merge_bound_symbols(
						rt,
						& rt_temp.bind_ret
				);

				// if the current pack matched,
				// we dont have to consider this pattern anymore
				// -> delete it!
				SubPatternsList_del(
						expected_packages,
						pEl
				);
				matched = TRUE;
				CharBuf_exit( & rt_temp.bind_sym );
				BoundData_exit( & rt_temp.bind_ret );
				break;
			}
			else
			{
				pEl = SubPatternsList_get_next(
						expected_packages,
						pEl
				);
			}

			CharBuf_exit( & rt_temp.bind_sym );
			BoundData_exit( & rt_temp.bind_ret );
		}

		// now we know if the current input pack matches
		// any pack in the pattern.
		// we still have to consume the input:
		BOOL
			temp_binding =
				! strcmp( CharBuf_get_array( & rt->bind_sym ), "" )
				&&
				!matched
				&&
				strcmp( bind_arbitrary_sym, "" )
				;
		RuntimeInfo rt_temp;
		memcpy( &rt_temp, rt, sizeof( RuntimeInfo ) );
		CharBuf_init( & rt_temp.bind_sym, CHARBUF_SIZE);
		strcpy( CharBuf_get_array( & rt_temp.bind_sym ), "" );
		rt_temp.bind_start_level = -1;
		BoundData_init( & rt_temp.bind_ret, BOUNDDATA_SIZE );
		if( temp_binding )
		{
			match_db_print(
				rt,
				"temp binding..."
			);
			if(
				! lexer_set_start_bind(
						& rt_temp,
						bind_arbitrary_sym
				)
			)
			{
				match_error(
					rt,
					"internal error!"
				);
				CharBuf_exit( & rt_temp.bind_sym);
				BoundData_exit( & rt_temp.bind_ret );
				return FALSE;
			}
		}
		for(int i=0; i< (2+pack_size); i++)
		{
			if(
					! lexer_input_next_tok(
						& rt_temp,
						!matched // don't bind!
					)
			)
			{
				CharBuf_exit( & rt_temp.bind_sym);
				BoundData_exit( & rt_temp.bind_ret );
				return FALSE;
			}
		}
		if( temp_binding )
		{
			if(
				!lexer_set_end_bind( &rt_temp )
			)
			{
				match_error(
					rt,
					"internal error!"
				);
				CharBuf_exit( & rt_temp.bind_sym);
				BoundData_exit( & rt_temp.bind_ret );
				return FALSE;
			}
		}
		pack_mode_merge_bound_symbols(
				rt,
				& rt_temp.bind_ret
		);
		rt->input_pos = rt_temp.input_pos;
		rt->pattern_pos = rt_temp.pattern_pos;
		CharBuf_exit( & rt_temp.bind_sym);
		BoundData_exit( & rt_temp.bind_ret );
	}
	if( !SubPatternsList_is_empty( expected_packages ) )
	{
		match_db_print(
				rt,
				"FAIL: packets missing in input"
		);
		return FALSE;
	}
	return TRUE;
}

void pack_mode_merge_bound_symbols(
		RuntimeInfo* rt,
		BoundData* new_bound_symbols
)
{
		match_db_print(
				rt,
				"appending..."
		);
	
	MAP_FORALL_KEYS_BEGIN(BoundData,t_symbol*,new_bound_symbols,sym)
		AtomDynA* content = BoundData_get( new_bound_symbols, sym );
		for(int i=0; i< AtomDynA_get_size( content ); i++)
		{
			t_atom* atom = & AtomDynA_get_array( content)[i];
			AtomDynA* entry =
				BoundData_get( & rt -> bind_ret, sym);
			if( ! entry )
			{
				entry = 
						getbytes( sizeof( AtomDynA ) );
				AtomDynA_init( entry );
				BoundData_insert(
						& rt -> bind_ret,
						sym,
						entry
				);
			}
			AtomDynA_append(
					entry,
					*atom
			);
		}
	MAP_FORALL_KEYS_END(BoundData,t_symbol*,new_bound_symbols,sym)
}

// ************************
// lexer:
// ************************

// of which kind is the current pattern token?
t_pattern_atom_type lexer_pattern_peek(
		RuntimeInfo* rt,
		int ahead // 0: current, 1,2,3 ...: look ahead
)
{
	int pattern_pos = rt->pattern_pos;
	for(int i = 0; i < ahead; i ++)
		pattern_pos++;
	if( pattern_pos >= rt->pattern_size )
		return END;
	// '*'
	if( atom_getsymbol( &rt->pattern[ pattern_pos ] ) == gensym("*") )
		return STAR;
	// '('
	else if( atom_getsymbol( &rt->pattern[ pattern_pos ] ) == gensym("(") )
		return LEFT_PARENT;
	// ')'
	else if( atom_getsymbol( &rt->pattern[ pattern_pos ] ) == gensym(")") )
		return RIGHT_PARENT;

	char buf[CharBuf_get_size( & rt->bind_sym )];
	atom_string( & rt->pattern[ pattern_pos], buf, CharBuf_get_size( & rt->bind_sym ) );
	int len = strlen( buf );
	int pos_sep = (strchr( buf, '#' ) - buf);
	// '?'
	if( len == 1 && buf[0] == '?' )
	{
		return ANY_SYM;
	}
	// ?<bind_sym>
	else if( len >= 1 && buf[0] == '?' )
	{
		return ANY_SYM_BIND;
	}
	// <bind_sym>#[
	else if(
			pos_sep + 1 == len - 1
			&& buf[pos_sep+1] == '['
	)
	{
		return START_BIND;
	}
	// #]
	else if(
			len == 2
			&& buf[0] == '#'
			&& buf[1] == ']'
	)
	{
		return END_BIND;
	}
	else if( rt->pattern[ pattern_pos ].a_type == A_FLOAT )
	{
		return FLOAT;
	}
	else
	{
		return SYMBOL;
	}
}

// ignorantly consume next pattern token of a specific kind
BOOL lexer_pattern_next_tok(
		RuntimeInfo* rt,
		t_pattern_atom_type expected
)
{
	t_pattern_atom_type pattern_peek =
		lexer_pattern_peek( rt, 0);
	if(
			pattern_peek != expected
	)
	{
		char buf_expected[CHARBUF_SIZE/2];
		char buf_got[CHARBUF_SIZE/2];
		pattern_atom_type_to_str(
				expected,
				buf_expected, CHARBUF_SIZE
		);
		pattern_atom_type_to_str(
				pattern_peek,
				buf_got, CHARBUF_SIZE
		);
		match_error(
				rt,
				"FAIL: expected %s, got %s", buf_expected, buf_got
		);
		return FALSE;
	}
	return lexer_pattern_next_tok_any(
			rt
	);
}

// ignorantly consume next pattern token
BOOL lexer_pattern_next_tok_any(
		RuntimeInfo* rt
)
{
	t_pattern_atom_type pattern_peek =
		lexer_pattern_peek( rt, 0);

	if( pattern_peek == END )
	{
		match_error(
				rt,
				"lexer_pattern_next_tok: EOF!"
		);
		return FALSE;
	}
	rt->pattern_pos ++;
	return TRUE;
}

// try to read next input symbol of a specific kind (raise an error otherwise)
// take into account the current pattern,
// maybe advance pattern position too.
BOOL lexer_match_next(
		RuntimeInfo* rt,
		BOOL anything,
		t_pattern_atom_type expected_symbol
)
{
	t_pattern_atom_type pattern_peek = lexer_pattern_peek( rt, 0 );

	if( pattern_peek != expected_symbol )
	{
		char buf_expected[CHARBUF_SIZE];
		char buf_got[CHARBUF_SIZE];
		pattern_atom_type_to_str(
				expected_symbol,
				buf_expected, CHARBUF_SIZE
		);
		pattern_atom_type_to_str(
				pattern_peek,
				buf_got, CHARBUF_SIZE
		);
		match_db_print(
				rt,
				"FAIL: expected %s, got %s", buf_expected, buf_got
		);
		return FALSE;
	}
	return lexer_match_next_any(
			rt,
			anything
	);
}

// read next input symbol, taking into account the current pattern,
// also advance through the pattern
BOOL lexer_match_next_any(
		RuntimeInfo* rt,
		BOOL anything // "* mode" - don't consider pattern
)
{
	t_pattern_atom_type pattern_peek = lexer_pattern_peek( rt, 0 );

	if(
			strcmp( CharBuf_get_array( & rt->bind_sym ), "" )
			// rt->bind_start_pos != -1
			&&
			(
				( anything )
				||
				(pattern_peek == FLOAT)
				||
				(pattern_peek == SYMBOL)
				||
				(pattern_peek == ANY_SYM)
				||
				(pattern_peek == ANY_SYM_BIND)
				||
				(pattern_peek == LEFT_PARENT) // (we need to bind the packet size here...)
			)
	)
	{
		if( !lexer_append_to_bind( rt, & rt->input[ rt->input_pos ] ) ) return FALSE;
	}

	if( anything )
	{
		match_db_print(
				rt,
				"ok (* mode)"
		);
		rt->input_pos ++;
	}
	else
	{
		switch( pattern_peek )
		{
			case FLOAT:
			case SYMBOL:
				if( !compareAtoms( & rt->input[rt->input_pos], & rt->pattern[rt->pattern_pos] )  )
				{
					match_db_print(
							rt,
							"FAIL: !="
					);
					return FALSE;
				}
				match_db_print(
						rt,
						"=="
				);
				rt->pattern_pos ++;
				rt->input_pos ++;
			break;
			case LEFT_PARENT:
				match_db_print(
						rt,
						"("
				);
				rt->pattern_pos ++;
				rt->input_pos ++;
			break;
			case RIGHT_PARENT:
				match_db_print(
						rt,
						")"
				);
				rt->pattern_pos ++;
			break;
			case ANY_SYM:
				match_db_print(
						rt,
						"?"
				);

				rt->pattern_pos ++;
				rt->input_pos ++;
			break;
			case ANY_SYM_BIND:
			{
				match_db_print(
						rt,
						"?<bind>"
				);
				char buf[CHARBUF_SIZE];
				atom_string( & rt->pattern[ rt->pattern_pos], buf, CHARBUF_SIZE);

				AtomDynA* entry =
					BoundData_get( & rt -> bind_ret, gensym( & buf[1] ) );
				if( ! entry )
				{
					match_db_print(rt, "no entry found, create one..." );
					entry =
							getbytes( sizeof( AtomDynA ) );
					AtomDynA_init( entry );
					BoundData_insert(
							& rt -> bind_ret,
							gensym( &buf[1] ),
							entry
					);
				}
				AtomDynA_append(
						entry,
						rt->input[ rt->input_pos ]
				);
				rt->pattern_pos ++;
				rt->input_pos ++;
			}
			break;
			case START_BIND:
			{
				match_error(
						rt,
						"bind found! internal error!"
				);
				return FALSE;
			}
			break;
			case END_BIND:
			{
				match_error(
						rt,
						"#] found! internal error!"
				);
				return FALSE;
			}
			break;
			case STAR:
				match_error(
						rt,
						"* found! internal error!"
				);
				return FALSE;
			break;
			case END:
				match_error(
						rt,
						"eof!"
				);
				return FALSE;
			break;
		};
	}
	return TRUE;
}

// ignorantly advance input
BOOL lexer_input_next_tok(
		RuntimeInfo* rt,
		BOOL bind // consider binding
)
{
	if( rt->input_pos >= rt->input_size )
	{
		match_error(
				rt,
				"EOF"
		);
		return FALSE;
	}
	if(
			bind
			&& strcmp( CharBuf_get_array( & rt->bind_sym ), "" )
	)
	{

		if( !lexer_append_to_bind( rt, & rt->input[ rt->input_pos ] ) ) return FALSE;
	}
	rt->input_pos ++;
	return TRUE;
}

// if input == "<bind>#[ ..." match "<bind>#["
// and start binding input to "<bind>"
BOOL lexer_match_start_bind(
		RuntimeInfo* rt
)
{
	t_pattern_atom_type pattern_peek = lexer_pattern_peek( rt, 0 );

	if( pattern_peek == START_BIND )
	{
		if( strcmp( CharBuf_get_array( & rt->bind_sym ), "" ) )
		{
			match_error(
					rt,
					"ERROR: #[ before closing #]!"
			);
			return FALSE;
		}
		char buf[CharBuf_get_size( & rt->bind_sym )];
		atom_string( & rt->pattern[ rt->pattern_pos], buf, CharBuf_get_size( & rt->bind_sym ) );
		int pos_sep = (strchr( buf, '#' ) - buf);
		buf[pos_sep] = '\0';
		match_db_print(
				rt,
				"%s#[",
				buf
		);
		if(
			!lexer_set_start_bind(
				rt,
				buf
			)
		)
			return FALSE;
		rt->pattern_pos ++;
	};
	return TRUE;
}

// if input == "#] ...", match "#]"
// and stop binding input
BOOL lexer_match_end_bind(
		RuntimeInfo* rt
)
{
	t_pattern_atom_type pattern_peek = lexer_pattern_peek( rt, 0 );

	switch( pattern_peek )
	{
			case END_BIND:
			{
				if( !strcmp( CharBuf_get_array( & rt->bind_sym ), "" ) )
				{
					match_error(
							rt,
							"ERROR in pattern: #] before #[ !"
					);
					return FALSE;
				}
				match_db_print(
						rt,
						"#]"
				);
				//
				if(
					!lexer_set_end_bind( rt )
				)
					return FALSE;
				rt->pattern_pos ++;
			}
			break;
			default:
				return TRUE;
			break;
	};
	return TRUE;
}

// lower level:

// start binding input to some symbol
BOOL lexer_set_start_bind(
		RuntimeInfo* rt,
		char* bind_sym // bind to this symbol
)
{
	if( !strcmp( CharBuf_get_array( & rt->bind_sym ), "" ) )
	{
		match_db_print(
				rt,
				"start binding to %s",
				bind_sym
		);
		strcpy(
				CharBuf_get_array( & rt-> bind_sym ),
				bind_sym
		);
		rt->bind_start_level = rt->rec_depth;
	}
	return TRUE;
}

// stop binding input
BOOL lexer_set_end_bind(
		RuntimeInfo* rt
)
{
	//
	if( strcmp( CharBuf_get_array( & rt->bind_sym ), "" ) )
	{
		if( rt->bind_start_level != rt->rec_depth )
		{
			match_error(
					rt,
					"ERROR in pattern: '#[' was opened on level %i, but #] on level %i!",
					rt->bind_start_level, rt->rec_depth
			);
			return FALSE;
		}
		match_db_print(
				rt,
				"stop bind: %s",
				CharBuf_get_array( & rt->bind_sym )
		);
		strcpy( CharBuf_get_array( & rt->bind_sym ), "" );
		rt->bind_start_level = -1;
	}
	return TRUE;
}

// ignorantly append to bind return list
BOOL lexer_append_to_bind(
		RuntimeInfo* rt,
		t_atom* a
)
{
	if( !strcmp( CharBuf_get_array( & rt->bind_sym ), "" ) )
	{
		match_error(
				rt,
				"ERROR \"lexer_append_to_bind\" called but not in bind mode"
		);
		return FALSE;
	}
	match_db_print(
			rt,
			"binding to %s",
			CharBuf_get_array( & rt->bind_sym )
	);
	AtomDynA* entry =
		BoundData_get( & rt -> bind_ret, gensym( CharBuf_get_array( & rt->bind_sym ) ) );
	if( ! entry )
	{
		entry = 
				getbytes( sizeof( AtomDynA ) );
		AtomDynA_init( entry );
		BoundData_insert(
				& rt -> bind_ret,
				gensym( CharBuf_get_array( & rt->bind_sym ) ),
				entry
		);
	}
	AtomDynA_append(
			entry,
			*a
	);
	return TRUE;
}


void pattern_atom_type_to_str(
		t_pattern_atom_type type,
		char* buf,
		int buf_size
)
{
	buf[0] = '\0';
	switch( type )
	{
		case FLOAT:
		case SYMBOL:
		{
			sprintf( buf, "<atom>" );
		}
		break;
		case LEFT_PARENT:
		{
			sprintf( buf, "(" );
		}
		break;
		case RIGHT_PARENT:
		{
			sprintf( buf, ")" );
		}
		break;
		case ANY_SYM:
		{
			sprintf( buf, "?" );
		}
		break;
		case ANY_SYM_BIND:
		{
			sprintf( buf, "?<bind>" );
		}
		break;
		case START_BIND:
		{
			sprintf( buf, "<bind>#[" );
		}
		break;
		case END_BIND:
		{
			sprintf( buf, "#]" );
		}
		break;
		case STAR:
		{
			sprintf( buf, "*" );
		}
		break;
		case END:
		{
			sprintf( buf, "EOF" );
		}
		break;
	};
}

