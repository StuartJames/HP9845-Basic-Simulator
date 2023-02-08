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

//////////////////////////////////////////////////////////////////////

Program_s *ProgramNew(Program_s *pProgram)
{
	pProgram->Size = FALSE;
	pProgram->Numbered = TRUE;
	pProgram->Capacity = 0;
	pProgram->Runable = FALSE;
	pProgram->Unsaved = FALSE;
	pProgram->ppCode = (struct Token_s**)0;
	pProgram->pScope = nullptr;
	return pProgram;
}

//////////////////////////////////////////////////////////////////////

void ProgramDestroy(Program_s *pProgram)
{
	while(pProgram->Size) TokenDestroy(pProgram->ppCode[--pProgram->Size]);
	if(pProgram->Capacity) free(pProgram->ppCode);
	pProgram->ppCode = (struct Token_s**)0;
	pProgram->pScope = nullptr;
	pProgram->Capacity = 0;
}

//////////////////////////////////////////////////////////////////////

void ProgramNoRun(Program_s *pProgram)
{
	pProgram->Runable = FALSE;
	pProgram->pScope = nullptr;
}

//////////////////////////////////////////////////////////////////////

int ProgramStore(Program_s *pProgram, struct Token_s *pLine, long Pos)
{
int Index = 0, Last = -1;

	assert(pLine->Type == T_INTNUM || pLine->Type == T_UNNUMBERED);
	pProgram->Runable = FALSE;
	pProgram->Unsaved = TRUE;
	if(pLine->Type == T_UNNUMBERED) pProgram->Numbered = FALSE;
	if(Pos){
		for(Index = 0; Index < pProgram->Size; ++Index){
			assert(pProgram->ppCode[Index]->Type == T_INTNUM || pProgram->ppCode[Index]->Type == T_UNNUMBERED);
			if(Pos > Last && Pos < pProgram->ppCode[Index]->Obj.Integer){
				if((pProgram->Size + 1) >= pProgram->Capacity) if((pProgram->ppCode = (Token_s**)realloc(pProgram->ppCode, sizeof(struct Token_s*) * (pProgram->Capacity += 256))) == nullptr) return -1;
				memmove(&pProgram->ppCode[Index + 1], &pProgram->ppCode[Index], (pProgram->Size - Index) * sizeof(struct Token_s*));
				pProgram->ppCode[Index] = pLine;
				++pProgram->Size;
				return Index;
			}
			else if(Pos == pProgram->ppCode[Index]->Obj.Integer){
				TokenDestroy(pProgram->ppCode[Index]);
				pProgram->ppCode[Index] = pLine;
				return Index;
			}
			Last = pProgram->ppCode[Index]->Obj.Integer;
		}
	}
	else{
		Index = pProgram->Size;
	}
	if((++pProgram->Size) >= pProgram->Capacity) if((pProgram->ppCode = (Token_s**)realloc(pProgram->ppCode, sizeof(struct Token_s*) * (pProgram->Capacity += 256))) == nullptr) return -1;
	pProgram->ppCode[Index] = pLine;
	return Index;
}

//////////////////////////////////////////////////////////////////////

int ProgramDelete(Program_s *pProgram, int LineNumber)
{
int i;

	pProgram->Runable = FALSE;
	pProgram->Unsaved = TRUE;
	for(i = 0; i < pProgram->Size; ++i){
		assert(pProgram->ppCode[i]->Type == T_INTNUM);
		if(LineNumber == pProgram->ppCode[i]->Obj.Integer){
			TokenDestroy(pProgram->ppCode[i]);
			if(i + 1 != pProgram->Size) memmove(&pProgram->ppCode[i], &pProgram->ppCode[i + 1], (pProgram->Size - i - 1) * sizeof(struct Token_s*));
			--pProgram->Size;
			return 0;
		}
	}
	return -1;
}

//////////////////////////////////////////////////////////////////////

void ProgramAddScope(Program_s *pProgram, Scope_s *pScope)
{
Scope_s *pPrevScope;

	pPrevScope = pProgram->pScope;
	pProgram->pScope = pScope;
	pScope->pNext = pPrevScope;
}

//////////////////////////////////////////////////////////////////////

long ProgramGetLabelLine(Program_s *pProgram, Label_s *pLabel, PC_s *pPc)
{
	pPc->Index = pLabel->pSym->uType.sLabel.Line.Index;
	pPc->pToken = pProgram->ppCode[pPc->Index] + 1;
	return 	pProgram->ppCode[pPc->Index]->Obj.Integer;
}

