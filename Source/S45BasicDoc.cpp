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

#include "./Basic/Basic.h"
#include "S45Basic.h"
#include "S45BasicView.h"
#include "S45BasicDoc.h"
#include "Globals.h"
//#include "Basic.h"

#include <propkey.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

char		CS45BasicDoc::m_SysCommands[10][20] = {_T("EDIT"), _T("LIST"), _T("STOP"), _T("PAUSE"), _T("CONT"),
																							 _T("STEP"), _T("RESULT"), _T("CLEAR"), _T("LINE"), _T("KEY")};

int			CS45BasicDoc::m_EntryLinePosition[DISP_MODE_COUNT] = {NORMALMODE_ENTRY_LINE, NORMALMODE_ENTRY_LINE, EDITMODE_ENTRY_LINE};


CRITICAL_SECTION ViewUpdateSemaphore;

static char BASED_CODE szPrinter[] = _T("SelectedPrinter");
static char BASED_CODE szPrintDriver[] = _T("PrintDriver");
static char BASED_CODE szPrintDevice[] = _T("PrintDevice");
static char BASED_CODE szPrintPort[] = _T("PrintPort");

//////////////////////////////////////////////////////////////////////////////////////////////

static void GetProfilePrinter(LPCSTR szSec, CString &Driver, CString &Device, CString &Port)
{
CWinApp* pApp = AfxGetApp();

	Driver = pApp->GetProfileString(szSec, szPrintDriver);
	Device = pApp->GetProfileString(szSec, szPrintDevice);
	Port = pApp->GetProfileString(szSec, szPrintPort);
}


static void WriteProfilePrinter(LPCSTR szSec, CString Driver, CString Device, CString Port)
{
CWinApp* pApp = AfxGetApp();

	if(!Driver.IsEmpty()) pApp->WriteProfileString(szSec, szPrintDriver, (LPCSTR)Driver);
	if(!Device.IsEmpty()) pApp->WriteProfileString(szSec, szPrintDevice, (LPCSTR)Device);
	if(!Port.IsEmpty()) pApp->WriteProfileString(szSec, szPrintPort, (LPCSTR)Port);
}


//////////////////////////////////////////////////////////////////////////////////////////////

CRecallList::CRecallList(void) noexcept
{
	m_NewNode = false;
}

//////////////////////////////////////////////////////////////////////////////////////////////

CRecallList::~CRecallList(void)
{
	m_RecallList.RemoveAll();
}

//////////////////////////////////////////////////////////////////////////////////////////////

void CRecallList::Append(Message_s *pMsg)
{
	if(m_RecallList.GetCount() > 19)	m_RecallList.Remove(0);
	m_pCurrentNode = m_RecallList.Add();
	m_pCurrentNode->GetString()->AppendChars(pMsg->pBuffer);
	m_NewNode = true;																											// this new node is the active node
}

//////////////////////////////////////////////////////////////////////////////////////////////

