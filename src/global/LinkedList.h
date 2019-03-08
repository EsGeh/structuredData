#ifndef _LINKED_LIST_HEADER_
#define _LINKED_LIST_HEADER_

#include "Global.h"


// algorithms:

#define LIST_FORALL_REV_BEGIN(LIST,ELEMENT,DATA,pList,index,pEl) \
{ \
	ELEMENT* pEl = LIST##GetLast( pList ); \
	for( unsigned int index=0; index<LIST##GetSize( pList ); index++ ) \
	{

#define LIST_FORALL_REV_END(LIST,ELEMENT,DATA,pList,index,pEl) \
		pEl = LIST##GetPrev( pList, pEl ); \
	} \
}

#define LIST_FORALL_BEGIN(LIST,ELEMENT,DATA,pList,index,pEl) \
{ \
	ELEMENT* pEl = LIST##GetFirst( pList ); \
	for( unsigned int index=0; index<LIST##GetSize( pList ); index++ ) \
	{

#define LIST_FORALL_END(LIST,ELEMENT,DATA,pList,index,pEl) \
		pEl = LIST##GetNext( pList, pEl ); \
	} \
}

// declare list:

#define DECL_LIST(LIST,ELEMENT,DATA,MALLOC,FREE,DELDATA)\
struct S##ELEMENT\
{\
    struct S##ELEMENT* pPrev;\
    struct S##ELEMENT* pNext;\
    DATA* pData;\
};\
\
typedef struct S##ELEMENT ELEMENT;\
\
struct S##LIST\
{\
	unsigned int size;\
	ELEMENT* pHead;\
};\
\
typedef BOOL (*LIST##PCompareFunction) (DATA* pInList, DATA* p);\
typedef struct S##LIST LIST;

// define list:

#define DEF_LIST(LIST,ELEMENT,DATA,MALLOC,FREE,DELDATA)\
INLINE void LIST##Init(LIST* pList);\
INLINE void LIST##Exit(LIST* pList);\
\
INLINE int LIST##IsEmpty(LIST* pList);\
INLINE int LIST##GetSize(LIST* pList);\
\
INLINE ELEMENT* LIST##GetFirst(LIST* pList) ;\
INLINE ELEMENT* LIST##GetLast(LIST* pList) ;\
INLINE int LIST##HasNext(LIST* pList, ELEMENT* element);\
INLINE int LIST##HasPrev(LIST* pList, ELEMENT* element);\
INLINE ELEMENT* LIST##GetElementFromPointer(LIST* pList, const DATA* pData);\
INLINE ELEMENT* LIST##GetElement(LIST* pList, DATA* pData, LIST##PCompareFunction pCompareFunction);\
INLINE ELEMENT* LIST##GetNext(LIST* pList, ELEMENT* element) ;\
INLINE ELEMENT* LIST##GetPrev(LIST* pList, ELEMENT* element);\
\
INLINE void LIST##Clear(LIST* pList);\
INLINE ELEMENT* LIST##Add(LIST* pList, DATA* pData);\
INLINE ELEMENT* LIST##Prepend(LIST* pList, DATA* pData);\
INLINE ELEMENT* LIST##Insert(LIST* pList, DATA* pData, ELEMENT* pElement);\
INLINE void LIST##Del(LIST* pList, ELEMENT* element);\
\
INLINE void LIST##Init(LIST* pList)\
{\
    pList->size= 0;\
    pList->pHead= NULL;\
}\
INLINE void LIST##Exit(LIST* pList)\
{ \
	LIST##Clear(pList); \
} \
\
INLINE void LIST##Clear(LIST* pList)\
{\
	if(! LIST##IsEmpty(pList))\
	{\
		ELEMENT* pCurrent= pList->pHead->pPrev;\
		pList->pHead->pPrev = NULL;\
		while(pCurrent->pPrev)\
		{\
			DELDATA(pCurrent->pData, sizeof( DATA ) ); \
			pCurrent = pCurrent -> pPrev;\
			FREE(pCurrent->pNext,sizeof(ELEMENT));\
			pCurrent->pNext = NULL;\
		}\
		DELDATA(pCurrent->pData, sizeof( DATA )); \
		FREE(pCurrent,sizeof(ELEMENT));\
	}\
	LIST##Init(pList); \
}\
\
INLINE int LIST##IsEmpty(LIST* pList)\
{\
    return pList->pHead == NULL;\
}\
INLINE int LIST##GetSize(LIST* pList)\
{\
    return pList->size;\
}\
\
INLINE ELEMENT* LIST##GetFirst(LIST* pList)\
{\
    if(!LIST##IsEmpty(pList))\
        return pList->pHead;\
    return NULL;\
}\
INLINE ELEMENT* LIST##GetLast(LIST* pList)\
{\
    if(!LIST##IsEmpty(pList))\
        return pList->pHead->pPrev;\
    return NULL;\
}\
INLINE int LIST##HasNext(LIST* pList, ELEMENT* element)\
{\
    return (element->pNext != pList-> pHead);\
}\
INLINE int LIST##HasPrev(LIST* pList, ELEMENT* element)\
{\
    return (element != pList-> pHead);\
}\
\
INLINE ELEMENT* LIST##GetElementFromPointer(LIST* pList,const DATA* pData)\
{\
	if(!LIST##IsEmpty(pList))\
	{\
		ELEMENT* pCurrent= pList-> pHead;\
		do\
		{\
			if( (pCurrent-> pData) == pData)\
				return pCurrent;\
			pCurrent = pCurrent-> pNext;\
		}\
		while(pCurrent!= pList-> pHead);\
	}\
	return NULL;\
}\
INLINE ELEMENT* LIST##GetElement(LIST* pList, DATA* pData, LIST##PCompareFunction pCompareFunction)\
{\
	if(!LIST##IsEmpty(pList))\
	{\
		ELEMENT* pCurrent= pList-> pHead;\
		do\
		{\
			if( pCompareFunction((pCurrent-> pData), pData) )\
				return pCurrent;\
			pCurrent = pCurrent-> pNext;\
		}\
		while(pCurrent!= pList-> pHead);\
	}\
	return NULL;\
}\
INLINE ELEMENT* LIST##GetNext(LIST* pList, ELEMENT* element)\
{\
    if(LIST##HasNext(pList,element))\
        return element->pNext;\
    return NULL;\
}\
INLINE ELEMENT* LIST##GetPrev(LIST* pList, ELEMENT* element)\
{\
    if(LIST##HasPrev(pList,element))\
        return element->pPrev;\
    return NULL;\
}\
\
INLINE ELEMENT* LIST##Add(LIST* pList, DATA* pData)\
{\
    ELEMENT* e= (ELEMENT* )MALLOC(sizeof(ELEMENT));\
    e -> pData = pData;\
	if( LIST##IsEmpty(pList))\
	{\
		pList->pHead = e;\
		e-> pPrev = e -> pNext = e;\
	}\
	else\
	{\
		ELEMENT* prev = pList->pHead->pPrev;\
		prev->pNext = e;\
		e-> pPrev = prev;\
		pList->pHead->pPrev = e;\
		e-> pNext = pList-> pHead;\
	}\
	pList->size++;\
	return e;\
}\
\
INLINE ELEMENT* LIST##Prepend(LIST* pList, DATA* pData)\
{\
	ELEMENT* e = LIST##Add( pList, pData ); \
	pList -> pHead = pList-> pHead -> pPrev; \
	return e;\
}\
\
INLINE ELEMENT* LIST##Insert(LIST* pList, DATA* pData, ELEMENT* pElement)\
{\
    ELEMENT* e= (ELEMENT* )MALLOC(sizeof(ELEMENT));\
    e-> pData = pData;\
        ELEMENT* pPrev= pElement;\
        ELEMENT* pNext= pElement->pNext;\
		pPrev->pNext = e;\
		e-> pPrev= pPrev;\
		pNext->pPrev = e;\
		e-> pNext = pNext;\
		pList->size++;\
	return e;\
}\
\
INLINE void LIST##Del(LIST* pList, ELEMENT* element)\
{\
	if( pList->size == 0 ) { \
		return; \
	} \
	else if( pList-> pHead-> pNext != pList-> pHead ) {\
		DELDATA(element->pData, sizeof( DATA )); \
		if( element == pList-> pHead )\
			pList-> pHead = pList-> pHead-> pNext;\
		element-> pPrev->pNext = element-> pNext;\
		element-> pNext-> pPrev = element-> pPrev;\
		FREE(element,sizeof(ELEMENT));\
	}\
	else\
	{\
		DELDATA(element->pData, sizeof( DATA )); \
		FREE(element,sizeof(ELEMENT));\
		pList->pHead = NULL;\
	}\
	pList->size--;\
}

#endif
