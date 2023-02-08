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

char		CSoftKeyToken::m_TokenNames[CSoftKeyToken::KT_MAX][20] = {_T("Undefined"), _T("Clear"), _T("Clear line"), _T("Left arrow"), _T("Right arrow"),
																							 _T("Up arrow"), _T("Down arrow"), _T("Insert character"), _T("Delete character"), _T("Home"), _T("Execute"), _T("")};

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_DYNCREATE(CSoftKeyToken, CObject)

CSoftKeyToken::CSoftKeyToken()
{
	m_Token = KT_UNDEFINED;
}

//////////////////////////////////////////////////////////////////////////

CSoftKeyToken::CSoftKeyToken(const CSoftKeyToken &Src)
{
	Clone(&Src);
}

//////////////////////////////////////////////////////////////////////////

CSoftKeyToken::CSoftKeyToken(int tok)
{
	m_Token = tok;
}

//////////////////////////////////////////////////////////////////////////

CSoftKeyToken::CSoftKeyToken(CString text)
{ 
	m_Token = KT_TEXT;
	m_Text = text;
}

//////////////////////////////////////////////////////////////////////////

CSoftKeyToken::~CSoftKeyToken()
{
}

////////////////////////////////////////////////////////////////////////////////////

void CSoftKeyToken::operator=(const CSoftKeyToken &Src) throw()
{
	Clone(&Src);
}

//////////////////////////////////////////////////////////////////////////

CString	CSoftKeyToken::GetString(void)
{
	if(m_Token == KT_TEXT) return m_Text;
	return CString(m_TokenNames[m_Token]);
}

//////////////////////////////////////////////////////////////////////////

void	CSoftKeyToken::SetString(char *pBuffer, int Count)
{
	if(m_Token != KT_TEXT) return;
	m_Text.SetString(pBuffer, Count);
}

//////////////////////////////////////////////////////////////////////////

bool 	CSoftKeyToken::IsType(int Token)
{
	return (m_Token == Token) ? true : false;
}

//////////////////////////////////////////////////////////////////////////

void CSoftKeyToken::Clone(const CSoftKeyToken *pSrc)
{
	m_Token = pSrc->m_Token;
	m_Text = pSrc->m_Text;
}

//////////////////////////////////////////////////////////////////////////

#ifdef _DEBUG
void CSoftKeyToken::AssertValid() const
{
	CObject::AssertValid();
}

void CSoftKeyToken::Dump(CDumpContext& dc) const
{
  CObject::Dump(dc);
  dc << m_Token;
}
#endif

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

void CS45BasicDoc::InitialiseSoftKey(UINT KeyNumber)
{
	switch(KeyNumber){
		case 8:{
			m_SoftKeys[KeyNumber].Add(new CSoftKeyToken(CSoftKeyToken::KT_CLEAR_LINE));
			m_SoftKeys[KeyNumber].Add(new CSoftKeyToken(_T("GET")));
			break;
		}
		case 9:{
			m_SoftKeys[KeyNumber].Add(new CSoftKeyToken(CSoftKeyToken::KT_CLEAR_LINE));
			m_SoftKeys[KeyNumber].Add(new CSoftKeyToken(_T("LOAD")));
			break;
		}
		case 10:{
			m_SoftKeys[KeyNumber].Add(new CSoftKeyToken(CSoftKeyToken::KT_CLEAR_LINE));
			m_SoftKeys[KeyNumber].Add(new CSoftKeyToken(_T("SAVE")));
			break;
		}
		case 11:{
			m_SoftKeys[KeyNumber].Add(new CSoftKeyToken(CSoftKeyToken::KT_CLEAR_LINE));
			m_SoftKeys[KeyNumber].Add(new CSoftKeyToken(_T("STORE")));
			break;
		}
		case 12:{
			m_SoftKeys[KeyNumber].Add(new CSoftKeyToken(CSoftKeyToken::KT_CLEAR_LINE));
			m_SoftKeys[KeyNumber].Add(new CSoftKeyToken(_T("EDIT")));
			break;
		}
		case 13:{
			m_SoftKeys[KeyNumber].Add(new CSoftKeyToken(CSoftKeyToken::KT_CLEAR_LINE));
			m_SoftKeys[KeyNumber].Add(new CSoftKeyToken(_T("EDIT LINE")));
			break;
		}
		case 14:{
			m_SoftKeys[KeyNumber].Add(new CSoftKeyToken(CSoftKeyToken::KT_CLEAR_LINE));
			m_SoftKeys[KeyNumber].Add(new CSoftKeyToken(_T("LIST")));
			break;
		}
		case 15:{
			m_SoftKeys[KeyNumber].Add(new CSoftKeyToken(CSoftKeyToken::KT_CLEAR_LINE));
			m_SoftKeys[KeyNumber].Add(new CSoftKeyToken(_T("SCRATCH")));
			break;
		}
		default:{
			m_SoftKeys[KeyNumber].Add(new CSoftKeyToken(CSoftKeyToken::KT_UNDEFINED));
			break;
		}
	}
}

