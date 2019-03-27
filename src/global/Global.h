#ifndef _GLOBAL_H_
#define _GLOBAL_H_

#include "Basic.h"

#include "m_pd.h"


#ifndef DEBUG
	#define DB_PRINT(message, ...)
#else
	#define DB_PRINT(message, ...)\
	post(message, ## __VA_ARGS__)
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


#endif
