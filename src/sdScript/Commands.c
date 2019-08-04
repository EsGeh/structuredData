#include "Commands.h"

#include "SymbolTable.h"
#include "LinkedList.h"
#include "sdScript.h"

#include <stdlib.h>

CommandInfo* pNOP;
CommandInfo* pRETURN_ALL;

#define PFUNCTION_HEADER(name) \
void name(t_rt* prog_rt, t_int countArgs, t_atom* pArgs)

PFUNCTION_HEADER(add);
PFUNCTION_HEADER(sub);
PFUNCTION_HEADER(mul);
PFUNCTION_HEADER(div_);
PFUNCTION_HEADER(mod);
// output:
PFUNCTION_HEADER(print);
PFUNCTION_HEADER(pack);
PFUNCTION_HEADER(out);
// variables:
PFUNCTION_HEADER(addVar);
PFUNCTION_HEADER(getVar);
PFUNCTION_HEADER(getVarA);
PFUNCTION_HEADER(setVar);
PFUNCTION_HEADER(setVarA);
// main variables:
PFUNCTION_HEADER(addMainVar);
PFUNCTION_HEADER(clearMain);
// conditionality:
PFUNCTION_HEADER(if_);
// sgScales:
PFUNCTION_HEADER(sgFunc);
PFUNCTION_HEADER(sgScale);
// boolean operators:
PFUNCTION_HEADER(and_);
PFUNCTION_HEADER(or_);
PFUNCTION_HEADER(not_);
// comparison operators:
PFUNCTION_HEADER(isEqual);
PFUNCTION_HEADER(isNotEqual);
PFUNCTION_HEADER(isLessThan);
PFUNCTION_HEADER(isGreaterThan);
PFUNCTION_HEADER(isLessOrEqual);
PFUNCTION_HEADER(isGreaterOrEqual);
// Set operations:
PFUNCTION_HEADER(setify);
PFUNCTION_HEADER(card);
PFUNCTION_HEADER(setOp);
PFUNCTION_HEADER(contains);
PFUNCTION_HEADER(calcTransp);

PFUNCTION_HEADER(addA);
PFUNCTION_HEADER(subA);
PFUNCTION_HEADER(mulA);
PFUNCTION_HEADER(divA);
PFUNCTION_HEADER(modA);
// random
PFUNCTION_HEADER(sdMinMax);
PFUNCTION_HEADER(random_);

// just throw away all parameters:
PFUNCTION_HEADER(nop);
PFUNCTION_HEADER(returnAll);

PFUNCTION_HEADER(rndInt);
PFUNCTION_HEADER(inc);
PFUNCTION_HEADER(dec);
PFUNCTION_HEADER(rndIntUnequal);

//sdPack:
PFUNCTION_HEADER( sdpackType );
PFUNCTION_HEADER( sdpackCount );
PFUNCTION_HEADER( sdpackParams );

// like the abstraction it converts one OR MANY sdPacks
// from the "humane" syntax using "(" and ")" to the 
// inhumane one, example:
// pack ( bli bla blubb ) becomes: pack 3 bli bla blubb
PFUNCTION_HEADER( sdPackFromHuman );

// sdData (lists of sdPacks):
PFUNCTION_HEADER( sdDataGetPackFromType );
PFUNCTION_HEADER( sdDataGetPackFromTypeRest );
PFUNCTION_HEADER( sdDataGetPackFromIndex );
// returns the first sdPack
PFUNCTION_HEADER( sdDataGetFirst );
// returns all sdPacks but the first sdPack
PFUNCTION_HEADER( sdDataGetRest );
// deletes first pack from a list of sdPacks and returns it

PFUNCTION_HEADER( delay );
// deletes first pack from a list of sdPacks and returns it

// call an other command as sub routine:
PFUNCTION_HEADER( callFunction );

#define ADD_FUNCTION_CONTINUATION( NAME,PFUNC,PARAMCOUNT,EXECAFTER, CONTINUATION) \
{ \
	t_atom atomName; \
	SETSYMBOL( &atomName, gensym( NAME ) ); \
 \
	CommandInfo func = { \
		.name = atomName, \
		.paramCount = PARAMCOUNT, \
		.executeAfter = EXECAFTER, \
		.continuation_func = CONTINUATION, \
		.pFunc = PFUNC \
	}; \
	CommandInfos_append( command_infos, \
			func \
	); \
}

#define ADD_FUNCTION( NAME,PFUNC,PARAMCOUNT,EXECAFTER) \
{ \
	t_atom atomName; \
	SETSYMBOL( &atomName, gensym( NAME ) ); \
 \
	CommandInfo func = { \
		.name = atomName, \
		.paramCount = PARAMCOUNT, \
		.executeAfter = EXECAFTER, \
		.continuation_func = NULL, \
		.pFunc = PFUNC \
	}; \
	CommandInfos_append( command_infos, \
			func \
	); \
}


