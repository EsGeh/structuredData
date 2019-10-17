#ifndef _GLOBAL_H_
#define _GLOBAL_H_

#include "Basic.h"
#include "LinkedList.h"
#include "DynArray.h"

#include "m_pd.h"


#ifndef DEBUG
	#define DB_PRINT(message, ...)
#else
	#define DB_PRINT(message, ...)\
	post(message, ## __VA_ARGS__)
#endif

// common container types:

DECL_LIST(AtomList,AtomListEl,t_atom,getbytes,freebytes,freebytes)
DEF_LIST(AtomList,AtomListEl,t_atom,getbytes,freebytes,freebytes);

// pointers to atoms are considered references, therefore not deleted automatically:
#pragma GCC diagnostic ignored "-Wunused-value"
DECL_LIST(AtomPointerList,ElementAtomPointer,t_atom,getbytes,freebytes,)
DEF_LIST(AtomPointerList,ElementAtomPointer,t_atom,getbytes,freebytes,);
#pragma GCC diagnostic pop

DECL_DYN_ARRAY(AtomDynA,t_atom,getbytes,freebytes)
DEF_DYN_ARRAY(AtomDynA,t_atom,getbytes,freebytes)

DECL_BUFFER(AtomBuf,t_atom,getbytes,freebytes)
DEF_BUFFER(AtomBuf,t_atom,getbytes,freebytes)

DECL_BUFFER(CharBuf, char, getbytes, freebytes)
DEF_BUFFER(CharBuf, char, getbytes, freebytes)

DECL_BUFFER(IntBuf, int, getbytes, freebytes)
DEF_BUFFER(IntBuf, int, getbytes, freebytes)

DECL_BUFFER(SymBuf, t_symbol*, getbytes, freebytes)
DEF_BUFFER(SymBuf, t_symbol*, getbytes, freebytes)

// used by maps of type "t_symbol* -> ?"
INLINE BOOL COMPARE_SYMBOLS( t_symbol** k1, t_symbol** k2 )
{
	return *k1 == *k2;
}



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

INLINE BOOL atomEqualsString(t_atom* pAtom, char* string)
{
	char buf[256];
	atom_string(pAtom,buf,256);
	if(!strncmp(buf,string,256))
		return TRUE;
	return FALSE;
}

#endif
