#include "sdPackMatch.h"
#include "LinkedList.h"
#include "Buffer.h"
#include "DynArray.h"
#include "Global.h"


#include <string.h>

#include "m_pd.h"


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

DECL_BUFFER(AtomBuf, t_atom, getbytes, freebytes)
DEF_BUFFER(AtomBuf, t_atom, getbytes, freebytes)

DECL_BUFFER(CharBuf, char, getbytes, freebytes)
DEF_BUFFER(CharBuf, char, getbytes, freebytes)

typedef struct
{
	t_symbol* name;
	AtomBuf pattern;
} PatternInfo;

#define FREE_PATTTERN_INFO(x,size) \
{ \
	AtomBuf_exit( & x->pattern ); \
	freebytes( x, size ); \
}

DECL_LIST(PatternList, PatternEl, PatternInfo, getbytes, freebytes, FREE_PATTTERN_INFO)
DEF_LIST(PatternList, PatternEl, PatternInfo, getbytes, freebytes, FREE_PATTTERN_INFO)

DECL_DYN_ARRAY(AtomDynArray, t_atom, getbytes, freebytes)
DEF_DYN_ARRAY(AtomDynArray, t_atom, getbytes, freebytes)

typedef struct {
	int pattern_pos;
	int pattern_size;
} SubPatternInfo;

DECL_LIST(SubPatternsList, SubPatternsListEl, SubPatternInfo, getbytes, freebytes, freebytes)
DEF_LIST(SubPatternsList, SubPatternsListEl, SubPatternInfo, getbytes, freebytes, freebytes)

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

void packMatch_patterns_clear(
	t_packMatch* x
);

void packMatch_input(
	t_packMatch* x,
	t_symbol *s,
	int argc,
	t_atom *argv
);

// 
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
	int bind_start_pos;
	// if bind sym, the buffer to take the name to bind to:
	CharBuf bind_sym;

	AtomDynArray bind_ret;
	
	int rec_depth;
} RuntimeInfo;

BOOL match(
		RuntimeInfo* rt
);

BOOL match_rec(
		RuntimeInfo* rt
);

// lexer:

typedef enum e_pattern_atom_type {
	FLOAT,
	SYMBOL,
	LEFT_PARENT,
	RIGHT_PARENT,
	ANY_SYM,
	ANY_SYM_BIND,
	START_BIND,
	END_BIND,
	STAR,
	END
} t_pattern_atom_type;

// utility mainly for debug output:
void pattern_atom_type_to_str(
		t_pattern_atom_type type,
		char* buf,
		int buf_size
);

t_pattern_atom_type lexer_pattern_peek(
		RuntimeInfo* rt,
		int pos
);

BOOL lexer_pattern_next_tok(
		RuntimeInfo* rt,
		t_pattern_atom_type expected
);

BOOL lexer_pattern_next_tok_any(
		RuntimeInfo* rt
);

BOOL lexer_match_next(
		RuntimeInfo* rt,
		BOOL anything,
		t_pattern_atom_type expected_symbol
);

BOOL lexer_match_next_any(
		RuntimeInfo* rt,
		BOOL anything
);

BOOL lexer_match_start_bind(
		RuntimeInfo* rt
);

BOOL lexer_match_end_bind(
		RuntimeInfo* rt
);

BOOL lexer_input_next_tok(
		RuntimeInfo* rt,
		BOOL bind
);

// DEBUG OUTPUT HELPERS:

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
			// 
			.bind_start_level = -1,
			.bind_start_pos = -1,
			.rec_depth = 0
		};
		AtomDynArray_init( & rt.bind_ret );
		CharBuf_init( & rt.bind_sym, 1024 );
		{
			t_atom pattern_name_atom;
			SETSYMBOL( & pattern_name_atom, patternInfo->name );
			AtomDynArray_append(
					& rt.bind_ret,
					pattern_name_atom
			);
		}
		{
			t_atom size;
			SETFLOAT( & size, 0 );
			AtomDynArray_append(
					& rt.bind_ret,
					size
			);
		}
		if(
				match(
					&rt
				)
		)
		{
			SETFLOAT(
					& AtomDynArray_get_array( & rt.bind_ret )[1],
					AtomDynArray_get_size( & rt.bind_ret ) - 2
			);
	
			outlet_list(
				x->outlet,
				& s_list,
				AtomDynArray_get_size( &rt.bind_ret ),
				AtomDynArray_get_array( &rt.bind_ret )
			);
			match_found = TRUE;
		}
		CharBuf_exit( & rt.bind_sym );
		AtomDynArray_exit( & rt.bind_ret );
		LIST_FORALL_END(PatternList, PatternEl, PatternInfo, & x->patterns, i_pattern, patternEl)
	if( !match_found )
	outlet_bang(
			x->outlet_reject
	);
}