CommandInfos* commands_init()
{

	CommandInfos* command_infos = getbytes( sizeof( CommandInfos ) );
	CommandInfos_init( command_infos );

	ADD_FUNCTION("NOP",&nop,-1,-1);
	ADD_FUNCTION("Add",&add,2,-1);
	ADD_FUNCTION("Sub",&sub,2,-1);
	ADD_FUNCTION("Mul",&mul,2,-1);
	ADD_FUNCTION("Div",&div_,2,-1);
	ADD_FUNCTION("Mod",&mod,2,-1);
	ADD_FUNCTION("Print",&print,-1,-1);
	ADD_FUNCTION("Pack",&pack,-1,-1);
	ADD_FUNCTION("Out",&out,-1,-1);
	ADD_FUNCTION("Var",&addVar,-1,-1);
	ADD_FUNCTION("Get",&getVar,1,-1);
	ADD_FUNCTION("GetA",&getVarA,2,-1);
	ADD_FUNCTION("Set",&setVar,-1,-1);
	ADD_FUNCTION("SetA",&setVarA,-1,-1);
	ADD_FUNCTION("VarMain",&addMainVar,-1,-1);
	ADD_FUNCTION("ClearMain",&clearMain,0,-1);

	// nice aliases:
	ADD_FUNCTION("+",&add,2,-1);
	ADD_FUNCTION("-",&sub,2,-1);
	ADD_FUNCTION("*",&mul,2,-1);
	ADD_FUNCTION("/",&div_,2,-1);
	ADD_FUNCTION("%",&mod,2,-1);
	ADD_FUNCTION("$",&getVar,1,-1);
	ADD_FUNCTION("$[]",&getVarA,2,-1);
	ADD_FUNCTION("=",&setVar,-1,-1);
	ADD_FUNCTION("=[]",&setVarA,-1,-1);

	ADD_FUNCTION("#",&card,-1,-1);
	ADD_FUNCTION("+[]",&addA,-1,-1);
	ADD_FUNCTION("-[]",&subA,-1,-1);
	ADD_FUNCTION("*[]",&mulA,-1,-1);
	ADD_FUNCTION("/[]",&divA,-1,-1);
	ADD_FUNCTION("%[]",&modA,-1,-1);

	ADD_FUNCTION("+1",&inc,1,-1);
	ADD_FUNCTION("-1",&dec,1,-1);


	// sgScales:
	// a | b , c , x
	ADD_FUNCTION("sgFunc",&sgFunc,4,-1);
	// # , a | b , c 
	ADD_FUNCTION("sgScale",&sgScale,4,-1);
	// boolean operators:

	ADD_FUNCTION("&&",&and_,2,-1);
	ADD_FUNCTION("||",&or_,2,-1);
	ADD_FUNCTION("!",&not_,1,-1);
	// comparison operators:
	ADD_FUNCTION("==",&isEqual,2,-1);
	ADD_FUNCTION("!=",&isNotEqual,2,-1);
	ADD_FUNCTION("<",&isLessThan,2,-1);
	ADD_FUNCTION(">",&isGreaterThan,2,-1);
	ADD_FUNCTION("<=",&isLessOrEqual,2,-1);
	ADD_FUNCTION(">=",&isGreaterOrEqual,2,-1);
	// Set operations:
	ADD_FUNCTION("Setify",&setify,-1,-1);
	ADD_FUNCTION("Card",&card,-1,-1);
	ADD_FUNCTION("SetOp",&setOp,-1,-1);
	ADD_FUNCTION("CalcTransp",&calcTransp,-1,-1);
	ADD_FUNCTION("Contains",&contains,-1,-1);
	ADD_FUNCTION("AddA",&addA,-1,-1);
	ADD_FUNCTION("SubA",&subA,-1,-1);
	ADD_FUNCTION("MulA",&mulA,-1,-1);
	ADD_FUNCTION("DivA",&divA,-1,-1);
	ADD_FUNCTION("ModA",&modA,-1,-1);
	ADD_FUNCTION("Rnd",&random_,2,-1);
	ADD_FUNCTION("MinMax",&sdMinMax,3,-1);
	ADD_FUNCTION("RETURN_ALL",&returnAll,-1,-1);
	ADD_FUNCTION("RndI",&rndInt,2,-1);
	ADD_FUNCTION("Inc",&inc,1,-1);
	ADD_FUNCTION("Dec",&dec,1,-1);
	ADD_FUNCTION("RndIUnequal",&rndIntUnequal,-1,-1);

	// structuredData:
	ADD_FUNCTION("sdPack_Type",&sdpackType,-1,-1);
	ADD_FUNCTION("sdPack_Count",&sdpackCount,-1,-1);
	ADD_FUNCTION("sdPack_Params",&sdpackParams,-1,-1);

	ADD_FUNCTION("sdPack_FromHuman",&sdPackFromHuman,-1,-1);

	ADD_FUNCTION("sdData_FilterAccept",&sdDataGetPackFromType,-1,-1);
	ADD_FUNCTION("sdData_FilterReject",&sdDataGetPackFromTypeRest,-1,-1);

	ADD_FUNCTION("sdData_First",&sdDataGetFirst,-1,-1);
	ADD_FUNCTION("sdData_Rest",&sdDataGetRest,-1,-1);

	/*
	ADD_FUNCTION("sdPack_ToMessage",&todo,-1,-1);
	ADD_FUNCTION("sdPack_FromMessage",&todo,-1,-1);
	*/

	ADD_FUNCTION("Delay", &delay, 1, -1 );
	ADD_FUNCTION("Call", &callFunction, 1, -1);

	t_atom atom_temp;
	SETSYMBOL( &atom_temp, gensym( "NOP" ) );
	pNOP = getCommandInfo(
			command_infos,
			&atom_temp
	);
	SETSYMBOL( &atom_temp, gensym( "RETURN_ALL" ) );
	pRETURN_ALL = getCommandInfo(
			command_infos,
			&atom_temp
	);

	ADD_FUNCTION_CONTINUATION("If",&if_,-1,1, pRETURN_ALL);
	return command_infos;
}

