#ifndef _SDSCRIPT_H_
#define _SDSCRIPT_H

#include "Global.h"
#include "SymbolTable.h"
#include "Commands.h"
#include "Map.h"

#include "m_pd.h"

/*
	Stmts 	-> 	Stmt Stmts | eps
	Stmt 	->	func ( Stmts )
		|	proc
		|	value
		|	var
*/


// information kept during runtime for each command:
typedef struct SCmdRuntimeData {
	t_int stackHeight0;
	CommandInfo* pCommandInfo;
} CmdRuntimeData;

DECL_LIST(CommandStack,ElementCommand,CmdRuntimeData,getbytes,freebytes,freebytes)
DEF_LIST(CommandStack,ElementCommand,CmdRuntimeData,getbytes,freebytes,freebytes);

DECL_BUFFER(CommandBuf,CmdRuntimeData,getbytes,freebytes)
DEF_BUFFER(CommandBuf,CmdRuntimeData,getbytes,freebytes)

#define DEL_PROG( prog, size ) \
		AtomBuf_exit( prog ); \
		freebytes( prog, size )

#define PROGS_HASH(sym) ((unsigned int ) sym)

// program name -> cmds ...
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpointer-to-int-cast"
DECL_MAP(ProgramMap,t_symbol*,AtomBuf,getbytes,freebytes,DEL_PROG,PROGS_HASH)
DEF_MAP(ProgramMap,t_symbol*,AtomBuf,getbytes,freebytes,DEL_PROG,PROGS_HASH)
#pragma GCC diagnostic pop

struct SScriptData;

typedef struct SProgramRTInfo {
	t_symbol* current_prog_name;
	AtomBuf* current_prog;

	Scope* scope;
	Scope* global_scope;

	// return stack
	AtomList stack;
	// CommandStack:
	CommandStack command_stack;
	// instruction pointer
	t_int peek;

	int countParenthesisRightIgnore;
	BOOL escape;
	// skip mode (default FALSE):
	BOOL skipMode;

	struct SScriptData* rt;
} ProgramRTInfo;

DECL_LIST( ProgStack, ProgStackEl, ProgramRTInfo, getbytes, freebytes, DEL_RT );

typedef struct SScriptData
{

	// program name -> cmds ...
	ProgramMap* programs;

	// program  name -> Scope
	SymbolTable* symbol_table;

	// global scope for all programs
	Scope global_scope;

	CommandInfos* command_infos;

	// while running: stores the execution state
	ProgStack program_stack;

	/* programs can set this variable
	 * in order to jump into another
	 * program:
	 */
	t_symbol* jump_to_program;

	/* programs can set this variable
	 * in order to pause themself
	 * for a number of ms
	 */
	double delay;

	// clock needed for the "Delay" command:
	t_clock *clock;

	// output buffer:
	OutputBuf output_buffer;

	t_outlet* outlet;
	//void (*output)( int argc, t_atom* argv );

	t_atom leftParenthesis, rightParenthesis, escapeBegin, escapeEnd;

} ScriptData;

void Script_init(
		ScriptData* rt,
		ProgramMap* programs, // program name -> cmds ...
		t_outlet* outlet,
		t_clock* clock
);

void Script_exit(
		ScriptData* rt
);

Scope* Script_get_global_scope(
		ScriptData* rt
);

void Script_exec(
	ScriptData* rt,
	t_symbol* prog_name
);

// continue after the program has been paused ...:
void Script_continue(
	ScriptData* rt
);

INLINE void DEL_RT(ProgramRTInfo* prog_rt, int size)
{
	AtomList_exit(  & prog_rt -> stack );
	CommandStack_exit( & prog_rt -> command_stack );
	symtab_del_scope(
			prog_rt -> rt -> symbol_table,
			prog_rt -> current_prog_name
	);
	freebytes( prog_rt, size );
}

DEF_LIST( ProgStack, ProgStackEl, ProgramRTInfo, getbytes, freebytes, DEL_RT );

#endif 
