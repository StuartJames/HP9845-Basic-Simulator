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

CPrinter0::CPrinter0(void) : CPrinter()
{
	m_EscSequence = eEscSequ::NONE;
}

//////////////////////////////////////////////////////////////////////////

CPrinter0::~CPrinter0(void)
{
}

//////////////////////////////////////////////////////////////////////////

void CPrinter0::EndPrint(void)
{
  if(m_pDoc == nullptr) return;
	m_pDoc->EndPrint();			
}

//////////////////////////////////////////////////////////////////////////

int CPrinter0::Flush(void)
{
  if(m_pDoc == nullptr) return DEV_ERROR;
	if(m_RowBuf.IsStale()) m_pDoc->PrintHardcopy(&m_RowBuf);			// print the buffer
	m_RowBuf.Clear();
	return DEV_OK;
}

//////////////////////////////////////////////////////////////////////////

int CPrinter0::NextTab()
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

void CPrinter0::CarridgeReturn()
{
	m_RowBuf.SetIndex(0);																					// do carridge return
}

//////////////////////////////////////////////////////////////////////////

void CPrinter0::LineFeed()
{
  if(m_pDoc == nullptr) return;
	m_RowBuf.Push('\n', 0);
	Flush();																											// write to scroll buffer	
}

//////////////////////////////////////////////////////////////////////////

int CPrinter0::PageFeed()
{
  if(m_pDoc == nullptr) return DEV_ERROR;
	m_RowBuf.Push('\f', 0);
	Flush();
	return DEV_OK;
}

//////////////////////////////////////////////////////////////////////////

int CPrinter0::PushChar(char ch, UCHAR Flags)
{
	if(m_RowBuf.GetIndex() >= m_LineWidth) LineFeed();						// add or get next line
	return m_RowBuf.Push(ch, Flags);
}

//////////////////////////////////////////////////////////////////////////

int CPrinter0::SetCursorColumn(UINT Column)
{
	UINT Col = Column % m_LineWidth;
	m_RowBuf.SetIndex(Col);
	return DEV_OK;
}

//////////////////////////////////////////////////////////////////////////

int CPrinter0::PrintLIN(int Count)
{
int i = 0;

  if(Count >= 0) CarridgeReturn();		 													// perform a carridge return
	while(i++ < abs(Count)) LineFeed();														// increment line count
	return DEV_OK;
}

//////////////////////////////////////////////////////////////////////////

int CPrinter0::PrintChars(const char *pChars)
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
					m_EscSequence = eEscSequ::NONE;
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
						LineFeed();																					// increment line count
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
