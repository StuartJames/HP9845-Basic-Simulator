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
#include "Strings.h"

////////////////////////////////////////////////////////////////////////////////////

BString::BString() noexcept
{
	m_pBuffer = nullptr;
	m_Length = 0;
	m_Size = 0;
	m_MaxSize = DEF_MAX_STRING_LEN;
	m_SubStr[0] = 0;
	m_SubStr[1] = 0;
	m_SubStrType = eSSType::NONE;
}

////////////////////////////////////////////////////////////////////////////////////

BString::~BString()
{
	delete [] m_pBuffer;
}

////////////////////////////////////////////////////////////////////////////////////

void BString::Clear(void)
{
	delete [] m_pBuffer;
	m_pBuffer = new char[1];
	m_pBuffer[0] = 0;
	m_Size = 0;
	m_Length = 0;
	m_SubStr[0] = 0;
	m_SubStr[1] = 0;
	m_SubStrType = eSSType::NONE;
}


////////////////////////////////////////////////////////////////////////////////////

void BString::operator=(const BString &Str) throw()
{
	Clone(&Str);
}

////////////////////////////////////////////////////////////////////////////////////

void BString::operator+=(const BString &Str) throw()
{
	AppendString(&Str);
}

////////////////////////////////////////////////////////////////////////////////////

void BString::operator==(const BString &Str) throw()
{
	Compare(&Str);
}

////////////////////////////////////////////////////////////////////////////////////

char BString::operator[](const UINT Index) throw()
{
	return (Index < m_Length) ? m_pBuffer[Index] : m_pBuffer[m_Length];
}

////////////////////////////////////////////////////////////////////////////////////

void BString::Clone(const BString *pStr)
{
	Clear();
	m_MaxSize = pStr->m_MaxSize;
	AppendString(pStr);
	m_SubStr[0] = pStr->m_SubStr[0];
	m_SubStr[1] = pStr->m_SubStr[1];
	m_SubStrType = pStr->m_SubStrType;
}

////////////////////////////////////////////////////////////////////////////////////

void BString::GetSubStr(const BString *pStr)
{
	Clear();
	m_MaxSize = pStr->m_MaxSize;
	m_SubStr[0] = 0;
	m_SubStr[1] = 0;
	m_SubStrType = eSSType::NONE;
	SubString(pStr);
}

////////////////////////////////////////////////////////////////////////////////////

UINT BString::Resize(UINT ReqSize)
{
char *pBuff = nullptr;
UINT Size;

	if(ReqSize > m_MaxSize) return 0;
	if(m_MaxSize < DEF_MIN_STRING_LEN) Size = (ReqSize >= m_MaxSize) ? m_MaxSize : ReqSize;																// set the max size
	else Size = (ReqSize >= DEF_MIN_STRING_LEN) ? ((ReqSize <= m_MaxSize) ? ReqSize : m_MaxSize) : DEF_MIN_STRING_LEN;		// bracket the size
	if(Size != m_Size){																																																		// check for actual resize required
		ReqSize = (ReqSize <= m_MaxSize) ? ReqSize : m_MaxSize;																															// don't overrun the buffer
		if((pBuff =  new char[Size + 1]) == nullptr) return 0;																															// plus 1 for the terminator
		memset(pBuff, 0, Size + 1);
		if(m_pBuffer != nullptr){
			if(ReqSize > m_Length) memcpy(pBuff, m_pBuffer, m_Length);																												// copy the full original
			else memcpy(pBuff, m_pBuffer, ReqSize);																																						// or copy what will fit
			delete [] m_pBuffer;
		}
		m_pBuffer = pBuff;
		m_Size = Size;
	}
	return Size;
}

////////////////////////////////////////////////////////////////////////////////////