void commands_exit(
		CommandInfos* x
)
{
	CommandInfos_exit( x );
	freebytes( x, sizeof( CommandInfos ) );
}

CommandInfo* getCommandInfo(
		CommandInfos* command_infos,
		t_atom* pName
)
{
	for(int i=0; i<CommandInfos_get_size( command_infos ); i++)
	{
		CommandInfo* current = & CommandInfos_get_array( command_infos )[i];
		if( compareAtoms( & current->name, pName))
			return current;
	}
	return NULL;
}

CommandInfo* get_NOP(
		CommandInfos* command_infos
)
{
	return pNOP;
}
CommandInfo* get_RETURN_ALL(
		CommandInfos* command_infos
)
{
	return pRETURN_ALL;
}

/*****************************************
 * command implementations:
 *****************************************/

PFUNCTION_HEADER( add )
{
	DB_PRINT("add");
	t_atom* pResult = getbytes(sizeof(t_atom));
	SETFLOAT( pResult, atom_getfloat(& pArgs[0]) + atom_getfloat(& pArgs[1]));
	//push result on stack:
	AtomList_append( & prog_rt -> stack, pResult);
}


PFUNCTION_HEADER( sub )
{
	DB_PRINT("sub");
	t_atom* pResult = getbytes(sizeof(t_atom));
	SETFLOAT( pResult, atom_getfloat(&pArgs[0]) - atom_getfloat(&pArgs[1]));
	//push result on stack:
	AtomList_append( & prog_rt -> stack, pResult);
}
PFUNCTION_HEADER( mul )
{
	DB_PRINT("mul");
	t_atom* pResult = getbytes(sizeof(t_atom));
	SETFLOAT( pResult, atom_getfloat(&pArgs[0]) * atom_getfloat(& pArgs[1]));
	//push result on stack:
	AtomList_append( & prog_rt -> stack, pResult);
}
PFUNCTION_HEADER( div_ )
{
	DB_PRINT("div");
	if( atom_getint(& pArgs[1]) == 0)
	{
		post("ERROR: div: division by 0");
		return;
	}
	t_atom* pResult = getbytes(sizeof(t_atom));
	SETFLOAT( pResult, atom_getfloat(&pArgs[0]) / atom_getfloat(& pArgs[1]));
	//push result on stack:
	AtomList_append( & prog_rt -> stack, pResult);
}
PFUNCTION_HEADER( mod )
{
	DB_PRINT("mod");
	if( atom_getint(& pArgs[1]) == 0)
	{
		post("ERROR: mod: division by 0");
		return;
	}
	t_atom* pResult = getbytes(sizeof(t_atom));
	SETFLOAT( pResult, atom_getint(&pArgs[0]) % atom_getint(& pArgs[1]));
	//push result on stack:
	AtomList_append( & prog_rt -> stack, pResult);
}

PFUNCTION_HEADER( print )
{
	for ( int i=0; i<countArgs; i++)
	{
		OutputBuf_append( & prog_rt->rt -> output_buffer, pArgs[i] );
	}
}

PFUNCTION_HEADER( pack )
{
	t_atom** pResult = getbytes(sizeof(t_atom* )* (countArgs + 2));
	pResult[0] = getbytes( sizeof(t_atom) );
	*pResult[0] = pArgs[0];
	t_atom atomSizePack;
	SETFLOAT( &atomSizePack, countArgs-1 );
	pResult[1] = getbytes( sizeof(t_atom) );
	*pResult[1] = atomSizePack;
	for(int i=0; i< countArgs-1; i++)
	{
		pResult[2+i] = getbytes( sizeof(t_atom) );
		* pResult[2+i] = pArgs[i+1];
	}
	AtomList_append( & prog_rt -> stack, pResult[0]);
	AtomList_append( & prog_rt -> stack, pResult[1]);
	for(int i=0; i< countArgs-1; i++)
	{
		AtomList_append( & prog_rt -> stack, pResult[2+i]);
	}
	freebytes( pResult, sizeof(t_atom* ) * (countArgs+2));
}

PFUNCTION_HEADER( out )
{
	t_atom* array =
		OutputBuf_get_array( & prog_rt -> rt -> output_buffer );
	t_int size =
		OutputBuf_get_size( & prog_rt -> rt -> output_buffer );
	t_outlet* outlet = prog_rt -> rt -> outlet;
	if( outlet )
	{
		if( countArgs == 0 )
		{
			if( size > 0 )
			{
				if( array[0].a_type == A_SYMBOL )
				{
					
					outlet_anything(
						outlet,
						atom_getsymbol( & array[0] ),
						size - 1,
						& array[1]
					);
				}
				else
				{
					outlet_anything(
						prog_rt -> rt -> outlet,
						&s_list,
						size,
						array
					);
				}
			}
		}
		else
		{
			t_symbol* selector = atom_getsymbol(
				& pArgs[0]
			);
			outlet_anything(
				prog_rt -> rt -> outlet,
				selector,
				size,
				array
			);
		}
	}
	OutputBuf_clear( & prog_rt -> rt -> output_buffer );
}

PFUNCTION_HEADER( addVar )
{
	if( countArgs == 0 )
	{
		post("WARNING: addVar with zero parameters called!");
		return;
	}

	AtomDynA* new_var = getbytes( sizeof( AtomDynA ) );
	AtomDynA_init( new_var );
	AtomDynA_set_size( new_var, countArgs-1 );
	memcpy(
			AtomDynA_get_array( new_var ),
			& pArgs[1],
			sizeof( t_atom ) * (countArgs-1)
	);
	Scope_insert(
			prog_rt -> scope,
			atom_getsymbol( &pArgs[0] ),
			new_var
	);
}


