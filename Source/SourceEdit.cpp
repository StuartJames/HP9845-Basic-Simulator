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

//////////////////////////////////////////////////////////////////////

void CS45BasicDoc::FormatEditScreen(long LineIndex)
{
Program_s *pProgram = &pCntx->Program;
BString Str;
int i, Indent = 0;

	m_EditLines.RemoveAll();
  for(i = 0; i < EDITMODE_SCROLL_SIZE; ++i){
  	if((LineIndex >= 0) && (LineIndex < pProgram->Size)){
  		TokenToString(pProgram->ppCode[LineIndex++], nullptr, &Str, &Indent, m_NumberWidth);
  		if(Str.GetLength()){
				CString cstr;																													    
				cstr.Format(_T("%s"), Str.GetBuffer());																													    
				m_EditLines.Add(cstr);
			}
			if(i == EDITMODE_SECTION_SIZE){	
				if(Str.GetLength()) CopyToEditBuffer(Str.GetBuffer());
				else ClearEditBuffer();
			}
  		Str.Clear();
  	}
  	else{
			CString cstr;																													    
			cstr = _T(" ");																													    
			m_EditLines.Add(cstr);
			LineIndex++;
		}
  }
}

//////////////////////////////////////////////////////////////////////

void CS45BasicDoc::EditLine(long LineNumber)
{
Program_s *pProgram = &pCntx->Program;
int i, TopLine, Indent = 0;
BOOL Found = FALSE;
BString Str;

	m_InsertLineMode = false;
	m_EditLines.RemoveAll();		
  pCntx->RunMode = eRunMode::STOP;
	SetSystemMode(SYS_EDITLINE);
	m_NumberWidth = GetLineNumberWidth(pProgram);
	if(!m_NumberWidth) m_NumberWidth = 4;
	m_CurrentEditIndex = 0;
	for(i = 0; i < pProgram->Size; ++i){
		if((LineNumber <= pProgram->ppCode[i]->Obj.Integer) && !Found){
			TopLine = i - EDITMODE_SECTION_SIZE;																							// Start of display
			Found = TRUE;
			m_CurrentEditIndex = i;
			TokenToString(pProgram->ppCode[i], nullptr, &Str, &Indent, m_NumberWidth);
			if(Str.GetLength()) CopyToEditBuffer(Str.GetBuffer());
			else ClearEditBuffer();
			Str.Clear();
			break;
		}
	}
	if(!Found){																																						// no program lines found so initialise a new line
		EditNewLine();
		TopLine = 0 - EDITMODE_SECTION_SIZE;																								// top of screen is a negative value
	}
	FormatEditScreen(TopLine);
}

//////////////////////////////////////////////////////////////////////

void CS45BasicDoc::EditExit()
{
CString Str;

	m_EditLines.RemoveAll();		
	SetSystemMode(SYS_IDLE);
}

//////////////////////////////////////////////////////////////////////

void CS45BasicDoc::DoInsertLine(void)
{
 	if(m_InsertLineMode) ExitInsertLine();
	else EditInsertLine();
}

//////////////////////////////////////////////////////////////////////

bool CS45BasicDoc::EditInsertLine(void)
{
PC_s Pc;
Program_s *pProgram = &pCntx->Program;
int InsertNumber = 1;
char Buffer[20];
CString cstr;																													    

	ProgramGetAtIndex(pProgram, &Pc, m_CurrentEditIndex);
	m_InsertEndNumber = Pc.pToken->Obj.Integer;																						// set the termination line number
	if(m_CurrentEditIndex > 0){
		m_InsertLineInc = 1;
		if(ProgramGetAtIndex(pProgram, &Pc, m_CurrentEditIndex - 1)){												// get the previous edit line in program memory
			InsertNumber = Pc.pToken->Obj.Integer + m_InsertLineInc;													// set the program line number
		}
	}
	if(InsertNumber < m_InsertEndNumber){																									// check to see if insert mode possible
		m_EditLines.RemoveAt(m_EditLines.GetCount() - 1);																		// remove the lowest reference
		sprintf_s(&Buffer[0], 20, _T("%*ld "), m_NumberWidth, InsertNumber);
		CopyToEditBuffer(Buffer);
		cstr.Format(_T("%s"), Buffer);
		m_EditLines.InsertAt(EDITMODE_SECTION_SIZE, cstr, 1);
		m_InsertLineMode = true;
	}
	return m_InsertLineMode;
}

//////////////////////////////////////////////////////////////////////

