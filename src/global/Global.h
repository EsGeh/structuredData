#ifndef _GLOBAL_H_
#define _GLOBAL_H_

#include "Basic.h"

#include "m_pd.h"


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