PFUNCTION_HEADER( getVar )
{
	AtomDynA* value = symtab_get_var(
			prog_rt->scope,
			prog_rt->global_scopes,
			atom_getsymbol( & pArgs[0] )
	);
	if( ! value )
	{
		char buf[256];
		atom_string( & pArgs[0], buf, 256 );
		post("ERROR: getVar: couldn't find variable \"%s\"", buf);
		return;
	}

	for( int i=0; i< AtomDynA_get_size( value ); i++ )
	{
		t_atom* pResult = getbytes(sizeof(t_atom));
		(*pResult) = AtomDynA_get_array( value )[i];
		AtomList_append( & prog_rt -> stack, pResult);
	}
}

PFUNCTION_HEADER( getVarA )
{
	AtomDynA* value = symtab_get_var(
			prog_rt->scope,
			prog_rt->global_scopes,
			atom_getsymbol( & pArgs[0] )
	);
	if( ! value )
	{
		char buf[256];
		atom_string( & pArgs[0], buf, 256 );
		post("ERROR: getVar: couldn't find variable \"%s\"", buf);
		return;
	}
	t_int index = atom_getint( & pArgs[1] );
	if( index < 0 || index >= AtomDynA_get_size( value ) )
	{
		post("ERROR: getVarA: index out of bounds: \"%i\"", (int )index);
		return;
	}

	t_atom* pResult = getbytes(sizeof(t_atom));
	(*pResult) = AtomDynA_get_array( value )[index];
	AtomList_append( & prog_rt -> stack, pResult);
}

PFUNCTION_HEADER( setVar )
{
	if( countArgs == 0 )
	{
		post("WARNING: setVar called with zero parameters");
		return;
	}
	DB_PRINT("setVar called with %i args", countArgs);

	AtomDynA* value = symtab_get_var(
			prog_rt->scope,
			prog_rt->global_scopes,
			atom_getsymbol( & pArgs[0] )
	);
	if( ! value )
	{
		char buf[256];
		atom_string( & pArgs[0], buf, 256 );
		post("ERROR: variable \"%s\" not found!", buf);
		return;
	}
	AtomDynA_set_size(
			value,
			countArgs-1
	);
	memcpy(
			AtomDynA_get_array( value ),
			& pArgs[1],
			sizeof( t_atom ) * (countArgs-1)
	);
}

PFUNCTION_HEADER( setVarA )
{
	AtomDynA* value = symtab_get_var(
			prog_rt->scope,
			prog_rt->global_scopes,
			atom_getsymbol( & pArgs[0] )
	);
	if( ! value )
	{
		char buf[256];
		atom_string( & pArgs[0], buf, 256 );
		post("ERROR: variable \"%s\" not found!", buf);
		return;
	}
	t_int index = atom_getfloat( &pArgs[1] );
	t_atom* new_value = & pArgs[2];
	if( index < 0 || index >= AtomDynA_get_size( value ) )
	{
		post("ERROR: setVarA: index out of bounds: \"%i\"", (int )index);
		return;
	}
	AtomDynA_get_array( value )[index] = *new_value;
}

PFUNCTION_HEADER( addMainVar )
{
	if( countArgs == 0 )
	{
		post("WARNING: addVar with zero parameters called!");
		return;
	}

	AtomDynA* new_var = getbytes( sizeof( AtomDynA ) );
	AtomDynA_init( new_var );
	AtomDynA_set_size( new_var, countArgs-1 );
	memcpy(
			AtomDynA_get_array( new_var ),
			& pArgs[1],
			sizeof( t_atom ) * (countArgs-1)
	);
	if( ScopeList_get_size( prog_rt -> global_scopes ) )
	{
		Scope_insert(
				ScopeList_get_last( prog_rt -> global_scopes)->pData,
				atom_getsymbol( &pArgs[0] ),
				new_var
		);
	}
}

PFUNCTION_HEADER( clearMain )
{
	if( ScopeList_get_size( prog_rt -> global_scopes ) )
	{
		Scope_clear(
				ScopeList_get_last( prog_rt -> global_scopes)->pData
		);
	}
}

PFUNCTION_HEADER( if_ )
{
	if( atom_getfloat(& pArgs[0]) )
	{
		/*
		CmdRuntimeData* pCurrentCmdRuntimeData = getbytes(sizeof(CmdRuntimeData));
		pCurrentCmdRuntimeData -> stackHeight0 = AtomList_get_size ( & prog_rt -> stack );
		pCurrentCmdRuntimeData -> pCommandInfo = get_RETURN_ALL(
				prog_rt -> rt -> command_infos
		);
		CommandStack_append( & prog_rt -> command_stack, pCurrentCmdRuntimeData);
		*/
	}
	else
	{
		prog_rt->skipMode = TRUE;
	}
}