//////////////////////////////////////////////////////////////////////////

void CS45BasicDoc::ResetSoftKey(UINT KeyNumber)
{
	DestroySoftToken(m_SoftKeys[KeyNumber]);
	InitialiseSoftKey(KeyNumber);
}

//////////////////////////////////////////////////////////////////////////

void CS45BasicDoc::ResetSoftKeys(void)
{
	DeleteSoftKeys();
	for(int i = 0; i < SOFT_KEY_COUNT * 2; ++i) InitialiseSoftKey(i);
}

//////////////////////////////////////////////////////////////////////////

void CS45BasicDoc::CopyTokens(CObArray& Dest, CObArray& Srce)
{
	for(int i = 0; i < Srce.GetCount(); ++i) Dest.Add(new CSoftKeyToken(*(CSoftKeyToken*)Srce.GetAt(i)));																												// add a new copy of the original token
}

//////////////////////////////////////////////////////////////////////////

void CS45BasicDoc::DeleteSoftKeys(void)
{
	for(int i = 0; i < SOFT_KEY_COUNT * 2; ++i)	DestroySoftToken(m_SoftKeys[i]);	
}

//////////////////////////////////////////////////////////////////////////////////////////////

void	CS45BasicDoc::DestroySoftToken(CObArray& KeyTokens)
{
CObject *pRow;

	int Count = KeyTokens.GetCount();																														// get the token count
	while(Count--){																															 
		pRow = KeyTokens.GetAt(0);																																	// get a pointer to the token object
		KeyTokens.RemoveAt(0);																																			// remove it from the array
		delete pRow;																																							// then delete it
	}
}

//////////////////////////////////////////////////////////////////////

void CS45BasicDoc::EditSoftKey(int KeyNumber)
{
CString cstr;

	m_InsertLineMode = false;
  pCntx->RunMode = eRunMode::STOP;
	SetSystemMode(SYS_EDITKEY);
	m_EditSoftKeyNumber = KeyNumber;
	CopyTokens(m_SoftKey, m_SoftKeys[KeyNumber]);
	m_CurrentTokenIndex = m_SoftKey.GetCount() - 1;																						// get the last token for this key
	m_pCurrentToken = (CSoftKeyToken*)m_SoftKey.GetAt(m_CurrentTokenIndex);										// maake that token current
	FormatEditKeyScreen(m_CurrentTokenIndex);
}

//////////////////////////////////////////////////////////////////////

void CS45BasicDoc::FormatEditKeyScreen(int TokenIndex)
{
int i;
CString cstr;																													    
CSoftKeyToken	*pToken;

	m_EditLines.RemoveAll();
	int top = m_CurrentTokenIndex - EDITMODE_SECTION_SIZE + 1;																			// calculate the position of the first token to be displayed
	cstr.Format(_T("KEY %d"), m_EditSoftKeyNumber); 
	m_EditLines.Add(cstr);
  for(i = 1; i < EDITMODE_SCROLL_SIZE; ++i){
  	if((top >= 0) && (top < m_SoftKey.GetCount())){
			pToken = (CSoftKeyToken*)m_SoftKey.GetAt(top++);
			if(pToken->IsType(CSoftKeyToken::KT_TEXT)) cstr.Format(_T(" %s"), pToken->GetString()); // if it's a text token then no hyphen
			else cstr.Format(_T("-%s"), pToken->GetString()); 
			m_EditLines.Add(cstr);
			if(i == EDITMODE_SECTION_SIZE){																													// the last token will be placed in the edit buffer
				if(cstr.GetLength()){
					CopyToEditBuffer(cstr.GetBuffer());
					cstr.ReleaseBuffer();
				}
				else ClearEditBuffer();
			}
  	}
  	else{
			cstr = _T(" ");																																					// add blanks    
			m_EditLines.Add(cstr);
			++top;
		}
  }
}

