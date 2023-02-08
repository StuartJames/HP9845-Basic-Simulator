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
#include <io.h>
#include <stdlib.h>
#include <stdio.h>
#include "FontInfo.h"

extern FontInfo_t Norm7x12;

//////////////////////////////////////////////////////////////////////

CPrinter::CPrinter(void) : CDevice()
{
	m_LineWidth = DEF_LINE_WIDTH;
	m_TabWidth = DEF_TAB_WIDTH;
}

//////////////////////////////////////////////////////////////////////

CPrinter::~CPrinter(void)
{
}

//////////////////////////////////////////////////////////////////////

void CPrinter::Initialise(void)
{
	m_pDoc = static_cast<CS45BasicDoc*>(g_pBasicDoc);
	m_TabWidth = DEF_TAB_WIDTH;
	m_LineWidth = DEF_LINE_WIDTH;
	m_ColorFore = m_pDoc->GetForeColour();

}

//////////////////////////////////////////////////////////////////////

void CPrinter::Destroy(void)
{
}

//////////////////////////////////////////////////////////////////////////

void CPrinter::EndPrint(void)
{
}

//////////////////////////////////////////////////////////////////////////

void CPrinter::RenderChar(CDC *pDC, int x, int y, BYTE c, bool Inverse, bool Undline)
{
UCHAR bits = 0;
int i, j;
const UCHAR *pBitStream;

	pBitStream = &Norm7x12.pBitmap[c * 12];
	for(j = 0; j < 12; ++j){
		if(Undline && (j == 11)) bits = 0xFF;
		else bits = pBitStream[j];  // line data
		if(Inverse){
			for(i = 0; i < 7; ++i){
				if(!(bits & 0x80)) pDC->SetPixelV(x + i, y + j, m_ColorFore);
				bits = bits << 1;
			}
		}
		else{
			for(i = 0; i < 7; ++i){
				if(bits & 0x80) pDC->SetPixelV(x + i, y + j, m_ColorFore);
				bits = bits << 1;
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////

void CPrinter::RenderText(CDC *pDC, CPoint To, LPTSTR pStr, bool DblLine, bool ShowCodes/*=false*/)
{
int xPos = To.x, yPos = To.y, cPos = To.x * SCRN_FONT_X;
BYTE c;
bool Undline = false, Inverse = false, Blink = false;

	if(m_pDoc == nullptr)	return;
	while(*pStr != 0){
		c = *(pStr++);
		if((c & 0x80) && !ShowCodes){																// check for control codes
			if(c & 0x01) Inverse = true;
			if(c & 0x02) Blink = true;
			if(c & 0x04) Undline = true;
			if(c == 0x80) Inverse = Blink = Undline = false;
		}
		else{
			RenderChar(pDC, xPos + SCRN_MARGIN_X, yPos + SCRN_MARGIN_Y, c, Inverse, Undline);
			xPos += SCRN_FONT_X;
			if(++cPos == SCRNCOLS){
				if(!DblLine) return;
				DblLine = false;																			// only once
				xPos = 0;																							// do carridge return
				yPos += SCRN_FONT_Y;																	// and line feed
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////

int CPrinter::Flush()
{
	return DEV_OK;
}

//////////////////////////////////////////////////////////////////////

void CPrinter::CarridgeReturn()
{
}

//////////////////////////////////////////////////////////////////////

void CPrinter::LineFeed()
{
}

//////////////////////////////////////////////////////////////////////
int CPrinter::PageFeed()
{
	return DEV_OK;
}

//////////////////////////////////////////////////////////////////////

int CPrinter::NextTab()
{
	return DEV_OK;
}

//////////////////////////////////////////////////////////////////////

int CPrinter::PrintLIN(int Count)
{
	return DEV_OK;
}

//////////////////////////////////////////////////////////////////////

int CPrinter::SetCursorColumn(UINT Column)
{
	return DEV_OK;
}

//////////////////////////////////////////////////////////////////////

int CPrinter::SetCursorRow(UINT Column)
{
	return DEV_OK;
}

//////////////////////////////////////////////////////////////////////

int CPrinter::PushChar(char ch)
{
	return DEV_OK;
}

//////////////////////////////////////////////////////////////////////

int CPrinter::PrintChars(const char *pChars)
{
	return DEV_OK;
}

//////////////////////////////////////////////////////////////////////

int CPrinter::PrintFormat(LPCSTR lpStrFmt, ...)
{
va_list  argptr;																											// Argument list pointer	      
char Buffer[1024];
int cnt;

  va_start(argptr, lpStrFmt);																					// Initialize va_ functions     
	cnt = vsprintf_s(Buffer, 1024, lpStrFmt, argptr);  
  va_end(argptr);																											// Close va_ functions		
	if(cnt) return PrintChars(Buffer);
	return DEV_OK;
}

//////////////////////////////////////////////////////////////////////

int CPrinter::PrintText(eDispArea DisplayArea, LPCSTR lpStr)
{
	return DEV_OK;
}

//////////////////////////////////////////////////////////////////////

int CPrinter::PrintArray(pValue_s pValue, bool Tabbed)
{
Var_s	*pVar = pValue->uData.pArray;
int Precision = -1, Width = 3, j = 0;
BString PrintStr;

	if((pVar->Dimensions < 1)) return DEV_ERROR;
	int RowSize = pVar->pGeometry[pVar->Dimensions - 1].Size;
	if(pVar->Type == eVType::V_REAL){ Precision = 2; Width = 6; }
	do{
		PrintStr.Clear();
		switch(pVar->Type){
			case eVType::V_STRING:{
				ValueToString(&(pVar->pValue[j++]), &PrintStr, ' ', -1, 0, 0, -1, 0, 0);
				if(PrintStr.GetLength() > (UINT)(m_LineWidth - GetPrintCol())) PrintChars("\n");
				PrintString(&PrintStr);
				break;
			}
			case eVType::V_INT:
			case eVType::V_REAL:{
				ValueToString(&(pVar->pValue[j++]), &PrintStr, ' ', -1, Width, 0, Precision, 0, 0);
				if(PrintStr.GetLength() > (UINT)(m_LineWidth - GetPrintCol())) PrintChars("\n");
				PrintString(&PrintStr);
				if((j % RowSize) == 0) PrintChars("\n");
				else{
					if(Tabbed) NextTab();
					else PrintChars("  "); 
				}
				break;
			}
		}
	}
	while(j < pVar->Size);
	PrintChars("\n\n");
	return DEV_OK;
}