BString *CRecallList::Recall(bool IsShift)
{
	if(m_RecallList.GetCount() > 0){
		if(IsShift)	m_pCurrentNode = m_pCurrentNode->GetNext();							// always get next node
		else if(!m_NewNode) m_pCurrentNode = m_pCurrentNode->GetPrev();			// but only previous when not a new node
		m_NewNode = false;
		if(m_pCurrentNode != nullptr) return m_pCurrentNode->GetString();
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////////////////////

IMPLEMENT_DYNCREATE(CS45BasicDoc, CDocument)


BEGIN_MESSAGE_MAP(CS45BasicDoc, CDocument)
	ON_COMMAND(ID_FILE_PRINT_SETUP, OnPrinterSetup)
	ON_UPDATE_COMMAND_UI_RANGE(ID_SOFTKEY_F1, ID_SOFTKEY_F10, OnUpdateSoftKeyUI)
	ON_COMMAND_RANGE(ID_SOFTKEY_F1, ID_SOFTKEY_F10, OnSoftKey)
	ON_UPDATE_COMMAND_UI_RANGE(ID_CONTROL_K1, ID_CONTROL_K10, OnUpdateControlUI)
	ON_COMMAND_RANGE(ID_CONTROL_K1, ID_CONTROL_K10, OnControlKey)
END_MESSAGE_MAP()

//////////////////////////////////////////////////////////////////////////////////////////////

CS45BasicDoc::CS45BasicDoc() noexcept
{
	m_pMainView = nullptr;
	m_pWnd = nullptr;
	m_bLoaded = false;
	m_KeyColumn = 0;
	m_KeyLineLength = 0;
	m_RowWriteIndex = 0;
	m_ScrollRowOffset = 0;
	m_TypingMode = TM_INSERT;
	m_ShiftMode = false;						// false for uppercase, true for lowercase
	m_InsertLineMode = false;				// insert a blank edit line for editing 
	m_SystemMode = SYS_IDLE;
	m_DisplayMode = DISPLAY_NORMAL;
	m_GraphicsOn = false;
	m_EventType = EVENT_NONE;
	m_hLoopEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
	m_hClosedEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
	m_UserInputStr.pBuffer = new char[ENTRYCOLS];
	m_pKeyEntryBuffer = new char[ENTRYCOLS];
	m_CtrlKeyState = false;
	m_pCurrentToken = nullptr;
	m_CurrentTokenIndex = 0;

	m_PrintRgn[SYS_PROMPT].Row[DISPLAY_NORMAL] = NORMALMODE_PROMPT_LINE;
	m_PrintRgn[SYS_PROMPT].Row[DISPLAY_EDIT] = INVALID_DISPLAY_POSITION;
	m_PrintRgn[SYS_PROMPT].DoubleLine = false;

	m_PrintRgn[USER_INPUT].Row[DISPLAY_NORMAL] = NORMALMODE_ENTRY_LINE;
	m_PrintRgn[USER_INPUT].Row[DISPLAY_EDIT] = EDITMODE_ENTRY_LINE;
	m_PrintRgn[USER_INPUT].DoubleLine = true;

	m_PrintRgn[SYS_COMMENT].Row[DISPLAY_NORMAL] = NORMALMODE_COMMENT_LINE;
	m_PrintRgn[SYS_COMMENT].Row[DISPLAY_EDIT] = EDITMODE_COMMENT_LINE;
	m_PrintRgn[SYS_COMMENT].DoubleLine = false;
	InitializeCriticalSection(&ViewUpdateSemaphore);

	m_DirectPrintList = new CStringList;
	m_DirectPrintComms.bConnected = false;
	m_DirectPrintComms.nType = CT_PARALLEL;
	m_DirectPrintComms.sPort = "LPT1";
	m_DirectPrintComms.nBaud = 9600;
	m_DirectPrintComms.nDataBits = 8;
	m_DirectPrintComms.bDTRDSR = false;
	m_DirectPrintComms.nParity = 0;
	m_DirectPrintComms.bRTSCTS = true;
	m_DirectPrintComms.nStopBits = 0;
	m_DirectPrintComms.bXONXOFF = false;
 	m_hDirectPrintEvent = CreateEvent(NULL, FALSE, FALSE, NULL); // auto reset, initially reset
	GetProfilePrinter(szPrinter, m_PrintDriver, m_PrintDevice, m_PrintPort);

	for(int i = 0; i < SOFT_KEY_COUNT * 2; ++i) InitialiseSoftKey(i);
}

//////////////////////////////////////////////////////////////////////////////////////////////

CS45BasicDoc::~CS45BasicDoc()
{
	m_EventType = EVENT_EXIT;
	SetEvent(m_hLoopEvent);																
	WaitForSingleObject(m_hClosedEvent, INFINITE);
  if(m_hLoopEvent){
    CloseHandle(m_hLoopEvent);
    m_hLoopEvent = nullptr;
  }
	delete [] m_UserInputStr.pBuffer;
	delete [] m_pKeyEntryBuffer;
	delete m_DirectPrintList;
	DestroyScrollLines();
	int Count = m_ScrollLines.GetCount();
	DeleteSoftKeys();
	WriteProfilePrinter(szPrinter, m_PrintDriver, m_PrintDevice, m_PrintPort);
	DeleteCriticalSection(&ViewUpdateSemaphore);
}

//////////////////////////////////////////////////////////////////////////////////////////////

void	CS45BasicDoc::DestroyScrollLines()
{
CObject *pRow;

	int Count = m_ScrollLines.GetCount();
	while(Count--){
		pRow = m_ScrollLines.GetAt(0);
		m_ScrollLines.RemoveAt(0);
		delete pRow;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////

#ifdef _DEBUG
void CS45BasicDoc::AssertValid() const
{
	CDocument::AssertValid();
}

void CS45BasicDoc::Dump(CDumpContext& dc) const
{
	CDocument::Dump(dc);
}
#endif //_DEBUG

//////////////////////////////////////////////////////////////////////////////////////////////

void CS45BasicDoc::InitialiseDocument()
{
	m_pWnd = (CMainFrame*)AfxGetMainWnd();
  ASSERT_VALID(m_pWnd);
  m_pMainView = (CS45BasicView*)m_pWnd->GetActiveView();
	m_pMainView->SetCaretType(m_TypingMode == TM_INSERT);
	SetSystemMode(SYS_IDLE);
	ClearEditBuffer();
	GetCommsSpecs();
}

//////////////////////////////////////////////////////////////////////////////////////////////

BOOL CS45BasicDoc::OnNewDocument()
{
	if(!CDocument::OnNewDocument())	return FALSE;
	return TRUE;
}

//////////////////////////////////////////////////////////////////////////////////////////////

BOOL CS45BasicDoc::OnOpenDocument(LPCTSTR lpszPathName)
{
	return TRUE;
}

//////////////////////////////////////////////////////////////////////////////////////////////

void CS45BasicDoc::OnCloseDocument()
{
	CloseDirectPrint();
	if(m_hDirectPrintEvent){
		CloseHandle(m_hDirectPrintEvent);
		m_hDirectPrintEvent = NULL;
	}
	SaveCommsSpecs();
	CDocument::OnCloseDocument();
}

//////////////////////////////////////////////////////////////////////////////////////////////

BOOL CS45BasicDoc::OnSaveDocument(LPCTSTR lpszPathName)
{
	return TRUE;
}

//////////////////////////////////////////////////////////////////////////////////////////////

void CS45BasicDoc::Serialize(CArchive& ar)
{
	if(ar.IsStoring()){
	}
	else{
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////

bool CS45BasicDoc::BasicNotify(WPARAM wParam, LPARAM lParam) 
{
  if(m_pWnd == nullptr) return nullptr;
	switch(LOWORD(lParam)){
		case CN_STATE_CHANGE:{
			if(pCntx->RunState == eRunState::PAUSED) SetSystemMode(SYS_PAUSED);
			else if(pCntx->RunState == eRunState::STOPPED) SetSystemMode(SYS_IDLE);
			else SetSystemMode(SYS_RUN);
			break;
		}
		case CN_CLEAR_SCREEN:{
			ClearScrollArea();
			if(pCntx->RunState == eRunState::STOPPED) ClearLine(SYS_PROMPT);
			ClearLine(USER_INPUT);
			ClearLine(SYS_COMMENT);
		}
		default:
			break;
	}
  return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////////////////////

void CS45BasicDoc::ScrollLine(bool UpDown /* = true*/)				 // scroll the normal alpha screen by 1 line
{
	if(UpDown){
		if(m_ScrollLines.GetCount() > NORMALMODE_SCROLL_SIZE + m_ScrollRowOffset) m_ScrollRowOffset++;			
	}
	else if(m_ScrollRowOffset != 0) m_ScrollRowOffset--;
}

//////////////////////////////////////////////////////////////////////////////////////////////

void CS45BasicDoc::ScrollPage(bool UpDown /* = true*/)			  // scroll the normal alpha screen by NORMALMODE_SYS_PRINT_SIZE lines
{
	if(UpDown){
		if(m_ScrollLines.GetCount() > NORMALMODE_SCROLL_SIZE * 2 + m_ScrollRowOffset) m_ScrollRowOffset += NORMALMODE_SCROLL_SIZE;	
		else m_ScrollRowOffset += (m_ScrollLines.GetCount() - (NORMALMODE_SCROLL_SIZE + m_ScrollRowOffset));
	}
	else if((m_ScrollRowOffset -= NORMALMODE_SCROLL_SIZE) < 0) m_ScrollRowOffset = 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////

void CS45BasicDoc::PrintScrollRgn(ExTextRow *pRow)
{
ExTextRow *pNew;

  if(m_pMainView == nullptr) return;
  EnterCriticalSection(&ViewUpdateSemaphore);																	// wait for view update to complete then claim semaphore
	int RowCount = m_ScrollLines.GetCount();																		// get the total number of rows
	if(m_RowWriteIndex < RowCount){																							// is the write row an existing row
		pNew = (ExTextRow*)m_ScrollLines.GetAt(m_RowWriteIndex);									// get the row object
		pNew->Clone(pRow);																												// replace the text 
		m_ScrollLines.SetAt(m_RowWriteIndex, pNew);																// write it back to the array
	}
	else{
		if(RowCount >= 500){																											// limit the number of rows
			pNew = (ExTextRow*)m_ScrollLines.GetAt(0);															// get the top row object for re-use
			m_ScrollLines.RemoveAt(0);																							// remove the reference from the array
		}
		else pNew	= new ExTextRow;																								// create a new row object
		pNew->Clone(pRow);																												// copy the text
		m_ScrollLines.Add(pNew);																									// add the new object to the array
		m_ScrollRowOffset = 0;																										// scroll to the bottom
 	}
  LeaveCriticalSection(&ViewUpdateSemaphore);
	m_pMainView->Invalidate(FALSE);
}

//////////////////////////////////////////////////////////////////////////////////////////////

int CS45BasicDoc::NextScrollLine(ExTextRow *pRow)
{
int RowIndex = m_RowWriteIndex + 1;

  SetRowIndex(RowIndex, pRow);
	return pRow->GetLength();
}

//////////////////////////////////////////////////////////////////////////////////////////////

int CS45BasicDoc::GetRowAt(int RowIndex, ExTextRow *pRow)
{
ExTextRow *pCur;

	if((RowIndex < m_ScrollLines.GetCount()) && (RowIndex >= 0)){								// check index is within array
		pCur = (ExTextRow*)m_ScrollLines.GetAt(RowIndex);													// get the row object
		pRow->Clone(pCur, false);																									// replace the text but mark as clean
	}
	else pRow->Clear();																													
	return pRow->GetLength();
}

//////////////////////////////////////////////////////////////////////////////////////////////

int  CS45BasicDoc::SetRowIndex(int RowIndex, ExTextRow *pRow/*=nullptr*/)
{
int RowCount = m_ScrollLines.GetCount();

	if(RowIndex < 500){
		if((RowIndex < RowCount) && (RowIndex >= 0))	m_RowWriteIndex = RowIndex;		// check index is within array
		else{
			while(RowIndex >= m_ScrollLines.GetCount()){
			  ExTextRow *pNew	= new ExTextRow;																				// create a new row object
			  m_ScrollLines.Add(pNew);																								// add the new object to the array
			}
			m_RowWriteIndex = RowCount;																								// else set to max plus 1
		}
	}
	SetRowVisible(m_RowWriteIndex);
	if(pRow != nullptr) return GetRowAt(m_RowWriteIndex, pRow);										// get text if it exists
	return 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////

void  CS45BasicDoc::SetRowVisible(int RowIndex)
{
int RowCount = m_ScrollLines.GetCount();

	if(RowCount > NORMALMODE_SCROLL_SIZE){																											// check index is within array
		int TopLine = RowCount - NORMALMODE_SCROLL_SIZE - m_ScrollRowOffset;											// get visible top row of screen
		if(RowIndex < TopLine) m_ScrollRowOffset =  RowCount - NORMALMODE_SCROLL_SIZE - RowIndex;	// scroll down
		if(RowIndex > TopLine + NORMALMODE_SCROLL_SIZE) m_ScrollRowOffset = RowCount - RowIndex;	// scroll up
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////

void CS45BasicDoc::PrintFormatAt(int Rgn, LPCSTR lpStrFmt, ...)								// print format to region must not contain control characters
{
va_list  argptr;																															// Argument list pointer	    
CString cstr;																																	// formatting string    

  if((m_pMainView == nullptr) || (m_PrintRgn[Rgn].Row[m_DisplayMode] == INVALID_DISPLAY_POSITION)) return;
  EnterCriticalSection(&ViewUpdateSemaphore);																// wait for view update to complete then claim semaphore
  va_start(argptr, lpStrFmt);																									// Initialize va_ functions     
	cstr.FormatV(lpStrFmt, argptr);																							// prints items to the string	   
  va_end(argptr);																															// Close va_ functions		
	m_PrintRgn[Rgn].Text = cstr;
	m_pMainView->Invalidate(FALSE);
  LeaveCriticalSection(&ViewUpdateSemaphore);
}

//////////////////////////////////////////////////////////////////////////////////////////////

void CS45BasicDoc::PrintRgnAt(int Rgn, CString Str)														// print string to region can include control characters
{
  if((m_pMainView == nullptr) || (m_PrintRgn[Rgn].Row[m_DisplayMode] == INVALID_DISPLAY_POSITION)) return;
  EnterCriticalSection(&ViewUpdateSemaphore);																// wait for view update to complete then claim semaphore
	m_PrintRgn[Rgn].Text = Str;
	m_pMainView->Invalidate(FALSE);
  LeaveCriticalSection(&ViewUpdateSemaphore);
}

//////////////////////////////////////////////////////////////////////////////////////////////

void CS45BasicDoc::ClearLine(int Rgn)
{
  if((m_pMainView == nullptr) || (m_PrintRgn[Rgn].Row[m_DisplayMode] == INVALID_DISPLAY_POSITION)) return;
  EnterCriticalSection(&ViewUpdateSemaphore);																// wait for view update to complete then claim semaphore
	m_PrintRgn[Rgn].Text.Delete(0, m_PrintRgn[Rgn].Text.GetLength());
	m_pMainView->Invalidate(FALSE);
  LeaveCriticalSection(&ViewUpdateSemaphore);
}

//////////////////////////////////////////////////////////////////////////////////////////////

void CS45BasicDoc::ClearScrollArea(void)
{
  EnterCriticalSection(&ViewUpdateSemaphore);																// wait for view update to complete then claim semaphore
	DestroyScrollLines();
	m_ScrollRowOffset = 0;
	m_RowWriteIndex = 0;
	m_pMainView->Invalidate(FALSE);
  LeaveCriticalSection(&ViewUpdateSemaphore);
}

//////////////////////////////////////////////////////////////////////////////////////////////

void CS45BasicDoc::ClearAllRgns(void)
{
	for(int i = 0; i < DISP_AREA_COUNT; ++i) ClearLine(i);
}

//////////////////////////////////////////////////////////////////////////////////////////////

void	CS45BasicDoc::SetInsertMode(enum TYPING_MODE_e Mode)
{
	m_TypingMode = Mode;
	m_pMainView->SetCaretType(m_TypingMode == TM_INSERT);
}

//////////////////////////////////////////////////////////////////////////////////////////////

UINT CS45BasicDoc::CopyFromEditBuffer(Message_s *pMsg)
{
	memcpy(pMsg->pBuffer, &m_pKeyEntryBuffer[0], m_KeyLineLength);
	pMsg->pBuffer[m_KeyLineLength] = 0;
	pMsg->Length = m_KeyLineLength;
	return m_KeyLineLength;
}

//////////////////////////////////////////////////////////////////////////////////////////////

UINT CS45BasicDoc::CopyToEditBuffer(char *pBuffer)
{
	char *pPos =	strchr(pBuffer, '\n'); // lose line feeds
	if(pPos != nullptr) *pPos = 0;
	m_KeyLineLength = strlen(pBuffer);
	m_KeyColumn = 0;
	memset(m_pKeyEntryBuffer, 0, ENTRYCOLS);
	memcpy(m_pKeyEntryBuffer, pBuffer, m_KeyLineLength);
	EditBufferPrint();
	m_KeyColumn = m_KeyLineLength;
	m_pMainView->NewCaretPosition(m_KeyColumn);
	return m_KeyLineLength;
}

//////////////////////////////////////////////////////////////////////////////////////////////

void CS45BasicDoc::ClearEditBuffer(void)			/* Clear the edit control of all characters */
{
	memset(m_pKeyEntryBuffer, 0, ENTRYCOLS);
	m_KeyColumn = 0;
	m_KeyLineLength = 0;
	EditBufferPrint();
	SetInsertMode(TM_INSERT);
	m_pMainView->NewCaretPosition(m_KeyColumn);
}

//////////////////////////////////////////////////////////////////////////////////////////////

void CS45BasicDoc::InputPrepend(CString cstr)			
{
CString Temp;

  Temp.Format(_T("%s %s"), cstr, m_UserInputStr.pBuffer);
	memcpy(m_UserInputStr.pBuffer, Temp, Temp.GetLength());
	m_UserInputStr.Length = Temp.GetLength();
	m_UserInputStr.pBuffer[Temp.GetLength()] = 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////

void CS45BasicDoc::EditBufferPrint(void)			/* Print all the characters in the command line from the pCurrent cursor position */
{
	m_PrintRgn[USER_INPUT].Text.Delete(0);
	m_PrintRgn[USER_INPUT].Text.Format(_T("%s"), m_pKeyEntryBuffer);
 	m_pMainView->Invalidate(FALSE);																							// update the view
}

//////////////////////////////////////////////////////////////////////////////////////////////

void CS45BasicDoc::KeyEntry(char nChar)
{
int KeyChar = (int) nChar;

	if(m_GraphicsOn) SetGraphicsMode(false);
	m_ShiftMode	= GetKeyState(VK_SHIFT) & ~1;
	if(m_ShiftMode) KeyChar = tolower(KeyChar);
	else KeyChar = toupper(KeyChar);
	if(m_SystemMode == SYS_EDITKEY)	CheckForTextToken();
	if(m_KeyColumn < ENTRYCOLS){
 		if(m_TypingMode == TM_INSERT){
			if((m_KeyColumn < ENTRYCOLS) && (m_KeyColumn < m_KeyLineLength)){
				for(int i = m_KeyLineLength; i >= m_KeyColumn; i--){
					m_pKeyEntryBuffer[i + 1] = m_pKeyEntryBuffer[i];
				}
				m_KeyLineLength++;
			}
		}
    m_pKeyEntryBuffer[m_KeyColumn++] = ((char)KeyChar & 0x7f);
		if(m_KeyColumn > m_KeyLineLength){
			m_pKeyEntryBuffer[m_KeyColumn] = ' '; // blank caret position
			m_KeyLineLength++;
		}
		m_pKeyEntryBuffer[m_KeyLineLength] = 0;
	}
 //  MyTrace(_T("len:%d, pos:%d\n"), m_KeyLineLength, m_KeyColumn); 
	EditBufferPrint();
  m_pMainView->NewCaretPosition(m_KeyColumn);
}

//////////////////////////////////////////////////////////////////////////////////////////////

void CS45BasicDoc::SpecialKeyEntry(char nChar, bool ShiftState, bool CtrlState)
{

	m_VKCode = nChar;
	m_CtrlKeyState = CtrlState;
	if((m_SystemMode == SYS_RUN) && !pCntx->WaitingInput){
		if(pCntx->WaitingVKey){
			m_EventType = EVENT_KEY_ACTION;
			SetEvent(m_hLoopEvent);																// kick the basic thread
		}
		switch(m_VKCode){
			case VK_LEFT:		ProcessOnKey(22); break;
			case VK_RIGHT:	ProcessOnKey(23); break;
			case VK_UP:			ProcessOnKey(24); break;
			case VK_DOWN:		ProcessOnKey(25); break;
			case VK_PRIOR:  ProcessOnKey(26); break;
			case VK_NEXT:		ProcessOnKey(27); break;
			case VK_HOME:		ProcessOnKey(28); break;
			case VK_END:		ProcessOnKey(30); break;
			case VK_DELETE: ProcessOnKey(31); break;
			case VK_INSERT: ProcessOnKey(32);	break;
			case VK_TAB:		ProcessOnKey(36); break;
			case VK_BACK:		ProcessOnKey(50); break;
			case VK_F1:
			case VK_F2:
			case VK_F3:
			case VK_F4:
			case VK_F5:
			case VK_F6:
			case VK_F7:
			case VK_F8:
			case VK_F9:
			case VK_F10:{
				ProcessSoftKey(m_VKCode);
				break;
			}
		}
	}
	else{
		switch(m_VKCode){
			case VK_F1:
			case VK_F2:
			case VK_F3:
			case VK_F4:
			case VK_F5:
			case VK_F6:
			case VK_F7:
			case VK_F8:
			case VK_F9:
			case VK_F10:{
				ProcessSoftKey(m_VKCode);
				break;
			}
		}
	}
	ClearLine(SYS_COMMENT);
	ProcessEditKey(m_VKCode);
//	MyTrace(_T("len:%d, pos:%d\n"), m_KeyLineLength, m_KeyColumn); 
	EditBufferPrint();
	m_pMainView->NewCaretPosition(m_KeyColumn);
}

//////////////////////////////////////////////////////////////////////////////////////////////

void CS45BasicDoc::DoEnter(bool SaveToHistory /*=false*/)
{
int CommandIndex;
long CommandArg = 0;
char Tokens[ENTRYCOLS];
char *pToken = nullptr, *pNextToken = nullptr;
CString Str;

  if(!CopyFromEditBuffer(&m_UserInputStr)){				// recover entry buffer
		m_EventType = EVENT_NULL_MESSAGE;						  // notify basic thread that user hit return only
		SetEvent(m_hLoopEvent);												// kick the basic thread
		return;		
	}	
	if(SaveToHistory) m_RecallList.Append(&m_UserInputStr);
	ClearLine(SYS_COMMENT);
	memcpy(Tokens, m_UserInputStr.pBuffer, m_KeyLineLength + 1); // make a copy
	pToken = strtok_s(Tokens, _T(" ,;"), &pNextToken); // get a token
	if(pToken == nullptr) return;
	CommandIndex = FindSysCommand(pToken); // see if it's a command
	switch(m_SystemMode){
		case SYS_IDLE:{
			ClearEditBuffer();
			switch(CommandIndex){
				case KC_EDIT:{
					pToken = strtok_s(nullptr, _T(" ,;"), &pNextToken);
					if(pToken == nullptr){
						EditLine(0);
						break;
					}
			 		CommandIndex = FindSysCommand(pToken);								// see if it's a key word
					if(CommandIndex == KC_LINE){ 
						pToken = strtok_s(nullptr, _T(" ,;"), &pNextToken);
						CommandArg = (pToken != nullptr) ? atol(pToken) : 0;
						EditLine(CommandArg);
						break;
					}
					if(CommandIndex == KC_KEY){
						pToken = strtok_s(nullptr, _T(" ,;"), &pNextToken);
						CommandArg = (pToken != nullptr) ? atol(pToken) : 0;
						EditSoftKey(CommandArg);
					}
					break;
				}
				case KC_LIST:{
					pToken = strtok_s(nullptr, _T(" ,;"), &pNextToken);
					if(pToken != nullptr){																
				 		CommandIndex = FindSysCommand(pToken);								// see if it's a key word
						if(CommandIndex == KC_KEY){
							if(!ListSoftKey(CommandArg)) g_pSysPrinter->PrintText(eDispArea::COMMENT, _T("Printer error"));
							break;
						}
					}
					m_EventType = EVENT_NEW_MESSAGE;											// list the programme
					SetEvent(m_hLoopEvent);																
					break;
				}
				case KC_CLEAR:{
					ClearScrollArea();
					break;
				}
				default:{																								// return key defaults to 'execute'
					m_EventType = EVENT_NEW_MESSAGE;
					SetEvent(m_hLoopEvent);																// kick the basic thread
					break;
				}
			}
			break;
		}
		case SYS_RUN:{
			ClearEditBuffer();
			switch(CommandIndex){
				case KC_PAUSE:{
					pCntx->RunMode = eRunMode::HALT;
					break;
				}
				case KC_STOP:{
					pCntx->RunMode = eRunMode::STOP;
					break;
				}
				default:{
					if(!pCntx->WaitingInput){
						bool Resume = false;
						if((pCntx->RunState != eRunState::STOPPED) || (pCntx->RunState != eRunState::PAUSED)){ 											// check to see if program is running
							Resume = true;
						}
						pCntx->RunMode = eRunMode::SUSPEND;
						m_EventType = EVENT_NEW_MESSAGE;
						SetEvent(m_hLoopEvent);																// kick the basic thread
						if(Resume){
							m_EventType = EVENT_NEW_MESSAGE;
							SetEvent(m_hLoopEvent);															// double event
						}
					}
					m_EventType = EVENT_NEW_MESSAGE;
					SetEvent(m_hLoopEvent);																// kick the basic thread
					break;
				}
			}
			break;
		}
		case SYS_PAUSED:{
			ClearEditBuffer();
			switch(CommandIndex){
				case KC_CONT:
				case KC_STEP:{
					m_EventType = EVENT_NEW_MESSAGE;
					SetEvent(m_hLoopEvent);																// kick the basic thread
					break;
				}
				case KC_CLEAR:
					ClearScrollArea();
					break;
				case KC_STOP:{
					pCntx->RunMode = eRunMode::STOP;
//					SetSystemMode(SYS_IDLE);
					break;
				}
				default:{
					m_EventType = EVENT_NEW_MESSAGE;
					SetEvent(m_hLoopEvent);																// kick the basic thread
					break;
				}
			}
			break;
		}
		case SYS_EDITLINE:{
			NextEditLine(true);
			break;
		}
		default:{
			m_EventType = EVENT_NEW_MESSAGE;
			SetEvent(m_hLoopEvent);																// kick the basic thread
			break;
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////

void CS45BasicDoc::DoEscape(void)
{
	switch(m_SystemMode){
		case SYS_IDLE:
			break;
		case SYS_RUN:
		case SYS_PAUSED:
			break;
		case SYS_EDITLINE:
			EditExit();
			break;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////

void CS45BasicDoc::DoRecall(BOOL	IsShift)
{
BString *pStr;

	if((pStr = m_RecallList.Recall(IsShift)) != nullptr) CopyToEditBuffer(pStr->GetBuffer());
	else MessageBeep(MB_ICONERROR); 
}

//////////////////////////////////////////////////////////////////////////////////////////////

void CS45BasicDoc::SetSystemMode(int NewMode)
{
  if(m_SystemMode != NewMode){
		m_SystemMode = NewMode;
		switch(m_SystemMode){
			case SYS_IDLE:
				m_GraphicsOn = false;
			case SYS_PAUSED:
			case SYS_RUN:
				ChangeDisplayMode();
				break;
			case SYS_EDITLINE:
			case SYS_EDITKEY:
				ChangeDisplayMode();
				break;
		}
	}
	m_pWnd->SetControlKeys(m_SystemMode);
}

//////////////////////////////////////////////////////////////////////////////////////////////

bool CS45BasicDoc::SetGraphicsMode(bool Mode)
{
bool EntryMode = m_GraphicsOn;

	if((g_pPlotter->GetType() == eDType::GRAPHICS) && (m_GraphicsOn != Mode)){
		m_GraphicsOn = Mode;
		ChangeDisplayMode();
	}
	return EntryMode;
}

//////////////////////////////////////////////////////////////////////////////////////////////

int CS45BasicDoc::ChangeDisplayMode(void)
{
	switch(m_SystemMode){
		case SYS_IDLE:
		case SYS_PAUSED:
		case SYS_RUN:
			m_DisplayMode = (m_GraphicsOn) ? DISPLAY_GRAPHIC : DISPLAY_NORMAL;
			break;
		case SYS_EDITLINE:
		case SYS_EDITKEY:
			m_DisplayMode = DISPLAY_EDIT;
			ClearAllRgns();
			break;
	}
	ClearEditBuffer();
 	m_pMainView->Invalidate(TRUE);
  return m_DisplayMode;
}

//////////////////////////////////////////////////////////////////////////////////////////////

int CS45BasicDoc::FindSysCommand(char *in)
{
int i = 0, len = strlen(in);

  while(m_SysCommands[i][0] != 0){
  	if((len == strlen(m_SysCommands[i])) && (!_strnicmp(in, m_SysCommands[i], strlen(m_SysCommands[i])))) return i;
  	++i;
  }
	return -1;
}

//////////////////////////////////////////////////////////////////////////////////////////////

void CS45BasicDoc::ProcessEditKey(int VKCode)
{
	if(m_SystemMode == SYS_EDITKEY){
		ProcessEditSoftKey(VKCode, m_CtrlKeyState);
		return;
	}
	switch(VKCode){
		case VK_ESCAPE:{
			DoEscape();
			break;
		}
		case VK_RETURN:{
			DoEnter(true);
			break;
		}
		case VK_CLEAR:{
			ClearEditBuffer();
			break;
		}
		case VK_TAB:
     	if((m_KeyColumn / TABSTOP) < ((ENTRYCOLS - 1) / TABSTOP)){
     		m_KeyColumn += TABSTOP - m_KeyColumn % TABSTOP;
     	}
			break;
		case VK_HOME:       
     	m_KeyColumn = 0;
			break; 
		case VK_END:      
			m_KeyColumn = strlen(m_pKeyEntryBuffer);
			break; 
		case VK_PRIOR:      // Page Up 
  		if(m_DisplayMode == DISPLAY_EDIT)	ScrollEditPage();  // scroll down
			ScrollPage();
			break; 
		case VK_NEXT:       // Page Down 
  		if(m_DisplayMode == DISPLAY_EDIT)	ScrollEditPage(false);  // scroll up
			ScrollPage(false);
			break; 
		case VK_LEFT:       // Left arrow 
			if(m_KeyColumn > 0)	m_KeyColumn--;
			break; 
		case VK_RIGHT:      // Right arrow 
			if(m_KeyColumn < (int)strlen(m_pKeyEntryBuffer)) m_KeyColumn++;
			break; 
		case VK_UP:         // Up arrow 
  		if(m_DisplayMode == DISPLAY_EDIT)	PreviousEditLine();  // scroll down
			ScrollLine();
			break; 
		case VK_DOWN:       // Down arrow 
  		if(m_DisplayMode == DISPLAY_EDIT)	NextEditLine(false);  // scroll up
			ScrollLine(false);
			break; 
		case VK_INSERT:
			SetInsertMode((m_TypingMode == TM_INSERT) ? TM_OVERWRITE : TM_INSERT);
			break;
		case VK_BACK:{
			if(m_KeyColumn > 0){
				for(int i = m_KeyColumn - 1; i < m_KeyLineLength; i++) m_pKeyEntryBuffer[i] = m_pKeyEntryBuffer[i + 1]; 
				m_pKeyEntryBuffer[m_KeyLineLength] = 0; // terminate pCurrent line
				--m_KeyColumn;
				--m_KeyLineLength;
			}
			else MessageBeep(MB_ICONERROR); 
			break;
		}
		case VK_DELETE:{     // Delete. Move all the characters that followed the deleted character (on the same line) one position back. 
			if(m_KeyColumn < m_KeyLineLength){
				for(int i = m_KeyColumn; i < m_KeyLineLength; i++) m_pKeyEntryBuffer[i] = m_pKeyEntryBuffer[i + 1]; 
				m_pKeyEntryBuffer[m_KeyLineLength] = 0; 								// Replace the last character on the line with a space. 
				--m_KeyLineLength;
			}
			else MessageBeep(MB_ICONERROR); 
			break; 		
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////

#include "SourceEdit.cpp"


#include "SoftKeys.cpp"

#include "S45BasicPrint.cpp"


//////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////

void CS45BasicDoc::ProcessControlKey(int Key)
{
	m_pMainView->SetFocus();
  bool ShiftState = m_pMainView->GetShiftState();
	switch(m_SystemMode){
		case SYS_IDLE:
			switch(Key){
	  		case VK_F11:{
	  			DoRecall(ShiftState);
	  			break;
	  		}
				case VK_F12:{			// store
					StoreLine();
					break;
				}
	  		case VK_F13:{				// run
	  			CopyToEditBuffer(_T("RUN"));
	  			DoEnter();
	  			break;
	  		}
				case VK_F14:{			// step
	  			CopyToEditBuffer(_T("STEP"));
	  			DoEnter();
					break;
				}
				case VK_F15:{			// step
	  			CopyToEditBuffer(_T("RESULT"));
	  			DoEnter();
					break;
				}
	  		case VK_F16:{  			// edit
	  			CopyToEditBuffer(_T("EDIT "));
	  			break;
	  		}
	  		case VK_F17:{				// list
	  			CopyToEditBuffer(_T("LIST "));
	  			break;
	  		}
				case VK_F19:{			// clear screen
	  			CopyToEditBuffer(_T("CLS"));
	  			DoEnter();
					break;
				}
	  		case VK_F20:{				// scratch
	  			CopyToEditBuffer(_T("SCRATCH "));
	  			break;
	  		}
			}
			break;
		case SYS_RUN:
			switch(Key){
	  		case VK_F11:{
	  			DoRecall(ShiftState);
	  			break;
	  		}
				case VK_F12:{			// store
					StoreLine();
					break;
				}
				case VK_F13:{			// pause
	  			CopyToEditBuffer(_T("PAUSE"));
	  			DoEnter();
					break;
				}
				case VK_F20:{			// stop
	  			CopyToEditBuffer(_T("STOP"));
	  			DoEnter();
					break;
				}
				default:{
					ProcessOnKey((ShiftState == true) ? Key - 0x66 : Key - 0x70);
					break;
				}
			}
			break;
		case SYS_PAUSED:
			switch(Key){
	  		case VK_F11:{
	  			DoRecall(ShiftState);
	  			break;
	  		}
				case VK_F12:{			// store
					StoreLine();
					break;
				}
				case VK_F13:{			// continue
	  			CopyToEditBuffer(_T("CONT"));
	  			DoEnter();
					break;
				}
				case VK_F14:{			// step
	  			CopyToEditBuffer(_T("STEP"));
	  			DoEnter();
					break;
				}
				case VK_F15:{			
	  			CopyToEditBuffer(_T("RESULT"));
	  			DoEnter();
					break;
				}
				case VK_F19:{			// clear screen
	  			CopyToEditBuffer(_T("CLS"));
	  			DoEnter();
					break;
				}
				case VK_F20:{			// stop
	  			CopyToEditBuffer(_T("STOP"));
	  			DoEnter();
					break;
				}
			}
			break;
		case SYS_EDITKEY:{
			switch(Key){
	  		case VK_F20:
					EditSoftKeyExit(true);
	  			break;
			}
			break;
		}
		case SYS_EDITLINE:{
			switch(Key){
	  		case VK_F11:
	  			DoRecall(ShiftState);
	  			break;
	  		case VK_F12:
	  			DoInsertLine();
	  			break;
	  		case VK_F13:
	  			DoDeleteLine();
	  			break;
	  		case VK_F14:
					SetInsertMode((m_TypingMode == TM_INSERT) ? TM_OVERWRITE : TM_INSERT);
	  			break;
	  		case VK_F20:
					EditExit();
	  			break;
			}
			break;
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////

void CS45BasicDoc::OnControlKey(UINT nID)
{
	ProcessControlKey(VK_F11 + (nID - ID_CONTROL_K1));
}

//////////////////////////////////////////////////////////////////////////////////////////////

void CS45BasicDoc::OnUpdateControlUI(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(TRUE);
}

//////////////////////////////////////////////////////////////////////////////////////////////

void CS45BasicDoc::UpdateViews(CView* pSender, LPARAM lHint/*=0L*/, CObject* pHint/*=nullptr*/)
{
	UpdateAllViews(pSender, lHint, pHint);
}