//////////////////////////////////////////////////////////////////////

void CS45BasicDoc::EditSoftKeyCursorMove(bool left)
{
	if(m_pCurrentToken->IsType(CSoftKeyToken::KT_TEXT)){
		if(left){
			if(m_KeyColumn > 1)	m_KeyColumn--;																											// only go back to column 1
			else EditSoftKeyGetPreviousToken();																											// get next token	in list
		}
		else{
			if(m_KeyColumn < (int)strlen(m_pKeyEntryBuffer)) m_KeyColumn++;
			else EditSoftKeyGetNextToken();																													// get previous token in list
		}
	}
	else{
		if(left) EditSoftKeyGetPreviousToken();
		else EditSoftKeyGetNextToken();
	}
}

//////////////////////////////////////////////////////////////////////

void CS45BasicDoc::EditSoftKeyGetPreviousToken(void)
{
	if(m_CurrentTokenIndex > 0){
		FormatEditKeyScreen(--m_CurrentTokenIndex);																								// no scroll, just re-display the whole screen
		m_pCurrentToken = (CSoftKeyToken*)m_SoftKey.GetAt(m_CurrentTokenIndex);
		if(m_pCurrentToken->IsType(CSoftKeyToken::KT_TEXT)) m_KeyColumn = (int)strlen(m_pKeyEntryBuffer);
		else m_KeyColumn = 1;
	}
}

//////////////////////////////////////////////////////////////////////

void CS45BasicDoc::EditSoftKeyGetNextToken(void)
{
	if(m_CurrentTokenIndex < (m_SoftKey.GetCount() - 1)){
		FormatEditKeyScreen(++m_CurrentTokenIndex);																								// no scroll, just re-display the whole screen
		m_pCurrentToken = (CSoftKeyToken*)m_SoftKey.GetAt(m_CurrentTokenIndex);
		m_KeyColumn = 1;
	}
}

//////////////////////////////////////////////////////////////////////

void CS45BasicDoc::EditSoftKeyBack(void)
{
	if(m_pCurrentToken->IsType(CSoftKeyToken::KT_TEXT)){
		if(m_KeyColumn > 1){
			for(int i = m_KeyColumn - 1; i < m_KeyLineLength; i++) m_pKeyEntryBuffer[i] = m_pKeyEntryBuffer[i + 1]; 
			m_pKeyEntryBuffer[m_KeyLineLength] = 0; // terminate pCurrent line
			--m_KeyColumn;
			--m_KeyLineLength;
			return;
		}
	}
	EditSoftKeyDeleteToken();
}

//////////////////////////////////////////////////////////////////////

void CS45BasicDoc::EditSoftKeyDelete(void)
{
	if(m_pCurrentToken->IsType(CSoftKeyToken::KT_TEXT)){
		if(m_KeyColumn < m_KeyLineLength){
			for(int i = m_KeyColumn; i < m_KeyLineLength; i++) m_pKeyEntryBuffer[i] = m_pKeyEntryBuffer[i + 1]; 
			m_pKeyEntryBuffer[m_KeyLineLength] = 0; 																								// Replace the last character on the line with a space. 
			--m_KeyLineLength;
			return;
		}
	}
	EditSoftKeyDeleteToken();
}

//////////////////////////////////////////////////////////////////////

void CS45BasicDoc::EditSoftKeyDeleteToken(void)
{
	CSoftKeyToken *pToken = (CSoftKeyToken*)m_SoftKey.GetAt(m_CurrentTokenIndex);							// get a pointer to the token object
	m_SoftKey.RemoveAt(m_CurrentTokenIndex);																									// remove it from the array
	delete pToken;																																							// then delete it
	if((m_CurrentTokenIndex	> 0) && (m_CurrentTokenIndex >= m_SoftKey.GetCount())){						// decrement index if this was the end token
		m_CurrentTokenIndex--;
	}
	if((m_CurrentTokenIndex	== 0) && (m_SoftKey.GetCount() == 0)){
		m_SoftKey.Add(new CSoftKeyToken(CSoftKeyToken::KT_UNDEFINED));													// must have at least one object
	}
	m_pCurrentToken = (CSoftKeyToken*)m_SoftKey.GetAt(m_CurrentTokenIndex);										
	FormatEditKeyScreen(m_CurrentTokenIndex);															
	if(m_pCurrentToken->IsType(CSoftKeyToken::KT_TEXT)) m_KeyColumn = (int)strlen(m_pKeyEntryBuffer); // place cursor at end
	else m_KeyColumn = 1;
}