// sgScales:
PFUNCTION_HEADER( sgFunc )
{
	t_float x = atom_getfloat( & pArgs[0] );
	t_float stepAdd = atom_getfloat( & pArgs[1] );
	t_float step0 = atom_getfloat( & pArgs[2] );
	t_float c = atom_getfloat( & pArgs[3] );
	t_atom* pResult = getbytes(sizeof(t_atom));
	SETFLOAT( pResult, c + stepAdd/2 * x*x + ( step0 - stepAdd/2 ) * x );
	AtomList_append( & prog_rt -> stack, pResult);
}
PFUNCTION_HEADER( sgScale )
{
	t_float n = atom_getfloat( & pArgs[0] );
	t_float stepAdd = atom_getfloat( & pArgs[1] );
	t_float step0 = atom_getfloat( & pArgs[2] );
	t_float c = atom_getfloat( & pArgs[3] );

	for( int i=0; i<n; i++ )
	{
		t_atom* pResult = getbytes(sizeof(t_atom));
		SETFLOAT( pResult, c + stepAdd/2 * i*i + ( step0 - stepAdd/2 ) * i );
		AtomList_append( & prog_rt -> stack, pResult);
	}
}

// boolean operators:
PFUNCTION_HEADER( and_ )
{
	t_float a = atom_getfloat( & pArgs[0] );
	t_float b = atom_getfloat( & pArgs[1] );

	t_atom* pResult = getbytes(sizeof(t_atom));
	SETFLOAT( pResult, a && b );
	AtomList_append( & prog_rt -> stack, pResult);
}
PFUNCTION_HEADER( or_ )
{
	t_float a = atom_getfloat( & pArgs[0] );
	t_float b = atom_getfloat( & pArgs[1] );

	t_atom* pResult = getbytes(sizeof(t_atom));
	SETFLOAT( pResult, a || b );
	AtomList_append( & prog_rt -> stack, pResult);
}
PFUNCTION_HEADER( not_ )
{
	t_float a = atom_getfloat( & pArgs[0] );

	t_atom* pResult = getbytes(sizeof(t_atom));
	SETFLOAT( pResult, ! a );
	AtomList_append( & prog_rt -> stack, pResult);
}

// comparison operators:
PFUNCTION_HEADER( isEqual )
{
	t_float a = atom_getfloat( & pArgs[0] );
	t_float b = atom_getfloat( & pArgs[1] );

	t_atom* pResult = getbytes(sizeof(t_atom));
	SETFLOAT( pResult, a == b );
	AtomList_append( & prog_rt -> stack, pResult);
}
PFUNCTION_HEADER( isNotEqual )
{
	t_float a = atom_getfloat( & pArgs[0] );
	t_float b = atom_getfloat( & pArgs[1] );

	t_atom* pResult = getbytes(sizeof(t_atom));
	SETFLOAT( pResult, a != b );
	AtomList_append( & prog_rt -> stack, pResult);

}
PFUNCTION_HEADER( isLessThan )
{
	t_float a = atom_getfloat( & pArgs[0] );
	t_float b = atom_getfloat( & pArgs[1] );

	t_atom* pResult = getbytes(sizeof(t_atom));
	SETFLOAT( pResult, a < b );
	AtomList_append( & prog_rt -> stack, pResult);
}
PFUNCTION_HEADER( isGreaterThan )
{
	t_float a = atom_getfloat( & pArgs[0] );
	t_float b = atom_getfloat( & pArgs[1] );

	t_atom* pResult = getbytes(sizeof(t_atom));
	SETFLOAT( pResult, a > b );
	AtomList_append( & prog_rt -> stack, pResult);
}
PFUNCTION_HEADER( isLessOrEqual )
{
	t_float a = atom_getfloat( & pArgs[0] );
	t_float b = atom_getfloat( & pArgs[1] );

	t_atom* pResult = getbytes(sizeof(t_atom));
	SETFLOAT( pResult, a <= b );
	AtomList_append( & prog_rt -> stack, pResult);
}
PFUNCTION_HEADER( isGreaterOrEqual )
{
	t_float a = atom_getfloat( & pArgs[0] );
	t_float b = atom_getfloat( & pArgs[1] );

	t_atom* pResult = getbytes(sizeof(t_atom));
	SETFLOAT( pResult, a >= b );
	AtomList_append( & prog_rt -> stack, pResult);
}

typedef enum ESetOp { UNION, MINUS } SetOp;
// Set operations:
BOOL AtomListCompareAtoms(t_atom* pInList, t_atom* p)
{
	return compareAtoms(pInList, p);
}
BOOL setContains(t_int count, t_atom* set, t_atom* element);
BOOL listContains(AtomList* pList, t_atom* pElement);

PFUNCTION_HEADER( setify )
{
	for( int i=0; i<countArgs; i++ )
	{
		t_atom* pCurrent = & pArgs[i];
		if( ! setContains( i-1, pArgs, pCurrent ) )
		{
			t_atom* pResult = getbytes(sizeof(t_atom));
			*pResult = *pCurrent;
			AtomList_append( & prog_rt -> stack, pResult);
		}
	}
}
PFUNCTION_HEADER( card )
{
	t_atom* pResult = getbytes(sizeof(t_atom));
	SETFLOAT( pResult, countArgs );
	AtomList_append( & prog_rt -> stack, pResult);
}

