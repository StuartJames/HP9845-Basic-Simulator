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

#include "BasicDefs.h"
#include "MainFrm.h"
#include "S45BasicDoc.h"
#include "S45BasicDefs.h"
#include <stdio.h>

static char m_BasicCommands[6][20] = {_T("RUN"), _T("CONT"), _T("STEP"), _T("LOAD"), _T("SAVE"), _T("SCRATCH")};

enum RunCommands {
	RC_RUN,
	RC_CONT,
	RC_STEP,
	RC_LOAD,
	RC_SAVE,
	RC_SCRATCH
};

class CMainFrame;

//////////////////////////////////////////////////////////////////////

void*							g_pBasicDoc;
CMainFrame*				g_pWnd;
Context_s					SystemContexts[eCntxType::END];
pContext_s				pCntx;
Common_s					g_CommonVars;												// Common variables
Registry_s*				pBaseLocalRegistry;									// user variable and function identifiers 
Registry_s*				g_pLocalRegistry;										// reference to local user variable and function identifiers in the current context
Registry_s*       g_pGlobalRegistry;									// reference to global identifiers
SubCntx_s*				pMainCntx;												  // Sub-context labels and settings
CPrinter*					g_pPrinter;
CPrinter*					g_pSysPrinter;
CPlotter*					g_pPlotter;
CMassStorage*			g_pMassStorage;
Value_s						g_ResultValue;
Trace_s						g_Trace;
BString						g_SufixErrorMsg;
bool							g_EventPending = false;
bool							g_ControlCodesDisabled = false;
bool							g_EscapeDisabled = false;

void				BasicInit(void);
Value_s 		*CompileProgram(Value_s *pValue, int ClearGlobals);
void 				ProcessTokens(Token_s *pToken);
void				Interpret(void);
Value_s 		*Statements(Value_s *pValue);
Value_s 		*Evaluate(Value_s *pValue, const char *Description);
void				TokenInit(void);
eRunState		ProgramState(eRunMode Mode);
void				OnKeysClear(OnKeys_s	*pKeys);

//////////////////////////////////////////////////////////////////////

inline UINT InDirectMode(void){ return pCntx->Pc.Index == -1; };

inline void	SetContext(eCntxType CntxType){	pCntx = &SystemContexts[(int)CntxType]; };

/////////////////////////////////////////////////////////////////////////////

UINT BasicLoop(LPVOID params)
{
CS45BasicDoc* pDoc = nullptr;
bool EndProc = false;
Message_s *pStr;
int CommandID;
char Tokens[ENTRYCOLS];
char *pToken, *pNextToken = nullptr;

	g_pWnd = (CMainFrame*)params;
  ASSERT(g_pWnd != nullptr);
	pDoc = g_pWnd->GetDocument();
  ASSERT(pDoc != nullptr);
	g_pBasicDoc = static_cast<void*>(pDoc);
	BasicInit();
  while(!EndProc){	// Loop until user requests exit
		WaitForSingleObject(pDoc->m_hLoopEvent, INFINITE);
		ResetEvent(pDoc->m_hLoopEvent);
		switch(pDoc->m_EventType){
			case CS45BasicDoc::EVENT_EXIT:{
				EndProc = true;
				break;
			}
			case CS45BasicDoc::EVENT_NEW_MESSAGE:{
				pStr = pDoc->GetCommandStr();
				memcpy(Tokens, pStr->pBuffer, pStr->Length + 1); // make a copy
				pToken = strtok_s(Tokens, " ,;", &pNextToken); // get a token
				CommandID = FindCommand(pStr->pBuffer);
 				switch(CommandID){
					case RC_RUN:{
						ExecuteRun(pStr->pBuffer);
						break;
					}
					case RC_STEP:
						pCntx->Step = TRUE;
					case RC_CONT:{
						pCntx->RunMode = eRunMode::GO;
						Interpret();
						pCntx->Step = FALSE;
						break;
					}
					default:{
						ExecuteCommand(pStr->pBuffer);
						if(pCntx->RunState == eRunState::SUSPENDED){
						  pCntx->RunMode = eRunMode::GO;
							Interpret();
						}
						break;
					}
				}
			}
			case CS45BasicDoc::EVENT_NULL_MESSAGE:{
				break;
			}
		}
 	}
 	BasicExit();
	SetEvent(pDoc->m_hClosedEvent);
  return TRUE;
}

//////////////////////////////////////////////////////////////////////////

