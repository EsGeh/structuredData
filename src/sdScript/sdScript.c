#include "sdScript.h"

#include <stdlib.h>
#include <string.h>



//*********************************************
// utils:

ProgramRTInfo* init_prog(
	ScriptData* rt,
	t_symbol* prog_name
);

void utils_push_cmd(
	ProgramRTInfo* rt,
	CommandInfo* pCommandInfo
);

void utils_pop_cmd_and_exec(
	ProgramRTInfo* rt
);

void utils_push_value(
	ProgramRTInfo* rt,
	t_atom* pCurrentToken
);

BOOL utils_is_value(t_atom* pToken);

// calls the command that is on the command_stack
void utils_call_command(
		ProgramRTInfo* rt,
		CommandInfo* pCommandInfo,
		t_int countParam
);

/*
  checks if the next command is one that can be executed before all parameters have been read. If this is the case, and if there are enough parameters on stack for it, executed it.
*/
void utils_try_to_exec_immediately(
		ProgramRTInfo* rt
);

//*********************************************
// lexer methods:

typedef BOOL TokenFits;

t_atom* lexer_peek(
		ProgramRTInfo* rt
);
t_atom* lexer_peek_next(
		ProgramRTInfo* rt
);
t_atom* lexer_pop(
		ProgramRTInfo* rt
);
TokenFits lexer_consumeNextToken(
		ProgramRTInfo* rt,
		t_atom* pExpectedSym
);

//*********************************************
// implementation
//*********************************************


void Script_init(
		ScriptData* rt,
		ProgramMap* programs, // program name -> cmds ...
		t_outlet* outlet,
		t_clock* clock
)
{
	SETSYMBOL( &rt->leftParenthesis, gensym("("));
	SETSYMBOL(& rt->rightParenthesis, gensym(")"));
	SETSYMBOL(& rt->escapeBegin, gensym("#["));
	SETSYMBOL(& rt->escapeEnd, gensym("#]"));

	rt->programs = programs;
	rt->symbol_table = symtab_init();
	ProgStack_init( & rt-> program_stack );
	OutputBuf_init( & rt->output_buffer );

	rt -> command_infos = commands_init();

	rt -> jump_to_program = NULL;

	rt -> clock = clock;

	rt -> outlet = outlet;

	ScopeList_init(
			& rt -> global_scopes
	);
}

void Script_exit(
		ScriptData* rt
)
{
	ScopeList_exit( & rt -> global_scopes );

	ProgStack_exit( & rt-> program_stack );
	symtab_exit( rt -> symbol_table );

	OutputBuf_exit( & rt -> output_buffer );

	commands_exit( rt -> command_infos );
}


ScopeList* Script_get_global_scopes(
		ScriptData* rt
)
{
	return & rt->global_scopes;
}

void Script_exec(
	ScriptData* rt,
	t_symbol* prog_name
)
{
	// if some program is still loaded, clean it up:
	if(
			ProgStack_get_size( & rt -> program_stack )
	)
	{
		ProgStack_clear( & rt -> program_stack );
		clock_unset( rt -> clock );
	}

	rt -> jump_to_program = prog_name;

	Script_continue(
			rt
	);
}


// ****************************
// low level execution control:
// ****************************