PFUNCTION_HEADER( setOp )
{
	AtomList listReturn;
	AtomList_init( & listReturn );
	// brute force.
	// a better solution would be to sort both lists, and then to merge them:
	t_int countFirst = 0;
	BOOL afterOp = FALSE;
	SetOp op;
	for( int i=0; i<countArgs; i++ )
	{
		t_atom* pCurrent = & pArgs[i];
		if( afterOp == FALSE )
		{
			if( atomEqualsString( pCurrent, "union" ) )
			{
				op = UNION;
				afterOp = TRUE;
			}
			else if( atomEqualsString( pCurrent, "minus" ) )
			{
				op = MINUS;
				afterOp = TRUE;
			}
			else
			{
				countFirst ++ ;
				t_atom* pAdd = getbytes(sizeof(t_atom));
				(* pAdd) = * pCurrent;
				AtomList_append( & listReturn, pAdd );
			}
		}
		else
		{
			AtomListEl* pElToDelete = NULL;
			switch( op )
			{
				case UNION:
					if( ! listContains( &listReturn, pCurrent ) )
					{
						t_atom* pAdd = getbytes(sizeof(t_atom));
						(* pAdd) = * pCurrent;
						AtomList_append( & listReturn, pAdd );
					}
				break;
				case MINUS:
					pElToDelete = AtomList_get_element( &listReturn, pCurrent, AtomListCompareAtoms);
					if( pElToDelete )
					{
						AtomList_del( & listReturn, pElToDelete );
					}
				break;
			}
		}
	}
	AtomListEl* pCurrent = AtomList_get_first( & listReturn );
	while ( pCurrent )
	{
		t_atom* pResult = getbytes(sizeof(t_atom));
		(*pResult) = (* pCurrent -> pData);
		AtomList_append( & prog_rt -> stack, pResult);
		
		pCurrent = AtomList_get_next( & listReturn, pCurrent );
	}
	AtomList_exit( & listReturn );
}

BOOL listContains(AtomList* pList, t_atom* pElement)
{
	AtomListEl* pElCurrent = AtomList_get_first( pList );
	while( pElCurrent )
	{
		if( atom_getfloat( pElCurrent->pData) == atom_getfloat( pElement ) )
			return TRUE;
		pElCurrent = AtomList_get_next( pList, pElCurrent );
	}
	return FALSE;
}

PFUNCTION_HEADER( contains )
{
	t_atom element = ( pArgs[0] );
	BOOL bRet = setContains( countArgs-1, & pArgs[1], &element);

	t_atom* pResult = getbytes(sizeof(t_atom));
	SETFLOAT( pResult, bRet );
	AtomList_append( & prog_rt -> stack, pResult);
}

BOOL setContains(t_int count, t_atom* set, t_atom* element)
{
	for( int i=0; i<count; i++ )
	{
		if( atom_getfloat(& set[i]) == atom_getfloat( element ))
			return TRUE;
	}
	return FALSE;
}

PFUNCTION_HEADER( calcTransp )
{
	// the sets should be separated by |
	AtomList listReturn;
	AtomList_init( & listReturn );
	// brute force.
	// a better solution would be to sort both lists, and then to merge them:
	t_int countFirst = 0;
	for( int i=0; i<countArgs; i++ )
	{
		t_atom* pCurrent = & pArgs[i];
		if( atomEqualsString( pCurrent, "|" ) )
		{
			break;
		}
		else
		{
			countFirst ++ ;
		}
	}
	if( countFirst == countArgs )
	{
		post("ERROR: separator \"|\" not found!");
		return;
	}
	for( int transp=0; transp<12; transp++ )
	{
		BOOL passed = TRUE;
		for( int i=countFirst+1; i<countArgs; i++ )
		{
			t_atom current;
			SETFLOAT( & current, (t_int )(atom_getfloat( & pArgs[i]) + transp) % 12 );
			if( setContains( countArgs-countFirst-1, pArgs, & current ) )
			{
				passed = FALSE;
			}
		}
		if( passed )
		{
			t_atom* pResult = getbytes(sizeof(t_atom));
			SETFLOAT( pResult, transp );
			AtomList_append( & listReturn, pResult );
		}
	}
	AtomListEl* pCurrent = AtomList_get_first( & listReturn );
	while ( pCurrent )
	{
		t_atom* pResult = getbytes(sizeof(t_atom));
		(*pResult) = (* pCurrent -> pData);
		AtomList_append( & prog_rt -> stack, pResult);
		
		pCurrent = AtomList_get_next( & listReturn, pCurrent );
	}
	AtomList_exit( & listReturn );
}

PFUNCTION_HEADER( addA )
{
	if( countArgs == 0 )
	{
		post("ERROR: addA called with no parameters!");
		return;
	}
	t_atom a = pArgs[0] ;
	for( int i=1; i<countArgs; i++)
	{
		t_atom x = pArgs[i] ;
		t_atom* pResult = getbytes(sizeof(t_atom));
		SETFLOAT( pResult, atom_getfloat(&x) + atom_getfloat(&a) );
		AtomList_append( & prog_rt -> stack, pResult);
	}
}
PFUNCTION_HEADER( subA )
{
	if( countArgs == 0 )
	{
		post("ERROR: subA called with no parameters!");
		return;
	}
	t_atom a = pArgs[0] ;
	for( int i=1; i<countArgs; i++)
	{
		t_atom x = pArgs[i] ;
		t_atom* pResult = getbytes(sizeof(t_atom));
		SETFLOAT( pResult, atom_getfloat(&x) - atom_getfloat(&a) );
		AtomList_append( & prog_rt -> stack, pResult);
	}
}
PFUNCTION_HEADER( mulA )
{
	if( countArgs == 0 )
	{
		post("ERROR: mulA called with no parameters!");
		return;
	}
	t_atom a = pArgs[0] ;
	for( int i=1; i<countArgs; i++)
	{
		t_atom x = pArgs[i] ;
		t_atom* pResult = getbytes(sizeof(t_atom));
		SETFLOAT( pResult, atom_getfloat(&x) * atom_getfloat(&a) );
		AtomList_append( & prog_rt -> stack, pResult);
	}
}
PFUNCTION_HEADER( divA )
{
	if( countArgs == 0 )
	{
		post("ERROR: divA called with no parameters!");
		return;
	}
	t_atom a = pArgs[0] ;
	if( atom_getint(& pArgs[1]) == 0)
	{
		post("ERROR: divA: division by 0");
		return;
	}
	for( int i=1; i<countArgs; i++)
	{
		t_atom x = pArgs[i] ;
		t_atom* pResult = getbytes(sizeof(t_atom));
		SETFLOAT( pResult, atom_getfloat(&x) / atom_getfloat(&a) );
		AtomList_append( & prog_rt -> stack, pResult);
	}
}