//////////////////////////////////////////////////////////////////////

void CS45BasicDoc::EditSoftKeyExit(bool Stop)
{
CString Str;

  if(!Stop){
		m_pCurrentToken->SetString(&m_pKeyEntryBuffer[1], m_KeyLineLength);											// save the edit buffer
		DestroySoftToken(m_SoftKeys[m_EditSoftKeyNumber]);																			// destroy the original
		CopyTokens(m_SoftKeys[m_EditSoftKeyNumber], m_SoftKey);																	// copy the modified
	}
	DestroySoftToken(m_SoftKey);																															// destroy the temporary tokens
	m_EditLines.RemoveAll();																																	// clear the screen
	SetSystemMode(SYS_IDLE);																																	// exit edit key mode
}

//////////////////////////////////////////////////////////////////////

void CS45BasicDoc::ProcessEditSoftKey(int VKCode, bool CtrlState)
{
	m_pCurrentToken->SetString(&m_pKeyEntryBuffer[1], m_KeyLineLength);												// save the edit buffer
	if(!CtrlState){
		switch(VKCode){
			case VK_ESCAPE:{
				EditSoftKeyExit(true);
				break;
			}
			case VK_HOME:{       
     		m_KeyColumn = 1;
				break; 
			}
			case VK_LEFT:{																																						// Left arrow 
				EditSoftKeyCursorMove(true);
				break; 
			}
			case VK_RIGHT:{																																						// Right arrow 
				EditSoftKeyCursorMove(false);
				break; 
			}
			case VK_INSERT:{
				if(m_pCurrentToken->IsType(CSoftKeyToken::KT_TEXT))	SetInsertMode((m_TypingMode == TM_INSERT) ? TM_OVERWRITE : TM_INSERT);
				break;
			}
			case VK_BACK:{
				EditSoftKeyBack();
				break;
			}
			case VK_DELETE:{     
				EditSoftKeyDelete();
				break; 		
			}
		}
	}
	else{
		CSoftKeyToken *pToken = nullptr;
		switch(VKCode){
			case VK_CLEAR:{
				pToken = new CSoftKeyToken(CSoftKeyToken::KT_CLEAR_LINE);
				break;
			}
			case VK_LEFT:{
				pToken = new CSoftKeyToken(CSoftKeyToken::KT_LEFT_ARROW);
				break;
			}
			case VK_RIGHT:{
				pToken = new CSoftKeyToken(CSoftKeyToken::KT_RIGHT_ARROW);
				break;
			}
			case VK_UP:{
				pToken = new CSoftKeyToken(CSoftKeyToken::KT_UP_ARROW);
				break;
			}
			case VK_DOWN:{
				pToken = new CSoftKeyToken(CSoftKeyToken::KT_DOWN_ARROW);
				break;
			}
			case VK_INSERT:{
				pToken = new CSoftKeyToken(CSoftKeyToken::KT_INS_CHAR);
				break;
			}
			case VK_DELETE:{
				pToken = new CSoftKeyToken(CSoftKeyToken::KT_CLEAR_LINE);
				break;
			}
			case VK_HOME:{
				pToken = new CSoftKeyToken(CSoftKeyToken::KT_HOME);
				break;
			}
			case VK_RETURN:{
				pToken = new CSoftKeyToken(CSoftKeyToken::KT_EXECUTE);
				break;
			}
		}
		if(pToken != nullptr){
			if(m_pCurrentToken->IsType(CSoftKeyToken::KT_UNDEFINED)){
				m_SoftKey.RemoveAt(m_CurrentTokenIndex);																							// remove it from the array
				delete m_pCurrentToken;																																	// then delete it
			}
			else ++m_CurrentTokenIndex;
			m_SoftKey.InsertAt(m_CurrentTokenIndex, pToken);
			m_pCurrentToken = pToken;
			FormatEditKeyScreen(m_CurrentTokenIndex);																									// no scroll, just re-display the whole screen
			m_KeyColumn = 1;
		}
	}
}

//////////////////////////////////////////////////////////////////////////

