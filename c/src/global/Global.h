#ifndef _GLOBAL_H_
#define _GLOBAL_H_

#include "m_pd.h"


#define BOOL int
#define TRUE 1
#define FALSE 0

#define INLINE static inline

#ifndef NULL
    #define NULL 0
#endif


INLINE int compareAtoms(t_atom* atoml, t_atom* atomr)
{
	if(atoml -> a_type == A_SYMBOL)
	{
		if(atomr -> a_type == A_SYMBOL)
		{
			return atom_getsymbol( atoml ) == atom_getsymbol( atomr );
		}
	}
	else
	{
		if(atomr -> a_type == A_FLOAT)
		{
			return atom_getfloat(atoml)==atom_getfloat(atomr);
		}
	}
	return 0;
}

/*
#define SETSEMI(atom) ((atom)->a_type = A_SEMI, (atom)->a_w.w_index = 0)
#define SETCOMMA(atom) ((atom)->a_type = A_COMMA, (atom)->a_w.w_index = 0)
#define SETPOINTER(atom, gp) ((atom)->a_type = A_POINTER, \
    (atom)->a_w.w_gpointer = (gp))
#define SETFLOAT(atom, f) ((atom)->a_type = A_FLOAT, (atom)->a_w.w_float = (f))
#define SETSYMBOL(atom, s) ((atom)->a_type = A_SYMBOL, \
    (atom)->a_w.w_symbol = (s))
#define SETDOLLAR(atom, n) ((atom)->a_type = A_DOLLAR, \
    (atom)->a_w.w_index = (n))
#define SETDOLLSYM(atom, s) ((atom)->a_type = A_DOLLSYM, \
    (atom)->a_w.w_symbol= (s))

INLINE int atom_assign(t_atom* atoml, t_atom* atomr)
{
	if(atomr -> a_type == A_SYMBOL)
	{
		SETSYMBOL( atoml,atom_getsymbol( atomr ) );
	}
	else if(atomr -> a_type == A_FLOAT)
	{
		SETFLOAT(atoml, atom_getfloat(atomr) );
	}
	return 0;
}
*/

/*
#include "m_pd.h"
#include "LinkedList.h"
#include <string.h>
#define CLASSNAME(name) name##Class
#define TYPENAME(name) t_##name

#define CONSTRUCTOR(name, ...) void* name##ObjNew(__VA_ARGS__)
#define DESTRUCTOR(name,pThis) void name##ObjFree(TYPENAME(name)* pThis)
#define NEW_INSTANCE(name,varname) TYPENAME(name)* varname = (TYPENAME(name)* )pd_new(CLASSNAME(name))


#include <stdio.h>


#define IN_RANGE(x, min,sup) (x>=min && x<=sup)

#define MAX_MESSAGE_LENGTH 256
#define POST(msg, ...)\
	do\
	{\
		char __buf[MAX_MESSAGE_LENGTH];\
		sprintf(__buf, msg, ## __VA_ARGS__);\
		post(__buf);\
	} while(0);

#ifndef DEBUG
	#define DB_PRINT(message, ...)
#else
	#define DB_PRINT(message, ...)\
	POST(message, ## __VA_ARGS__)
#endif

#define TRY(code,errorMsg)\
	do \
	{\
		if(!(code))\
		{\
			error(errorMsg);\
		}\
	}\
	while(0);

#define TRY_RET(code,errorMsg,ret)\
	do \
	{\
		if(!(code))\
		{\
			error(errorMsg);\
			return ret;\
		}\
	}\
	while(0);

INLINE BOOL atomEqualsString(t_atom* pAtom, char* string)
{
	char buf[256];
	atom_string(pAtom,buf,256);
	if(!strncmp(buf,string,256))
		return TRUE;
	return FALSE;
}
*/

#endif