PFUNCTION_HEADER( modA )
{
	if( countArgs == 0 )
	{
		post("ERROR: modA called with no parameters!");
		return;
	}
	t_atom m = pArgs[0];
	if( atom_getint(& m) == 0)
	{
		post("ERROR: modA error: division by 0");
		return;
	}
	for( int i=1; i<countArgs; i++)
	{
		t_atom x = pArgs[i] ;
		t_atom* pResult = getbytes(sizeof(t_atom));
		SETFLOAT( pResult, atom_getint(&x) % atom_getint(&m) );
		AtomList_append( & prog_rt -> stack, pResult);
	}
}

PFUNCTION_HEADER( sdMinMax )
{
	t_float pMin = atom_getfloat( & pArgs[0] );
	t_float pMax = atom_getfloat( & pArgs[1] );
	t_float pInput = atom_getfloat( & pArgs[2] );
	t_atom* pResult = getbytes(sizeof(t_atom));
	SETFLOAT(
		pResult,
		((pMax) - (pMin)) * (pInput) + (pMin)
	);
	AtomList_append( & prog_rt -> stack, pResult);
}

// random
// Rand ( )
PFUNCTION_HEADER( random_ )
{
	t_float min = atom_getfloat( & pArgs[0] );
	t_float max = atom_getfloat( & pArgs[1] );
	t_atom* pResult = getbytes(sizeof(t_atom));
	SETFLOAT( pResult, min + (rand() % 1000000)/1000000.0 * (max-min) );
	AtomList_append( & prog_rt -> stack, pResult);
}

PFUNCTION_HEADER( nop )
{
}

PFUNCTION_HEADER( returnAll )
{
	for( int i=0; i<countArgs; i++ )
	{
		t_atom* pResult = getbytes(sizeof(t_atom));
		*pResult = pArgs[i] ;
		AtomList_append( & prog_rt -> stack, pResult);
	}
}


PFUNCTION_HEADER( rndInt )
{
	t_int min = atom_getfloat( & pArgs[0] );
	t_int max = atom_getfloat( & pArgs[1] );
	t_atom* pResult = getbytes(sizeof(t_atom));
	SETFLOAT( pResult, min + (rand() % (max - min + 1) ) );
	AtomList_append( & prog_rt -> stack, pResult);
}
PFUNCTION_HEADER( inc )
{
	if( countArgs == 0 )
	{
		post("WARNING: inc called with zero parameters");
		return;
	}
	DB_PRINT("inc called with %i args", countArgs);

	AtomDynA* value = symtab_get_var(
			prog_rt->scope,
			prog_rt->global_scopes,
			atom_getsymbol( & pArgs[0] )
	);

	char buf[256];
	atom_string( & pArgs[0], buf, 256 );
	if( ! value )
	{
		post("ERROR: variable \"%s\" not found!", buf);
		return;
	}
	if( ! AtomDynA_get_size( value ) )
	{
		post("ERROR: inc: variable \"%s\" is empty!", buf);
		return;
	}
	t_int old_val =
		atom_getint(
				& AtomDynA_get_array( value )[0]
		);
	SETFLOAT(
			& AtomDynA_get_array( value )[0],
			old_val+1
	);
}
PFUNCTION_HEADER( dec )
{
	if( countArgs == 0 )
	{
		post("WARNING: inc called with zero parameters");
		return;
	}
	DB_PRINT("inc called with %i args", countArgs);

	AtomDynA* value = symtab_get_var(
			prog_rt->scope,
			prog_rt->global_scopes,
			atom_getsymbol( & pArgs[0] )
	);
	char buf[256];
	atom_string( & pArgs[0], buf, 256 );
	if( ! value )
	{
		post("ERROR: variable \"%s\" not found!", buf);
		return;
	}
	if( ! AtomDynA_get_size( value ) )
	{
		post("ERROR: inc: variable \"%s\" is empty!", buf);
		return;
	}
	t_int old_val =
		atom_getint(
				& AtomDynA_get_array( value )[0]
		);
	SETFLOAT(
			& AtomDynA_get_array( value )[0],
			old_val-1
	);
}

PFUNCTION_HEADER( rndIntUnequal )
{
	t_int count = countArgs;
	t_atom* pResult = getbytes(sizeof(t_atom));
	SETFLOAT( pResult, -1 ); // (set a default value)
	t_float rand_ = (rand() % 1000000) / 1000000.0; 
	t_float current = 0;
	for( int i=0; i<count; i++ )
	{
		current += atom_getfloat( & pArgs[i] );
		if( rand_ < current )
		{
			SETFLOAT( pResult,  i );
			break;
		}
	}
	AtomList_append( & prog_rt -> stack, pResult);
}


