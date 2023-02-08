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

//////////////////////////////////////////////////////////////////////////

CPrinter16::CPrinter16(void) : CPrinter()
{
	m_EscSequence = eEscSequ::NONE;
}

//////////////////////////////////////////////////////////////////////////

CPrinter16::~CPrinter16(void)
{
}

//////////////////////////////////////////////////////////////////////////

void CPrinter16::EndPrint(void)
{
}

//////////////////////////////////////////////////////////////////////////

void CPrinter16::RenderText(CDC *pDC, CPoint To, ExTextRow *pRow, bool DblLine)
{
int xPos = To.x, yPos = To.y, cPos = To.x * SCRN_FONT_X;
BYTE c;
bool Undline = false, Inverse = false, Blink = false;
USHORT *pBuf = pRow->GetBuffer();
ExtChar Character;

	if(m_pDoc == nullptr)	return;
	while(*pBuf != 0){
		Character.Code = *(pBuf++);
		c = Character.b.Flags;
		if(c & 0x80){																								// check for control flags
			if(c & 0x01) Inverse = true;
			if(c & 0x02) Blink = true;
			if(c & 0x04) Undline = true;
			if(c == 0x80) Inverse = Blink = Undline = false;
		}
 		c = Character.b.Char;
		RenderChar(pDC, xPos + SCRN_MARGIN_X, yPos + SCRN_MARGIN_Y, c, Inverse, Undline);
		xPos += SCRN_FONT_X;
		if(++cPos == SCRNCOLS){
			if(!DblLine) return;
			DblLine = false;																				// only once
			xPos = 0;																								// do carridge return
			yPos += SCRN_FONT_Y;																		// and line feed
		}
	}
}

//////////////////////////////////////////////////////////////////////////

int CPrinter16::Flush(void)
{
  if(m_pDoc == nullptr) return DEV_ERROR;
	if(m_RowBuf.IsStale()) m_pDoc->PrintScrollRgn(&m_RowBuf);			// write to scroll buffer
	return DEV_OK;
}

//////////////////////////////////////////////////////////////////////////

int CPrinter16::NextTab()
{
	if(!m_TabWidth) return DEV_OK;
	int Pos = m_RowBuf.GetIndex();
	if((Pos % m_TabWidth) && (m_LineWidth) && (((Pos / m_TabWidth + 2) * m_TabWidth) > m_LineWidth)){
		LineFeed();
		return DEV_OK;
	}
	return m_RowBuf.NextTab(m_TabWidth);
}

//////////////////////////////////////////////////////////////////////////

void CPrinter16::CarridgeReturn()
{
	m_RowBuf.SetIndex(0);																					// do carridge return
}

//////////////////////////////////////////////////////////////////////////

void CPrinter16::LineFeed()
{
  if(m_pDoc == nullptr) return;
	int Col = m_RowBuf.GetIndex();																// save the current column index
	if(m_RowBuf.IsStale()) m_pDoc->PrintScrollRgn(&m_RowBuf);			// write to scroll buffer	
	m_pDoc->NextScrollLine(&m_RowBuf);
	m_RowBuf.SetIndex(Col);																				// reposition the column index 
	m_RowBuf.Clean();																							// mark it as unaltered
}

//////////////////////////////////////////////////////////////////////////

int CPrinter16::PageFeed()
{
  if(m_pDoc == nullptr) return DEV_ERROR;
	m_pDoc->ClearScrollArea();
	m_RowBuf.Clear();
	return DEV_OK;
}

//////////////////////////////////////////////////////////////////////////

int CPrinter16::PushChar(char ch, UCHAR Flags)
{
	if(m_RowBuf.GetIndex() >= m_LineWidth){
 		Flush();																										// write to screen
		m_RowBuf.Clear();																						// do carridge return
		LineFeed();																									// add or get next line
	}
	return m_RowBuf.Push(ch, Flags);
}

//////////////////////////////////////////////////////////////////////////

int CPrinter16::SetCursorColumn(UINT Column)
{
	UINT Col = Column % m_LineWidth;
	m_RowBuf.SetIndex(Col);
	return DEV_OK;
}

//////////////////////////////////////////////////////////////////////////