void CS45BasicDoc::ExitInsertLine(void)
{
Program_s *pProgram = &pCntx->Program;
BString Str;
PC_s Pc;
long LastLineNumber = 0;
CString cstr;																													    

	if(!m_InsertLineMode || (pProgram->Size <= 0)) return;
	m_InsertLineMode = false;
	m_EditLines.RemoveAt(EDITMODE_SECTION_SIZE);																					// remove the edit line reference
	if(m_CurrentEditIndex < pProgram->Size){
		int BottomIndex =	m_CurrentEditIndex + EDITMODE_SECTION_SIZE;												// move lines up
		if(BottomIndex < pProgram->Size){
			if(GetProgLineNumber(pProgram, BottomIndex, &Str) != nullptr) cstr.Format(_T("%s"), Str.GetBuffer());																													    
			else cstr = _T(" ");																															// else insert a blank		    
		}
		else cstr = _T(" ");																																// else insert a blank		    
		m_EditLines.Add(cstr);
	}
	else{																																									// move line down
		if(m_CurrentEditIndex	>= pProgram->Size) m_CurrentEditIndex = pProgram->Size - 1;
		int TopLine = m_CurrentEditIndex - EDITMODE_SECTION_SIZE;
		if(TopLine >= 0){
			if(GetProgLineNumber(pProgram, TopLine, &Str) != nullptr) cstr.Format(_T("%s"), Str.GetBuffer());																													    
			else cstr = _T(" ");																															// else insert a blank		    
		}
		else cstr = _T(" ");																																// else insert a blank		    
		m_EditLines.InsertAt(0, cstr);
	}
	if(ProgramGetAtIndex(pProgram, &Pc, m_CurrentEditIndex) == nullptr){
		PrintFormatAt(SYS_COMMENT, _T("CURRENT LINE NOT FOUND"));	
	}
	else{
		if(Pc.pToken != nullptr) EditFillCntrl(Pc.pToken);
	}
}

//////////////////////////////////////////////////////////////////////

int CS45BasicDoc::EditNewLine(void)																											// insert a new line at the top or the bottom
{
char Buffer[20];
int  NewNumber;


	if(m_CurrentEditIndex >= pCntx->Program.Size){																				// at the end of the program?
		m_InsertLineInc = 10;
		NewNumber = ProgramGetLineNumber(&pCntx->Program, pCntx->Program.Size - 1);					// get previous line number
		NewNumber += m_InsertLineInc;
		m_InsertEndNumber = 99999;;																													// end number at max 
	}
	else{
		m_InsertLineInc = 1;
		NewNumber = m_InsertLineInc;
		m_CurrentEditIndex = 0;
		m_InsertEndNumber = ProgramGetLineNumber(&pCntx->Program, 0);												// get first line number
		if(m_InsertEndNumber <= NewNumber){																									// check for no more numbers
			return 0;
		}
	}
	sprintf_s(&Buffer[0], 20, _T("%*ld "), m_NumberWidth, NewNumber);
	CopyToEditBuffer(Buffer);
	m_InsertLineMode = true;
	return NewNumber;
}

//////////////////////////////////////////////////////////////////////

