#ifndef _COMMANDS_
#define _COMMANDS_

#include "Global.h"
//#include "DynArray.h"

typedef struct SProgramRTInfo t_rt;

typedef void (*POperatorFunction) (t_rt* pThis, t_int countParam, t_atom* pArgs);


// describes the restrictions for a command:
/*
  Please notice:
  if executeAfter != -1, the command may be executed BEFORE its closing ")".
  it is therefore important, that such a command either jumps after after its corresponding ")" via "jump", or explicitely puts a new command on the command stack!
*/
typedef struct SCommandInfo {
	t_atom name;
	t_int paramCount; // -1 means variable

	t_int executeAfter;
	/* if != -1 this means the command is called after "executeAfter" parameters,
		just ignoring the parameters that might follow
	  -1 is the usual behaviour. the command is called when all parameters have
	  	been parsed, that means when the ')' is found.
	*/

	POperatorFunction pFunc;
} CommandInfo;

DECL_DYN_ARRAY(OutputBuf,t_atom,getbytes,freebytes)
DEF_DYN_ARRAY(OutputBuf,t_atom,getbytes,freebytes)

DECL_DYN_ARRAY(CommandInfos, CommandInfo,getbytes,freebytes)
DEF_DYN_ARRAY(CommandInfos, CommandInfo,getbytes,freebytes)

CommandInfos* commands_init();
void commands_exit(
		CommandInfos* x
);

CommandInfo* getCommandInfo(
		CommandInfos* command_infos,
		t_atom* pName
);

CommandInfo* get_NOP(
		CommandInfos* command_infos
);
CommandInfo* get_RETURN_ALL(
		CommandInfos* command_infos
);

#endif 