int CPrinter16::SetCursorRow(UINT Line)
{
  if(m_pDoc == nullptr) return DEV_ERROR;
	int Col = m_RowBuf.GetIndex();																// save the current column index
	if(m_RowBuf.IsStale()) m_pDoc->PrintScrollRgn(&m_RowBuf);			// write line data to scroll area
	m_pDoc->SetRowIndex(Line, &m_RowBuf);													// set row index and get existing row data
	m_RowBuf.SetIndex(Col);																				// reposition the column index 
	m_RowBuf.Clean();																							// mark it as unaltered
	return DEV_OK;
}

//////////////////////////////////////////////////////////////////////////

int CPrinter16::IncDecRow(bool Down)
{
  if(m_pDoc == nullptr) return DEV_ERROR;
	int Max = m_pDoc->GetScrollLineCount();
	int Row = m_pDoc->GetLinePos();
	if(!Down){
		if(Row > 0) --Row;
		else Row = Max;
	}
	else{
		if(Row < Max) ++Row;
		else Row = Max;
	}
	SetCursorRow(Row);
	return DEV_OK;
}

//////////////////////////////////////////////////////////////////////////

int CPrinter16::IncDecCol(bool Left)
{
	int Index = m_RowBuf.GetIndex();
	if(!Left){
		if(Index < m_LineWidth) m_RowBuf.SetIndex(++Index);
		else{
			Index = 0;
			IncDecRow(true);
			m_RowBuf.SetIndex(Index);
		}
	}
	else{
		if(Index > 0) m_RowBuf.SetIndex(--Index);
		else{
			Index = m_LineWidth;
			IncDecRow(false);
			m_RowBuf.SetIndex(Index);
		}
	}
	return DEV_OK;
}

//////////////////////////////////////////////////////////////////////////

int CPrinter16::PrintLIN(int Count)
{
int i = 0;

  if(Count >= 0) CarridgeReturn();		 													// perform a carridge return
	while(i++ < abs(Count)) LineFeed();														// increment line count
	return DEV_OK;
}

//////////////////////////////////////////////////////////////////////////