UINT BString::AppendString(const BString *pAppStr)
{
UINT AppendLength = pAppStr->m_Length;

	if(AppendLength == 0) return 0;
	if(Resize(m_Length + AppendLength) == -1) return 0;
	AppendLength = (m_Size > m_Length + AppendLength) ? AppendLength : m_Size - m_Length;	 // total can't be greater than maximum size
	memcpy(m_pBuffer + m_Length, pAppStr->m_pBuffer, AppendLength);
	m_Length += AppendLength;
	return AppendLength;
}

////////////////////////////////////////////////////////////////////////////////////

UINT BString::AppendChar(char Chr)
{
	if(Resize(m_Length + 1) == -1) return 0;
	m_pBuffer[m_Length++] = Chr;
	return m_Length;
}

////////////////////////////////////////////////////////////////////////////////////

UINT BString::AppendChars(const char *pChr)
{
UINT AppendLength = strlen(pChr);

	if(Resize(m_Length + AppendLength) == -1) return 0;
	AppendLength = (m_Size > m_Length + AppendLength) ? AppendLength : m_Size - m_Length;	 // total can't be greater than maximum size
	memcpy(m_pBuffer + m_Length, pChr, AppendLength);
	m_Length += AppendLength;
	return m_Length;
}

////////////////////////////////////////////////////////////////////////////////////

UINT BString::AppendPrintf(const char *pFmt, ...)
{
char Buffer[1024];
UINT AppendLength;
va_list args;

	va_start(args, pFmt);
	AppendLength = vsprintf_s(Buffer, pFmt, args);
	va_end(args);
	if(Resize(m_Length + AppendLength)  ==  -1) return 0;
	AppendLength = (m_Size > m_Length + AppendLength) ? AppendLength : m_Size - m_Length;					// total can't be greater than maximum size
	memcpy(m_pBuffer + m_Length, Buffer, AppendLength);
	m_Length += AppendLength;
	return m_Length;
}

////////////////////////////////////////////////////////////////////////////////////

UINT BString::Format(const char *pFmt, ...)
{
char Buffer[1024];
UINT Length;
va_list args;

	va_start(args, pFmt);
	Length = vsprintf_s(Buffer, pFmt, args);
	va_end(args);
	Clear();
	Length = (Length < m_MaxSize) ? Length : m_MaxSize;																				// total can't be greater than maximum size
	if(Resize(Length)  ==  -1) return 0;
	memcpy(m_pBuffer, Buffer, Length);
	m_Length = Length;
	return m_Length;
}

////////////////////////////////////////////////////////////////////////////////////

UINT BString::AppendPadded(const char *pChr, UINT Width)																		// append to minimum width padding out if necessary
{
UINT AppendLength = m_Length + Width, SrcLen = strlen(pChr);

	UINT Blanks = (SrcLen < Width ? (Width - SrcLen) : 0);																		// calculate the number of trailing blanks
	Resize(m_Length + AppendLength);																													// resize to required width
	memcpy(m_pBuffer + m_Length, pChr, Blanks ? SrcLen : Width);															// copy the buffer length or width whichever is greater
	if(Blanks) memset(m_pBuffer + m_Length + SrcLen, ' ', Blanks);														// pad out with blanks to the required width
	m_Length += AppendLength;
	return m_Length;
}

////////////////////////////////////////////////////////////////////////////////////

UINT BString::Copy(const char *pChr, UINT Len)
{
  UINT Size = (Len < m_MaxSize) ? Len : m_MaxSize;
	if(Resize(Size) == -1) return 0;
	memcpy(m_pBuffer, pChr, Len);
	m_Length = Len;
	return m_Length;
}

////////////////////////////////////////////////////////////////////////////////////

UINT BString::FilterCopy(const char *pChr, UINT Len, const char Filter)
{
UINT i = 0;

  UINT Size = (Len < m_MaxSize) ? Len : m_MaxSize;
	if(Resize(Size) == -1) return 0;
	const char *pSrc = pChr;
	while(i < Size){
		if(*pSrc != Filter){
			m_pBuffer[i++] = *pSrc;
			m_Length++;
		}
		++pSrc;
	}
	return m_Length;
}