void CS45BasicDoc::DoDeleteLine(void)
{
BString Str;
PC_s Pc;
Program_s *pProgram = &pCntx->Program;
CString cstr = _T(" ");																													    
int Index, LineNumber = 10;

 	if(m_InsertLineMode) return ExitInsertLine();
  if((pProgram->Size == 0) || (m_CurrentEditIndex == pProgram->Size)) return;		// no more to delete
	if(m_CurrentEditIndex < pProgram->Size){
		if(ProgramGetAtIndex(pProgram, &Pc, m_CurrentEditIndex)){
			LineNumber = Pc.pToken->Obj.Integer;																			// this is used when appending a blank line to the end of the programme
			if(ProgramDelete(pProgram, Pc.pToken->Obj.Integer) == -1){								// delete the current line
				EditExit();																															// error so just bomb out
				return;
			}
		}
		m_EditLines.RemoveAt(EDITMODE_ENTRY_LINE - 1);															// remove the screen reference
		if(pProgram->Size == 0){
			m_EditLines.Add(cstr);																										// add a blank line to the screen
			Str.Format(_T("%*ld "), m_NumberWidth, LineNumber);												// format with the next line number
			CopyToEditBuffer(Str.GetBuffer());																				// plug it into the editor
			return;
		}
		if(m_CurrentEditIndex == pProgram->Size){																		// scroll down
			int TopIndex =	m_CurrentEditIndex - EDITMODE_SECTION_SIZE - 1;						// get the index of the top display line
			if(TopIndex >= 0){																												// if it's valid get the line as a text string 
				if(GetProgLineNumber(pProgram, TopIndex, &Str) != nullptr) cstr.Format(_T("%s"), Str.GetBuffer());																													    
			}
			m_EditLines.InsertAt(0, cstr);
			if(ProgramGetAtIndex(pProgram, &Pc, --m_CurrentEditIndex) == nullptr) EditNewLine();
			else if(Pc.pToken != nullptr) EditFillCntrl(Pc.pToken);
			return;
		}
		if(ProgramGetAtIndex(pProgram, &Pc, m_CurrentEditIndex) != nullptr){				// scroll up
			EditFillCntrl(Pc.pToken);																									// get the line below and plug it into the editor
			Index =	m_CurrentEditIndex + EDITMODE_SECTION_SIZE;												// now get the bottom line index
			if(Index < pProgram->Size){																								// is it actualy in program space
				if(GetProgLineNumber(pProgram, Index, &Str) != nullptr) cstr.Format(_T("%s"), Str.GetBuffer());																													    
			}
			m_EditLines.Add(cstr);																										// add the line to the bottom of the screen
			return;
		}
	}
}

//////////////////////////////////////////////////////////////////////

void CS45BasicDoc::NextEditLine(bool SaveLine)				// Scroll Up
{
Program_s *pProgram = &pCntx->Program;
BString Str;
Token_s *pTok;
PC_s Pc;
CString cstr;																													    
char Buffer[20];
long StoredIndex = m_CurrentEditIndex, LineNumber;

 	if(m_InsertLineMode && !SaveLine) return ExitInsertLine();									// insert without save? drop out of insert line mode
	CopyFromEditBuffer(&m_UserInputStr);																				// recover entry buffer
	Str.AppendChars(&m_UserInputStr.pBuffer[0]);																// save it to a string type
	pTok = TokenNewCode(Str.GetBuffer());																				// get a set of tokens for the line of code
	if(pTok != nullptr){																												// could be rubish
		if(pTok->Type == T_INTNUM){																								// must have line number
			LineNumber = pTok->Obj.Integer;																					// this is used when appending a blank line to the end of the programme
			if((pTok + 1)->Type != T_EOL){																					// check for at least 1 statement
				if(SaveLine){
					StoredIndex = ProgramStore(pProgram, pTok, pProgram->Numbered ? pTok->Obj.Integer : 0);
					CString &Update = m_EditLines.ElementAt(EDITMODE_SECTION_SIZE);			// get reference to edit string
					Update.Format(_T("%s"), Str.GetBuffer());														// update it with new text													    
				}
				else TokenDestroy(pTok);																							// delete the unused tokens
			}
		}
	}
	if(m_CurrentEditIndex != StoredIndex) return EditLine(LineNumber); 					// check for non-consecutive line numbers. Re-do page if necessary
	Str.Clear();
	if(m_CurrentEditIndex < pProgram->Size){
		m_EditLines.RemoveAt(0);																									// remove the top line reference
		if(m_InsertLineMode){																											// insert mode? Insert another line
			LineNumber += m_InsertLineInc;
			if(LineNumber < m_InsertEndNumber){																			// check to see if insert mode possible
				sprintf_s(&Buffer[0], 20, _T("%*ld "), m_NumberWidth, LineNumber);		// generate new line 
				CopyToEditBuffer(Buffer);
				cstr.Format(_T("%s"), Buffer);
				m_EditLines.InsertAt(EDITMODE_SECTION_SIZE, cstr);										// insert it into the list
				++m_CurrentEditIndex;																									// increment the program index
				return;
			}
			PrintFormatAt(SYS_COMMENT, _T("OUT OF LINE NUMBERS"));									// drop through and scroll bottom lines up
		}
		int BottomIndex =	m_CurrentEditIndex + EDITMODE_SECTION_SIZE + 1;
		if(BottomIndex < pProgram->Size){
			if(GetProgLineNumber(pProgram, BottomIndex, &Str) != nullptr) cstr.Format(_T("%s"), Str.GetBuffer());																													    
			else cstr = _T(" ");																										// else insert a blank		    
		}
		else cstr = _T(" ");																											// else insert a blank		    
		m_EditLines.Add(cstr);
		if(ProgramGetAtIndex(pProgram, &Pc, ++m_CurrentEditIndex) == nullptr) EditNewLine();		// no more lines?
		else{
			if(Pc.pToken != nullptr) EditFillCntrl(Pc.pToken);
		}
	}
}

