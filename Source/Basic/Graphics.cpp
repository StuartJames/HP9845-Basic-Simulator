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
#include <direct.h>
#include <stdlib.h>
#include <stdio.h>

//////////////////////////////////////////////////////////////////////////////////////

static pValue_s GetLocation(pValue_s pValue, CPointDbl *pPos, int *pPen, PC_s EntryPc)
{
CS45BasicDoc* pDoc = static_cast<CS45BasicDoc*>(g_pBasicDoc);
DWORD Res;
bool IsDone = false;

	pCntx->WaitingVKey = true;
	while(!IsDone){
		if(pCntx->OnEventsEnabled && g_EventPending) return nullptr;																	
		Res = WaitForSingleObject(pDoc->m_hLoopEvent, 50);
		switch(Res){
			case WAIT_OBJECT_0:{
				ResetEvent(pDoc->m_hLoopEvent);
				switch(pDoc->m_EventType){																	// shutting down
					case CS45BasicDoc::EVENT_EXIT:{
						pCntx->RunMode = eRunMode::STOP;
						ProgramState(pCntx->RunMode);
						pCntx->WaitingVKey = false;
						return nullptr;
					}
					case CS45BasicDoc::EVENT_KEY_ACTION:{
						*pPos = g_pPlotter->GetCursorPos(pPen);
						switch(pDoc->GetVKCode()){
							case VK_LEFT:		--pPos->x; break;
							case VK_RIGHT:	++pPos->x; break;
							case VK_UP:			++pPos->y; break;
							case VK_DOWN:		--pPos->y; break;
							case VK_HOME:		*pPos = g_pPlotter->GetHome(); break;
							case VK_RETURN:{
								IsDone = true;
								pCntx->WaitingVKey = false;
								break;
							}
						}
						g_pPlotter->SetPointer(*pPos, -1);
					}
				}
				break;
			}
			default:{
				if((pCntx->RunMode == eRunMode::STOP) || ((pCntx->RunMode == eRunMode::HALT) && !pCntx->Step)){				// poll these flags at regular intervals
					ProgramState(pCntx->RunMode);																																				// IsStopped and IsPaused get set here
					if((pCntx->RunMode == eRunMode::STOP) || (pCntx->RunMode == eRunMode::HALT)) pCntx->Pc = EntryPc;		// return to the beging of the statement
					if(pCntx->RunMode == eRunMode::STOP) return ValueNewError(pValue, BREAK);
					pCntx->Stack.Resumeable = 1;																// make it resumable
					pCntx->WaitingVKey = false;
					return ValueNewError(pValue, PAUSE);
				}
				break;
			}
		}
	}
	return nullptr;
}
//////////////////////////////////////////////////////////////////////////////////////

