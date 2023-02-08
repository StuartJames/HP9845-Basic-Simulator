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

#include "assert.h"

//////////////////////////////////////////////////////////////////////////////

class ExTextRow	: public CObject
{
public:
													ExTextRow() noexcept;
	virtual									~ExTextRow();
	void										Clear(void);																																						// deletes all contents and resets substring parameters
	int											Push(USHORT wChr);																																		// append a character, cannot append past the maximum size
	int											Push(char Chr, UCHAR Flags);																																		// append a character, cannot append past the maximum size
	int											SetAt(USHORT Index, char Chr, UCHAR Flags);
	void										Clone(const ExTextRow *pSrce, bool Clean = true);
	int											SetIndex(int indx);
	int											GetIndex(){ return m_Index; };
	inline int							GetLength(void){ return m_Length; };																										// returns the number of characters in the string
	inline USHORT*					GetBuffer(void){ return m_pBuffer; };																										// returns the number of characters in the string
	int											NextTab(UINT TabWidth);
	inline bool							IsStale(void){ return m_IsChanged; };
	inline void							Clean(void){ m_IsChanged = false; };
 
	void 										operator=(const ExTextRow &pSrce) throw();
	USHORT									operator[](const int Index) throw();


protected: 
	int											m_Size;
	int											m_Length;
	USHORT									*m_pBuffer;
	int											m_Index;
	bool										m_IsChanged;
};