//////////////////////////////////////////////////////////////////////

void CS45BasicDoc::PreviousEditLine(void)			// Scroll Down
{
Program_s *pProgram = &pCntx->Program;
BString Str;
Token_s *pTok = nullptr;
PC_s Pc;
long LastLineNumber = 0;
CString cstr;																													    

 	if(m_InsertLineMode) return ExitInsertLine();
	CopyFromEditBuffer(&m_UserInputStr);																				// first we get the current line number
	Str.AppendChars(&m_UserInputStr.pBuffer[0]);																// save it to a string type
	pTok = TokenNewCode(Str.GetBuffer());																				// get a set of tokens for the string
	if(pTok != nullptr){
		if(pTok->Type == T_INTNUM) LastLineNumber = pTok->Obj.Integer;						// used as reference for new line at top
		TokenDestroy(pTok);																												// delete the unused tokens
	}
	if(m_CurrentEditIndex >= 0){
		m_EditLines.RemoveAt(m_EditLines.GetCount() - 1);													// remove the reference
		int TopIndex =	m_CurrentEditIndex - EDITMODE_SECTION_SIZE - 1;						// get the index of the top display line
		if(TopIndex >= 0){																												// if it's valid get the line as a text string 
			if(GetProgLineNumber(pProgram, TopIndex, &Str) != nullptr) cstr.Format(_T("%s"), Str.GetBuffer());																													    
			else cstr = _T(" ");																										// else insert a blank		    
		}
		else cstr = _T(" ");																											// else insert a blank		    
		m_EditLines.InsertAt(0, cstr);
		if(ProgramGetAtIndex(pProgram, &Pc, --m_CurrentEditIndex) == nullptr) EditNewLine();
		else{
			if(Pc.pToken != nullptr) EditFillCntrl(Pc.pToken);
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////

void CS45BasicDoc::ScrollEditPage(bool UpDown /* = true*/)										// scroll the edit screen by EDITMODE_SECTION_SIZE lines
{
Program_s *pProgram = &pCntx->Program;
int TopLine, NewIndex, Indent = 0;
Token_s *pTok = nullptr;
BString Str;

 	if(m_InsertLineMode) return ExitInsertLine();
	if(!UpDown){
		NewIndex = m_CurrentEditIndex + EDITMODE_SECTION_SIZE;
		if(NewIndex >= pProgram->Size) NewIndex = pProgram->Size - 1;
	}
	else{
		NewIndex = m_CurrentEditIndex - EDITMODE_SECTION_SIZE;
		if(NewIndex < 0) NewIndex = 0;
	}
	TopLine = NewIndex - EDITMODE_SECTION_SIZE;																	// Start of display
	m_CurrentEditIndex = NewIndex;
	TokenToString(pProgram->ppCode[NewIndex], nullptr, &Str, &Indent, m_NumberWidth);
	if(Str.GetLength()) CopyToEditBuffer(Str.GetBuffer());
	else ClearEditBuffer();
	FormatEditScreen(TopLine);
}

//////////////////////////////////////////////////////////////////////

void CS45BasicDoc::EditFillCntrl(Token_s *pToken)
{
int Indent = 0;
BString Str;

	TokenToString(pToken, nullptr, &Str, &Indent, m_NumberWidth);
	if(Str.GetLength()) CopyToEditBuffer(Str.GetBuffer());
	else ClearEditBuffer();
}

//////////////////////////////////////////////////////////////////////

void CS45BasicDoc::StoreLine(void)			
{
Program_s *pProgram = &pCntx->Program;
BString Str;
Token_s *pTok;

	CopyFromEditBuffer(&m_UserInputStr);																						// recover entry buffer
	if(m_UserInputStr.Length){
		Str.AppendChars(&m_UserInputStr.pBuffer[0]);																	// save it to a string type
		pTok = TokenNewCode(Str.GetBuffer());																					// get a set of tokens for the line of code
		if(pTok != nullptr){
			if((pTok->Type == T_INTNUM) && ((pTok + 1)->Type != T_EOL))	m_CurrentEditIndex = ProgramStore(pProgram, pTok, pProgram->Numbered ? pTok->Obj.Integer : 0);
			else TokenDestroy(pTok);																										// delete the unused tokens
		}
	}
}