pValue_s stmt_AXES(pValue_s pValue)
{
Axes_Grid_s	Axes;

	Axes.TicSize = 2;				
	Axes.MajorCount = CPointDbl(1, 1);
	++pCntx->Pc.pToken;
	if(Evaluate(pValue, _T("X axes minor tic"))->Type == eVType::V_ERROR || ValueRetype(pValue, eVType::V_REAL)->Type == eVType::V_ERROR) return pValue;
	Axes.TicSpacing.x = pValue->uData.Real;
	ValueDestroy(pValue);
	if(pCntx->Pc.pToken->Type != T_COMMA) return ValueNewError(pValue, MISSINGCOMMA);
	++pCntx->Pc.pToken;
	if(Evaluate(pValue, _T("Y axes minor tic"))->Type == eVType::V_ERROR || ValueRetype(pValue, eVType::V_REAL)->Type == eVType::V_ERROR) return pValue;
	Axes.TicSpacing.y = pValue->uData.Real;
	ValueDestroy(pValue);
	if(pCntx->Pc.pToken->Type == T_COMMA){
		++pCntx->Pc.pToken;
		if(Evaluate(pValue, _T("X axes intersect"))->Type == eVType::V_ERROR || ValueRetype(pValue, eVType::V_REAL)->Type == eVType::V_ERROR) return pValue;
		Axes.Intersect.x = pValue->uData.Real;
		ValueDestroy(pValue);
		if(pCntx->Pc.pToken->Type != T_COMMA) return ValueNewError(pValue, MISSINGCOMMA);
		++pCntx->Pc.pToken;
		if(Evaluate(pValue, _T("Y axes interect"))->Type == eVType::V_ERROR || ValueRetype(pValue, eVType::V_REAL)->Type == eVType::V_ERROR) return pValue;
		Axes.Intersect.y = pValue->uData.Real;
		ValueDestroy(pValue);
		if(pCntx->Pc.pToken->Type == T_COMMA){
			++pCntx->Pc.pToken;
			if(Evaluate(pValue, _T("X axes major tic"))->Type == eVType::V_ERROR || ValueRetype(pValue, eVType::V_REAL)->Type == eVType::V_ERROR) return pValue;
			Axes.MajorCount.x = pValue->uData.Real;
			ValueDestroy(pValue);
			if(pCntx->Pc.pToken->Type != T_COMMA) return ValueNewError(pValue, MISSINGCOMMA);
			++pCntx->Pc.pToken;
			if(Evaluate(pValue, _T("Y axes major tic"))->Type == eVType::V_ERROR || ValueRetype(pValue, eVType::V_REAL)->Type == eVType::V_ERROR) return pValue;
			Axes.MajorCount.y = pValue->uData.Real;
			ValueDestroy(pValue);
			if(pCntx->Pc.pToken->Type == T_COMMA){
				++pCntx->Pc.pToken;
				if(Evaluate(pValue, _T("X axes minor tic size"))->Type == eVType::V_ERROR || ValueRetype(pValue, eVType::V_INT)->Type == eVType::V_ERROR) return pValue;
				Axes.TicSize = pValue->uData.Integer;
				ValueDestroy(pValue);
			}
		}
	}
	if(pCntx->Pass == ePass::INTERPRET){
		g_pPlotter->Axes(Axes);
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////

pValue_s stmt_CLIP(pValue_s pValue)
{
CRectDbl Clip;
PC_s EntryPc = pCntx->Pc;

	++pCntx->Pc.pToken;
	if(pCntx->Pc.pToken->Type == T_EOL){
		int PenStatus;
		CPointDbl Pnt1, Pnt2;
		if(pCntx->Pass == ePass::INTERPRET){
			if(GetLocation(pValue, &Pnt1, &PenStatus, EntryPc) != nullptr) return pValue;
			if(GetLocation(pValue, &Pnt2, &PenStatus, EntryPc) != nullptr) return pValue;
			Clip.SetRect(Pnt1, Pnt2);
			Clip.Normalize();
		}
	}
	else{
		if(Evaluate(pValue, _T("clip left"))->Type == eVType::V_ERROR || ValueRetype(pValue, eVType::V_REAL)->Type == eVType::V_ERROR) return pValue;
		Clip.left = pValue->uData.Real;
		ValueDestroy(pValue);
		if(pCntx->Pc.pToken->Type != T_COMMA) return ValueNewError(pValue, MISSINGCOMMA);
		++pCntx->Pc.pToken;
		if(Evaluate(pValue, _T("clip right"))->Type == eVType::V_ERROR || ValueRetype(pValue, eVType::V_REAL)->Type == eVType::V_ERROR) return pValue;
		Clip.right = pValue->uData.Real;
		ValueDestroy(pValue);
		if(pCntx->Pc.pToken->Type != T_COMMA) return ValueNewError(pValue, MISSINGCOMMA);
		++pCntx->Pc.pToken;
		if(Evaluate(pValue, _T("clip bottom"))->Type == eVType::V_ERROR || ValueRetype(pValue, eVType::V_REAL)->Type == eVType::V_ERROR) return pValue;
		Clip.bottom = pValue->uData.Real;
		ValueDestroy(pValue);
		if(pCntx->Pc.pToken->Type != T_COMMA) return ValueNewError(pValue, MISSINGCOMMA);
		++pCntx->Pc.pToken;
		if(Evaluate(pValue, _T("clip top"))->Type == eVType::V_ERROR || ValueRetype(pValue, eVType::V_REAL)->Type == eVType::V_ERROR) return pValue;
		Clip.top = pValue->uData.Real;
		ValueDestroy(pValue);
	}
	if(pCntx->Pass == ePass::INTERPRET){
		g_pPlotter->Clip(Clip);
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////

pValue_s stmt_COLOUR(pValue_s pValue)
{
int Foreground = -1, Background = -1, BorderColour = -1;
PC_s StatementPc = pCntx->Pc;

	++pCntx->Pc.pToken;
	if(Evaluate(pValue, nullptr)){
		if(pValue->Type == eVType::V_ERROR || (pCntx->Pass != ePass::DECLARE && ValueRetype(pValue, eVType::V_INT)->Type == eVType::V_ERROR)) return pValue;
		Foreground = pValue->uData.Integer;
		if((Foreground < 0) || (Foreground > 15)){
			ValueDestroy(pValue);
			pCntx->Pc = StatementPc;
			return ValueNewError(pValue, OUTOFRANGE, "Colour");
		}
	}
	ValueDestroy(pValue);
	if(pCntx->Pc.pToken->Type == T_COMMA){
		++pCntx->Pc.pToken;
		if(Evaluate(pValue, nullptr)){
			if(pValue->Type == eVType::V_ERROR || (pCntx->Pass != ePass::DECLARE && ValueRetype(pValue, eVType::V_INT)->Type == eVType::V_ERROR)) return pValue;
			Background = pValue->uData.Integer;
			if((Background < 0) || (Background > 15)){
				ValueDestroy(pValue);
				pCntx->Pc = StatementPc;
				return ValueNewError(pValue, OUTOFRANGE, "Colour");
			}
		}
		ValueDestroy(pValue);
		if(pCntx->Pc.pToken->Type == T_COMMA){
			++pCntx->Pc.pToken;
			if(Evaluate(pValue, nullptr)){
				if(pValue->Type == eVType::V_ERROR || (pCntx->Pass != ePass::DECLARE && ValueRetype(pValue, eVType::V_INT)->Type == eVType::V_ERROR)) return pValue;
				BorderColour = pValue->uData.Integer;
				if((BorderColour < 0) || (BorderColour > 15)){
					ValueDestroy(pValue);
					pCntx->Pc = StatementPc;
					return ValueNewError(pValue, OUTOFRANGE, _T("Colour"));
				}
			}
			ValueDestroy(pValue);
		}
	}
	if(pCntx->Pass == ePass::INTERPRET){
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////

pValue_s stmt_CSIZE(pValue_s pValue)
{
float CharSize = DEF_CHAR_SIZE, CharRatio = DEF_CHAR_RATIO;	// set defaults

	++pCntx->Pc.pToken;
	if(Evaluate(pValue, _T("hight"))->Type == eVType::V_ERROR || ValueRetype(pValue, eVType::V_REAL)->Type == eVType::V_ERROR) return pValue;
	CharSize = (float)pValue->uData.Real;
	ValueDestroy(pValue);
	if(pCntx->Pc.pToken->Type == T_COMMA){
		++pCntx->Pc.pToken;
		if(Evaluate(pValue, _T("ratio"))->Type == eVType::V_ERROR || ValueRetype(pValue, eVType::V_REAL)->Type == eVType::V_ERROR) return pValue;
		CharRatio = (float)pValue->uData.Real;
		ValueDestroy(pValue);
	}
	if(pCntx->Pass == ePass::INTERPRET){
		g_pPlotter->CSize(CharSize, CharRatio);
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////

pValue_s stmt_CURSOR(pValue_s pValue)
{
int PenStatus = 0;
CPointDbl CursorPos;
pValue_s pVal;

	++pCntx->Pc.pToken;
	if(pCntx->Pass == ePass::INTERPRET){
		CursorPos = g_pPlotter->GetCursorPos(&PenStatus);
	}
	if(pCntx->Pc.pToken->Type != T_IDENTIFIER) return ValueNewError(pValue, MISSINGVARIDENT);
	if((pCntx->Pass == ePass::DECLARE) && (FindOrRegister(g_pLocalRegistry, pCntx->Pc.pToken->Obj.pIdentifier) == 0))	 return ValueNewError(pValue, UNDECLARED);	
	if((pVal = GetValue(pValue))->Type == eVType::V_ERROR) return pValue;
	if((pCntx->Pass == ePass::COMPILE) && (pVal->Type != eVType::V_REAL) && (pVal->Type != eVType::V_INT)) return ValueNewError(pValue, TYPEMISMATCH5);
	if(pVal->Type == eVType::V_INT) pVal->uData.Integer = (int)CursorPos.x;
	else pVal->uData.Real = CursorPos.x;
	if(pCntx->Pc.pToken->Type != T_COMMA) return ValueNewError(pValue, MISSINGCOMMA);
	++pCntx->Pc.pToken;
	if(pCntx->Pc.pToken->Type != T_IDENTIFIER) return ValueNewError(pValue, MISSINGVARIDENT);
	if((pCntx->Pass == ePass::DECLARE) && (FindOrRegister(g_pLocalRegistry, pCntx->Pc.pToken->Obj.pIdentifier) == 0))	 return ValueNewError(pValue, UNDECLARED);	
	if((pVal = GetValue(pValue))->Type == eVType::V_ERROR) return pValue;
	if((pCntx->Pass == ePass::COMPILE) && (pVal->Type != eVType::V_REAL) && (pVal->Type != eVType::V_INT)) return ValueNewError(pValue, TYPEMISMATCH5);
	if(pVal->Type == eVType::V_INT) pVal->uData.Integer = (int)CursorPos.y;
	else pVal->uData.Real = CursorPos.y;
	if(pCntx->Pc.pToken->Type == T_COMMA){
		++pCntx->Pc.pToken;
		if(pCntx->Pc.pToken->Type != T_IDENTIFIER) return ValueNewError(pValue, MISSINGVARIDENT);
		if((pCntx->Pass == ePass::DECLARE) && (FindOrRegister(g_pLocalRegistry, pCntx->Pc.pToken->Obj.pIdentifier) == 0))	 return ValueNewError(pValue, UNDECLARED);	
		if((pVal = GetValue(pValue))->Type == eVType::V_ERROR) return pValue;
		if((pCntx->Pass == ePass::COMPILE) && (pVal->Type != eVType::V_STRING)) return ValueNewError(pValue, TYPEMISMATCH4);
		if(pCntx->Pass == ePass::INTERPRET) pVal->uData.pString->AppendChar(((PenStatus % 2) ? '0' : '1'));
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////

pValue_s stmt_DIGITIZE(pValue_s pValue)
{
int PenStatus = 0;
CPointDbl CursorPos;
pValue_s pVal;
PC_s EntryPc = pCntx->Pc;

	++pCntx->Pc.pToken;
	if(pCntx->Pass == ePass::INTERPRET){
		if(GetLocation(pValue, &CursorPos, &PenStatus, EntryPc) != nullptr)	return pValue;
	}
	if(pCntx->Pc.pToken->Type != T_IDENTIFIER) return ValueNewError(pValue, MISSINGVARIDENT);
	if((pCntx->Pass == ePass::DECLARE) && (FindOrRegister(g_pLocalRegistry, pCntx->Pc.pToken->Obj.pIdentifier) == 0))	 return ValueNewError(pValue, UNDECLARED);	
	if((pVal = GetValue(pValue))->Type == eVType::V_ERROR) return pValue;
	if((pCntx->Pass == ePass::COMPILE) && (pVal->Type != eVType::V_REAL) && (pVal->Type != eVType::V_INT)) return ValueNewError(pValue, TYPEMISMATCH5);
	if(pVal->Type == eVType::V_INT) pVal->uData.Integer = (int)CursorPos.x;
	else pVal->uData.Real = CursorPos.x;
	if(pCntx->Pc.pToken->Type != T_COMMA) return ValueNewError(pValue, MISSINGCOMMA);
	++pCntx->Pc.pToken;
	if(pCntx->Pc.pToken->Type != T_IDENTIFIER) return ValueNewError(pValue, MISSINGVARIDENT);
	if((pCntx->Pass == ePass::DECLARE) && (FindOrRegister(g_pLocalRegistry, pCntx->Pc.pToken->Obj.pIdentifier) == 0))	 return ValueNewError(pValue, UNDECLARED);	
	if((pVal = GetValue(pValue))->Type == eVType::V_ERROR) return pValue;
	if((pCntx->Pass == ePass::COMPILE) && (pVal->Type != eVType::V_REAL) && (pVal->Type != eVType::V_INT)) return ValueNewError(pValue, TYPEMISMATCH5);
	if(pVal->Type == eVType::V_INT) pVal->uData.Integer = (int)CursorPos.y;
	else pVal->uData.Real = CursorPos.y;
	if(pCntx->Pc.pToken->Type == T_COMMA){
		++pCntx->Pc.pToken;
		if(pCntx->Pc.pToken->Type != T_IDENTIFIER) return ValueNewError(pValue, MISSINGVARIDENT);
		if((pCntx->Pass == ePass::DECLARE) && (FindOrRegister(g_pLocalRegistry, pCntx->Pc.pToken->Obj.pIdentifier) == 0))	 return ValueNewError(pValue, UNDECLARED);	
		if((pVal = GetValue(pValue))->Type == eVType::V_ERROR) return pValue;
		if((pCntx->Pass == ePass::COMPILE) && (pVal->Type != eVType::V_STRING)) return ValueNewError(pValue, TYPEMISMATCH4);
		if(pCntx->Pass == ePass::INTERPRET) pVal->uData.pString->AppendChar(((PenStatus % 2) ? '0' : '1'));
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////

pValue_s stmt_FRAME(pValue_s pValue)
{
	if(pCntx->Pass == ePass::INTERPRET){
		g_pPlotter->Frame();
	}
	++pCntx->Pc.pToken;
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////

pValue_s stmt_GCHARSET(pValue_s pValue)
{
CSze Size;
int Width, Height, xKerning, yKerning;
Var_s *pVar;

	Size = CSze(0, 0);
	++pCntx->Pc.pToken;
	PC_s ImagePc = pCntx->Pc;
	if(Evaluate(pValue, _T("charset variable"))->Type == eVType::V_ERROR) return pValue;
	ValueDestroy(pValue);
	pVar = &ImagePc.pToken->Obj.pIdentifier->pSym->uType.Var;
	if(pCntx->Pc.pToken->Type != T_COMMA) return ValueNewError(pValue, MISSINGCOMMA);
	++pCntx->Pc.pToken;
	if(Evaluate(pValue, _T("X size"))->Type == eVType::V_ERROR || ValueRetype(pValue, eVType::V_INT)->Type == eVType::V_ERROR) return pValue;
	Width = pValue->uData.Integer;
	ValueDestroy(pValue);
	if(pCntx->Pc.pToken->Type != T_COMMA) return ValueNewError(pValue, MISSINGCOMMA);
	++pCntx->Pc.pToken;
	if(Evaluate(pValue, _T("Y size"))->Type == eVType::V_ERROR || ValueRetype(pValue, eVType::V_INT)->Type == eVType::V_ERROR) return pValue;
	Height = pValue->uData.Integer;
	ValueDestroy(pValue);
	if(pCntx->Pc.pToken->Type != T_COMMA) return ValueNewError(pValue, MISSINGCOMMA);
	++pCntx->Pc.pToken;
	if(Evaluate(pValue, _T("X kerning"))->Type == eVType::V_ERROR || ValueRetype(pValue, eVType::V_INT)->Type == eVType::V_ERROR) return pValue;
	xKerning = pValue->uData.Integer;
	ValueDestroy(pValue);
	if(pCntx->Pc.pToken->Type != T_COMMA) return ValueNewError(pValue, MISSINGCOMMA);
	++pCntx->Pc.pToken;
	if(Evaluate(pValue, _T("Y kerning"))->Type == eVType::V_ERROR || ValueRetype(pValue, eVType::V_INT)->Type == eVType::V_ERROR) return pValue;
	yKerning = pValue->uData.Integer;
	ValueDestroy(pValue);
	if(pCntx->Pass == ePass::INTERPRET){
		((CPlotter13*)g_pPlotter)->SetCharset(pVar, Width, Height, xKerning, yKerning);
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////

pValue_s stmt_GCLEAR(pValue_s pValue)
{
	if(pCntx->Pass == ePass::INTERPRET){
		g_pPlotter->GClear();
	}
	++pCntx->Pc.pToken;
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////

pValue_s stmt_GLOAD(pValue_s pValue)
{
	++pCntx->Pc.pToken;
	if(pCntx->Pass == ePass::INTERPRET){
//		g_pPlotter->LOrg(Origin);
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////

pValue_s stmt_GPLOT(pValue_s pValue)
{
BString BitStream;
CPoint Position;
CSze Size;
int Flag;

	Position = CPoint(0, 0);
	Size = CSze(0, 0);
	PC_s ImagePc = pCntx->Pc;
	++pCntx->Pc.pToken;
	if(Evaluate(pValue, _T("bitmap variable"))->Type == eVType::V_ERROR || ValueRetype(pValue, eVType::V_STRING)->Type == eVType::V_ERROR) return pValue;
	BitStream.Clone(pValue->uData.pString);
	ValueDestroy(pValue);
	if(pCntx->Pc.pToken->Type != T_COMMA) return ValueNewError(pValue, MISSINGCOMMA);
	++pCntx->Pc.pToken;
	if(Evaluate(pValue, _T("X position"))->Type == eVType::V_ERROR || ValueRetype(pValue, eVType::V_INT)->Type == eVType::V_ERROR) return pValue;
	Position.x = pValue->uData.Integer;
	ValueDestroy(pValue);
	if(pCntx->Pc.pToken->Type != T_COMMA) return ValueNewError(pValue, MISSINGCOMMA);
	++pCntx->Pc.pToken;
	if(Evaluate(pValue, _T("X size"))->Type == eVType::V_ERROR || ValueRetype(pValue, eVType::V_INT)->Type == eVType::V_ERROR) return pValue;
	Size.sx = pValue->uData.Integer;
	ValueDestroy(pValue);
	if(pCntx->Pc.pToken->Type != T_COMMA) return ValueNewError(pValue, MISSINGCOMMA);
	++pCntx->Pc.pToken;
	if(Evaluate(pValue, _T("Y position"))->Type == eVType::V_ERROR || ValueRetype(pValue, eVType::V_INT)->Type == eVType::V_ERROR) return pValue;
	Position.y = pValue->uData.Integer;
	ValueDestroy(pValue);
	if(pCntx->Pc.pToken->Type != T_COMMA) return ValueNewError(pValue, MISSINGCOMMA);
	++pCntx->Pc.pToken;
	if(Evaluate(pValue, _T("Y size"))->Type == eVType::V_ERROR || ValueRetype(pValue, eVType::V_INT)->Type == eVType::V_ERROR) return pValue;
	Size.sy = pValue->uData.Integer;
	ValueDestroy(pValue);
	if(pCntx->Pc.pToken->Type != T_COMMA) return ValueNewError(pValue, MISSINGCOMMA);
	++pCntx->Pc.pToken;
	if(Evaluate(pValue, _T("Flag bit"))->Type == eVType::V_ERROR || ValueRetype(pValue, eVType::V_INT)->Type == eVType::V_ERROR) return pValue;
	Flag = pValue->uData.Integer;
	ValueDestroy(pValue);
	if((pCntx->Pass == ePass::INTERPRET) && (BitStream.GetLength() > 0)){
		g_pPlotter->GPrint(BitStream.GetBuffer(), Position, Size, Flag, 0);
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////

pValue_s stmt_GPRINT(pValue_s pValue)
{
BString Str;
CPoint Position;
GCharset_s GCharset;

	Position = CPoint(0, 0);
	PC_s ImagePc = pCntx->Pc;
	++pCntx->Pc.pToken;
	if(Evaluate(pValue, _T("X position"))->Type == eVType::V_ERROR || ValueRetype(pValue, eVType::V_INT)->Type == eVType::V_ERROR) return pValue;
	Position.x = pValue->uData.Integer;
	ValueDestroy(pValue);
	if(pCntx->Pc.pToken->Type != T_COMMA) return ValueNewError(pValue, MISSINGCOMMA);
	++pCntx->Pc.pToken;
	if(Evaluate(pValue, _T("Y position"))->Type == eVType::V_ERROR || ValueRetype(pValue, eVType::V_INT)->Type == eVType::V_ERROR) return pValue;
	Position.y = pValue->uData.Integer;
	ValueDestroy(pValue);
	if(pCntx->Pc.pToken->Type != T_COMMA) return ValueNewError(pValue, MISSINGCOMMA);
	++pCntx->Pc.pToken;
	if(Evaluate(pValue, _T("string"))->Type == eVType::V_ERROR || ValueRetype(pValue, eVType::V_STRING)->Type == eVType::V_ERROR) return pValue;
	Str.Clone(pValue->uData.pString);
	ValueDestroy(pValue);
	if(pCntx->Pass == ePass::INTERPRET){
		GCharset = ((CPlotter13*)g_pPlotter)->GetCharset();
		if((GCharset.IsSet == false) || (GCharset.pVar->Dimensions != 1)	|| (GCharset.pVar->Type != eVType::V_STRING))  return ValueNewError(pValue, TYPEMISMATCH4);
		CSze Size(GCharset.Width, GCharset.Rows);
		for(UINT i = 0; i < Str.GetLength(); ++i){
			int c = Str.GetAt(i) - GCharset.pVar->pGeometry[0].Base;
			if(GCharset.pVar->Size > c){
				g_pPlotter->GPrint(GCharset.pVar->pValue[c].uData.pString->GetBuffer(), Position, Size, 2, GCharset.BitOffset); //GCharset.Flag);
			}
			Position.x += GCharset.xKerning;
			Position.y += GCharset.yKerning;
		}
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////

pValue_s stmt_GSTORE(pValue_s pValue)
{
	++pCntx->Pc.pToken;
	if(pCntx->Pass == ePass::INTERPRET){
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////

pValue_s stmt_GRAPHICS(pValue_s pValue)
{
bool IsOn = true;
CS45BasicDoc* pDoc = static_cast<CS45BasicDoc*>(g_pBasicDoc);

	if(pCntx->Pc.pToken->Type == T_EXITGRAPH) IsOn = false;
	++pCntx->Pc.pToken;
	if(pCntx->Pass == ePass::INTERPRET){
		pDoc->SetGraphicsMode(IsOn);
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////

pValue_s stmt_GRID(pValue_s pValue)
{
Axes_Grid_s	Grid;

	Grid.TicSize = 2;
	Grid.MajorCount = CPointDbl(1, 1);
	++pCntx->Pc.pToken;
	if(Evaluate(pValue, _T("X grid minor tic"))->Type == eVType::V_ERROR || ValueRetype(pValue, eVType::V_REAL)->Type == eVType::V_ERROR) return pValue;
	Grid.TicSpacing.x = pValue->uData.Real;
	ValueDestroy(pValue);
	if(pCntx->Pc.pToken->Type != T_COMMA) return ValueNewError(pValue, MISSINGCOMMA);
	++pCntx->Pc.pToken;
	if(Evaluate(pValue, _T("Y grid minor tic"))->Type == eVType::V_ERROR || ValueRetype(pValue, eVType::V_REAL)->Type == eVType::V_ERROR) return pValue;
	Grid.TicSpacing.y = pValue->uData.Real;
	ValueDestroy(pValue);
	if(pCntx->Pc.pToken->Type == T_COMMA){
		++pCntx->Pc.pToken;
		if(Evaluate(pValue, _T("X grid intersect"))->Type == eVType::V_ERROR || ValueRetype(pValue, eVType::V_REAL)->Type == eVType::V_ERROR) return pValue;
		Grid.Intersect.x = pValue->uData.Real;
		ValueDestroy(pValue);
		if(pCntx->Pc.pToken->Type != T_COMMA) return ValueNewError(pValue, MISSINGCOMMA);
		++pCntx->Pc.pToken;
		if(Evaluate(pValue, _T("Y grid interect"))->Type == eVType::V_ERROR || ValueRetype(pValue, eVType::V_REAL)->Type == eVType::V_ERROR) return pValue;
		Grid.Intersect.y = pValue->uData.Real;
		ValueDestroy(pValue);
		if(pCntx->Pc.pToken->Type == T_COMMA){
			++pCntx->Pc.pToken;
			if(Evaluate(pValue, _T("X grid major tic"))->Type == eVType::V_ERROR || ValueRetype(pValue, eVType::V_REAL)->Type == eVType::V_ERROR) return pValue;
			Grid.MajorCount.x = pValue->uData.Real;
			ValueDestroy(pValue);
			if(pCntx->Pc.pToken->Type != T_COMMA) return ValueNewError(pValue, MISSINGCOMMA);
			++pCntx->Pc.pToken;
			if(Evaluate(pValue, _T("Y grid major tic"))->Type == eVType::V_ERROR || ValueRetype(pValue, eVType::V_REAL)->Type == eVType::V_ERROR) return pValue;
			Grid.MajorCount.y = pValue->uData.Real;
			ValueDestroy(pValue);
			if(pCntx->Pc.pToken->Type == T_COMMA){
				++pCntx->Pc.pToken;
				if(Evaluate(pValue, _T("X grid minor tic size"))->Type == eVType::V_ERROR || ValueRetype(pValue, eVType::V_INT)->Type == eVType::V_ERROR) return pValue;
				Grid.TicSize = pValue->uData.Integer;
				ValueDestroy(pValue);
			}
		}
	}
	if(pCntx->Pass == ePass::INTERPRET){
		g_pPlotter->Grid(Grid);
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////

pValue_s stmt_LDIR(pValue_s pValue)
{
double Angle;

	++pCntx->Pc.pToken;
	if(Evaluate(pValue, _T("label dir"))->Type == eVType::V_ERROR || ValueRetype(pValue, eVType::V_REAL)->Type == eVType::V_ERROR) return pValue;
	Angle = pValue->uData.Real;
	ValueDestroy(pValue);
	if(pCntx->Pass == ePass::INTERPRET){
		g_pPlotter->LDir(Angle);
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////

pValue_s stmt_LETTER(pValue_s pValue)
{
	if(pCntx->Pass == ePass::INTERPRET){
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////

pValue_s stmt_LIMIT(pValue_s pValue)
{
CRectDbl Limits;
PC_s EntryPc = pCntx->Pc;

	++pCntx->Pc.pToken;
	if(pCntx->Pc.pToken->Type == T_EOL){
		int PenStatus;
		CPointDbl Pnt1, Pnt2;
		if(pCntx->Pass == ePass::INTERPRET){
			if(GetLocation(pValue, &Pnt1, &PenStatus, EntryPc) != nullptr) return pValue;
			if(GetLocation(pValue, &Pnt2, &PenStatus, EntryPc) != nullptr) return pValue;
			Limits.SetRect(Pnt1, Pnt2);
			Limits.Normalize();
		}
	}
	else{
		if(Evaluate(pValue, _T("clip left"))->Type == eVType::V_ERROR || ValueRetype(pValue, eVType::V_REAL)->Type == eVType::V_ERROR) return pValue;
		Limits.left = pValue->uData.Real;
		ValueDestroy(pValue);
		if(pCntx->Pc.pToken->Type != T_COMMA) return ValueNewError(pValue, MISSINGCOMMA);
		++pCntx->Pc.pToken;
		if(Evaluate(pValue, _T("clip right"))->Type == eVType::V_ERROR || ValueRetype(pValue, eVType::V_REAL)->Type == eVType::V_ERROR) return pValue;
		Limits.right = pValue->uData.Real;
		ValueDestroy(pValue);
		if(pCntx->Pc.pToken->Type != T_COMMA) return ValueNewError(pValue, MISSINGCOMMA);
		++pCntx->Pc.pToken;
		if(Evaluate(pValue, _T("clip top"))->Type == eVType::V_ERROR || ValueRetype(pValue, eVType::V_REAL)->Type == eVType::V_ERROR) return pValue;
		Limits.top = pValue->uData.Real;
		ValueDestroy(pValue);
		if(pCntx->Pc.pToken->Type != T_COMMA) return ValueNewError(pValue, MISSINGCOMMA);
		++pCntx->Pc.pToken;
		if(Evaluate(pValue, _T("clip bottom"))->Type == eVType::V_ERROR || ValueRetype(pValue, eVType::V_REAL)->Type == eVType::V_ERROR) return pValue;
		Limits.bottom = pValue->uData.Real;
		ValueDestroy(pValue);
	}
	if(pCntx->Pass == ePass::INTERPRET){
		if(g_pPlotter->Limit(Limits)) return ValueNewError(pValue, INVALIDLIMITS);
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////

pValue_s stmt_LINETYPE(pValue_s pValue)
{
int Type, Length;

	++pCntx->Pc.pToken;
	if(Evaluate(pValue, _T("linetype id"))->Type == eVType::V_ERROR || ValueRetype(pValue, eVType::V_INT)->Type == eVType::V_ERROR) return pValue;
	Type = pValue->uData.Integer;
	ValueDestroy(pValue);
	if(pCntx->Pc.pToken->Type == T_COMMA){
		++pCntx->Pc.pToken;
		if(Evaluate(pValue, _T("Linetype Length"))->Type == eVType::V_ERROR || ValueRetype(pValue, eVType::V_INT)->Type == eVType::V_ERROR) return pValue;
		Length = pValue->uData.Integer;
		ValueDestroy(pValue);
	}
	else Length = 4;
	if(pCntx->Pass == ePass::INTERPRET){
		g_pPlotter->LineType(Type, Length);
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////

pValue_s stmt_LOCATE(pValue_s pValue)
{
CRectDbl Locate;
PC_s EntryPc = pCntx->Pc;

	++pCntx->Pc.pToken;
	if(pCntx->Pc.pToken->Type == T_EOL){
		int PenStatus;
		CPointDbl Pnt1, Pnt2;
		if(pCntx->Pass == ePass::INTERPRET){
			if(GetLocation(pValue, &Pnt1, &PenStatus, EntryPc) != nullptr) return pValue;
			if(GetLocation(pValue, &Pnt2, &PenStatus, EntryPc) != nullptr) return pValue;
			Locate.SetRect(Pnt1, Pnt2);
			Locate.Normalize();
		}
	}
	else{
		if(Evaluate(pValue, _T("X min"))->Type == eVType::V_ERROR || ValueRetype(pValue, eVType::V_REAL)->Type == eVType::V_ERROR) return pValue;
		Locate.left = pValue->uData.Real;
		ValueDestroy(pValue);
		if(pCntx->Pc.pToken->Type != T_COMMA) return ValueNewError(pValue, MISSINGCOMMA);
		++pCntx->Pc.pToken;
		if(Evaluate(pValue, _T("X max"))->Type == eVType::V_ERROR || ValueRetype(pValue, eVType::V_REAL)->Type == eVType::V_ERROR) return pValue;
		Locate.right = pValue->uData.Real;
		ValueDestroy(pValue);
		if(pCntx->Pc.pToken->Type != T_COMMA) return ValueNewError(pValue, MISSINGCOMMA);
		++pCntx->Pc.pToken;
		if(Evaluate(pValue, _T("Y min"))->Type == eVType::V_ERROR || ValueRetype(pValue, eVType::V_REAL)->Type == eVType::V_ERROR) return pValue;
		Locate.bottom = pValue->uData.Real;
		ValueDestroy(pValue);
		if(pCntx->Pc.pToken->Type != T_COMMA) return ValueNewError(pValue, MISSINGCOMMA);
		++pCntx->Pc.pToken;
		if(Evaluate(pValue, _T("Y max"))->Type == eVType::V_ERROR || ValueRetype(pValue, eVType::V_REAL)->Type == eVType::V_ERROR) return pValue;
		Locate.top = pValue->uData.Real;
		ValueDestroy(pValue);
	}
	if(pCntx->Pass == ePass::INTERPRET){
		g_pPlotter->Locate(Locate);
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////

pValue_s stmt_LORG(pValue_s pValue)
{
int Origin;

	++pCntx->Pc.pToken;
	if(Evaluate(pValue, _T("label origin"))->Type == eVType::V_ERROR || ValueRetype(pValue, eVType::V_INT)->Type == eVType::V_ERROR) return pValue;
	Origin = pValue->uData.Integer;
	ValueDestroy(pValue);
	if(pCntx->Pass == ePass::INTERPRET){
		g_pPlotter->LOrg(Origin);
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////

pValue_s stmt_MOVE(pValue_s pValue)
{
CPointDbl To;

	++pCntx->Pc.pToken;
	if(Evaluate(pValue, _T("X coordinate"))->Type == eVType::V_ERROR || ValueRetype(pValue, eVType::V_REAL)->Type == eVType::V_ERROR) return pValue;
	To.x = pValue->uData.Real;
	ValueDestroy(pValue);
	if(pCntx->Pc.pToken->Type != T_COMMA) return ValueNewError(pValue, MISSINGCOMMA);
	++pCntx->Pc.pToken;
	if(Evaluate(pValue, _T("Y coordinate"))->Type == eVType::V_ERROR || ValueRetype(pValue, eVType::V_REAL)->Type == eVType::V_ERROR) return pValue;
	To.y = pValue->uData.Real;
	ValueDestroy(pValue);
	if(pCntx->Pass == ePass::INTERPRET){
		g_pPlotter->Move(To);
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////

pValue_s stmt_PDIR(pValue_s pValue)
{
double Angle;

	++pCntx->Pc.pToken;
	if(Evaluate(pValue, _T("angle"))->Type == eVType::V_ERROR || ValueRetype(pValue, eVType::V_REAL)->Type == eVType::V_ERROR) return pValue;
	Angle = pValue->uData.Real;
	ValueDestroy(pValue);
	if(pCntx->Pass == ePass::INTERPRET){
		g_pPlotter->PDir(Angle);
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////

pValue_s stmt_PEN(pValue_s pValue)
{
int Number;

	++pCntx->Pc.pToken;
  if((Evaluate(pValue, _T("pen"))->Type == eVType::V_ERROR) || (ValueRetype(pValue, eVType::V_INT)->Type == eVType::V_ERROR)) return pValue;
	Number = pValue->uData.Integer;
	ValueDestroy(pValue);
	if(pCntx->Pass == ePass::INTERPRET){
		g_pPlotter->Pen(Number);
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////

pValue_s stmt_PENUP(pValue_s pValue)
{
	++pCntx->Pc.pToken;
	if(pCntx->Pass == ePass::INTERPRET){
		g_pPlotter->PenUp();
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////

pValue_s stmt_PLOT(pValue_s pValue)
{
int PenCtrl = 1, PlotType = (int)pCntx->Pc.pToken->Type;
CPointDbl To;

	++pCntx->Pc.pToken;
	if(Evaluate(pValue, _T("X coordinate"))->Type == eVType::V_ERROR || ValueRetype(pValue, eVType::V_REAL)->Type == eVType::V_ERROR) return pValue;
	To.x = pValue->uData.Real;
	ValueDestroy(pValue);
	if(pCntx->Pc.pToken->Type != T_COMMA) return ValueNewError(pValue, MISSINGCOMMA);
	++pCntx->Pc.pToken;
	if(Evaluate(pValue, _T("Y coordinate"))->Type == eVType::V_ERROR || ValueRetype(pValue, eVType::V_REAL)->Type == eVType::V_ERROR) return pValue;
	To.y = pValue->uData.Real;
	ValueDestroy(pValue);
	if(pCntx->Pc.pToken->Type == T_COMMA){
		++pCntx->Pc.pToken;
		if(Evaluate(pValue, _T("pen control"))->Type == eVType::V_ERROR || ValueRetype(pValue, eVType::V_INT)->Type == eVType::V_ERROR) return pValue;
		PenCtrl = pValue->uData.Integer;
		ValueDestroy(pValue);
	}
	if(pCntx->Pass == ePass::INTERPRET){
		g_pPlotter->Plot(To, PlotType, PenCtrl);
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////

pValue_s stmt_PLOTTERIS(pValue_s pValue)
{
int Device = 13, Address = 0;

	++pCntx->Pc.pToken;
  if(Evaluate(pValue, _T("device or name"))->Type == eVType::V_ERROR) return pValue;				// get device number or type discriptor
	if(pValue->Type != eVType::V_STRING){
		if(ValueRetype(pValue, eVType::V_INT)->Type == eVType::V_ERROR) return pValue;					// got a device number
 		Device = pValue->uData.Integer;
		ValueDestroy(pValue);
		if(pCntx->Pc.pToken->Type == T_COMMA){																									// must have an address or descritor
			++pCntx->Pc.pToken;
			if((Evaluate(pValue, _T("address or name"))->Type == eVType::V_ERROR)) return pValue;	// get the device address or type descritor
			if(pValue->Type != eVType::V_STRING){																									
				if((ValueRetype(pValue, eVType::V_INT)->Type == eVType::V_ERROR)) return pValue;		// got an address specifier
				Address = pValue->uData.Integer;
				ValueDestroy(pValue);												
				if(pCntx->Pc.pToken->Type == T_COMMA){																							// must have a type discritor
					++pCntx->Pc.pToken;
					if(Evaluate(pValue, _T("name"))->Type == eVType::V_ERROR) return pValue;
				}
				else return ValueNewError(pValue, MISSINGVARIDENT);
			}
		}
		else return ValueNewError(pValue, MISSINGVARIDENT);
	}
	if(ValueRetype(pValue, eVType::V_STRING)->Type == eVType::V_ERROR) return pValue;
 	ValueDestroy(pValue);																																			// the device descritor is not used
	if(pCntx->Pass == ePass::INTERPRET && SetPlotterIs(Device, Address) == -1) return ValueNewError(pValue, PLOTTERERROR);
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////

pValue_s stmt_POINTER(pValue_s pValue)
{
int CursorType = 2;
CPointDbl To;

	++pCntx->Pc.pToken;
	if(Evaluate(pValue, _T("X coordinate"))->Type == eVType::V_ERROR || ValueRetype(pValue, eVType::V_REAL)->Type == eVType::V_ERROR) return pValue;
	To.x = pValue->uData.Real;
	ValueDestroy(pValue);
	if(pCntx->Pc.pToken->Type != T_COMMA) return ValueNewError(pValue, MISSINGCOMMA);
	++pCntx->Pc.pToken;
	if(Evaluate(pValue, _T("Y coordinate"))->Type == eVType::V_ERROR || ValueRetype(pValue, eVType::V_REAL)->Type == eVType::V_ERROR) return pValue;
	To.y = pValue->uData.Real;
	ValueDestroy(pValue);
	if(pCntx->Pc.pToken->Type == T_COMMA){
		++pCntx->Pc.pToken;
		if(Evaluate(pValue, _T("cursor type"))->Type == eVType::V_ERROR || ValueRetype(pValue, eVType::V_INT)->Type == eVType::V_ERROR) return pValue;
		CursorType = pValue->uData.Integer;
		ValueDestroy(pValue);
	}
	if(pCntx->Pass == ePass::INTERPRET){
		g_pPlotter->SetPointer(To, CursorType);
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////

pValue_s stmt_RATIO(pValue_s pValue)
{
	if(pCntx->Pass == ePass::INTERPRET){
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////

pValue_s stmt_SCALE(pValue_s pValue)
{
CRectDbl Scale;

	++pCntx->Pc.pToken;
	if(Evaluate(pValue, _T("X min"))->Type == eVType::V_ERROR || ValueRetype(pValue, eVType::V_REAL)->Type == eVType::V_ERROR) return pValue;
	Scale.left = pValue->uData.Real;
	ValueDestroy(pValue);
	if(pCntx->Pc.pToken->Type != T_COMMA) return ValueNewError(pValue, MISSINGCOMMA);
	++pCntx->Pc.pToken;
	if(Evaluate(pValue, _T("X max"))->Type == eVType::V_ERROR || ValueRetype(pValue, eVType::V_REAL)->Type == eVType::V_ERROR) return pValue;
	Scale.right = pValue->uData.Real;
	ValueDestroy(pValue);
	if(pCntx->Pc.pToken->Type != T_COMMA) return ValueNewError(pValue, MISSINGCOMMA);
	++pCntx->Pc.pToken;
	if(Evaluate(pValue, _T("Y min"))->Type == eVType::V_ERROR || ValueRetype(pValue, eVType::V_REAL)->Type == eVType::V_ERROR) return pValue;
	Scale.bottom = pValue->uData.Real;
	ValueDestroy(pValue);
	if(pCntx->Pc.pToken->Type != T_COMMA) return ValueNewError(pValue, MISSINGCOMMA);
	++pCntx->Pc.pToken;
	if(Evaluate(pValue, _T("Y max"))->Type == eVType::V_ERROR || ValueRetype(pValue, eVType::V_REAL)->Type == eVType::V_ERROR) return pValue;
	Scale.top = pValue->uData.Real;
	ValueDestroy(pValue);
	if(pCntx->Pass == ePass::INTERPRET){
		g_pPlotter->Scale(Scale);
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////

pValue_s stmt_SHOW(pValue_s pValue)
{
CRectDbl Show;

	++pCntx->Pc.pToken;
	if(Evaluate(pValue, _T("X min"))->Type == eVType::V_ERROR || ValueRetype(pValue, eVType::V_REAL)->Type == eVType::V_ERROR) return pValue;
	Show.left = pValue->uData.Real;
	ValueDestroy(pValue);
	if(pCntx->Pc.pToken->Type != T_COMMA) return ValueNewError(pValue, MISSINGCOMMA);
	++pCntx->Pc.pToken;
	if(Evaluate(pValue, _T("X max"))->Type == eVType::V_ERROR || ValueRetype(pValue, eVType::V_REAL)->Type == eVType::V_ERROR) return pValue;
	Show.right = pValue->uData.Real;
	ValueDestroy(pValue);
	if(pCntx->Pc.pToken->Type != T_COMMA) return ValueNewError(pValue, MISSINGCOMMA);
	++pCntx->Pc.pToken;
	if(Evaluate(pValue, _T("Y min"))->Type == eVType::V_ERROR || ValueRetype(pValue, eVType::V_REAL)->Type == eVType::V_ERROR) return pValue;
	Show.bottom = pValue->uData.Real;
	ValueDestroy(pValue);
	if(pCntx->Pc.pToken->Type != T_COMMA) return ValueNewError(pValue, MISSINGCOMMA);
	++pCntx->Pc.pToken;
	if(Evaluate(pValue, _T("Y max"))->Type == eVType::V_ERROR || ValueRetype(pValue, eVType::V_REAL)->Type == eVType::V_ERROR) return pValue;
	Show.top = pValue->uData.Real;
	ValueDestroy(pValue);
	if(pCntx->Pass == ePass::INTERPRET){
		g_pPlotter->Show(Show);
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////

pValue_s stmt_WHERE(pValue_s pValue)
{
int PenStatus = 0;
CPointDbl PenPos;
pValue_s pVal;

	++pCntx->Pc.pToken;
	if(pCntx->Pass == ePass::INTERPRET){
		PenPos = g_pPlotter->GetPenPos(&PenStatus);
	}
	if(pCntx->Pc.pToken->Type != T_IDENTIFIER) return ValueNewError(pValue, MISSINGVARIDENT);
	if((pCntx->Pass == ePass::DECLARE) && (FindOrRegister(g_pLocalRegistry, pCntx->Pc.pToken->Obj.pIdentifier) == 0))	 return ValueNewError(pValue, UNDECLARED);	
	if((pVal = GetValue(pValue))->Type == eVType::V_ERROR) return pValue;
	if((pCntx->Pass == ePass::COMPILE) && (pVal->Type != eVType::V_REAL) && (pVal->Type != eVType::V_INT)) return ValueNewError(pValue, TYPEMISMATCH5);
	if(pVal->Type == eVType::V_INT) pVal->uData.Integer = (int)PenPos.x;
	else pVal->uData.Real = PenPos.x;
	if(pCntx->Pc.pToken->Type != T_COMMA) return ValueNewError(pValue, MISSINGCOMMA);
	++pCntx->Pc.pToken;
	if(pCntx->Pc.pToken->Type != T_IDENTIFIER) return ValueNewError(pValue, MISSINGVARIDENT);
	if((pCntx->Pass == ePass::DECLARE) && (FindOrRegister(g_pLocalRegistry, pCntx->Pc.pToken->Obj.pIdentifier) == 0))	 return ValueNewError(pValue, UNDECLARED);	
	if((pVal = GetValue(pValue))->Type == eVType::V_ERROR) return pValue;
	if((pCntx->Pass == ePass::COMPILE) && (pVal->Type != eVType::V_REAL) && (pVal->Type != eVType::V_INT)) return ValueNewError(pValue, TYPEMISMATCH5);
	if(pVal->Type == eVType::V_INT) pVal->uData.Integer = (int)PenPos.y;
	else pVal->uData.Real = PenPos.y;
	if(pCntx->Pc.pToken->Type == T_COMMA){
		++pCntx->Pc.pToken;
		if(pCntx->Pc.pToken->Type != T_IDENTIFIER) return ValueNewError(pValue, MISSINGVARIDENT);
		if((pCntx->Pass == ePass::DECLARE) && (FindOrRegister(g_pLocalRegistry, pCntx->Pc.pToken->Obj.pIdentifier) == 0))	 return ValueNewError(pValue, UNDECLARED);	
		if((pVal = GetValue(pValue))->Type == eVType::V_ERROR) return pValue;
		if((pCntx->Pass == ePass::COMPILE) && (pVal->Type != eVType::V_STRING)) return ValueNewError(pValue, TYPEMISMATCH4);
		if(pCntx->Pass == ePass::INTERPRET) pVal->uData.pString->AppendChar(((PenStatus % 2) ? '0' : '1'));
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////

pValue_s stmt_UNCLIP(pValue_s pValue)
{
	++pCntx->Pc.pToken;
	if(pCntx->Pass == ePass::INTERPRET)	g_pPlotter->Unclip();
	return nullptr;
}