int CPrinter16::PrintChars(const char *pChars)
{
BYTE ch, Flags = 0;

	while(*pChars){
		if(m_EscSequence != eEscSequ::NONE){
			switch(m_EscSequence){
				case eEscSequ::ESCCHAR:{																// primary: escape character
					switch(*(pChars++)){
						case '&':
							m_EscSequence = eEscSequ::AMPCHAR;
							continue;
						case '1':
						case '2':
						case '3':	break;
						case 'A':
							IncDecRow(false);
							break;
						case 'B':
							IncDecRow(true);
							break;
						case 'C':
							IncDecCol(false);
							break;
						case 'D':
							IncDecCol(true);
							break;
						case 'E':
							break;
						case 'F':
							SetCursorColumn(0);
							SetCursorRow(m_pDoc->GetScrollLineCount());
							break;
						case 'G':
							SetCursorColumn(0);
							break;
						case 'H':
							SetCursorColumn(0);
							SetCursorRow(0);
							break;
						case 'I':
							break;
						case 'Y':
							g_ControlCodesDisabled = true;
							Flags = 0;
							break;
					}
					m_EscSequence = eEscSequ::NONE;
					break;
				}
				case eEscSequ::AMPCHAR:{																// secondary: ampersand '&'
					ch = *(pChars++);
					if(ch == 'd'){
						m_EscSequence = eEscSequ::LCDCHAR;
						continue;
					}
					if(ch == 'a'){
						m_EscSequence = eEscSequ::LCACHAR;
						continue;
					}
					m_EscSequence = eEscSequ::NONE;
					break;
				}
				case eEscSequ::LCACHAR:{																// secondary: lowercase 'a'
					m_EscSequence = eEscSequ::NONE;
					bool Done = false;
					do{
						UCHAR num = (UCHAR)atoi(pChars);
						bool Next = true, Relative = false;
						do{
							switch(tolower(*pChars)){
								case '+':
								case '-':{
									Relative = true;
									break;
								}
								case 'c':
									Next = false;
									SetCursorColumn(num);
									if(*(pChars++) == 'C') Done = true;
									break;
								case 'r':
									Next = false;
									SetCursorRow(num);
									if(*(pChars++) == 'R') Done = true;
									break;
								case 'y':
									Next = false;
									SetCursorRow(num);
									if(*(pChars++) == 'Y') Done = true;
									break;
								default:{
									if(*(pChars++) == 0) return DEV_ERROR;
									break;
								}
							}
						}
						while(Next);
					}
					while(!Done);
					break;
				}
				case eEscSequ::LCDCHAR:{											// secondary: lowercase 'd'
					switch(tolower(*(pChars++))){
						case '@': Flags = (char)0x80; break;			// clear all
						case 'a':	Flags = (char)0x82; break;			// set blinking
						case 'b': Flags = (char)0x81; break;			// set inverse
						case 'c':	Flags = (char)0x83; break;			// set blinking and inverse
						case 'd':	Flags = (char)0x84; break;			// set underline
						case 'e':	Flags = (char)0x86; break;			// set undeline and blinking
						case 'f':	Flags = (char)0x85; break;			// set undeline and inverse
						case 'g':	Flags = (char)0x87; break;			// set underline, inverse and blinking
					}
					m_EscSequence = eEscSequ::NONE;
					break;
				}
				default:{
					m_EscSequence = eEscSequ::NONE;
					break;
				}
			}
		}
		else{
			ch = *(pChars++);
		  if(!g_ControlCodesDisabled){
				switch(ch){
					case BEL_CHAR:{
						Beep(DEF_BEEP_FREQU, DEF_BEEP_LENGTH);							// produce the default sound
						break;
					}
					case LF_CHAR:{
						LineFeed();																					// increment line count
						Flags = 0;
						break;
					}
					case CR_CHAR:{
						CarridgeReturn();																		// write to screen buffer and clear the line buffer
						Flags = 0;
						break;
					}
					case BS_CHAR:{
						int idx = m_RowBuf.GetIndex();
						if(idx > 0)	m_RowBuf.SetIndex(idx - 1);							// do back space
						break;
					}
					case HT_CHAR:{
						if(NextTab() == -1) return DEV_ERROR;
						Flags = 0;
						break;
					}
					case FF_CHAR:{
						if(PageFeed() == -1) return DEV_ERROR;
						Flags = 0;
						break;
					}
					case ESC_CHAR:{																				// ESC character
						if(!g_EscapeDisabled) m_EscSequence = eEscSequ::ESCCHAR;
 						else PushChar(ch, Flags);
						break;
					}
					case 128:											// clear all
					case 129: 										// set inverse
					case 130:											// set blinking
					case 131:											// set blinking and inverse
					case 132:											// set underline
					case 133:											// set undeline and inverse
					case 134:											// set undeline and blinking
					case 135:	Flags = ch; break;	// set underline, inverse and blinking
					default:{
 						PushChar(ch, Flags);
						break;
					}
				}
			}
			else{
				switch(ch){
					case ESC_CHAR:{																				// ESC character
						m_EscSequence = eEscSequ::ESCCHAR;
						PushChar(ch, 0);
						break;
					}
					case CR_CHAR:{																				// perform a CR-LF operation
						PushChar(ch, 0);
						Flush();																						// write to screen buffer
						IncDecRow(true);																		// increment line count
						m_RowBuf.SetIndex(0);																// reset to the begining of the row
						break;
					}
					case 'Z':
						if(m_EscSequence == eEscSequ::ESCCHAR) g_ControlCodesDisabled = false;
					default:{
						PushChar(ch, 0);
					}
				}
			}
		}
	}
	Flush();
	return DEV_OK;
}

//////////////////////////////////////////////////////////////////////////

int CPrinter16::PrintText(eDispArea DisplayArea, LPCSTR lpStr)
{
  if(m_pDoc == nullptr) return DEV_ERROR;
	switch(DisplayArea){
		case eDispArea::SCROLL:	break;
		case eDispArea::PROMPT:{ m_pDoc->PrintRgnAt(SYS_PROMPT, lpStr); break; }
		case eDispArea::COMMENT:{ m_pDoc->PrintRgnAt(SYS_COMMENT, lpStr);	break; }
	}
	return DEV_OK;
}

//////////////////////////////////////////////////////////////////////////