////////////////////////////////////////////////////////////////////////////////////

UINT BString::Insert(UINT Index, char Chr)
{
	UINT OldLength = m_Length;
	assert(Index < m_Length);
	if(Resize(m_Length + 1) == -1) return 0;
	memmove(m_pBuffer + Index + 1, m_pBuffer + Index, OldLength - Index);
	m_pBuffer[Index] = Chr;
	m_Length++;
	return m_Length;
}

////////////////////////////////////////////////////////////////////////////////////

UINT BString::Delete(UINT Index, UINT Length)
{
UINT OldLength = m_Length;

	assert(Index < m_Length);
	assert(Length > 0);
	if((Index + Length) < m_Length) memmove(m_pBuffer + Index, m_pBuffer + Index + Length, m_Length - Index - Length);
	m_pBuffer[m_Length -= Length] = '\0';
	return m_Length;
}

////////////////////////////////////////////////////////////////////////////////////

void BString::FindTruncate(char Chr)
{
	char *pPos =	strchr(m_pBuffer, Chr);
	if(pPos != nullptr) *pPos = 0;
}

////////////////////////////////////////////////////////////////////////////////////

void BString::ToUpperCase(void)
{
	for(UINT i = 0; i < m_Length; ++i) m_pBuffer[i] = toupper((int)m_pBuffer[i]);
}

////////////////////////////////////////////////////////////////////////////////////

void BString::ToLowerCase()
{
	for(UINT i = 0; i < m_Length; ++i) m_pBuffer[i] = tolower((int)m_pBuffer[i]);
}

////////////////////////////////////////////////////////////////////////////////////

UINT BString::Compare(const char *pStr)
{
int Result;
const char *pPtr1 = m_pBuffer, *pPtr2 = pStr;

  UINT Len = strlen(pStr);
	for(UINT Position = 0; (Position < m_Length) && (Position < Len); ++Position, ++pPtr1, ++pPtr2) if((Result = (*pPtr1 - *pPtr2))) return Result;
	return (m_Length - Len);
}

////////////////////////////////////////////////////////////////////////////////////

UINT BString::Compare(const BString *pStr)
{
int Result;
const char *pPtr1 = m_pBuffer, *pPtr2 = pStr->m_pBuffer;

	for(UINT Position = 0; (Position < m_Length) && (Position < pStr->m_Length); ++Position, ++pPtr1, ++pPtr2) if((Result = (*pPtr1 - *pPtr2))) return Result;
	return (m_Length - pStr->m_Length);
}

////////////////////////////////////////////////////////////////////////////////////

void BString::SetLeft(const BString *pStr)
{
UINT Count = (m_Length < pStr->m_Length) ? m_Length : pStr->m_Length;

	if(Count) memcpy(m_pBuffer, pStr->m_pBuffer, Count);															// source copied to start of destination
	if(Count < m_Length) memset(m_pBuffer + Count, ' ', pStr->m_Length - Count);			// fill rest of destination with spaces
}

////////////////////////////////////////////////////////////////////////////////////

void BString::SetRight(const BString *pStr)
{
UINT Count = (m_Length < pStr->m_Length) ? m_Length : pStr->m_Length;

	if(Count) memcpy(m_pBuffer + m_Length - Count, pStr->m_pBuffer, Count);						// source copied to end of destination
	if(Count < m_Length) memset(m_pBuffer, ' ', m_Length - Count);										// fill rest of destination with spaces
}

////////////////////////////////////////////////////////////////////////////////////

void BString::SetFrom(UINT Index, const BString *pStr, UINT Length)
{
	if(m_Length >= Index){
		if(m_Length < (Index + Length)) Length = m_Length - Index;										 
		if(Length) memcpy(m_pBuffer + Index, pStr->m_pBuffer, Length);									// source copied to destination starting at index to a maximum of the destination length
	}
}