//////////////////////////////////////////////////////////////////////

PC_s *ProgramGoLine(Program_s *pProgram, int LineNumber, PC_s *pPc)
{
int i;

	for(i = 0; i < pProgram->Size; ++i){
		if(pProgram->ppCode[i]->Type == T_INTNUM && LineNumber == pProgram->ppCode[i]->Obj.Integer){
			pPc->Index = i;
			pPc->pToken = pProgram->ppCode[i] + 1;
			return pPc;
		}
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////

PC_s *ProgramFromLine(Program_s *pProgram, int LineNumber, PC_s *pPc)
{
int i;

	for(i = 0; i < pProgram->Size; ++i){
		if(pProgram->ppCode[i]->Type == T_INTNUM && pProgram->ppCode[i]->Obj.Integer >= LineNumber){
			pPc->Index = i;
			pPc->pToken = pProgram->ppCode[i] + 1;
			return pPc;
		}
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////

PC_s *ProgramToLine(Program_s *pProgram, int LineNumber, PC_s *pPc)
{
int i;

	for(i = pProgram->Size - 1; i >= 0; --i){
		if(pProgram->ppCode[i]->Type == T_INTNUM && pProgram->ppCode[i]->Obj.Integer <= LineNumber){
			pPc->Index = i;
			pPc->pToken = pProgram->ppCode[i] + 1;
			return pPc;
		}
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////

int ProgramScopeCheck(Program_s *pProgram, PC_s *pc, PC_s *fn)
{
Scope_s *pScope;

	if(fn == nullptr){ /* jump from global block must go to global pc */
		for(pScope = pProgram->pScope; pScope; pScope = pScope->pNext){
			if(pc->Index < pScope->Begin.Index) continue;
			if(pc->Index == pScope->Begin.Index && pc->pToken <= pScope->Begin.pToken) continue;
			if(pc->Index > pScope->End.Index) continue;
			if(pc->Index == pScope->End.Index && pc->pToken > pScope->End.pToken) continue;
			return -1;
		}
	}
	else{ /* jump from local block must go to local block */
		pScope = &(fn->pToken + 1)->Obj.pIdentifier->pSym->uType.sSubrtn.uFunc.sUser.Scope;
		if(pc->Index < pScope->Begin.Index) return -1;
		if(pc->Index == pScope->Begin.Index && pc->pToken <= pScope->Begin.pToken) return -1;
		if(pc->Index > pScope->End.Index) return -1;
		if(pc->Index == pScope->End.Index && pc->pToken>pScope->End.pToken) return -1;
	}
	return 0;
}

//////////////////////////////////////////////////////////////////////

PC_s *ProgramDataLine(Program_s *pProgram, int LineNumber, PC_s *pPc)
{
	if((pPc = ProgramGoLine(pProgram, LineNumber, pPc)) == nullptr) return nullptr;
	while(pPc->pToken->Type != T_DATA){
		if(pPc->pToken->Type == T_EOL) return nullptr;
		else ++pPc->pToken;
	}
	return pPc;
}

//////////////////////////////////////////////////////////////////////

PC_s *ProgramImageLine(Program_s *pProgram, int LineNumber, PC_s *pPc)
{
	if((pPc = ProgramGoLine(pProgram, LineNumber, pPc)) == nullptr) return nullptr;
	while(pPc->pToken->Type != T_IMAGE){
		if(pPc->pToken->Type == T_EOL) return nullptr;
		else ++pPc->pToken;
	}
	++pPc->pToken;
	if(pPc->pToken->Type != T_STRING) return nullptr;
	return pPc;
}

//////////////////////////////////////////////////////////////////////

long ProgramLineNumber(const Program_s *pProgram, const PC_s *pPc)
{
	if(pPc->Index == -1) return 0;
	if(pProgram->Numbered) return (pProgram->ppCode[pPc->Index]->Obj.Integer);
	else return (pPc->Index + 1);
}

//////////////////////////////////////////////////////////////////////

PC_s *ProgramBeginning(Program_s *pProgram, PC_s *pPc)
{
	if(pProgram->Size == 0) return nullptr;
	else{
		pPc->Index = 0;
		pPc->pToken = pProgram->ppCode[0] + 1;
		return pPc;
	}
}

//////////////////////////////////////////////////////////////////////

PC_s *ProgramEnd(Program_s *pProgram, PC_s *pPc)
{
	if(pProgram->Size == 0) return nullptr;
	else{
		pPc->Index = pProgram->Size - 1;
		pPc->pToken = pProgram->ppCode[pProgram->Size - 1];
		while(pPc->pToken->Type != T_EOL) ++pPc->pToken;
		return pPc;
	}
}

//////////////////////////////////////////////////////////////////////

long ProgramNextLine(Program_s *pProgram, PC_s *pPc)
{
	if(pPc->Index + 1 >= pProgram->Size) return 0;
	else{
		pPc->pToken = pProgram->ppCode[++pPc->Index] + 1;
		return pPc->Index;
	}
}

//////////////////////////////////////////////////////////////////////

PC_s *ProgramPreviousLine(Program_s *pProgram, PC_s *pPc)
{
	if(pPc->Index <= 0) return nullptr;
	else{
		pPc->pToken = pProgram->ppCode[--pPc->Index] + 1;
		return pPc;
	}
}

//////////////////////////////////////////////////////////////////////

PC_s *ProgramGetAtIndex(Program_s *pProgram, PC_s *pPc, int Index)
{
	if((Index < 0) || (Index >= pProgram->Size)) return nullptr;
	else{
		pPc->pToken = pProgram->ppCode[Index];
		pPc->Index = Index;
		return pPc;
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////

PC_s *ProgramGetAtLine(Program_s *pProgram, PC_s *pPc, int LineNumber)
{
int i;

	for(i = 0; i < pProgram->Size; ++i){
		if(pProgram->ppCode[i]->Type == T_INTNUM && pProgram->ppCode[i]->Obj.Integer == LineNumber){
			pPc->Index = i;
			pPc->pToken = pProgram->ppCode[i];
			return pPc;
		}
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////

long ProgramGetLineNumber(Program_s *pProgram, int Index)
{
	if((Index < 0) || (Index >= pProgram->Size)) return 0;
	if(pProgram->ppCode[Index]->Type == T_INTNUM) return pProgram->ppCode[Index]->Obj.Integer;
	else return 0;
}

//////////////////////////////////////////////////////////////////////

int ProgramSkipEOL(Program_s *pProgram, PC_s *pPc)									// will return the next statemnt line or the position of the current one
{
	if(pPc->pToken->Type == T_EOL){
		if(pPc->Index == -1 || pPc->Index + 1 == pProgram->Size) return 0;
		pPc->pToken = pProgram->ppCode[++pPc->Index] + 1;
		if(g_Trace.Pause) ProgramTracePause(pProgram, pPc);
		return 1;
	}
	return 1;
}

//////////////////////////////////////////////////////////////////////

void ProgramTraceClear(void)
{
	g_Trace.Mode = eTraceMode::NONE;
	g_Trace.Pause = false;
	g_Trace.Wait = false;
	g_Trace.WaitPeriod = 0;
	g_Trace.PauseLine = 0;
	g_Trace.PauseCount = 0;
	g_Trace.VarList.RemoveAll();
}

//////////////////////////////////////////////////////////////////////

void ProgramTracePause(Program_s *pProgram, PC_s *pPc)
{
	if((g_Trace.PauseLine == 0) || (g_Trace.PauseLine == pProgram->ppCode[pPc->Index]->Obj.Integer)){	 // check for line number reached
		if(g_Trace.PauseCount > 0) --g_Trace.PauseCount;																								 // check for loop count
		else pCntx->RunMode = eRunMode::HALT;
	}
}

//////////////////////////////////////////////////////////////////////

void ProgramTraceFlow(Program_s *pProgram, PC_s *pToPc)
{
	if(g_Trace.Mode != eTraceMode::NONE){
		if(!g_Trace.IsActive && (g_Trace.pLastPc != nullptr))	if((g_Trace.From == 0) || (g_Trace.From == pProgram->ppCode[g_Trace.pLastPc->Index]->Obj.Integer))	g_Trace.IsActive = true;
		if(g_Trace.IsActive){
			assert(g_Trace.pLastPc != nullptr);
			if((g_Trace.Mode == eTraceMode::ALL) || (g_Trace.Mode == eTraceMode::FLOW)){										// trace branch statements
				assert(pToPc != nullptr);
				if(pToPc != nullptr){
					BString bStr;
					bStr.AppendPrintf(_T("TRACE--FROM %ld TO %ld"), pProgram->ppCode[g_Trace.pLastPc->Index]->Obj.Integer, pProgram->ppCode[pToPc->Index]->Obj.Integer);
					g_pPrinter->PrintText(eDispArea::COMMENT, bStr.GetBuffer());
					if(g_Trace.Wait) Sleep((DWORD)g_Trace.WaitPeriod);
				}
			}
			if((g_Trace.To == pProgram->ppCode[g_Trace.pLastPc->Index]->Obj.Integer))	g_Trace.IsActive = false;
		}
	}
}

//////////////////////////////////////////////////////////////////////

void ProgramTraceVar(Program_s *pProgram, pValue_s pVal)
{
	if(g_Trace.Mode != eTraceMode::NONE){
		if(g_Trace.IsActive){
			if(g_Trace.Mode == eTraceMode::VARS){																														// trace specific variables
				assert(g_Trace.pLastPc != nullptr);
				int i = 0, Count = g_Trace.VarList.GetCount();
				while(i < Count){
					StringListNode *pNode = g_Trace.VarList.GetAt(i++);			
					if(!pNode->GetString()->Compare(g_Trace.pIdentifier->Name)){
						BString bStr;
						switch(pVal->Type){
							case eVType::V_INT:{
								bStr.AppendPrintf(_T("TRACE--LINE %ld, %s = %d"), pProgram->ppCode[g_Trace.pLastPc->Index]->Obj.Integer, g_Trace.pIdentifier->Name, pVal->uData.Integer);
								break;
							}
							case eVType::V_REAL:{
								bStr.AppendPrintf(_T("TRACE--LINE %ld, %s = %0.00f"), pProgram->ppCode[g_Trace.pLastPc->Index]->Obj.Integer, g_Trace.pIdentifier->Name, pVal->uData.Real);
								break;
							}
							case eVType::V_STRING:{
								bStr.AppendPrintf(_T("TRACE--LINE %ld, %s = %s"), pProgram->ppCode[g_Trace.pLastPc->Index]->Obj.Integer, g_Trace.pIdentifier->Name, pVal->uData.pString->GetBuffer());
								break;
							}
							case eVType::V_ARRAY:{
								bStr.AppendPrintf(_T("TRACE--LINE %ld, %s CHANGED VALUE"), pProgram->ppCode[g_Trace.pLastPc->Index]->Obj.Integer, g_Trace.pIdentifier->Name);
								break;
							}
						}
						g_pPrinter->PrintText(eDispArea::COMMENT, bStr.GetBuffer());
						if(g_Trace.Wait) Sleep((DWORD)g_Trace.WaitPeriod);
						break;
					}
				}
			}
			if((g_Trace.Mode == eTraceMode::ALL) || (g_Trace.Mode == eTraceMode::ALLVARS)){								 // trace all variables
				assert(g_Trace.pLastPc != nullptr);
				BString bStr;
				switch(pVal->Type){
					case eVType::V_INT:{
						bStr.AppendPrintf(_T("TRACE--LINE %ld, %s = %d"), pProgram->ppCode[g_Trace.pLastPc->Index]->Obj.Integer, g_Trace.pIdentifier->Name, pVal->uData.Integer);
						break;
					}
					case eVType::V_REAL:{
						bStr.AppendPrintf(_T("TRACE--LINE %ld, %s = %0.00f"), pProgram->ppCode[g_Trace.pLastPc->Index]->Obj.Integer, g_Trace.pIdentifier->Name, pVal->uData.Real);
						break;
					}
					case eVType::V_STRING:{
						bStr.AppendPrintf(_T("TRACE--LINE %ld, %s = %s"), pProgram->ppCode[g_Trace.pLastPc->Index]->Obj.Integer, g_Trace.pIdentifier->Name, pVal->uData.pString->GetBuffer());
						break;
					}
					case eVType::V_ARRAY:{
						bStr.AppendPrintf(_T("TRACE--LINE %ld, %s CHANGED VALUE"), pProgram->ppCode[g_Trace.pLastPc->Index]->Obj.Integer, g_Trace.pIdentifier->Name);
						break;
					}
				}
				g_pPrinter->PrintText(eDispArea::COMMENT, bStr.GetBuffer());
				if(g_Trace.Wait) Sleep((DWORD)g_Trace.WaitPeriod);
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////

void ProgramTraceMat(Program_s *pProgram, const char *pStr)
{
	if(g_Trace.Mode != eTraceMode::NONE){
		if(g_Trace.IsActive){
			if(g_Trace.Mode == eTraceMode::VARS){																													// trace specific matricies
				assert(g_Trace.pLastPc != nullptr);
				int i = 0, Count = g_Trace.VarList.GetCount();
				while((i < Count)){
					StringListNode *pNode = g_Trace.VarList.GetAt(i++);			
					if(!pNode->GetString()->Compare(g_Trace.pIdentifier->Name)){
						BString bStr;
						if(pStr == nullptr)	bStr.AppendPrintf(_T("TRACE--LINE %ld, %s(*) CHANGED VALUE"), pProgram->ppCode[g_Trace.pLastPc->Index]->Obj.Integer, g_Trace.pIdentifier->Name);
						else bStr.AppendPrintf(_T("TRACE--LINE %ld, %s(*) = %s"), pProgram->ppCode[g_Trace.pLastPc->Index]->Obj.Integer, g_Trace.pIdentifier->Name, pStr);
						g_pPrinter->PrintText(eDispArea::COMMENT, bStr.GetBuffer());
						if(g_Trace.Wait)	Sleep((DWORD)g_Trace.WaitPeriod);
						break;
					}
				}
			}
			if((g_Trace.Mode == eTraceMode::ALL) || (g_Trace.Mode == eTraceMode::ALLVARS)){								 // trace all matricies
				assert(g_Trace.pLastPc != nullptr);
				BString bStr;
				if(pStr == nullptr)	bStr.AppendPrintf(_T("TRACE--LINE %ld, %s(*) CHANGED VALUE"), pProgram->ppCode[g_Trace.pLastPc->Index]->Obj.Integer, g_Trace.pIdentifier->Name);
				else bStr.AppendPrintf(_T("TRACE--LINE %ld, %s(*) = %s"), pProgram->ppCode[g_Trace.pLastPc->Index]->Obj.Integer, g_Trace.pIdentifier->Name, pStr);
				g_pPrinter->PrintText(eDispArea::COMMENT, bStr.GetBuffer());
				if(g_Trace.Wait)	Sleep((DWORD)g_Trace.WaitPeriod);
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////

void ProgramPCtoError(Program_s *pProgram, PC_s *pPc, Value_s *pValue)
{
BString Str;

	if(pPc->Index >= 0){
		if(pPc->Index < (pProgram->Size - 1) || pPc->pToken->Type != T_EOL){
			if(pCntx->RunState == eRunState::PAUSED){
				Str.AppendPrintf(_T(" %ld "), ProgramLineNumber(pProgram, pPc));
			}
			else{
				Str.AppendPrintf(_T(" at %ld"), ProgramLineNumber(pProgram, pPc));
//				TokenToString(pProgram->ppCode[pPc->Index], nullptr, &Str, nullptr, -1);
//				TokenToString(pProgram->ppCode[pPc->Index], pPc->pToken, &Str, nullptr, -1);
			}
		}
		else Str.AppendPrintf(_T(" at: end of program\n"));
		ValueErrorSuffix(pValue, Str.GetBuffer());
	}
}

//////////////////////////////////////////////////////////////////////

int GetLineNumberWidth(Program_s *pProgram)
{
int i, Width = 0;

	for(i = 0; i < pProgram->Size; ++i){
		if(pProgram->ppCode[i]->Type == T_INTNUM){
			int NewWidth = 1, LineNumber = pProgram->ppCode[i]->Obj.Integer;
			while( LineNumber /= 10) ++NewWidth;
			if(NewWidth > Width) Width = NewWidth;
		}
	}
	return Width;
}

//////////////////////////////////////////////////////////////////////

Value_s *ProgramMerge(Program_s *pDest, Program_s *pSrc, Value_s *pValue)
{
Token_s *pLine;

	if(pSrc->Size > 0){
		for(int i = 0; i < pSrc->Size; ++i){
			pLine = pSrc->ppCode[i];
			ProgramStore(pDest, pLine, pLine->Obj.Integer);
		}
		return nullptr;
	}
	return ValueNewError(pValue, NOPROGRAM);
}

//////////////////////////////////////////////////////////////////////

Value_s *ProgramGet(Program_s *pProgram, int FileIndex, Value_s *pError)
{
BString Str;
int l = 0, Error = 0;
Token_s *pLine;

	while(Str.Clear(), (Error = g_pMassStorage->ReadString(FileIndex, &Str, '\r')) != -1 && Str.GetLength()){
		++l;
		if((l != 1 || Str.GetAt(0) != '#') && (Str.GetLength() > 2)){
			pLine = TokenNewCode(Str.GetBuffer());
			if(pLine->Type == T_INTNUM && pLine->Obj.Integer > 0) ProgramStore(pProgram, pLine, pProgram->Numbered ? pLine->Obj.Integer : 0);
			else{
				if(pLine->Type == T_UNNUMBERED) ProgramStore(pProgram, pLine, 0);
				else{
					TokenDestroy(pLine);
					return ValueNewError(pError, INVALIDLINE, l);
				}
			}	 
		}
	}
	if(Error == DEV_ERROR) return ValueNewError(pError, MASSSTROREERR, g_SufixErrorMsg.GetBuffer());
	return nullptr;
}

//////////////////////////////////////////////////////////////////////

Value_s *ProgramList(Program_s *pProgram, int FileIndex, PC_s *pFrom, PC_s *pTo, Value_s *pValue)
{
int i, Width;
int Indent = 0;
BString Str;

	if(pProgram->Size > 0){
		g_EscapeDisabled = true;
		Width = GetLineNumberWidth(pProgram);
		for(i = 0; i < pProgram->Size; ++i){
			TokenToString(pProgram->ppCode[i], nullptr, &Str, &Indent, Width);
			if((pFrom == nullptr || pFrom->Index >= i) && (pTo == nullptr || pTo->Index <= i)){
				Str.AppendChars("\r\n");
				if(FileIndex < 0){
					if(g_pPrinter->PrintString(&Str) == -1) return ValueNewError(pValue, PRINTERERROR);
				}
				else if(g_pMassStorage->PrintString(FileIndex, &Str) == -1) return ValueNewError(pValue, MASSSTROREERR, g_SufixErrorMsg.GetBuffer());
			}
			Str.Clear();
		}
		g_EscapeDisabled = false;
	}
	else return ValueNewError(pValue, NOPROGRAM);
	if(FileIndex < 0) g_pPrinter->EndPrint();
	return nullptr;
}

//////////////////////////////////////////////////////////////////////

Value_s *ProgramStore(Program_s *pProgram, int FileIndex, PC_s *pFrom, PC_s *pTo, Value_s *pValue)
{
	if(pProgram->Size > 0){
		for(int i = 0; i < pProgram->Size; ++i){
			if((pFrom == nullptr || pFrom->Index >= i) && (pTo == nullptr || pTo->Index <= i)){
				if(g_pMassStorage->PrintTokens(FileIndex, pProgram->ppCode[i]) == -1) return ValueNewError(pValue, MASSSTROREERR, g_SufixErrorMsg.GetBuffer());
			}
		}
	}
	else return ValueNewError(pValue, NOPROGRAM);
	return nullptr;
}

//////////////////////////////////////////////////////////////////////

Value_s *ProgramLoad(Program_s *pProgram, int FileIndex, Value_s *pError)
{
int Error = 0, Index = 0;
Token_s *pToken = nullptr;

	while((Error = g_pMassStorage->ReadTokens(FileIndex, &pToken)) != -1) {
  	if(pToken->Type == T_INTNUM && pToken->Obj.Integer > 0){
			if((++pProgram->Size) >= pProgram->Capacity) if((pProgram->ppCode = (Token_s**)realloc(pProgram->ppCode, sizeof(struct Token_s*) * (pProgram->Capacity += 20))) == nullptr) return ValueNewError(pError, OUTOFMEMORY);
			pProgram->ppCode[Index++] = pToken;
		}
		if(g_pMassStorage->IsEOF(FileIndex)) break;
	}
	if(Error) return ValueNewError(pError, MASSSTROREERR, g_SufixErrorMsg.GetBuffer());
	return nullptr;
}

//////////////////////////////////////////////////////////////////////

BString *GetProgLineNumber(Program_s *pProgram, int Index, BString *pStr)
{
int Width;
int Indent = 0;

	if((Index < 0) || (Index >= pProgram->Size)) return nullptr;
	if(pProgram->ppCode[Index]->Type == T_INTNUM){
		pStr->Clear();
		Width = GetLineNumberWidth(pProgram);
		TokenToString(pProgram->ppCode[Index], nullptr, pStr, &Indent, Width);
		return pStr;
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////

Value_s *ProgramAnalyse(Program_s *pProgram, PC_s *pPc, Value_s *pValue)
{
int i;

	for(i = 0; i < pProgram->Size; ++i){
		pPc->pToken = pProgram->ppCode[i];
		pPc->Index = i;
		if(pPc->pToken->Type == T_INTNUM || pPc->pToken->Type == T_UNNUMBERED) ++pPc->pToken;
		for(;;){
			if(pPc->pToken->Type == T_GOTO || pPc->pToken->Type == T_RESUME || pPc->pToken->Type == T_RETURN || pPc->pToken->Type == T_END || pPc->pToken->Type == T_STOP){
				++pPc->pToken;
				while(pPc->pToken->Type == T_INTNUM){
					++pPc->pToken;
					if(pPc->pToken->Type == T_COMMA) ++pPc->pToken;
					else break;
				}
			}
			if(pPc->pToken->Type == T_EOL) break;
			else ++pPc->pToken;
		}
	}
	return (struct Value_s*)0;
}

//////////////////////////////////////////////////////////////////////

void ProgramRenumber(Program_s *pProgram, int First, int Inc)
{
int i;
struct Token_s *token;

	for(i = 0; i < pProgram->Size; ++i){
		for(token = pProgram->ppCode[i]; token->Type != T_EOL;){
			if(token->Type == T_GOTO || token->Type == T_GOSUB || token->Type == T_RESTORE || token->Type == T_RESUME || token->Type == T_USING){
				++token;
				while(token->Type == T_INTNUM){
					struct PC_s dst;
					if(ProgramGoLine(pProgram, token->Obj.Integer, &dst)) token->Obj.Integer = First + dst.Index * Inc;
					++token;
					if(token->Type == T_COMMA) ++token;
					else break;
				}
			}
			else ++token;
		}
	}
	for(i = 0; i < pProgram->Size; ++i){
		assert(pProgram->ppCode[i]->Type == T_INTNUM || pProgram->ppCode[i]->Type == T_UNNUMBERED);
		pProgram->ppCode[i]->Type = T_INTNUM;
		pProgram->ppCode[i]->Obj.Integer = First + i * Inc;
	}
	pProgram->Numbered = TRUE;
	pProgram->Runable = FALSE;
	pProgram->Unsaved = TRUE;
}

//////////////////////////////////////////////////////////////////////

void ProgramUnnumber(Program_s *pProgram)
{
char *pRef;
int i;
Token_s *pToken;

	pRef = (char*)malloc(pProgram->Size);
	memset(pRef, 0, pProgram->Size);
	for(i = 0; i < pProgram->Size; ++i){
		for(pToken = pProgram->ppCode[i]; pToken->Type != T_EOL; ++pToken){
			if(pToken->Type == T_GOTO || pToken->Type == T_GOSUB || pToken->Type == T_RESTORE || pToken->Type == T_RESUME){
				++pToken;
				while(pToken->Type == T_INTNUM){
					struct PC_s dst;
					if(ProgramGoLine(pProgram, pToken->Obj.Integer, &dst)) pRef[dst.Index] = 1;
					++pToken;
					if(pToken->Type == T_COMMA) ++pToken;
					else break;
				}
			}
		}
	}
	for(i = 0; i < pProgram->Size; ++i){
		assert(pProgram->ppCode[i]->Type == T_INTNUM || pProgram->ppCode[i]->Type == T_UNNUMBERED);
		if(!pRef[i]){
			pProgram->ppCode[i]->Type = T_UNNUMBERED;
			pProgram->Numbered = FALSE;
		}
	}
	free(pRef);
	pProgram->Runable = FALSE;
	pProgram->Unsaved = TRUE;
}

//////////////////////////////////////////////////////////////////////

int ProgramSetName(Program_s *pProgram, const char *pFilename)
{
	pProgram->Name.Clear();
	if(pFilename) return pProgram->Name.AppendChars(pFilename);
	else return 0;
}

//////////////////////////////////////////////////////////////////////

static int CompareName(const void *pA, const void *pB)
{
const register char *pStrA = (const char*)pA, *pStrB = (const char*)pB;

	return strcmp(pStrA, pStrB);
}

//////////////////////////////////////////////////////////////////////