PFUNCTION_HEADER(sdpackType)
{
	t_atom* pResult = getbytes(sizeof(t_atom));
	(*pResult) = pArgs[0];
	AtomList_append( & prog_rt->stack, pResult);
}
PFUNCTION_HEADER(sdpackCount)
{
	t_atom* pResult = getbytes(sizeof(t_atom));
	(*pResult) = pArgs[1];
	AtomList_append( & prog_rt->stack, pResult);
}
PFUNCTION_HEADER(sdpackParams)
{
	t_int count = atom_getfloat( & pArgs[1]);
	for( int i=0; i<count; i++)
	{
		t_atom* pResult = getbytes(sizeof(t_atom));
		(*pResult) = pArgs[2+i];
		AtomList_append( & prog_rt->stack, pResult);
	}
}

PFUNCTION_HEADER( sdPackFromHuman )
{
	AtomPointerList stackSizeInfo; // a stack of the atoms with the current packs size
	t_int indexNew = 0; // index in the return values
	AtomPointerList_init( &stackSizeInfo );
	for( int index=0; index < countArgs; index++ )
	{
		t_atom* pCurrent = & pArgs[index];
		if( atom_getsymbol(pCurrent) == gensym("(") )
		{
			t_atom* pAtom = getbytes( sizeof(t_atom));
			SETFLOAT(pAtom, indexNew);
			AtomList_append( & prog_rt->stack, pAtom);

			AtomPointerList_append( &stackSizeInfo,pAtom);
			indexNew ++;
		}
		else if( atom_getsymbol(pCurrent) == gensym(")") )
		{
			if( AtomPointerList_get_size( &stackSizeInfo ) > 0 )
			{
				ElementAtomPointer* pEl = AtomPointerList_get_last(&stackSizeInfo);
				SETFLOAT( pEl->pData, indexNew - atom_getfloat(pEl->pData) -1 );
				AtomPointerList_del( &stackSizeInfo, pEl );
			}
			else
			{
				post("ERROR: sdPackFromHuman: \")\" found without corresponding \"(\"!");
			}
		}
		else
		{
			t_atom* pAtom = getbytes( sizeof(t_atom));
			(*pAtom) = pArgs[index] ;
			AtomList_append( & prog_rt->stack, pAtom);
			indexNew ++;
		}
	}
	if( AtomPointerList_get_size( &stackSizeInfo ) > 0 )
	{
		post("WARNING: sdPackFromHuman: more \"(\" than \")\"!");
	}
	AtomPointerList_exit( &stackSizeInfo );
}

// sdData (lists of sdPacks):
PFUNCTION_HEADER( sdDataGetPackFromType )
{
	t_atom* pType = &pArgs[0];
	t_int pos = 1;
	while( pos < countArgs )
	{
		t_atom* pCurrentType = & pArgs[pos];
		t_int count = atom_getfloat(& pArgs[pos+1]);
		if( atom_getsymbol(pCurrentType) == atom_getsymbol(pType))
		{
			for( int i=pos; i<pos+2+count; i++ )
			{
				t_atom* pAtom = getbytes( sizeof(t_atom));
				(*pAtom) = pArgs[i] ;
				AtomList_append( & prog_rt->stack, pAtom);
			}
		}
		pos += (2 + count);
	}
}
PFUNCTION_HEADER( sdDataGetPackFromTypeRest )
{
	t_atom* pType = &pArgs[0];
	t_int pos = 1;
	while( pos < countArgs )
	{
		t_atom* pCurrentType = & pArgs[pos];
		t_int count = atom_getfloat(& pArgs[pos+1]);
		if( atom_getsymbol(pCurrentType) != atom_getsymbol(pType))
		{
			for( int i=pos; i<pos+2+count; i++ )
			{
				t_atom* pAtom = getbytes( sizeof(t_atom));
				(*pAtom) = pArgs[i] ;
				AtomList_append( & prog_rt->stack, pAtom);
			}
		}
		pos += (2 + count);
	}
}

// returns the first sdPack
PFUNCTION_HEADER( sdDataGetFirst )
{
	if( countArgs == 0)
		return;
	if( countArgs < 2)
	{
		post("WARNING: sdDataGetRest: invalid pack!");
		return;
	}
	t_int pos = 0;
	
	t_int count = atom_getfloat(& pArgs[pos+1]);
	for( int i=pos; i<pos+2+count; i++ )
	{
		t_atom* pAtom = getbytes( sizeof(t_atom));
		(*pAtom) = pArgs[i] ;
		AtomList_append( & prog_rt->stack, pAtom);
	}
}


PFUNCTION_HEADER( sdDataGetRest )
{
	if( countArgs == 0)
		return;
	if( countArgs < 2)
	{
		post("WARNING: sdDataGetRest: invalid pack!");
		return;
	}
	t_int pos = 0;
	
	t_int count = atom_getfloat(& pArgs[pos+1]);
	pos += (2 + count);
	for( ; pos<countArgs; pos++ )
	{
		t_atom* pAtom = getbytes( sizeof(t_atom));
		(*pAtom) = pArgs[pos] ;
		AtomList_append( & prog_rt->stack, pAtom);
	}
}

PFUNCTION_HEADER( delay )
{
	prog_rt -> rt -> delay =
		atom_getint( & pArgs[0] );
}

PFUNCTION_HEADER( callFunction )
{
	prog_rt -> rt -> jump_to_program =
			atom_getsymbol( & pArgs[0] );
}