#define match_db_print( \
		rt, \
		msg, ... \
) \
{ \
	char pattern_name_buf[1024]; \
	{ \
		t_atom pattern_name_atom; \
		SETSYMBOL( & pattern_name_atom, rt->pattern_name ); \
		atom_string( &pattern_name_atom, pattern_name_buf, 1024 ); \
	} \
	char pattern_buf[1024]; \
	{ \
		sprintf( pattern_buf, "'"); \
		for(int i=rt->pattern_pos; i< rt->pattern_size; i++) \
		{ \
			t_atom* current = & rt->pattern[ i ]; \
			char current_buf[1024]; \
			atom_string( current, current_buf, 1024 ); \
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
	char input_buf[1024]; \
	{ \
		sprintf( input_buf, "'"); \
		for(int i=rt->input_pos; i< rt->input_size; i++) \
		{ \
			t_atom* current = & rt->input[ i ]; \
			char current_buf[1024]; \
			atom_string( current, current_buf, 1024 ); \
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
	char pattern_name_buf[1024]; \
	{ \
		t_atom pattern_name_atom; \
		SETSYMBOL( & pattern_name_atom, rt->pattern_name ); \
		atom_string( &pattern_name_atom, pattern_name_buf, 1024 ); \
	} \
	char pattern_buf[1024]; \
	{ \
		sprintf( pattern_buf, "'"); \
		for(int i=rt->pattern_pos; i< rt->pattern_size; i++) \
		{ \
			t_atom* current = & rt->pattern[ i ]; \
			char current_buf[1024]; \
			atom_string( current, current_buf, 1024 ); \
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
	char input_buf[1024]; \
	{ \
		sprintf( input_buf, "'"); \
		for(int i=rt->input_pos; i< rt->input_size; i++) \
		{ \
			t_atom* current = & rt->input[ i ]; \
			char current_buf[1024]; \
			atom_string( current, current_buf, 1024 ); \
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

BOOL match_rec(
		RuntimeInfo* rt
)
{
	match_db_print(
			rt,
			"start"
	);

	lexer_match_start_bind( rt );

	// match NON strict:
	if(
			lexer_pattern_peek( rt, 0) == STAR
	)
	{
		// eat '*'
		if(
				!lexer_pattern_next_tok(
					rt,
					STAR
				)
		)
			return FALSE;

		;

		// if '*' was the last symbol in the pattern:
		if(
			lexer_pattern_peek( rt, 0) == END
			||
			lexer_pattern_peek( rt, 0) == RIGHT_PARENT
			|| 
			( (lexer_pattern_peek( rt, 0) == START_BIND || lexer_pattern_peek( rt, 0) == END_BIND) &&
				(
				 lexer_pattern_peek( rt, 1) == END
				 ||
				 lexer_pattern_peek( rt, 1) == RIGHT_PARENT
				)
			)
		)
		{
			match_db_print(
					rt,
					"* arbitrary mode!"
			);
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
			lexer_match_end_bind( rt );
		}
		// make shure the input contains certain packages
		else
		{
			match_db_print(
					rt,
					"* pack mode!"
			);

			// 1. read all packages in the pattern:
			SubPatternsList expected_packages;
			SubPatternsList_init ( & expected_packages );
			while(
					lexer_pattern_peek( rt, 0) != END
					&&
					lexer_pattern_peek( rt, 0) != RIGHT_PARENT
					&&
					lexer_pattern_peek( rt, 0) != END_BIND
			)
			{
				SubPatternInfo* subPatternInfo = getbytes( sizeof( SubPatternInfo ) );
				SubPatternsList_append(
						& expected_packages,
						subPatternInfo
				);
				int depth = 0;
				subPatternInfo->pattern_pos =
						rt->pattern_pos;

				if(
						lexer_pattern_peek( rt, 0 ) == START_BIND
				)
				{
					lexer_pattern_next_tok_any( rt );
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
					SubPatternsList_exit( & expected_packages );
					return FALSE;
				}
				// if there was no #[ opened in the beginning, we consider it part of the subpattern:
				if( rt->bind_start_level != rt->rec_depth )
				{
					if(
							lexer_pattern_peek( rt, 0 ) == END_BIND
					)
						lexer_pattern_next_tok(
								rt, END_BIND
						);
				}

				subPatternInfo->pattern_size =
					rt->pattern_pos;
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
						"pack: pos: %i, size: %i",
						subPatternInfo->pattern_pos,
						subPatternInfo->pattern_size
				);
				pEl = SubPatternsList_get_next(
						& expected_packages,
						pEl
				);
			}

			// 2. try to find all patterns in the input:
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

						SubPatternsList_exit( & expected_packages );
						return FALSE;
					}
					int pack_size = 
							atom_getint( & rt->input[ rt->input_pos + 1 ] );
					//int pos_temp = rt->input_pos;
					// check if this is an expected packet:

					SubPatternsListEl* pEl = SubPatternsList_get_first( &expected_packages );
					RuntimeInfo rt_temp;
					memcpy( &rt_temp, rt, sizeof( RuntimeInfo ) );

					rt_temp.rec_depth ++;
					BOOL matched = FALSE;
					while( pEl != NULL )
					{
						AtomDynArray_init( & rt_temp.bind_ret );
						SubPatternInfo* subPatternInfo = pEl->pData;
						rt_temp.pattern_pos = subPatternInfo->pattern_pos;
						rt_temp.pattern_size = subPatternInfo->pattern_size;
						rt_temp.input_size = rt->input_pos + 2 + pack_size;
						if(
								match_rec(
									&rt_temp
								)
						)
						{
							for( int i=0; i< AtomDynArray_get_size( & rt_temp.bind_ret ); i++)
							{
								AtomDynArray_append(
									& rt-> bind_ret,
									AtomDynArray_get_array( & rt_temp.bind_ret )[ i ]
								);
							}
							AtomDynArray_exit( & rt_temp.bind_ret );

							SubPatternsList_del(
									& expected_packages,
									pEl
							);
							matched = TRUE;
							break;
						}
						else
						{
							pEl = SubPatternsList_get_next(
									& expected_packages,
									pEl
							);
						}
					}
					//rt->input_pos += (2 + pack_size);
					for(int i=0; i< (2+pack_size); i++)
					{
						if(
								! lexer_input_next_tok(
									rt,
									!matched
								)
						)
							return FALSE;
					}
				}
				if( !SubPatternsList_is_empty( & expected_packages ) )
				{
					match_db_print(
							rt,
							"FAIL: packets missing in input"
					);

					SubPatternsList_exit( & expected_packages );
					return FALSE;
				}
			}

			SubPatternsList_exit( & expected_packages );
		}
		if(
			!lexer_match_end_bind( rt )
		)
		{
			return FALSE;
		}
	}

	// match strict:
	else
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

	}

	match_db_print(
			rt,
			"SUCCESS"
	);

	return TRUE;

}

t_pattern_atom_type lexer_pattern_peek(
		RuntimeInfo* rt,
		int ahead
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

	CharBuf_get_array( & rt->bind_sym )[0] = '\0';

	char buf[CharBuf_get_size( & rt->bind_sym )];
	atom_string( & rt->pattern[ rt->pattern_pos], buf, CharBuf_get_size( & rt->bind_sym ) );
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
		strcat( CharBuf_get_array( & rt->bind_sym ) , & buf[1] );
		return ANY_SYM_BIND;
	}
	// <bind_sym>#[
	else if(
			pos_sep + 1 == len - 1
			&& buf[pos_sep+1] == '['
	)
	{
		buf[pos_sep] = '\0';
		strcat( CharBuf_get_array( & rt->bind_sym ) , buf );
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
	else if( rt->pattern[ rt->pattern_pos ].a_type == A_FLOAT )
	{
		return FLOAT;
	}
	else
	{
		return SYMBOL;
	}
}

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
		char buf_expected[1024];
		char buf_got[1024];
		pattern_atom_type_to_str(
				expected,
				buf_expected, 1024
		);
		pattern_atom_type_to_str(
				pattern_peek,
				buf_got, 1024
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

BOOL lexer_match_next(
		RuntimeInfo* rt,
		BOOL anything,
		t_pattern_atom_type expected_symbol
)
{
	t_pattern_atom_type pattern_peek = lexer_pattern_peek( rt, 0 );

	if( pattern_peek != expected_symbol )
	{
		char buf_expected[1024];
		char buf_got[1024];
		pattern_atom_type_to_str(
				expected_symbol,
				buf_expected, 1024
		);
		pattern_atom_type_to_str(
				pattern_peek,
				buf_got, 1024
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

BOOL lexer_match_next_any(
		RuntimeInfo* rt,
		BOOL anything
)
{
	t_pattern_atom_type pattern_peek = lexer_pattern_peek( rt, 0 );

	if(
			rt->bind_start_pos != -1
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
		match_db_print(
				rt,
				"binding"
		);
		AtomDynArray_append(
				& rt->bind_ret,
				rt->input[ rt->input_pos ]
		);

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
						"<bind>?"
				);
				{
					t_atom append;
					SETSYMBOL( & append, gensym( CharBuf_get_array( & rt->bind_sym ) ) );
					AtomDynArray_append(
							& rt->bind_ret,
							append
					);
				}
				{
					t_atom append;
					SETFLOAT( & append, 1 );
					AtomDynArray_append(
							& rt->bind_ret,
							append
					);
				}
				AtomDynArray_append(
						& rt->bind_ret,
						rt->input[ rt->input_pos ]
				);
				rt->pattern_pos ++;
				rt->input_pos ++;
			}
			break;
			case START_BIND:
			{
				match_db_print(
						rt,
						"<bind>#["
				);
				rt->bind_start_pos =
					AtomDynArray_get_size( & rt->bind_ret );
				{
					t_atom append;
					SETSYMBOL( & append, gensym( CharBuf_get_array( & rt->bind_sym) ) );
					AtomDynArray_append(
							& rt->bind_ret,
							append
					);
				}
				{
					t_atom append;
					SETFLOAT( & append, 0 );
					AtomDynArray_append(
							& rt->bind_ret,
							append
					);
				}
				rt->pattern_pos ++;
				if(
						!lexer_match_next_any(
							rt,
							anything
						)
				)
					return FALSE;
			}
			break;
			case END_BIND:
			{
				match_error(
						rt,
						"#] found!"
				);
				return FALSE;
			}
			break;
			case STAR:
				match_error(
						rt,
						"*!"
				);
				return FALSE;
			break;
			case END:
				match_error(
						rt,
						"#] found!"
				);
				return FALSE;
			break;
		};
	}
	return TRUE;
}

BOOL lexer_match_start_bind(
		RuntimeInfo* rt
)
{
	t_pattern_atom_type pattern_peek = lexer_pattern_peek( rt, 0 );


	if( pattern_peek == START_BIND )
	{
		if(
				rt->bind_start_pos != -1
		)
		{
			match_error(
					rt,
					"ERROR: #[ before closing #]!"
			);
			return FALSE;
		}
		match_db_print(
				rt,
				"<bind>#["
		);
		rt->bind_start_pos =
			AtomDynArray_get_size( & rt -> bind_ret );
		rt->bind_start_level = rt->rec_depth;
		{
			t_atom append;
			SETSYMBOL( & append, gensym( CharBuf_get_array( & rt->bind_sym ) ) );
			AtomDynArray_append(
					& rt->bind_ret,
					append
			);
		}
		{
			t_atom append;
			SETFLOAT( & append, 0 );
			AtomDynArray_append(
					& rt->bind_ret,
					append
			);
		}
		rt->pattern_pos ++;
		/*
		if(
				!lexer_match_next_any(
					rt,
					FALSE
				)
		)
			return FALSE;
		*/
	};
	return TRUE;
}

BOOL lexer_match_end_bind(
		RuntimeInfo* rt
)
{
	t_pattern_atom_type pattern_peek = lexer_pattern_peek( rt, 0 );

	switch( pattern_peek )
	{
			case END_BIND:
			{
				match_db_print(
						rt,
						"#]"
				);
				//
				if(  rt->bind_start_pos == -1 )
				{
					match_error(
							rt,
							"ERROR in pattern: #] before #[ !"
					);
					return FALSE;
				}
				//
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
						"stop bind"
				);
				SETFLOAT(
						& AtomDynArray_get_array( & rt->bind_ret )[ rt->bind_start_pos + 1 ],
						AtomDynArray_get_size( & rt->bind_ret ) - rt->bind_start_pos - 2
				);
				rt->bind_start_pos = -1;
				rt->bind_start_level = -1;
				rt->pattern_pos ++;
			}
			break;
			default:
				return TRUE;
			break;
	};
	return TRUE;
}

BOOL lexer_input_next_tok(
		RuntimeInfo* rt,
		BOOL bind
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
			&& rt->bind_start_pos != -1
	)
	{
		match_db_print(
				rt,
				"binding"
		);
		AtomDynArray_append(
				& rt->bind_ret,
				rt->input[ rt->input_pos ]
		);

	}
	rt->input_pos ++;
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
