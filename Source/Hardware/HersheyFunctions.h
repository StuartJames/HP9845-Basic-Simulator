/*
 *        Copyright 1996 Coherent Research Inc.
 *
 *      Author:Randy More
 *
 *			Revision: 5 $
 *
 * Edit Date/Ver   Edit Description
 * ==============  ===================================================
 * RM   11/06/1996  v5.00  Original
 * RM		01/11/1999	v9.04a Added this header block and comment blocks
 *
 */

#pragma once

#include "stdafx.h"
#include "../Basic/BasicDefs.h"
#include "HersheyFont.h"

constexpr auto  TRANSLATION_TYPES	= 12;
constexpr auto  TRANSLATION_SIZE	= 256;

//////////////////////////////////////////////////////////////////////

enum FONT_TYPE
{
	SMALL_SIMPLEX,
	SMALL_DUPLEX,
	SIMPLEX,
	DUPLEX,
	TRIPLEX,
	MODERN,
	SCRIPT_SIMPLEX,
	SCRIPT_DUPLEX,
	ITALLIC_DUPLEX,
	ITALLIC_TRIPLEX,
	FANCY,
	GOTHIC
};


//////////////////////////////////////////////////////////////////////

class CHersheyFunctions
{
public:
											CHersheyFunctions();
											CHersheyFunctions(bool ForceReload);
	virtual							~CHersheyFunctions();

	CPoint							DrawString(CDC *pDC, CPoint Location, double Rotation, double CharHeight, double CharRatio, int Origin, FONT_TYPE pFontType, CString pString);		

	int									GetMaxChar(void){	return(m_CharCount - 1); };
	int									AddCharacter(CHersheyFont *pChar){ m_CharCount++; return m_FontCharList.Add(pChar);	};
	void								SetCharID(unsigned char pCharValue,	FONT_TYPE pFont, int ID){	m_XlationTable[(int)pFont][(int)pCharValue] = ID;	};
	CHersheyFont				*GetCharacter(int pCharID){	return(m_FontCharList[pCharID]); };
	int									GetCharID(unsigned char CharValue,	FONT_TYPE FontType);


protected:
	void								LoadOldStyleFile(void);
	void								LoadFontData(void);
	void								SaveFontData(void);
	void								LoadXlationData(void);
	void								SaveXlationData(void);
	void								LoadOldStyleXlation(void);

private:
	CTypedPtrArray<CPtrArray, CHersheyFont*> m_FontCharList;

	UINT								m_CharCount;
	int									m_XlationTable[12][256];

};

