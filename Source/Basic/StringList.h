/*
 *        Copyright (c) 2020-2023 HydraSystems.
 *
 *  This program is free software; you can redistribute it and/or   
 *  modify it under the terms of the GNU General Public License as  
 *  published by the Free Software Foundation; either version 2 of  
 *  the License, or (at your option) any later version.             
 *                                                                  
 *  This program is distributed in the hope that it will be useful, 
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of  
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the   
 *  GNU General Public License for more details.                    
 *                                                                  
 *
 *  Based on a design by Michael Haardt
 *
 * Edit Date/Ver   Edit Description
 * ==============  ===================================================
 * SJ   19/08/2020  Original
 *
 */

#pragma once

//#include <sys/types.h>
#include "BObject.h"
//#include "BasicDefs.h"
#include "Strings.h"

/////////////////////////////////////////////////////////////////////////////

class StringListNode
{
public:
													StringListNode() noexcept;
	virtual									~StringListNode();
	StringListNode*					GetPrev(){ return m_pPrev; };
	void										SetPrev(StringListNode *pNode){ m_pPrev = pNode; };
	StringListNode*					GetNext(){ return m_pNext; };
	void										SetNext(StringListNode *pNode){ m_pNext = pNode; };
	void										ClearString(void){ m_String.Clear(); };
	BString*								GetString(void){ return &m_String; };

protected: 
	StringListNode					*m_pPrev;
	StringListNode					*m_pNext;
	BString									m_String;
};

/////////////////////////////////////////////////////////////////////////////

class StringList : BObject
{
public:
													StringList() noexcept;
	virtual									~StringList();

	UINT										GetCount(void){ return m_Count; };
	StringListNode*					GetAt(UINT Pos);
	StringListNode*					Add();
	StringListNode* 				Insert(UINT AfterPos);
	StringListNode*					Remove(UINT DelPos);
	void										RemoveAll(void);
	StringListNode*					GetHead(void){ return m_pHead; };
	StringListNode*					GetTail(void){ return m_pTail; };
	StringListNode*					GetNext();
	StringListNode*					GetPrev();
	UINT										GetIndex(StringListNode *pRef);

protected: 
	StringListNode					*m_pHead;
	StringListNode					*m_pTail;
	StringListNode					*m_pCurrent;
	UINT 										m_Count;

};