void CS45BasicDoc::CheckForTextToken(void)
{
	m_pCurrentToken = (CSoftKeyToken*)m_SoftKey.GetAt(m_CurrentTokenIndex);											// get a pointer to the token object
	if(m_pCurrentToken->IsType(CSoftKeyToken::KT_TEXT)) return;																		// return if it's a text token
	if(m_pCurrentToken->IsType(CSoftKeyToken::KT_UNDEFINED)){
		m_SoftKey.RemoveAt(m_CurrentTokenIndex);																									// remove it from the array
		delete m_pCurrentToken;																																			// then delete it
	}
	else ++m_CurrentTokenIndex;
	m_pCurrentToken = new CSoftKeyToken(_T(" "));
	m_SoftKey.InsertAt(m_CurrentTokenIndex, m_pCurrentToken);																		// else insert one
	ClearEditBuffer();
	FormatEditKeyScreen(m_CurrentTokenIndex);																											// no scroll, just re-display the whole screen
	m_KeyColumn = 1;
}

//////////////////////////////////////////////////////////////////////////

bool CS45BasicDoc::ListSoftKey(long OutDev)
{
CString cstr;

	for(int i = 0; i < SOFT_KEY_COUNT * 2; ++i){
		for(int j = 0; j < m_SoftKeys[i].GetCount(); ++j){
			CSoftKeyToken *pKey = (CSoftKeyToken*)m_SoftKeys[i].GetAt(j);
			if(pKey->GetToken() == CSoftKeyToken::KT_TEXT) cstr.Format(_T(" %s\r\n"), pKey->GetString()); 
			else cstr.Format(_T("-%s\r\n"), pKey->GetString()); 
			if(j == 0){
				if(g_pPrinter->PrintFormat(_T("KEY%2d%s"), i, cstr) == -1) return false;
			}
			else{
				if(g_pPrinter->PrintFormat(_T("     %s"), cstr) == -1) return false;
			}
		}
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////

void CS45BasicDoc::ProcessSoftKey(int Key)
{
	MyTrace("Got soft key\n");
	m_pMainView->SetFocus();
  bool ShiftState = m_pMainView->GetShiftState();
	switch(m_SystemMode){
		case SYS_IDLE:
		case SYS_PAUSED:{
			int i = (ShiftState == true) ? Key - 0x66 : Key - 0x70;
			for(int j = 0; j < m_SoftKeys[i].GetCount(); ++j){
				CSoftKeyToken *pKey = (CSoftKeyToken*)m_SoftKeys[i].GetAt(j);
				switch(pKey->GetToken()){
					case CSoftKeyToken::KT_CLEAR:
					case CSoftKeyToken::KT_CLEAR_LINE:{
						ProcessEditKey(VK_CLEAR);
						break;
					}
					case CSoftKeyToken::KT_LEFT_ARROW:{
						ProcessEditKey(VK_LEFT);
						break;
					}
					case CSoftKeyToken::KT_RIGHT_ARROW:{
						ProcessEditKey(VK_RIGHT);
						break;
					}
					case CSoftKeyToken::KT_UP_ARROW:{
						ProcessEditKey(VK_UP);
						break;
					}
					case CSoftKeyToken::KT_DOWN_ARROW:{
						ProcessEditKey(VK_DOWN);
						break;
					}
					case CSoftKeyToken::KT_INS_CHAR:{
						ProcessEditKey(VK_INSERT);
						break;
					}
					case CSoftKeyToken::KT_DEL_CHAR:{
						ProcessEditKey(VK_DELETE);
						break;
					}
					case CSoftKeyToken::KT_HOME:{
						ProcessEditKey(VK_HOME);
						break;
					}
					case CSoftKeyToken::KT_EXECUTE:{
		  			DoEnter();
						break;
					}
					case CSoftKeyToken::KT_TEXT:{
						CString cstr = pKey->GetString();
						char *pBuff = cstr.GetBuffer();
						CopyToEditBuffer(pBuff);
						cstr.ReleaseBuffer();
						break;
					}
				}
			}	
			break;
		}
		case SYS_RUN:{
			ProcessOnKey((ShiftState == true) ? Key - 0x66 : Key - 0x70);
			break;
		}
		case SYS_EDITKEY:{
			EditSoftKeyExit(false);
			break;
		}
		case SYS_EDITLINE:{
			break;
		}
	}
}

//////////////////////////////////////////////////////////////////////////

void CS45BasicDoc::OnSoftKey(UINT nID)
{
	ProcessSoftKey(VK_F1 + (nID - ID_SOFTKEY_F1));
}

//////////////////////////////////////////////////////////////////////////

void CS45BasicDoc::OnUpdateSoftKeyUI(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(TRUE);
}


