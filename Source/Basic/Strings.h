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

#include "BObject.h"
#include "assert.h"

constexpr auto  DEF_MIN_STRING_LEN  = 18;
constexpr auto  DEF_MAX_STRING_LEN  = 1024;

enum class eSSType{
	NONE =  0,
	POSITN,
	RANGE,
	LENGTH
};

//////////////////////////////////////////////////////////////////////////////

class BString	: BObject
{
public:
													BString() noexcept;
	virtual									~BString();
	inline void							Add(const char Chr){ if(m_Length < m_Size)  m_pBuffer[m_Length++] = Chr; };							// appends characters to the buffer. cannot add more than the pre-configured size 
	inline void							AndChar(UINT Pos, const char Chr){ if(Pos < m_Length)  m_pBuffer[Pos] &= Chr; };				// performs a logical AND of the character at 'Pos' and the 'Chr' argument 
	UINT										AppendChar(char Chr);																																		// append a character, cannot append past the maximum size
	UINT										AppendChars(const char *pChr);																													// append a number of character, cannot append past the maximum size
	UINT										AppendPadded(const char *pChr, UINT Width);																							// append characters, and pads out to width if necesssary
	UINT										AppendPrintf(const char *pFmt, ...);																										// append a formated string, cannot append past the maximum size
	UINT										AppendString(const BString *pAppStr);																										// append a string object, cannot append past the maximum size
	UINT										Format(const char *pFmt, ...);																													// formated string, clears string first
	void										Clear(void);																																						// deletes all contents and resets substring parameters
	void										Clone(const BString *pStr);																															// clone another string object
	void										GetSubStr(const BString *pStr);																													// get the sub-string of the source string object
	UINT										Compare(const char *pStr);																															// compare with char buffer
	UINT										Compare(const BString *pStr);																														// compare with another string object
	UINT										Copy(const char *pChr, UINT ReqSize);																										// copy caracters and resize
	UINT										Delete(UINT Index, UINT Length);																												// delete a number of character starting at index
	UINT										FilterCopy(const char *pChr, UINT Len, const char Filter);															// copy characters from the source unless they match the filter
	void										FindTruncate(char Chr);																																	// find a character and truncate to that position if found
	void										FormatDouble(double Value, int Width, int Precision, int Exponent);											// format a double to the string
	inline char*						GetBuffer(UINT Pos = 0){ return (Pos < m_Length) ? &m_pBuffer[Pos] : m_pBuffer; };			// returns the object buffer
	inline char							GetAt(UINT Pos){ return (Pos < m_Length) ? m_pBuffer[Pos] : 0; };												// returns the caracter at 'Pos'
	inline UINT							GetLength(void){ return m_Length; };																										// returns the number of characters in the string
	inline UINT							GetMaxSize(void){ return m_MaxSize; };																									// returns the maximum size the buffer can be
	eSSType									GetSSType(void){ return m_SubStrType; };																								// returns the substring type
	UINT										Insert(UINT Index, char Chr);																														// insert a character at the 'index' position
	bool										IsEmpty(){ return (m_Length == 0) ? true : false; };																		// returns true if buffer is empty otherwise false
	UINT										Resize(UINT ReqSize);																																		// resize the buffer. It cannot be greater than the maximum size
	inline void							SetAt(UINT Pos, const char Chr){ if(Pos < m_Length)  m_pBuffer[Pos] = Chr; };						// set the character at 'Pos'
	void										SetFrom(UINT Index, const BString *pStr, UINT Length);																	// set a number of characters from 'Index'. Cannot set past the length of the string
	void										SetLeft(const BString *pStr);
	inline void							SetLength(UINT Len){ m_Length = (Len < m_Size) ? Len : m_Size; };
	inline void							SetMaxSize(UINT Max){ m_MaxSize = (Max < DEF_MAX_STRING_LEN) ? Max : DEF_MAX_STRING_LEN; };
	void										SetRight(const BString *pStr);
	void										SetSSType(eSSType Type, UINT Sub1 = 0, UINT Sub2 = 0){ m_SubStrType = Type; m_SubStr[0] = Sub1; m_SubStr[1] = Sub2; }; // set the substring perameters
	inline eSSType					SSType(void){ return m_SubStrType; };																										// returns the substring type
	bool										SubString(const BString *pSrc);																													// replaces one substring with another
	void										ToLowerCase(void);
	void										ToUpperCase(void);
	inline UINT							Truncate(void){ if(m_Length > 0) m_pBuffer[--m_Length] = '\0'; return m_Length; };			// delete the last character from the buffer
	void										PadToColumn(UINT Column);																																// pad out with spaces to column index
	bool										IsValidName(void);

	void 										operator=(const BString &StrOrg) throw();
	void 										operator+=(const BString &StrOrg) throw();
	void 										operator==(const BString &StrOrg) throw();
	char 										operator[](const UINT Index) throw();


protected: 
	UINT										m_Size;
	UINT										m_MaxSize;		
	UINT										m_Length;
	char										*m_pBuffer;
	eSSType									m_SubStrType;
	UINT										m_SubStr[2];
};