BOOL GetInput(BString *pStr)
{
CS45BasicDoc* pDoc = static_cast<CS45BasicDoc*>(g_pBasicDoc);
Message_s *pMsg;
DWORD Res;

	pCntx->WaitingInput = true;
	while(TRUE){	
		Res = WaitForSingleObject(pDoc->m_hLoopEvent, 50);
		switch(Res){
			case WAIT_OBJECT_0:{
				ResetEvent(pDoc->m_hLoopEvent);
				switch(pDoc->m_EventType){														// shutting down
					case CS45BasicDoc::EVENT_EXIT:{
						pCntx->WaitingInput = false;
						pCntx->RunMode = eRunMode::STOP;
						return FALSE;
					}
					case CS45BasicDoc::EVENT_NEW_MESSAGE:{
						pMsg = pDoc->GetCommandStr();
						pStr->AppendChars(pMsg->pBuffer);
						pCntx->WaitingInput = false;
						return TRUE;
					}
					case CS45BasicDoc::EVENT_NULL_MESSAGE:{
						pCntx->WaitingInput = false;
						return TRUE;
					}
				}
				break;
			}
			default:{
				if((pCntx->RunMode == eRunMode::STOP) || ((pCntx->RunMode == eRunMode::HALT) && !pCntx->Step)){				// poll these flags at regular intervals
					pCntx->WaitingInput = false;
					ProgramState(pCntx->RunMode);												// IsStopped and IsPaused get set here
					return FALSE;
				}
				break;
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////

BOOL PauseLoop()
{
CS45BasicDoc* pDoc = static_cast<CS45BasicDoc*>(g_pBasicDoc);
char Tokens[ENTRYCOLS], *pToken, *pNextToken = nullptr;
Message_s *pMsg;
int CommandID;
DWORD Res;

	pCntx->Step = FALSE;
	pCntx->RunMode = eRunMode::GO;
	while(TRUE){	
		Res = WaitForSingleObject(pDoc->m_hLoopEvent, 50);
		switch(Res){
			case WAIT_OBJECT_0:{
				ResetEvent(pDoc->m_hLoopEvent);
				switch(pDoc->m_EventType){														// shutting down
					case CS45BasicDoc::EVENT_EXIT:{
						pCntx->RunMode = eRunMode::STOP;
						return FALSE;
					}
					case CS45BasicDoc::EVENT_NEW_MESSAGE:{
						pMsg = pDoc->GetCommandStr();
						memcpy(Tokens, pMsg->pBuffer, pMsg->Length + 1);	// make a copy
						pToken = strtok_s(Tokens, " ,;", &pNextToken);		// get a token
						CommandID = FindCommand(pMsg->pBuffer);
 						switch(CommandID){
							case RC_STEP:
								pCntx->Step = TRUE;
								pCntx->RunMode = eRunMode::HALT;
							case RC_CONT:{
								return TRUE;
							}
							default:{
								ExecuteCommand(pMsg->pBuffer);
								break;
							}
						}
					}
					case CS45BasicDoc::EVENT_NULL_MESSAGE:{
						break;
					}
				}
			}
			default:{
				if((g_EventPending) || (pCntx->RunMode == eRunMode::STOP)) return TRUE;												// poll these controls at regular intervals
				break;
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////

#include "Process.cpp"

//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////

void BasicInit(void)
{
	TokenInit();
	BuiltIns();
	pMainCntx =  NewSubCntx();
	g_pGlobalRegistry = NewGlobalRegistry();
	pBaseLocalRegistry = NewIdentifiers();
	g_pLocalRegistry = pBaseLocalRegistry;
	for(int i = (int)eCntxType::RUN; i < (int)eCntxType::END; i++){
		NewStack(&SystemContexts[i].Stack);
		ProgramNew(&SystemContexts[i].Program);
		SystemContexts[i].Step = FALSE;
		SystemContexts[i].RunState = eRunState::STOPPED;
		OnKeysClear(SystemContexts[i].EventKeys);
		SystemContexts[i].Marker.pStack = nullptr;
		SystemContexts[i].OnEventsEnabled = true;
		SystemContexts[i].pSubCntx = pMainCntx;
		SystemContexts[i].pSubCntx->BeginData.Index = -1;
	}
	g_EventPending = false;
	InitialiseHardware();
	SetContext(eCntxType::RUN);												 // set working programme context
	ValueNewInteger(&g_ResultValue, 0);
	ProgramTraceClear();
}

//////////////////////////////////////////////////////////////////////

void BasicExit(void)
{
	DestroyBuiltInFunctions();
	DestroyCommon();
	DeleteSubCntx(pMainCntx);																	// destroy main context and labels
	DeleteGlobals(g_pGlobalRegistry);													// destroy sub-programme identifiers and labels
	DeleteIdentifiers(pBaseLocalRegistry);										// destroy main identifiers & delete container
	for(int i = (int)eCntxType::RUN; i < (int)eCntxType::END; i++){
		DestroyStack(&SystemContexts[i].Stack);
		if(SystemContexts[i].Marker.pStack != nullptr) free(SystemContexts[i].Marker.pStack);
		ProgramDestroy(&SystemContexts[i].Program);
	}
	CloseHardware();
}

//////////////////////////////////////////////////////////////////////

void BasicNew(void)
{
	g_pMassStorage->CloseAll();															// close all FCB
	g_pLocalRegistry = pBaseLocalRegistry;									// get base table
	pCntx->pSubCntx = pMainCntx;														// make sure the programme subcontext is set correctly 
	DeleteGlobals(g_pGlobalRegistry);												// destroy sub-programme identifiers and labels
	DestroyAllIdentifiers(g_pLocalRegistry);								// destroy main context identifiers...
	DestroyLabels(pCntx->pSubCntx->pLabels);								// ...and labels
	CleanStack(&pCntx->Stack);
	g_pGlobalRegistry = NewGlobalRegistry();
	ProgramDestroy(&pCntx->Program);
	ProgramNew(&pCntx->Program);
	pCntx->OnEventsEnabled = true;
	g_EventPending = false;
	OnKeysClear(pCntx->EventKeys);												// clear any soft key assignments
	pCntx->WaitingVKey = false;
	pCntx->WaitingInput = false;
	pCntx->IsAssign = false;
}

//////////////////////////////////////////////////////////////////////

void ExecuteRun(const char *pCommand)										// process in the 'run' context
{
Token_s *pToken;
BString Str;

	SetContext(eCntxType::RUN);														// set the programme context
	pToken = TokenNewCode(pCommand);
	if(pToken->Type != T_EOL){
		if(pToken->Type == T_INTNUM && pToken->Obj.Integer > 0)	ProgramStore(&pCntx->Program, pToken, pCntx->Program.Numbered ? pToken->Obj.Integer : 0);	// save it as a program line
		else{
			if(pToken->Type == T_UNNUMBERED)	ProcessTokens(pToken + 1);
			else g_pPrinter->PrintText(eDispArea::COMMENT, _T("Invalid expression"));
		}
	}
	TokenDestroy(pToken);
}

//////////////////////////////////////////////////////////////////////

void ExecuteCommand(const char *pCommand)								// process in the 'execute' context
{
Token_s *pToken;

	SetContext(eCntxType::EXECUTE);												// set the user context
	pToken = TokenNewCode(pCommand);
	if(pToken->Type == T_UNNUMBERED) ProcessTokens(pToken + 1);		// statments in direct mode
	else{
		TokenDestroy(pToken);																// execute math functions
		BString Str;
		Str.AppendPrintf("execute %s", pCommand);
		pToken = TokenNewCode(Str.GetBuffer());
		ProcessTokens(pToken + 1);
	}
	TokenDestroy(pToken);
	SetContext(eCntxType::RUN);														// set the programme context
}

//////////////////////////////////////////////////////////////////////

int FindCommand(char *pCommand)
{
int i = 0;

  while(m_BasicCommands[i][0] != 0){
  	if(!_strnicmp(pCommand, m_BasicCommands[i], strlen(m_BasicCommands[i]))) return i;
  	++i;
  }
	return -1;
}

//////////////////////////////////////////////////////////////////////////

BOOL GetFilename(char *pIn, char **pOut)
{
	if((*pOut = strchr(pIn, ' ')) != nullptr){
		++(*pOut);
		if(atoi(*pOut) == 0)	return TRUE;
	}
	return FALSE;
}

//////////////////////////////////////////////////////////////////////

static eRunState ProgramState(eRunMode Mode)
{
	pCntx->RunMode = Mode;
	switch(pCntx->RunMode){
		case eRunMode::GO:  pCntx->RunState = eRunState::RUNNING; break;
		case eRunMode::HALT: pCntx->RunState = eRunState::PAUSED; break;
		case eRunMode::STOP: pCntx->RunState = eRunState::STOPPED;	break;
		case eRunMode::SUSPEND: pCntx->RunState = eRunState::SUSPENDED; break;
		default: break;
	}
  g_pWnd->PostMessage(WM_BASICNOTIFY, 0, MAKELONG(CN_STATE_CHANGE, 0));
	return pCntx->RunState;
}


/////////////////////////////////////////////////////////////////////////////

void OnKeysClear(OnKeys_s	*pKeys)
{
	for(int i = 0; i < MAX_SOFT_KEYS; ++i){								 // clear any soft key assignments
		pKeys[i].State = eOnKeyState::INACTIVE;
		pKeys[i].Priority = 1;
		pKeys[i].JumpType = T_NOTOKEN;
	}
}

/////////////////////////////////////////////////////////////////////////////

void ProcessOnKey(int Key)
{
	if((Key >= 0)	&& (Key < MAX_SOFT_KEYS)){
		if(SystemContexts[(int)eCntxType::RUN].EventKeys[Key].State == eOnKeyState::ACTIVE){
			SystemContexts[(int)eCntxType::RUN].EventKeys[Key].State = eOnKeyState::FIRED;
			g_EventPending = true;
		}
	}
}

//////////////////////////////////////////////////////////////////////////

void SetMainTitle(LPCSTR pStr)
{
CS45BasicDoc* pDoc = static_cast<CS45BasicDoc*>(g_pBasicDoc);

 	pDoc->GetMainFrame()->SetWindowTitle(pStr); 
}