// executes the current code:
void Script_continue(
	ScriptData* rt
)
{
	do
		/*
		while(
			rt->jump_to_program != NULL
			||
			// 
			(
			 rt->delay == 0
			 &&
			 ProgStack_get_size( & rt-> program_stack ) != 0
			)
		)
		*/
	{
		// this means load a new program and execute it:
		if( rt->jump_to_program )
		{
			// create new runtime context for the program:
			ProgramRTInfo* prog_rt = init_prog(
					rt,
					rt->jump_to_program
			);
			if(
					!prog_rt
			)
			{
				t_atom a_prog_name;
				SETSYMBOL( & a_prog_name, rt->jump_to_program );
				char buffer[256];
				atom_string( & a_prog_name, buffer, 255 );
				pd_error( rt, "sdScript: no such comand or program: '%s'", buffer );
				return; 
			}
		}

		// load current program:
		ProgStackEl* last_prog_el = ProgStack_get_last(
				& rt-> program_stack
		);
		ProgramRTInfo* prog_rt = last_prog_el -> pData;

		rt-> delay = 0;
		rt-> jump_to_program = NULL;
		t_atom* pCurrentToken=NULL;
		while(
			( rt->delay == 0 )
			&&
			( rt->jump_to_program == NULL )
			&&
			(pCurrentToken = lexer_pop(prog_rt))
		)
		{
			char buf[256];
			atom_string(pCurrentToken, buf, 256);
			DB_PRINT("current Token: '%s'", buf);

			// 1. switch escape mode on/off:
			if( compareAtoms(pCurrentToken, & rt->escapeBegin) )
			{
				prog_rt -> escape = TRUE;
			}
			else if( compareAtoms(pCurrentToken, &rt->escapeEnd) )
			{
				if ( prog_rt->escape == FALSE)
					post("ERROR: #] found, without corresponding #[!");
				prog_rt->escape = FALSE;
			}
			else if( ! prog_rt->escape )
			{
				CommandInfo* pCommandInfo = getCommandInfo(
						rt -> command_infos,
						pCurrentToken
				);
				// <Command>
				if( pCommandInfo )
				{
					if( ! prog_rt -> skipMode )
					{
						DB_PRINT("Is a command");
						utils_push_cmd( prog_rt, pCommandInfo);	
					}
					// eat the "(" that follows:
					if( ! lexer_consumeNextToken( prog_rt, & rt->leftParenthesis) )
					{
						// report syntax error:
						post("ERROR: '(' expected, after ...");
					}
					else if( prog_rt->skipMode )
						(prog_rt -> countParenthesisRightIgnore) ++;
				}
				// ')'
				else if ( compareAtoms(pCurrentToken,&rt->rightParenthesis) )
				{
					if( prog_rt->skipMode )
					{
						prog_rt -> countParenthesisRightIgnore --;
						if( prog_rt -> countParenthesisRightIgnore == 0 )
						{
							prog_rt -> skipMode = FALSE;
							prog_rt -> countParenthesisRightIgnore = 1;
						}
					}
					else if( CommandStack_get_size( & prog_rt->command_stack ) )
					{
						utils_pop_cmd_and_exec( prog_rt );
					}
					else
						post("ERROR: ')' found, but no corresponding 'cmd ('");
				}
				// <value>
				else if( ! prog_rt->skipMode )
				{
					// if in escape mode, everything is considered a value:
					if (utils_is_value(pCurrentToken))
					{
						DB_PRINT("is a value");
						utils_push_value(prog_rt,pCurrentToken);
					}
					else
					{
						post("ERROR: token is neither cmd, var, or procedure: '%s'",buf);
					}
					utils_try_to_exec_immediately(prog_rt);
				}
			}
			else // we are in escape mode:
			{
				DB_PRINT("is a value");
				if( ! prog_rt->skipMode )
				{
					// if in escape mode, everything is considered a value:
					utils_push_value(prog_rt,pCurrentToken);
					utils_try_to_exec_immediately(prog_rt);
				}
			}
		}
		/*
			while(
				( rt->delay == 0 )
				&&
				( rt->jump_to_program == NULL )
				&&
				(pCurrentToken = lexer_pop(rt))
			)
		*/
		// if the the program set itself asleep:
		if(
				rt->delay
		)
		{
			// start a timer to continue execution:
			clock_delay( rt->clock, rt->delay );
		}
		// if the program is finished:
		else if(
				! lexer_peek_next( prog_rt )
		)
		{
			if( rt->jump_to_program )
			{
				char buf[256];
				t_atom prog_name_atom;
				SETSYMBOL( &prog_name_atom, prog_rt -> current_prog_name );
				atom_string( &prog_name_atom, buf, 256);
				DB_PRINT( "program freed before jump: '%s'", buf );
			}
			// the program has finished, so we clean it up:
			ProgStack_del(
					& rt -> program_stack,
					last_prog_el
			);
		}
	}
	while(
			rt->jump_to_program != NULL
			||
			// 
			(
			 rt->delay == 0
			 &&
			 ProgStack_get_size( & rt-> program_stack ) != 0
			)
	);
}

ProgramRTInfo* init_prog(
	ScriptData* rt,
	t_symbol* prog_name
)
{
	AtomBuf* prog = ProgramMap_get(
			rt -> programs,
			prog_name
	);
	if( !prog )
	{
		return NULL;
	}

	ProgramRTInfo* prog_rt = getbytes( sizeof( ProgramRTInfo ) );

	AtomList_init( & prog_rt -> stack );
	CommandStack_init( & prog_rt -> command_stack  );
	Scope* scope =
		symtab_add_scope(
				rt -> symbol_table,
				prog_name
		);

	(*prog_rt) = (ProgramRTInfo) {
		.current_prog_name = prog_name,
		.current_prog = prog,
		.scope = scope,
		.global_scopes = & rt->global_scopes,
		.peek = 0,

		.countParenthesisRightIgnore = 1,
		.escape = FALSE,
		.skipMode = FALSE,

		.rt = rt
	};
	ProgStack_append(
			& rt->program_stack,
			prog_rt
	);
	return prog_rt;
}

//*********************************************
// utils:

void utils_push_cmd(
	ProgramRTInfo* rt,
	CommandInfo* pCommandInfo
)
{
	// add the cmd to the command stack:
	CmdRuntimeData* pCurrentCmdRuntimeData = getbytes(sizeof(CmdRuntimeData));
	pCurrentCmdRuntimeData -> stackHeight0 =
		AtomList_get_size ( & rt -> stack );
	pCurrentCmdRuntimeData -> pCommandInfo = pCommandInfo;
	CommandStack_append (
			& rt->command_stack,
			pCurrentCmdRuntimeData
	);
}