////////////////////////////////////////////////////////////////////////////////////

bool BString::SubString(const BString *pSrc)																				// copy sub-string tosub-string
{
UINT StartPos = 0, SrcLen = 0;																											// include termination
BString TempStr;
bool Result = false;

	switch(pSrc->m_SubStrType){																												// no source sub-string, copy complete text
		case eSSType::NONE:{
			SrcLen = pSrc->m_Length;
			if(TempStr.Resize(SrcLen)  ==  -1) break;
			memcpy(TempStr.m_pBuffer, pSrc->m_pBuffer, SrcLen);
			Result = true;
			break;
		}
		case eSSType::POSITN:{																													// from source index to end
			StartPos = pSrc->m_SubStr[0] - 1;
			SrcLen = pSrc->m_Length - StartPos;
			if(TempStr.Resize(SrcLen)  ==  -1) break;
			memcpy(TempStr.m_pBuffer, pSrc->m_pBuffer + StartPos, SrcLen);
			Result = true;
			break;
		}
		case eSSType::RANGE:{																														// Range [n,m] substring specifier, from index to index  
			StartPos = pSrc->m_SubStr[0] - 1;
			SrcLen = pSrc->m_SubStr[1] - StartPos - 1;																		// length is difference of 2 positions
			if(SrcLen > pSrc->m_Length) break;
			if(TempStr.Resize(SrcLen) == -1) break;
			memcpy(TempStr.m_pBuffer, pSrc->m_pBuffer + StartPos, SrcLen);
			Result = true;
			break;
		}
		case eSSType::LENGTH:{																													// Position and length [n;m] substring specifier, from index for m number of characters
			if((pSrc->m_SubStr[0] > pSrc->m_Length) || (pSrc->m_SubStr[1] > pSrc->m_Length)) break;
			StartPos = pSrc->m_SubStr[0] - 1;
			SrcLen = pSrc->m_SubStr[1];
			UINT EndPos = StartPos + SrcLen;
			if(TempStr.Resize(SrcLen) == -1) break;
			memcpy(TempStr.m_pBuffer, pSrc->m_pBuffer + StartPos, SrcLen);
			Result = true;
			break;
		}
	}
	if(Result == true){
		Result = false;
		TempStr.m_Length = SrcLen;
		StartPos = m_SubStr[0] - 1;
		switch(m_SubStrType){
			case eSSType::NONE:{
				UINT Length = (SrcLen > m_MaxSize) ? m_MaxSize	: SrcLen;
				if(Resize(Length)  ==  -1) break;
				memcpy(m_pBuffer, TempStr.m_pBuffer, Length);
				m_Length = Length;
				Result = true;
				break;
			}
			case eSSType::POSITN:{																																			// Single substring specifier
				if(StartPos > m_Length) break;																																// must be contiguous
				UINT CopyCount = (StartPos + SrcLen > m_MaxSize) ? m_MaxSize - StartPos : SrcLen;							// truncate source length to fit
				UINT Length = StartPos + CopyCount;
				if(Length > m_Length) if(Resize(Length)  ==  -1) break;																				// will only enlarge
				memcpy(m_pBuffer + StartPos, TempStr.m_pBuffer, CopyCount);
				if(Length > m_Length) m_Length = Length;
				Result = true;
				break;
			}
			case eSSType::RANGE:{																																				// Range [n,m] substring specifier 
				UINT EndPos = m_SubStr[1] - 1;
				UINT Length = EndPos - StartPos + 1;																													// characters to be filled
				if((StartPos > m_Length) || (EndPos > m_MaxSize)) break;																			// must be contiguous
				if(EndPos > m_Length) if(Resize(EndPos)  ==  -1) break;
				UINT CopyCount = (Length > TempStr.m_Length) ? TempStr.m_Length	: Length;
				memcpy(m_pBuffer + StartPos, TempStr.m_pBuffer, CopyCount);
				if(CopyCount < Length) for(UINT i = CopyCount; i < Length; ++i) m_pBuffer[i] = ' ';						// pad out if destination was short
				if(EndPos > m_Length) m_Length = EndPos;
				Result = true;
				break;
			}
			case eSSType::LENGTH:{																																				// Position and length [n;m] substring specifier
				UINT Length = m_SubStr[1];
				UINT EndPos = StartPos + Length;
				if((StartPos > m_Length) || (EndPos > m_MaxSize)) break;																			// must be contiguous
				if(EndPos > m_Length) if(Resize(EndPos)  ==  -1) break;
				UINT CopyCount = (Length > TempStr.m_Length) ? TempStr.m_Length	: Length;
				memcpy(m_pBuffer + StartPos, TempStr.m_pBuffer, Length);
				if(CopyCount < Length) for(UINT i = CopyCount; i < Length; ++i) m_pBuffer[i] = ' ';						// pad out if destination was short
				if(EndPos > m_Length) m_Length = EndPos;
				Result = true;
				break;
			}
		}
	}
	return Result;
}

