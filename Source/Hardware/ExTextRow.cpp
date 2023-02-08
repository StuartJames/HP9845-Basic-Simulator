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

#include "stdafx.h"
#include "../Basic/BasicDefs.h"
#include "ExTextRow.h"

////////////////////////////////////////////////////////////////////////////////////

ExTextRow::ExTextRow() noexcept
{
	m_Size = DEF_MAX_PRINT_WIDTH;
	m_pBuffer = new USHORT[m_Size];;
	Clear();
}

////////////////////////////////////////////////////////////////////////////////////

ExTextRow::~ExTextRow()
{
	delete [] m_pBuffer;
}

////////////////////////////////////////////////////////////////////////////////////

void ExTextRow::Clear(void)
{
	FillMemory(m_pBuffer, sizeof(USHORT) * m_Size, 0);
	m_Index = 0;
	m_Length = 0;
	m_IsChanged = false;
}


////////////////////////////////////////////////////////////////////////////////////

void ExTextRow::Clone(const ExTextRow *pSrce, bool Changed/*=true*/)
{
	Clear();
	CopyMemory(m_pBuffer, pSrce->m_pBuffer, sizeof(USHORT) * m_Size);
	m_Length = pSrce->m_Length;
	m_Index = pSrce->m_Index;
	m_IsChanged = Changed;
}

////////////////////////////////////////////////////////////////////////////////////

void ExTextRow::operator=(const ExTextRow &pSrce) throw()
{
	Clone(&pSrce);
}

////////////////////////////////////////////////////////////////////////////////////

USHORT ExTextRow::operator[](const int Index) throw()
{
	if(Index >= 0) return (Index < m_Length) ? m_pBuffer[Index] : m_pBuffer[m_Length];
	else return m_pBuffer[0];
}

////////////////////////////////////////////////////////////////////////////////////

int ExTextRow::Push(USHORT wChr)
{
	while(m_Index > m_Length) m_pBuffer[m_Length++]	= 0x0020;		 // pad out to required length
	m_pBuffer[m_Index++] = wChr;																 // place new character
	if(m_Index > m_Length) m_pBuffer[++m_Length] = 0;						 // reterminate
	m_IsChanged = true;
	return m_Index;
}

////////////////////////////////////////////////////////////////////////////////////

int ExTextRow::Push(char Chr, UCHAR Flags)
{
USHORT wChr;
	if(Flags == 0) wChr = (m_pBuffer[m_Index] & 0xff00) | Chr;			// get existing character and mix with new character 
	else wChr = (USHORT)(Flags << 8) + Chr;
	return Push(wChr);																							// place new character
}

////////////////////////////////////////////////////////////////////////////////////

int ExTextRow::SetAt(USHORT Indx, char Chr, UCHAR Flags)
{
	while(Indx > m_Length) m_pBuffer[m_Length++]	= 0x0020;		 // pad out to required length
	m_Index	= Indx;
	return Push((USHORT)(Flags << 8) + Chr);									// place new character
}

////////////////////////////////////////////////////////////////////////////////////

int ExTextRow::SetIndex(int Indx)
{
	if(Indx <= m_Length) m_Index = Indx;
	else while(m_Length < Indx) Push(' ', 0);
	return m_Index;
}

//////////////////////////////////////////////////////////////////////

int ExTextRow::NextTab(UINT TabWidth)
{
	do{
		if(m_Index >= m_Length) Push(' ', 0);
		else ++m_Index;
	}
	while((m_Index % TabWidth) && (m_Index < m_Size));
	return m_Index;
}