void utils_pop_cmd_and_exec(
	ProgramRTInfo* rt
)
{
	ElementCommand* pElCurrentCommand =
		CommandStack_get_last( & rt->command_stack );
	CommandInfo* pCommandInfo = pElCurrentCommand -> pData -> pCommandInfo;
	t_int paramCount =
		AtomList_get_size ( & rt -> stack )
		- pElCurrentCommand -> pData -> stackHeight0
	;
	if(
		( pCommandInfo -> paramCount == -1 )
		|| ( pCommandInfo -> paramCount == paramCount )
	)
	{
		// execute the command:
		utils_call_command(
				rt,
				pCommandInfo,
				paramCount
		);
		pElCurrentCommand = CommandStack_get_last ( & rt -> command_stack );
	}
	else
	{
		char buf[256];
		atom_string( &pCommandInfo->name, buf, 255 );
		post("ERROR: wrong number of parameters for command %s", buf);
	}

	//delete the command from Stack, because it has been executed:
	CommandStack_del( & rt -> command_stack, pElCurrentCommand);
	utils_try_to_exec_immediately(
			rt
	);
}

void utils_push_value(
	ProgramRTInfo* rt,
	t_atom* pCurrentToken
)
{
	// put value on stack:
	t_atom* pValue = getbytes(sizeof(t_atom));
	*pValue = *pCurrentToken;
	AtomList_append( & rt -> stack, pValue);
}

BOOL utils_is_value(t_atom* pToken)
{
	if( atomEqualsString( pToken, "(" ) )
		return FALSE;
	return TRUE;
}

// calls the command that is on the command_stack
void utils_call_command(
	ProgramRTInfo* rt,
	CommandInfo* pCommandInfo,
	t_int countParam
)
{
	// first copy params:
	t_atom* pArgs = getbytes( sizeof(t_atom )* countParam);
	{
		AtomListEl* pElOpNext = AtomList_get_last( & rt -> stack);
		for ( int i=countParam-1; i>=0; i--)
		{
			pArgs[i] = *(pElOpNext-> pData);
			pElOpNext = AtomList_get_prev( & rt -> stack, pElOpNext);
		}
	}
	// ... delete them from stack
	for ( int i=0; i<countParam; i++)
	{
		AtomListEl* pElParam = AtomList_get_last ( & rt -> stack );
		AtomList_del( & rt -> stack, pElParam );
	}

	char buf[256];
	atom_string( & pCommandInfo->name, buf, 256 );
	DB_PRINT("executing command %s", buf);
	// call command:
	(pCommandInfo -> pFunc) (
			rt,
			countParam,
			pArgs
	);
	// free array of parameters:
	freebytes( pArgs, sizeof(t_atom ) * countParam );
}

void utils_try_to_exec_immediately(
		ProgramRTInfo* rt
)
{
	ElementCommand* pElCurrentCommand =
		CommandStack_get_last ( & rt -> command_stack );
	if( !pElCurrentCommand )
		return ;
	// if the topmost command is a dont-read-all-parameters-command:
	if( pElCurrentCommand->pData -> pCommandInfo -> executeAfter != -1)
	{
		DB_PRINT("trying to executed immediately...:");
		CommandInfo* pCommandInfo = pElCurrentCommand->pData -> pCommandInfo;
		// check if there are enough values on stack now to call the next command
		t_int paramCount =
			AtomList_get_size ( & rt -> stack )
			- pElCurrentCommand -> pData -> stackHeight0
		;
		if( paramCount== pCommandInfo -> executeAfter )
		{
			utils_call_command(rt, pCommandInfo, paramCount);
		}
		else if( paramCount > pCommandInfo -> executeAfter)
		{
			char buf[256];
			atom_string(& pCommandInfo->name, buf, 256);
			post("ERROR: wrong number of parameters for %s", buf);
		}
		CommandStack_del( & rt -> command_stack, pElCurrentCommand);
	}
}

//*********************************************
// lexer methods:

t_atom* lexer_peek(
		ProgramRTInfo* rt
)
{
	t_atom* pRet = 0;
	if( rt -> peek < AtomBuf_get_size( rt -> current_prog ) )
	{
		pRet = & AtomBuf_get_array( rt -> current_prog )[ rt -> peek ];
	}
	return pRet;
}

t_atom* lexer_peek_next(
		ProgramRTInfo* rt
)
{
	t_atom* pRet = 0;
	if( rt -> peek + 1 < AtomBuf_get_size( rt -> current_prog ) )
	{
		pRet = & AtomBuf_get_array( rt -> current_prog )[ rt -> peek + 1];
	}
	return pRet;
}

t_atom* lexer_pop(
		ProgramRTInfo* rt
)
{
	t_atom* ret = lexer_peek( rt );
	rt -> peek ++;
	return ret;
}

TokenFits lexer_consumeNextToken(
	ProgramRTInfo* rt,
	t_atom* pExpectedSym
)
{
	t_atom* pNextToken = lexer_pop( rt );
	return compareAtoms( pNextToken, pExpectedSym );
}