////////////////////////////////////////////////////////////////////////////////////

void BString::FormatDouble(double Value, int Width, int Precision, int Exponent)
{
	if(Exponent){
		UINT Length = m_Length;
		char *pChr;
		int Num;

		AppendPrintf(_T("%.*E"), Width - 1 - (Precision >= 0), Value);
		if(m_pBuffer[Length + 1] == '.') Delete(Length + 1, 1);
		if(Precision >= 0) Insert(Length + Width - Precision - 1, '.');
		for(pChr = m_pBuffer + m_Length - 1; (pChr >= m_pBuffer) && (*pChr != 'E'); --pChr) ;
		++pChr;
		Num = atol(pChr);
		Num = Num + 2 - (Width - Precision);
		Length = pChr - m_pBuffer;
		Delete(Length, m_Length - Length);
		AppendPrintf(_T("%+0*d"), (Exponent - 1), Num);
	}
	else if(Precision > 0) AppendPrintf(_T("%*.*f"), Width, Precision, Value);
	else if(Precision == 0) AppendPrintf(_T("%.f."), Value);
	else if(Width) AppendPrintf(_T("%*.0f"), Width, Value);
	else{
		double Real = Value;
		if((Real < 0.0001) || (Real >= 10000000.0)){
			AppendPrintf(_T("%.7g"), Value); 																					/* print scientific notation */
		}
		else{ 																																						/* print decimal numbers or integers, if possible */
			int i, n, j = 6;
			while(Real >= 10.0 && j > 0){
				Real /= 10.0; --j;
			}
			i = m_Length;
			AppendPrintf(_T("%.*f"), j, Value);
			n = m_Length;
			if(memchr(m_pBuffer + i, '.', n - i)){
				while(m_pBuffer[m_Length - 1] == '0') --m_Length;
				if(m_pBuffer[m_Length - 1] == '.') --m_Length;
			}
		}
	}
}

////////////////////////////////////////////////////////////////////////////////////

void BString::PadToColumn(UINT Column)
{
UINT i = m_Length, Size;

	if(Column <= m_Length) return;
	Size = (Column < m_MaxSize) ? Column : m_MaxSize;
	if(Resize(Size) == -1) return;
	while(m_Length < Size) m_pBuffer[m_Length++] = ' ';
}

////////////////////////////////////////////////////////////////////////////////////

bool BString::IsValidName(void)
{
	if(m_Length > 6) return false;
	for(UINT i = 0; i < m_Length; ++i)	if((m_pBuffer[i] == ':') || (m_pBuffer[i] == '"') || (m_pBuffer[i] == 255)) return false;
  return true;
}

////////////////////////////////////////////////////////////////////////////////////
