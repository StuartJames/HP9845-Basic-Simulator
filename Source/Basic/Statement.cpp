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

pValue_s stmt_ASSIGN(pValue_s pValue)
{
long File;
Value_s	*pRetVal = nullptr, NameVal;
PC_s ErrorPc;
int Res;
bool DoClose = false;

	++pCntx->Pc.pToken;
	if(pCntx->Pc.pToken->Type == T_CHANNEL){
		++pCntx->Pc.pToken;
		ErrorPc = pCntx->Pc;
		if(Evaluate(pValue, _T("file #"))->Type == eVType::V_ERROR || ValueRetype(pValue, eVType::V_INT)->Type == eVType::V_ERROR){
			pCntx->Pc = ErrorPc;
			return pValue;
		}
		File = pValue->uData.Integer;
		ValueDestroy(pValue);
		if((pCntx->Pass == ePass::INTERPRET) && !g_pMassStorage->IsValid(File)) return ValueNewError(pValue, INVALIDFILENUM);
		if(pCntx->Pc.pToken->Type != T_TO) return ValueNewError(pValue, MISSINGTO);
		++pCntx->Pc.pToken;
		if(pCntx->Pc.pToken->Type == T_MULT) DoClose = true;								// 'star' indicates to close file
		else{
			if((Evaluate(pValue, _T("file name"))->Type == eVType::V_ERROR) || (ValueRetype(pValue, eVType::V_STRING)->Type == eVType::V_ERROR)) return pValue;
			if((pCntx->Pass == ePass::INTERPRET) && !pValue->uData.pString->IsValidName()) return ValueNewError(pValue, INVALIDFILENAME);
			NameVal = *pValue;
			if(pCntx->Pc.pToken->Type == T_COMMA){
				++pCntx->Pc.pToken;
				if(pCntx->Pc.pToken->Type != T_IDENTIFIER) return ValueNewError(pValue, MISSINGVARIDENT);
				if(pCntx->Pass == ePass::DECLARE && RegisterVariable(g_pLocalRegistry, pCntx->Pc.pToken->Obj.pIdentifier, eVType::V_INT, eSymType::LOCALVAR, 0) == 0) return ValueNewError(pValue, REDECLARATION);
				ErrorPc = pCntx->Pc;
				if(((pRetVal = GetValue(pValue))->Type) == eVType::V_ERROR) return pValue;
			}
		}
	}
	if(pCntx->Pass == ePass::INTERPRET){
		ErrorPc = pCntx->Pc;
		if(DoClose){
			if(g_pMassStorage->Close(pValue->uData.Integer) == -1){
				pCntx->Pc = ErrorPc;
				ValueDestroy(pValue);
				return ValueNewError(pValue, MASSSTROREERR, g_SufixErrorMsg.GetBuffer());
			}
		}
		else{
			if((Res = g_pMassStorage->Assign(File, NameVal.uData.pString->GetBuffer(), true)) == -1){	  /* open file with name pValue */
				if(pRetVal == nullptr){
					pCntx->Pc = ErrorPc;
					ValueDestroy(pValue);
					return ValueNewError(pValue, MASSSTROREERR, g_SufixErrorMsg.GetBuffer());
				}
				pRetVal->uData.Integer = Res;
			}
		}
	}
	if(!DoClose) ValueDestroy(&NameVal);
	return nullptr;
}

///////////////////////////////////////////////////////////////////////////////////////////////

pValue_s stmt_BEEP(pValue_s pValue)
{
	++pCntx->Pc.pToken;
	if(pCntx->Pass == ePass::INTERPRET)	Beep(DEF_BEEP_FREQU, DEF_BEEP_LENGTH);
	return NULL;
}

//////////////////////////////////////////////////////////////////////////////////////

pValue_s stmt_CALL(pValue_s pValue)
{
	++pCntx->Pc.pToken;
	if(pCntx->Pc.pToken->Type != T_IDENTIFIER) return ValueNewError(pValue, MISSINGPROCIDENT);
	if(pCntx->Pass == ePass::DECLARE){
		if(SubProgram(pValue)->Type == eVType::V_ERROR) return pValue;
		else ValueDestroy(pValue);
	}
	else{
		if(pCntx->Pass == ePass::COMPILE)	if(GlobalFind(pCntx->Pc.pToken->Obj.pIdentifier, eSymType::SUBPROGRAMME) == 0) return ValueNewError(pValue, UNDECLARED);
		SubProgram(pValue);
		if(ValueRetype(pValue, eVType::V_VOID)->Type == eVType::V_ERROR) return pValue;
		ValueDestroy(pValue);
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////

pValue_s stmt_CASE(pValue_s pValue)
{
PC_s CasePc = pCntx->Pc, *pSelectCase, *pNextCaseValue;

	if(pCntx->Pass == ePass::DECLARE || pCntx->Pass == ePass::COMPILE){
		if((pSelectCase = FindMarker(eMType::SELECT)) == nullptr) return ValueNewError(pValue, STRAYCASE);
		for(pNextCaseValue = &pSelectCase->pToken->Obj.pSelectCase->NextCaseValue; pNextCaseValue->Index != -1; pNextCaseValue = &pNextCaseValue->pToken->Obj.pCaseValue->NextCaseValue);
		*pNextCaseValue = pCntx->Pc;
		if(pCntx->Pass == ePass::COMPILE) pCntx->Pc.pToken->Obj.pCaseValue->EndSelect = pSelectCase->pToken->Obj.pSelectCase->EndSelect;
		pCntx->Pc.pToken->Obj.pCaseValue->NextCaseValue.Index = -1;
		++pCntx->Pc.pToken;
		switch(CasePc.pToken->Type){
			case T_CASEELSE:
				break;
			case T_CASE:{
				do{
					switch(pCntx->Pc.pToken->Type){
						case T_LT:
						case T_LE:
						case T_EQ:
						case T_GE:
						case T_GT:
						case T_NE:{
							++pCntx->Pc.pToken;
							CasePc = pCntx->Pc;
							if(Evaluate(pValue, _T("`is'"))->Type == eVType::V_ERROR) return pValue;
							if(ValueRetype(pValue, pSelectCase->pToken->Obj.pSelectCase->ValueType)->Type == eVType::V_ERROR){
								pCntx->Pc = CasePc;
								return pValue;
							}
							ValueDestroy(pValue);
							break;
						}
						default:{ /* pValue or range */
							CasePc = pCntx->Pc;
							if(Evaluate(pValue, _T("`case'"))->Type == eVType::V_ERROR) return pValue;
							if(ValueRetype(pValue, pSelectCase->pToken->Obj.pSelectCase->ValueType)->Type == eVType::V_ERROR){
								pCntx->Pc = CasePc;
								return pValue;
							}
							ValueDestroy(pValue);
							if(pCntx->Pc.pToken->Type == T_TO){
								++pCntx->Pc.pToken;
								CasePc = pCntx->Pc;
								if(Evaluate(pValue, _T("`case'"))->Type == eVType::V_ERROR) return pValue;
								if(ValueRetype(pValue, pSelectCase->pToken->Obj.pSelectCase->ValueType)->Type == eVType::V_ERROR){
									pCntx->Pc = CasePc;
									return pValue;
								}
								ValueDestroy(pValue);
							}
							break;
						}
					}
					if(pCntx->Pc.pToken->Type == T_COMMA) ++pCntx->Pc.pToken;
					else break;
				}
				while(1);
				break;
			}
			default:
				assert(0);
		}
	}
	else pCntx->Pc = pCntx->Pc.pToken->Obj.pCaseValue->EndSelect;
	return nullptr;
}

///////////////////////////////////////////////////////////////////////////////////////////////

pValue_s stmt_CAT(pValue_s pValue)
{
TokenType_e CatType = pCntx->Pc.pToken->Type;

	if(pCntx->Pass == ePass::INTERPRET){
		if(g_pMassStorage->Cat()) return ValueNewError(pValue, MASSSTROREERR, _T("FindFirstFile"));
	}
	++pCntx->Pc.pToken;
	return NULL;
}

//////////////////////////////////////////////////////////////////////////////////////

pValue_s stmt_CHECK_READ(pValue_s pValue)
{
bool CheckAll = false;
long File;
PC_s ErrorPc = pCntx->Pc;

	++pCntx->Pc.pToken;
	ErrorPc = pCntx->Pc;
	if(pCntx->Pc.pToken->Type == T_CHANNEL){
		++pCntx->Pc.pToken;
		ErrorPc = pCntx->Pc;
		if(Evaluate(pValue, _T("file #"))->Type == eVType::V_ERROR || ValueRetype(pValue, eVType::V_INT)->Type == eVType::V_ERROR){
			pCntx->Pc = ErrorPc;
			return pValue;
		}
		File = pValue->uData.Integer;
		ValueDestroy(pValue);
		if((pCntx->Pass == ePass::INTERPRET) && (File < 0)) return ValueNewError(pValue, INVALIDFILENUM);
		if(pCntx->Pc.pToken->Type != T_TO) return ValueNewError(pValue, MISSINGTO);
		++pCntx->Pc.pToken;
	}
	else CheckAll = true;
	if(pCntx->Pass == ePass::INTERPRET){
		if(CheckAll) g_pMassStorage->VerifyAll();
		else{
			if(g_pMassStorage->SetVerify(File)){
				pCntx->Pc = ErrorPc;
				return ValueNewError(pValue, MASSSTROREERR, g_SufixErrorMsg.GetBuffer());
			}
		}
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////

pValue_s stmt_CLEAR(pValue_s pValue)
{
	if(pCntx->Pass == ePass::INTERPRET){
		ResetVariables(g_pLocalRegistry);
		g_pMassStorage->CloseAll();
	}
	++pCntx->Pc.pToken;
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////

pValue_s stmt_CLOSE(pValue_s pValue)
{
bool HasArgs = false;

	++pCntx->Pc.pToken;
	while(1){
		PC_s ErrorPc = pCntx->Pc;
		if(pCntx->Pc.pToken->Type == T_CHANNEL){
			HasArgs = true;
			++pCntx->Pc.pToken; 
		}
		if(Evaluate(pValue, nullptr) == nullptr){
			if(HasArgs) return ValueNewError(pValue, MISSINGEXPR, _T("file #"));
			else break;
		}
		if(pValue->Type == eVType::V_ERROR || ValueRetype(pValue, eVType::V_INT)->Type == eVType::V_ERROR) return pValue;
		if(pCntx->Pass == ePass::INTERPRET && g_pMassStorage->Close(pValue->uData.Integer) == -1){
			ValueDestroy(pValue);
			pCntx->Pc = ErrorPc;
			return ValueNewError(pValue, MASSSTROREERR, g_SufixErrorMsg.GetBuffer());
		}
		if(pCntx->Pc.pToken->Type == T_COMMA) ++pCntx->Pc.pToken;
		else break;
	}
	if(!HasArgs && pCntx->Pass == ePass::INTERPRET) g_pMassStorage->CloseAll();
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////

pValue_s stmt_CLS(pValue_s pValue)
{
	++pCntx->Pc.pToken;
	if(pCntx->Pass == ePass::INTERPRET){
		g_pWnd->PostMessage(WM_BASICNOTIFY, 0, MAKELONG(CN_CLEAR_SCREEN, 0));
		g_ControlCodesDisabled = false;
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////

pValue_s stmt_COM(pValue_s pValue)
{
PC_s ErrorPc = pCntx->Pc;

	++pCntx->Pc.pToken;
	while(1){
		if(pCntx->Pc.pToken->Type != T_IDENTIFIER) return ValueNewError(pValue, MISSINGARRIDENT);
		if(((pCntx->Pc.pToken + 1)->Type != T_OP) && ((pCntx->Pc.pToken + 1)->Type != T_OSB)){				 // simple type
			if(pCntx->Pass == ePass::DECLARE) CommonVariable(pCntx->Pc.pToken->Obj.pIdentifier, pCntx->Pc.pToken->Obj.pIdentifier->DefType, eSymType::COMNVAR);
			++pCntx->Pc.pToken;
			if(pCntx->Pc.pToken->Type == T_COMMA) ++pCntx->Pc.pToken;  /* advance to next var */
			else break;
		}
		else{
			int Dim = 0, StrLen = 0;
			int BoundLow, BoundHigh;
			Geometry_s Geometry[DEF_MAX_DIMENSIONS];
			if(pCntx->Pass == ePass::DECLARE) CommonVariable(pCntx->Pc.pToken->Obj.pIdentifier, pCntx->Pc.pToken->Obj.pIdentifier->DefType, eSymType::COMNARRAY);
			Var_s *pVar = &pCntx->Pc.pToken->Obj.pIdentifier->pSym->uType.Var;
			eVType VarType = pVar->Type;
			++pCntx->Pc.pToken;
			if(pCntx->Pc.pToken->Type == T_OP){
				++pCntx->Pc.pToken;
				while(Dim < DEF_MAX_DIMENSIONS){
					ErrorPc = pCntx->Pc;
					BoundLow = pCntx->pSubCntx->OptionBase;
					BoundHigh = 0;
					if(Evaluate(pValue, _T("dimension"))->Type == eVType::V_ERROR || (pCntx->Pass != ePass::DECLARE && ValueRetype(pValue, eVType::V_INT)->Type == eVType::V_ERROR)){
						return pValue;
					}
					if(pCntx->Pass == ePass::INTERPRET){
					  if(((pValue->uData.Integer != -1) || (!pVar->IsDefined)) && (pValue->uData.Integer < 1)){												// the dimension must be greater than 1
							ValueDestroy(pValue);
							pCntx->Pc = ErrorPc;
							return ValueNewError(pValue, OUTOFRANGE, _T("Dimension"));
						}
					}
					BoundHigh = pValue->uData.Integer;
					ValueDestroy(pValue);
					if(pCntx->Pc.pToken->Type == T_COLON){																																				// Get the upper bounds
						++pCntx->Pc.pToken;
						if(Evaluate(pValue, _T("bounds"))->Type == eVType::V_ERROR || (pCntx->Pass != ePass::DECLARE && ValueRetype(pValue, eVType::V_INT)->Type == eVType::V_ERROR)){
							return pValue;
						}
						if(pCntx->Pass == ePass::INTERPRET && pValue->uData.Integer <= BoundHigh){																				// must be grater than the lower dimension				
							ValueDestroy(pValue);
							pCntx->Pc = ErrorPc;
							return ValueNewError(pValue, OUTOFRANGE, _T("Subscript"));
						}
						BoundLow = BoundHigh;
						BoundHigh = pValue->uData.Integer;
						ValueDestroy(pValue);
					}
					if(pCntx->Pass == ePass::INTERPRET){
						if(pVar->IsDefined){
							if((pValue->uData.Integer != -1) && ((BoundHigh - BoundLow + 1) != pVar->pGeometry[Dim].Size)) return ValueNewError(pValue, COMREDIM);		// check subscript for wildcard and equality
						}
						else{
							Geometry[Dim].Size = BoundHigh - BoundLow + 1;																		 
							Geometry[Dim].Base = BoundLow;
						}
						++Dim;
					}
					if(pCntx->Pc.pToken->Type == T_COMMA) ++pCntx->Pc.pToken;
					else break;
				}
				if(pCntx->Pc.pToken->Type != T_CP){						// abort 
					return ValueNewError(pValue, MISSINGCP);
				}
				++pCntx->Pc.pToken;
 			}
			if(pCntx->Pc.pToken->Type == T_OSB){
				++pCntx->Pc.pToken;
				ErrorPc = pCntx->Pc;
				if(Evaluate(pValue, _T("size"))->Type == eVType::V_ERROR || (pCntx->Pass != ePass::DECLARE && ValueRetype(pValue, eVType::V_INT)->Type == eVType::V_ERROR)){
					return pValue;
				}
				if(pCntx->Pass == ePass::INTERPRET && pValue->uData.Integer < 1){									// check for error 
					ValueDestroy(pValue);
					pCntx->Pc = ErrorPc;
					return ValueNewError(pValue, OUTOFRANGE, _T("Size"));
				}
 				if(pCntx->Pass == ePass::INTERPRET){
					StrLen = pValue->uData.Integer;
				}
				ValueDestroy(pValue);
				if(pCntx->Pc.pToken->Type != T_CSB){
					return ValueNewError(pValue, MISSINGOSB);
				}
				++pCntx->Pc.pToken;
			}
			if((pCntx->Pass == ePass::INTERPRET) && !pVar->IsDefined){
				Var_s NewArray;
				if(VarNew(&NewArray, VarType, Dim, Geometry) == nullptr){
					return ValueNewError(pValue, OUTOFMEMORY);
				}
				if((VarType == eVType::V_STRING) && StrLen) for(Dim = 0; Dim < NewArray.Size; ++Dim) NewArray.pValue[Dim].uData.pString->SetMaxSize(StrLen);	// set maximum string size (was definition of string size)
				VarDestroy(pVar);
				*pVar = NewArray;
				pVar->IsDefined = true;
			}
			if(pCntx->Pc.pToken->Type == T_COMMA) ++pCntx->Pc.pToken;															// advance to next var 
			else if(pCntx->Pc.pToken->Type != T_OSB) break;
		}
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////

pValue_s stmt_CREATE(pValue_s pValue)
{
Value_s	NameVal;
int RecordCount = 4, RecordLength = 256;
PC_s ErrorPc = pCntx->Pc;

	++pCntx->Pc.pToken;
	ErrorPc = pCntx->Pc;
	if((Evaluate(pValue, _T("file name"))->Type == eVType::V_ERROR) || (ValueRetype(pValue, eVType::V_STRING)->Type == eVType::V_ERROR)) return pValue;
	NameVal = *pValue;
	if(pCntx->Pc.pToken->Type == T_COMMA){
		++pCntx->Pc.pToken;
		if(Evaluate(pValue, _T("record count"))->Type == eVType::V_ERROR || ValueRetype(pValue, eVType::V_INT)->Type == eVType::V_ERROR) return pValue;
		RecordCount = pValue->uData.Integer;
		ValueDestroy(pValue);
		if(pCntx->Pc.pToken->Type != T_COMMA){
			++pCntx->Pc.pToken;
			if(Evaluate(pValue, _T("record length"))->Type == eVType::V_ERROR || ValueRetype(pValue, eVType::V_INT)->Type == eVType::V_ERROR) return pValue;
			RecordLength = pValue->uData.Integer;
			ValueDestroy(pValue);
		}
	}
	if(pCntx->Pass == ePass::INTERPRET){
		if(g_pMassStorage->Create(NameVal.uData.pString->GetBuffer(), RecordCount, RecordLength) == -1){
			pCntx->Pc = ErrorPc;
			ValueDestroy(pValue);
			return ValueNewError(pValue, MASSSTROREERR, g_SufixErrorMsg.GetBuffer());
		}
	}
	ValueDestroy(&NameVal);
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////

pValue_s stmt_DATA(pValue_s pValue)
{
	if(InDirectMode()) return ValueNewError(pValue, NOTINDIRECTMODE);
	if(pCntx->Pass == ePass::DECLARE){
		*pCntx->pDataListEnd = pCntx->Pc;
		(pCntx->pDataListEnd = &(pCntx->Pc.pToken->Obj.NextDataPc))->Index = -1;							 // add line to data linked list
	}
	++pCntx->Pc.pToken;
	while(1){																														 // walk through the data items
		if(pCntx->Pc.pToken->Type != T_STRING && pCntx->Pc.pToken->Type != T_DATAINPUT) return ValueNewError(pValue, MISSINGDATAINPUT);
		++pCntx->Pc.pToken;
		if(pCntx->Pc.pToken->Type != T_COMMA) break;
		else ++pCntx->Pc.pToken;
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////

pValue_s stmt_DEFAULT(pValue_s pValue)
{
	++pCntx->Pc.pToken;
	if(pCntx->Pc.pToken->Type != T_DEFAULT_OFF){ // TO DO
	}
	if(pCntx->Pc.pToken->Type != T_DEFAULT_ON){	 // TO DO
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////

pValue_s stmt_DEFFN(pValue_s pValue)
{
PC_s ErrorPc = pCntx->Pc;

	if(pCntx->Pass == ePass::DECLARE || pCntx->Pass == ePass::COMPILE){
		Identifier_s *pFn;
		int Args = 0, Res;
		if(InDirectMode()) return ValueNewError(pValue, NOTINDIRECTMODE);
		++pCntx->Pc.pToken;
		if(pCntx->Pc.pToken->Type != T_IDENTIFIER) return ValueNewError(pValue, MISSINGFUNCIDENT);
		pFn = pCntx->Pc.pToken->Obj.pIdentifier;
		++pCntx->Pc.pToken;
		if(FindMarker(eMType::FUNC)){
			pCntx->Pc = ErrorPc;
			return ValueNewError(pValue, NESTEDDEFINITION);
		}
		StackVariable(&pCntx->Stack, pFn);
		if(pCntx->Pc.pToken->Type == T_OP){																														// if parentheses...
			if(GetFormalArgs(pValue, &Args) != nullptr) return pValue;																	// ...must specify at least 1 formal parameter
		}
		if(pCntx->Pass == ePass::DECLARE){
			enum eVType *pValType = (Args) ? (eVType*)malloc(Args * sizeof(enum eVType)) : nullptr;
			int i;
			for(i = 0; i < Args; ++i) pValType[i] = StackArgType(&pCntx->Stack, i);
			if(pCntx->Pc.pToken->Type == T_EQ) Res = RegisterLocalFunc(g_pLocalRegistry, pFn, pFn->DefType, &pCntx->Pc, &ErrorPc, Args, pValType);
			else Res = RegisterGlobalFunc(g_pGlobalRegistry, pFn, pFn->DefType, &pCntx->Pc, &ErrorPc, Args, pValType);
			if(Res == 0){
				free(pValType);
				StackFuncEnd(&pCntx->Stack);
				pCntx->Pc = ErrorPc;
				return ValueNewError(pValue, REDECLARATION);
			}
			ProgramAddScope(&pCntx->Program, &pFn->pSym->uType.sSubrtn.uFunc.sUser.Scope);
		}
		PushMarker(eMType::FUNC, &ErrorPc);
		if(pCntx->Pc.pToken->Type == T_EQ) return stmt_EQ_FNEND(pValue);															// single-line functions body defined here
	}																																																// multi-line functions defined through multiple statements followed by a FNEND.
	else pCntx->Pc = (pCntx->Pc.pToken + 1)->Obj.pIdentifier->pSym->uType.sSubrtn.uFunc.sUser.Scope.End;
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////

pValue_s stmt_DEFINT_DEFDBL_DEFSTR(pValue_s pValue)
{
enum eVType DestType = eVType::V_NIL;

	switch(pCntx->Pc.pToken->Type){
		case T_DEFINT: DestType = eVType::V_INT; break;
		case T_DEFDBL: DestType = eVType::V_REAL; break;
		case T_DEFSTR: DestType = eVType::V_STRING; break;
		default: assert(0);
	}
	++pCntx->Pc.pToken;
	while(1){
		Identifier_s *pIdent;
		if(pCntx->Pc.pToken->Type != T_IDENTIFIER) return ValueNewError(pValue, MISSINGVARIDENT);
		if(pCntx->Pc.pToken->Obj.pIdentifier->DefType != eVType::V_REAL){
			switch(DestType){
				case eVType::V_INT: return ValueNewError(pValue, BADIDENTIFIER, _T("integer"));
				case eVType::V_REAL: return ValueNewError(pValue, BADIDENTIFIER, _T("real"));
				case eVType::V_STRING: return ValueNewError(pValue, BADIDENTIFIER, _T("string"));
				default: assert(0);
			}
		}
		pIdent = pCntx->Pc.pToken->Obj.pIdentifier;
		++pCntx->Pc.pToken;
		if(pCntx->Pc.pToken->Type == T_MINUS){
			Identifier_s i;
			if(strlen(pIdent->Name) != 1) return ValueNewError(pValue, BADRANGE);
			++pCntx->Pc.pToken;
			if(pCntx->Pc.pToken->Type != T_IDENTIFIER) return ValueNewError(pValue, MISSINGVARIDENT);
			if(strlen(pCntx->Pc.pToken->Obj.pIdentifier->Name) != 1) return ValueNewError(pValue, BADRANGE);
			for(i.Name[0] = tolower((int)pIdent->Name[0]), i.Name[1] = '\0'; i.Name[0] <= tolower((int)pCntx->Pc.pToken->Obj.pIdentifier->Name[0]); ++i.Name[0]){
				RegisterVariable(g_pLocalRegistry, &i, DestType, eSymType::LOCALVAR, 1);
			}
			++pCntx->Pc.pToken;
		}
		else RegisterVariable(g_pLocalRegistry, pIdent, DestType, eSymType::LOCALVAR, 1);
		if(pCntx->Pc.pToken->Type == T_COMMA) ++pCntx->Pc.pToken;
		else break;
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////

pValue_s stmt_DELETE(pValue_s pValue)
{
	if(pCntx->Pass == ePass::INTERPRET && !InDirectMode()) return ValueNewError(pValue, NOTINPROGRAMMODE);
	++pCntx->Pc.pToken;
	if(pCntx->Pc.pToken->Type == T_INTNUM){
		if(pCntx->Pass == ePass::INTERPRET){
			if(!pCntx->Program.Numbered) return ValueNewError(pValue, UNNUMBERED);
			else if(pCntx->Pc.pToken->Obj.Integer<=0) return ValueNewError(pValue, OUTOFRANGE, _T("Line number"));
			else if(ProgramDelete(&pCntx->Program, pCntx->Pc.pToken->Obj.Integer) == -1) return ValueNewError(pValue, NOSUCHLINE);
		}
		++pCntx->Pc.pToken;
	}
	return nullptr;
}

///////////////////////////////////////////////////////////////////////////////////////////////

pValue_s stmt_DELAY(pValue_s pValue)
{
	++pCntx->Pc.pToken;
	if(pCntx->Pass == ePass::INTERPRET){
	}
	return NULL;
}

//////////////////////////////////////////////////////////////////////////////////////

pValue_s stmt_DIM(pValue_s pValue)
{
PC_s ErrorPc = pCntx->Pc;

	++pCntx->Pc.pToken;
	while(1){
		int Dim = 0, StrLen = 0;
		int BoundLow, BoundHigh;
		bool IsDefined = false;
		Geometry_s *pGeometry = nullptr;
		Geometry_s Geometry[DEF_MAX_DIMENSIONS];
		if(pCntx->Pc.pToken->Type != T_IDENTIFIER) return ValueNewError(pValue, MISSINGARRIDENT);
		if(pCntx->Pass == ePass::DECLARE){
			if((pCntx->Pc.pToken->Obj.pIdentifier->DefType != eVType::V_STRING) && ((pCntx->Pc.pToken + 1)->Type != T_OP)){
			  if(RegisterVariable(g_pLocalRegistry, pCntx->Pc.pToken->Obj.pIdentifier, pCntx->Pc.pToken->Obj.pIdentifier->DefType, eSymType::LOCALVAR, 0) == 0)	return ValueNewError(pValue, REDECLARATION);
			}
			else if(RegisterVariable(g_pLocalRegistry, pCntx->Pc.pToken->Obj.pIdentifier, pCntx->Pc.pToken->Obj.pIdentifier->DefType, eSymType::LOCALARRAY, 0) == 0)	return ValueNewError(pValue, REDECLARATION);
		}
		Var_s *pVar = &pCntx->Pc.pToken->Obj.pIdentifier->pSym->uType.Var;
		eVType VarType = pVar->Type;
		++pCntx->Pc.pToken;
		if(pCntx->Pc.pToken->Type == T_OP){
			++pCntx->Pc.pToken;
			while(Dim < DEF_MAX_DIMENSIONS){
				ErrorPc = pCntx->Pc;
				BoundLow = pCntx->pSubCntx->OptionBase;
				BoundHigh = 0;
				if(Evaluate(pValue, _T("dimension"))->Type == eVType::V_ERROR || (pCntx->Pass != ePass::DECLARE && ValueRetype(pValue, eVType::V_INT)->Type == eVType::V_ERROR)){
					return pValue;
				}
				if(pCntx->Pass == ePass::INTERPRET){
					if(((pValue->uData.Integer != -1) || (!pVar->IsDefined)) && (pValue->uData.Integer < 1)){												// the dimension must be greater than 1
						ValueDestroy(pValue);
						ValueNewError(pValue, OUTOFRANGE, _T("Dimension"));
					}
				}
 				if(pValue->Type == eVType::V_ERROR){																																							
					pCntx->Pc = ErrorPc;
					return pValue;
				}
				BoundHigh = pValue->uData.Integer;
				ValueDestroy(pValue);
				if(pCntx->Pc.pToken->Type == T_COLON){																																				// Get the upper bounds
					++pCntx->Pc.pToken;
					if(Evaluate(pValue, _T("subscript"))->Type == eVType::V_ERROR || (pCntx->Pass != ePass::DECLARE && ValueRetype(pValue, eVType::V_INT)->Type == eVType::V_ERROR)){
						return pValue;
					}
					if(pCntx->Pass == ePass::INTERPRET && pValue->uData.Integer <= BoundHigh){																				// must be grater than the lower dimension				
						ValueDestroy(pValue);
						ValueNewError(pValue, OUTOFRANGE, _T("Subscript"));
					}
 					if(pValue->Type == eVType::V_ERROR){																																							
						pCntx->Pc = ErrorPc;
						return pValue;
					}
					BoundLow = BoundHigh;
					BoundHigh = pValue->uData.Integer;
					ValueDestroy(pValue);
				}
				if(pCntx->Pass == ePass::INTERPRET){
					if(IsDefined){
						if((pValue->uData.Integer != -1) && ((BoundHigh - BoundLow + 1) != pVar->pGeometry[Dim].Size)) return ValueNewError(pValue, REDIM);		// check subscript for wildcard and equality
					}
					else{
						Geometry[Dim].Size = BoundHigh - BoundLow + 1;
						Geometry[Dim].Base = BoundLow;
					}
					++Dim;
				}
				if(pCntx->Pc.pToken->Type == T_COMMA) ++pCntx->Pc.pToken;
				else break;
			}
			if(pCntx->Pc.pToken->Type != T_CP){																																						// abort: no closing parenthisis 
				return ValueNewError(pValue, MISSINGCP);
			}
			++pCntx->Pc.pToken;
 		}
		if(pCntx->Pc.pToken->Type == T_OSB){
			++pCntx->Pc.pToken;
			ErrorPc = pCntx->Pc;
			if(Evaluate(pValue, _T("size"))->Type == eVType::V_ERROR || (pCntx->Pass != ePass::DECLARE && ValueRetype(pValue, eVType::V_INT)->Type == eVType::V_ERROR)){
				return pValue;
			}
			if(pCntx->Pass == ePass::INTERPRET && pValue->uData.Integer < 1){																							// error: size must be 1 or greater 
				ValueDestroy(pValue);
				ValueNewError(pValue, OUTOFRANGE, _T("Size"));
			}
 			if(pValue->Type == eVType::V_ERROR){																																					// abort: syntax fault
				pCntx->Pc = ErrorPc;
				return pValue;
			}
 			if(pCntx->Pass == ePass::INTERPRET){
				StrLen = pValue->uData.Integer;
			}
			ValueDestroy(pValue);
			if(pCntx->Pc.pToken->Type != T_CSB){																																					// abort: no closing square bracket 
				return ValueNewError(pValue, MISSINGOSB);
			}
			++pCntx->Pc.pToken;
		}
 		if((pCntx->Pass == ePass::INTERPRET) && !pVar->IsDefined){
			Var_s NewArray;
			if(VarNew(&NewArray, VarType, Dim, Geometry) == nullptr){
				return ValueNewError(pValue, OUTOFMEMORY);
			}
			if((VarType == eVType::V_STRING) && StrLen) for(Dim = 0; Dim < NewArray.Size; ++Dim) NewArray.pValue[Dim].uData.pString->SetMaxSize(StrLen);	// set maximum string size (was definition of string size)
			VarDestroy(pVar);
			*pVar = NewArray;
			pVar->IsDefined = true;
		}
		if(pCntx->Pc.pToken->Type == T_COMMA) ++pCntx->Pc.pToken;																													// advance to next var 
		else if(pCntx->Pc.pToken->Type != T_OSB) break;
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////

pValue_s stmt_DO(pValue_s pValue)
{
	if(pCntx->Pass == ePass::DECLARE || pCntx->Pass == ePass::COMPILE) PushMarker(eMType::DO, &pCntx->Pc);
	++pCntx->Pc.pToken;
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////

pValue_s stmt_DO_CONDITION(pValue_s pValue)
{
PC_s DoWhilePc = pCntx->Pc;

	int Negate = (pCntx->Pc.pToken->Type == T_DOUNTIL);
	if(pCntx->Pass == ePass::DECLARE || pCntx->Pass == ePass::COMPILE) PushMarker(eMType::DO_CND, &pCntx->Pc);
	++pCntx->Pc.pToken;
	if(Evaluate(pValue, _T("condition"))->Type == eVType::V_ERROR) return pValue;
	if(pCntx->Pass == ePass::INTERPRET){
		int Condition = ValueIsNull(pValue);
		if(Negate) Condition = !Condition;
		if(Condition) pCntx->Pc = DoWhilePc.pToken->Obj.ExitDoPc;
		ValueDestroy(pValue);
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////

pValue_s stmt_DUMP(pValue_s pValue)
{
bool DumpGraphics	= false;
int Device = 0;

	++pCntx->Pc.pToken;
	if(pCntx->Pc.pToken->Type == T_GRAPHICS) DumpGraphics = true;
	++pCntx->Pc.pToken;
	if(pCntx->Pc.pToken->Type != T_EOL){
		if((Evaluate(pValue, _T("device"))->Type == eVType::V_ERROR) || (ValueRetype(pValue, eVType::V_INT)->Type == eVType::V_ERROR)) return pValue;
		Device = pValue->uData.Integer;
		ValueDestroy(pValue);
	}
	if(pCntx->Pass == ePass::INTERPRET){
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////

pValue_s stmt_ELSE_ELSEIFELSE(pValue_s pValue)
{
	if(pCntx->Pass == ePass::INTERPRET) pCntx->Pc = pCntx->Pc.pToken->Obj.EndIfPc;
	if((pCntx->Pass == ePass::DECLARE) || (pCntx->Pass == ePass::COMPILE)){
		PC_s pElsePc = pCntx->Pc, *IfPc;
		int ElseIfElse = (pCntx->Pc.pToken->Type == T_ELSEIFELSE);
		if((IfPc = PopMarker(eMType::IF)) == nullptr) return ValueNewError(pValue, STRAYELSE1);
		if(IfPc->pToken->Type == T_ELSEIFIF) (IfPc->pToken - 1)->Obj.ElsePc = pCntx->Pc;
		++pCntx->Pc.pToken;
		IfPc->pToken->Obj.ElsePc = pCntx->Pc;
		assert(IfPc->pToken->Type == T_ELSEIFIF || IfPc->pToken->Type == T_IF);
		if(ElseIfElse) return &MoreStatements;
		else PushMarker(eMType::ELSE, &pElsePc);
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////

pValue_s stmt_ENABLE_DISABLE(pValue_s pValue)
{
	if(pCntx->Pass == ePass::INTERPRET){
		if(pCntx->Pc.pToken->Type == T_ENABLE) pCntx->OnEventsEnabled = true;
		else pCntx->OnEventsEnabled = false;
	}
	++pCntx->Pc.pToken;
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////

pValue_s stmt_END(pValue_s pValue)
{
	if(InDirectMode()) return ValueNewError(pValue, NOTINDIRECTMODE);
	if(pCntx->Pass == ePass::INTERPRET) pCntx->Pc = pCntx->Pc.pToken->Obj.EndPc;
	else if(pCntx->Pass == ePass::DECLARE || pCntx->Pass == ePass::COMPILE){
		ProgramEnd(&pCntx->Program, &pCntx->Pc.pToken->Obj.EndPc);
		++pCntx->Pc.pToken;
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////

pValue_s stmt_ENDIF(pValue_s pValue)
{
	if(pCntx->Pass == ePass::DECLARE || pCntx->Pass == ePass::COMPILE){
		PC_s EndIfPc = pCntx->Pc, *pIfPc, *pElsePc;
		if((pIfPc = PopMarker(eMType::IF))){
			pIfPc->pToken->Obj.ElsePc = EndIfPc;
			if(pIfPc->pToken->Type == T_ELSEIFIF) (pIfPc->pToken - 1)->Obj.ElsePc = pCntx->Pc;
		}
		else if((pElsePc = PopMarker(eMType::ELSE))) pElsePc->pToken->Obj.EndIfPc = EndIfPc;
		else return ValueNewError(pValue, STRAYENDIF);
	}
	++pCntx->Pc.pToken;
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////

pValue_s stmt_ENDSELECT(pValue_s pValue)
{
PC_s ErrorPc = pCntx->Pc, *SelectCasePc;

	++pCntx->Pc.pToken;
	if(pCntx->Pass == ePass::DECLARE || pCntx->Pass == ePass::COMPILE){
		if((SelectCasePc = PopMarker(eMType::SELECT))) SelectCasePc->pToken->Obj.pSelectCase->EndSelect = pCntx->Pc;
		else{
			pCntx->Pc = ErrorPc;
			return ValueNewError(pValue, STRAYENDSELECT);
		}
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////

pValue_s stmt_EXECUTE(pValue_s pValue)
{
Value_s UsingValue;
BString *pUsingStr = nullptr, PrintStr;
UINT UsingPos = 0;

	++pCntx->Pc.pToken;
	ValueNewString(&UsingValue);																						// dummy using string
	pUsingStr = UsingValue.uData.pString;
	PC_s ErrorPc = pCntx->Pc;
	if(Evaluate(pValue, nullptr)){
		if(pValue->Type == eVType::V_ERROR){
			ValueDestroy(&UsingValue);
			return pValue;
		}
		if(pCntx->Pass == ePass::INTERPRET){
			if(pValue->Type == eVType::V_ARRAY){
				if(ValueArrayToStringUsing(pValue, &PrintStr, pUsingStr, &UsingPos)->Type == eVType::V_ERROR){
					ValueDestroy(&UsingValue);
					pCntx->Pc = ErrorPc;
					return pValue;
				}
			}
			else{
				if(ValueToStringUsing(pValue, &PrintStr, pUsingStr, &UsingPos)->Type == eVType::V_ERROR){
					ValueDestroy(&UsingValue);
					pCntx->Pc = ErrorPc;
					return pValue;
				}
			}
		}
		ValueDestroy(pValue);
	}
	if(PrintStr.GetLength() > 0)	g_pPrinter->PrintText(eDispArea::COMMENT, PrintStr.GetBuffer());			// send it directley to the system line
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////

pValue_s stmt_EXITFUNC(pValue_s pValue)
{
PC_s *pCurrentFunction = nullptr;

	if(pCntx->Pass == ePass::DECLARE || pCntx->Pass == ePass::COMPILE) if((pCurrentFunction = FindMarker(eMType::FUNC)) == nullptr || (pCurrentFunction->pToken + 1)->Obj.pIdentifier->DefType == eVType::V_VOID) return ValueNewError(pValue, STRAYFNEXIT);
	++pCntx->Pc.pToken;
	if(pCntx->Pass == ePass::INTERPRET) return ValueClone(pValue, VarValue(StackGetAt(&pCntx->Stack, 0), 0, nullptr, nullptr));
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////

pValue_s stmt_COLON_EOL(pValue_s pValue)
{
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////

pValue_s stmt_QUOTE_REM(pValue_s pValue)
{
	++pCntx->Pc.pToken;
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////

pValue_s stmt_EQ_FNEND(pValue_s pValue)
{
PC_s *pCurrentFunction = nullptr;
PC_s EntryPc = pCntx->Pc;
TokenType_e TokenType = pCntx->Pc.pToken->Type;

	if(pCntx->Pass == ePass::DECLARE || pCntx->Pass == ePass::COMPILE){
		if(TokenType == T_EQ){
			if((pCurrentFunction = PopMarker(eMType::FUNC)) == nullptr) return ValueNewError(pValue, STRAYENDEQ);
			if((EntryPc.pToken->Obj.Type = (pCurrentFunction->pToken + 1)->Obj.pIdentifier->DefType) == eVType::V_VOID) return ValueNewError(pValue, STRAYENDEQ);
		}
		else if(TokenType == T_FNEND){
			if((pCurrentFunction = PopMarker(eMType::FUNC)) == nullptr) return ValueNewError(pValue, STRAYFNEND);
			if((EntryPc.pToken->Obj.Type = (pCurrentFunction->pToken + 1)->Obj.pIdentifier->DefType) == eVType::V_VOID) return ValueNewError(pValue, STRAYFNEND);
		}
		else{
			if((pCurrentFunction = FindMarker(eMType::FUNC)) == nullptr) return ValueNewError(pValue, STRAYFNRETURN);
			if((EntryPc.pToken->Obj.Type = (pCurrentFunction->pToken + 1)->Obj.pIdentifier->DefType) == eVType::V_VOID) return ValueNewError(pValue, STRAYFNRETURN);
		}
	}
	++pCntx->Pc.pToken;
	if(TokenType != T_FNEND){																																	 // no return value for FNEND
		if(Evaluate(pValue, _T("return"))->Type == eVType::V_ERROR || ValueRetype(pValue, EntryPc.pToken->Obj.Type)->Type == eVType::V_ERROR){
			if(pCntx->Pass != ePass::INTERPRET) StackFuncEnd(&pCntx->Stack);
			pCntx->Pc = EntryPc;
			return pValue;
		}
	}
	if(pCntx->Pass == ePass::INTERPRET) return pValue;
	else{
		ValueDestroy(pValue);
		if(TokenType == T_EQ || TokenType == T_FNEND){
			if(pCntx->Pass == ePass::DECLARE){
				if(TokenType == T_FNEND) RegisterGlobalFuncEnd(g_pGlobalRegistry, (pCurrentFunction->pToken + 1)->Obj.pIdentifier, &pCntx->Pc);
				else RegisterLocalFuncEnd(g_pLocalRegistry, (pCurrentFunction->pToken + 1)->Obj.pIdentifier, &pCntx->Pc);
			}
			StackFuncEnd(&pCntx->Stack);
		}
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////

pValue_s stmt_EXITDO(pValue_s pValue)
{
	if(pCntx->Pass == ePass::INTERPRET) pCntx->Pc = pCntx->Pc.pToken->Obj.ExitDoPc;
	else{
		if(pCntx->Pass == ePass::COMPILE){
			PC_s *pMarkerPc;
			if((pMarkerPc = FindMarker(eMType::DO)) == nullptr && (pMarkerPc = FindMarker(eMType::DO_CND)) == nullptr) return ValueNewError(pValue, STRAYEXITDO);
			pCntx->Pc.pToken->Obj.ExitDoPc = pMarkerPc->pToken->Obj.ExitDoPc;
		}
		++pCntx->Pc.pToken;
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////

pValue_s stmt_EXITFOR(pValue_s pValue)
{
	if(pCntx->Pass == ePass::INTERPRET) pCntx->Pc = pCntx->Pc.pToken->Obj.ExitForPc;
	else{
		if(pCntx->Pass == ePass::COMPILE){
			PC_s *pMarkerPc;
			if((pMarkerPc = FindMarker(eMType::FOR)) == nullptr) return ValueNewError(pValue, STRAYEXITFOR);
			pCntx->Pc.pToken->Obj.ExitForPc = pMarkerPc->pToken->Obj.ExitForPc;
		}
		++pCntx->Pc.pToken;
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////

pValue_s stmt_FIXED(pValue_s pValue)
{
	++pCntx->Pc.pToken;
	if(Evaluate(pValue, _T("fixed rounding"))->Type == eVType::V_ERROR || (pCntx->Pass != ePass::DECLARE && ValueRetype(pValue, eVType::V_INT)->Type == eVType::V_ERROR)) return pValue;
	if(pCntx->Pass == ePass::INTERPRET){
		pCntx->pSubCntx->RoundingPrecision = pValue->uData.Integer;
		pCntx->pSubCntx->RoundingMode = eRounding::FIXED;
	}
	ValueDestroy(pValue);
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////

pValue_s stmt_FLOAT(pValue_s pValue)
{
	++pCntx->Pc.pToken;
	if(Evaluate(pValue, _T("float rounding"))->Type == eVType::V_ERROR || (pCntx->Pass != ePass::DECLARE && ValueRetype(pValue, eVType::V_INT)->Type == eVType::V_ERROR)) return pValue;
	if(pCntx->Pass == ePass::INTERPRET){
		pCntx->pSubCntx->RoundingPrecision = pValue->uData.Integer;
		pCntx->pSubCntx->RoundingMode = eRounding::FLOAT;
	}
	ValueDestroy(pValue);
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////

pValue_s stmt_FOLDER(pValue_s pValue)
{
int Result = -1, Error = -1;
PC_s EntryPc = pCntx->Pc, ErrorPc;

	++pCntx->Pc.pToken;
	ErrorPc = pCntx->Pc;
	if(Evaluate(pValue, _T("directory"))->Type == eVType::V_ERROR || ValueRetype(pValue, eVType::V_STRING)->Type == eVType::V_ERROR) return pValue;
	if(pCntx->Pass == ePass::INTERPRET){
		switch(EntryPc.pToken->Type){
			case T_CHDIR:
				Result = _chdir(pValue->uData.pString->GetBuffer());
				break;
			case T_CREATEDIR:
				Result = _mkdir(pValue->uData.pString->GetBuffer());
				break;
			default:
				assert(0);
		}
	}
	Error = errno;
	ValueDestroy(pValue);
	if(pCntx->Pass == ePass::INTERPRET && Result == -1){
		pCntx->Pc = ErrorPc;
		char Buff[MSG_BUFFER_SIZE];
		strerror_s(Buff, MSG_BUFFER_SIZE, Error);
		g_SufixErrorMsg.AppendChars(Buff);
		return ValueNewError(pValue, MASSSTROREERR, g_SufixErrorMsg.GetBuffer());
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////

pValue_s stmt_FOR(pValue_s pValue)
{
PC_s ForPc = pCntx->Pc, VarPc, ToPc, LimitPc;
Value_s LimitVal, StepValue;

	++pCntx->Pc.pToken;
	VarPc = pCntx->Pc;
	if(pCntx->Pc.pToken->Type != T_IDENTIFIER) return ValueNewError(pValue, MISSINGLOOPIDENT);
	if(AssignValue(pValue)->Type == eVType::V_ERROR) return pValue;
	if(pCntx->Pass == ePass::INTERPRET){
		ToPc = pCntx->Pc;
		++pCntx->Pc.pToken;
		if(Evaluate(&LimitVal, nullptr)->Type == eVType::V_ERROR){
			*pValue = LimitVal;
			return pValue;
		}
		ValueRetype(&LimitVal, pValue->Type);
		assert(LimitVal.Type != eVType::V_ERROR);
		if(pCntx->Pc.pToken->Type == T_STEP){ /* STEP pValue */
			PC_s StepPc;
			++pCntx->Pc.pToken;
			StepPc = pCntx->Pc;
			if(Evaluate(&StepValue,_T( "`step'"))->Type == eVType::V_ERROR){
				ValueDestroy(pValue);
				*pValue = StepValue;
				pCntx->Pc = StepPc;
				return pValue;
			}
			ValueRetype(&StepValue, pValue->Type);
			assert(StepValue.Type != eVType::V_ERROR);
		}
		else{ /* implicit numeric STEP */
			if(pValue->Type == eVType::V_INT) NewIntegerValue(&StepValue, 1);
			else NewRealValue(&StepValue, 1.0);
		}

		if(ValueExitFor(pValue, &LimitVal, &StepValue)) pCntx->Pc = ForPc.pToken->Obj.ExitForPc;
		ValueDestroy(&LimitVal);
		ValueDestroy(&StepValue);
		ValueDestroy(pValue);
	}
	else{
		PushMarker(eMType::FOR, &ForPc);
		PushMarker(eMType::FOR_VAR, &VarPc);
		if(pCntx->Pc.pToken->Type != T_TO){
			ValueDestroy(pValue);
			return ValueNewError(pValue, MISSINGTO);
		}
		ToPc = pCntx->Pc;
		++pCntx->Pc.pToken;
		PushMarker(eMType::FOR_LIMIT, &pCntx->Pc);
		LimitPc = pCntx->Pc;
		if(Evaluate(&LimitVal, nullptr) == nullptr){
			ValueDestroy(pValue);
			return ValueNewError(pValue, MISSINGEXPR, _T("`to'"));
		}
		if(LimitVal.Type == eVType::V_ERROR){
			ValueDestroy(pValue);
			*pValue = LimitVal;
			return pValue;
		}
		if(pCntx->Pass != ePass::DECLARE){
			Symbol_s *sym = VarPc.pToken->Obj.pIdentifier->pSym;
			if(DoValueRetype(&LimitVal, sym->SymbolType == eSymType::LOCALVAR || sym->SymbolType == eSymType::LOCALARRAY ? sym->uType.Var.Type : StackVarType(&pCntx->Stack, sym))->Type == eVType::V_ERROR){
				ValueDestroy(pValue);
				*pValue = LimitVal;
				pCntx->Pc = LimitPc;
				return pValue;
			}
		}
		ValueDestroy(&LimitVal);
		if(pCntx->Pc.pToken->Type == T_STEP){ /* STEP pValue */
			PC_s StepPc;

			++pCntx->Pc.pToken;
			StepPc = pCntx->Pc;
			if(Evaluate(&StepValue, _T("`step'"))->Type == eVType::V_ERROR || (pCntx->Pass != ePass::DECLARE && ValueRetype(&StepValue, pValue->Type)->Type == eVType::V_ERROR)){
				ValueDestroy(pValue);
				*pValue = StepValue;
				pCntx->Pc = StepPc;
				return pValue;
			}
		}
		else{ /* implicit numeric STEP */
			NewIntegerValue(&StepValue, 1);
			if(pCntx->Pass != ePass::DECLARE && DoValueRetype(&StepValue, pValue->Type)->Type == eVType::V_ERROR){
				ValueDestroy(pValue);
				*pValue = StepValue;
				ValueErrorPrefix(pValue, _T("implicit STEP 1:"));
				return pValue;
			}
		}
		PushMarker(eMType::FOR_BODY, &pCntx->Pc);
		ValueDestroy(&StepValue);
		ValueDestroy(pValue);
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////

pValue_s stmt_GET(pValue_s pValue)
{
Context_s	*pRunCntx = &SystemContexts[(int)eCntxType::RUN];
Value_s	*pRetVal = nullptr;
BString FileName;
int LoadLine = -1, RunLine = -1;
PC_s GetPc, ErrorPc;

	++pCntx->Pc.pToken;
	GetPc = pCntx->Pc;
	if(Evaluate(pValue, _T("file name"))->Type == eVType::V_ERROR || ValueRetype(pValue, eVType::V_STRING)->Type == eVType::V_ERROR){
		pCntx->Pc = GetPc;
		return pValue;
	}
	FileName.AppendString(pValue->uData.pString);
	ValueDestroy(pValue);
	if(pCntx->Pc.pToken->Type == T_COMMA){																											// load point
		++pCntx->Pc.pToken;
		ErrorPc = pCntx->Pc;
		if(Evaluate(pValue, _T( "load point"))->Type == eVType::V_ERROR){
			pCntx->Pc = ErrorPc;
			return pValue;
		}
		ValueRetype(pValue, eVType::V_INT);
		LoadLine = pValue->uData.Integer;
		ValueDestroy(pValue);
	}
	if(pCntx->Pc.pToken->Type == T_COMMA){																											// run point
		++pCntx->Pc.pToken;
		ErrorPc = pCntx->Pc;
		if(Evaluate(pValue, _T( "run from"))->Type == eVType::V_ERROR){
			pCntx->Pc = ErrorPc;
			return pValue;
		}
		ValueRetype(pValue, eVType::V_INT);
		RunLine = pValue->uData.Integer;
		ValueDestroy(pValue);
	}
	if(pCntx->Pass == ePass::INTERPRET){
		int FileIndex;
		Program_s newProg;
		if((FileIndex = g_pMassStorage->ReadOpen(FileName.GetBuffer(), MT_BAS, true)) == -1){
			pCntx->Pc = GetPc;
			return ValueNewError(pValue, MASSSTROREERR, g_SufixErrorMsg.GetBuffer());
		}
  	ProgramNew(&newProg);
		ProgramSetName(&newProg, FileName.GetBuffer());																							// set new program name
		if(ProgramGet(&newProg, FileIndex, pValue)){																								// save new statements into program space
			pCntx->Pc = GetPc;
			return pValue;																																						// return with error
		}
		g_pMassStorage->Close(FileIndex);
		ProgramDestroy(&pRunCntx->Program);																													// destroy previous statements if any
		BasicNew();
		pRunCntx->Program = newProg;
		pRunCntx->Program.Unsaved = FALSE;
		SetMainTitle(FileName.GetBuffer()); 
		if(!InDirectMode()){
			if(RunLine > 0){																																					// run from line number
				if(ProgramGoLine(&pCntx->Program, RunLine, &GetPc) == nullptr) return ValueNewError(pValue, NOSUCHLINE);
				if(ProgramBeginning(&pCntx->Program, &GetPc) == nullptr)	return ValueNewError(pValue, NOPROGRAM);
			}
			if(CompileProgram(pValue, 1)->Type == eVType::V_ERROR) return pValue;											// we now compile the programme for the first time after loading
			pCntx->Pc = GetPc;																																			  // set the start line
			pCntx->pSubCntx->CurrentData = pCntx->pSubCntx->BeginData;																// initialise the data stack pointer
			ResetVariables(g_pLocalRegistry);																													// get ready to run the programme 
			g_pMassStorage->CloseAll();																																// close all files
		}
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////

pValue_s stmt_GOSUB(pValue_s pValue)
{
	if(pCntx->Pass == ePass::INTERPRET){
		if(!pCntx->Program.Runable && CompileProgram(pValue, !InDirectMode())->Type == eVType::V_ERROR) return pValue;
		pCntx->Pc.pToken += 2;
		g_Trace.pLastPc = &pCntx->Pc;
		StackPushGosubRet(&pCntx->Stack, &pCntx->Pc, -1);
		pCntx->Pc = (pCntx->Pc.pToken - 2)->Obj.GosubPc;
		ProgramTraceFlow(&pCntx->Program, &pCntx->Pc);
	}
	if(pCntx->Pass == ePass::DECLARE || pCntx->Pass == ePass::COMPILE){
		Token_s *pGotoToken = pCntx->Pc.pToken;
		++pCntx->Pc.pToken;
		switch(pCntx->Pc.pToken->Type){
			case T_IDENTIFIER:{
				if(pCntx->Pass == ePass::COMPILE){
					if(FindLabel(pCntx->pSubCntx->pLabels, pCntx->Pc.pToken->Obj.pIdentifier) == 0) return ValueNewError(pValue, NOSUCHLINE);
					ProgramGetLabelLine(&pCntx->Program, pCntx->Pc.pToken->Obj.pLabel, &pGotoToken->Obj.GosubPc);
					if(ProgramScopeCheck(&pCntx->Program, &pGotoToken->Obj.GosubPc, FindMarker(eMType::FUNC))) return ValueNewError(pValue, OUTOFSCOPE);
				}
				break;
			}
			case T_INTNUM:{
				if(ProgramGoLine(&pCntx->Program, pCntx->Pc.pToken->Obj.Integer, &pGotoToken->Obj.GosubPc) == nullptr) return ValueNewError(pValue, NOSUCHLINE);
				if((pCntx->Pass == ePass::COMPILE) && ProgramScopeCheck(&pCntx->Program, &pGotoToken->Obj.GosubPc, FindMarker(eMType::FUNC))) return ValueNewError(pValue, OUTOFSCOPE);
				break;
			}
			default:{
				return ValueNewError(pValue, MISSINGLINENUMBER);
			}
		}
		++pCntx->Pc.pToken;
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////


pValue_s stmt_GOTO_RESUME(pValue_s pValue)
{
	if(pCntx->Pass == ePass::INTERPRET){
		if(!pCntx->Program.Runable && CompileProgram(pValue, !InDirectMode())->Type == eVType::V_ERROR) return pValue;
		if(pCntx->Pc.pToken->Type == T_RESUME){
			if(!pCntx->Stack.Resumeable) return ValueNewError(pValue, STRAYRESUME);
			pCntx->Stack.Resumeable = 0;
		}
		g_Trace.pLastPc = &pCntx->Pc;
		pCntx->Pc = pCntx->Pc.pToken->Obj.GotoPc;
		ProgramTraceFlow(&pCntx->Program, &pCntx->Pc);
	}
	else if(pCntx->Pass == ePass::DECLARE || pCntx->Pass == ePass::COMPILE){
		Token_s *pGotoToken = pCntx->Pc.pToken;
		++pCntx->Pc.pToken;
		switch(pCntx->Pc.pToken->Type){
			case T_IDENTIFIER:{
				if(pCntx->Pass == ePass::COMPILE){
					if(FindLabel(pCntx->pSubCntx->pLabels, pCntx->Pc.pToken->Obj.pIdentifier) == 0) return ValueNewError(pValue, NOSUCHLINE);
					ProgramGetLabelLine(&pCntx->Program, pCntx->Pc.pToken->Obj.pLabel, &pGotoToken->Obj.GotoPc);
					if(ProgramScopeCheck(&pCntx->Program, &pGotoToken->Obj.GotoPc, FindMarker(eMType::FUNC))) return ValueNewError(pValue, OUTOFSCOPE);
				}
				break;
			}
			case T_INTNUM:{
				if(ProgramGoLine(&pCntx->Program, pCntx->Pc.pToken->Obj.Integer, &pGotoToken->Obj.GotoPc) == nullptr) return ValueNewError(pValue, NOSUCHLINE);
				if((pCntx->Pass == ePass::COMPILE) && ProgramScopeCheck(&pCntx->Program, &pGotoToken->Obj.GotoPc, FindMarker(eMType::FUNC))) return ValueNewError(pValue, OUTOFSCOPE);
				break;
			}
			default:{
				return ValueNewError(pValue, MISSINGLINENUMBER);
			}
		}
		++pCntx->Pc.pToken;
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////

pValue_s stmt_IDENTIFIER(pValue_s pValue)
{
PC_s EntryPc = pCntx->Pc;
Identifier_s *pIdentifier = pCntx->Pc.pToken->Obj.pIdentifier;

	if((pIdentifier->DefType == eVType::V_INT) || (pIdentifier->DefType == eVType::V_REAL) || (pIdentifier->DefType == eVType::V_STRING)){	// a$[n] = b$ | a$(m)[n] = b$ 
		if(pCntx->Pc.pToken->Type != T_IDENTIFIER) return ValueNewError(pValue, MISSINGSTRIDENT);
		pCntx->IsAssign = true;																																																								// allow consecutive assignments 
		if(AssignValue(pValue)->Type == eVType::V_ERROR) return pValue;
		pCntx->IsAssign = false;
		ValueDestroy(pValue);
		return nullptr;
	}
	if(pCntx->Pass == ePass::DECLARE){																																																			// function call 
		if(Function(pValue)->Type == eVType::V_ERROR) return pValue;
		else ValueDestroy(pValue);
		if(pCntx->Pc.pToken->Type == T_EQ){
			pCntx->Pc = EntryPc;
			if(AssignValue(pValue)->Type == eVType::V_ERROR) return pValue;
			ValueDestroy(pValue);
		}
	}
	else{
		if(pCntx->Pass == ePass::COMPILE){
			if((((pCntx->Pc.pToken + 1)->Type == T_OP) || (StackFind(&pCntx->Stack, pCntx->Pc.pToken->Obj.pIdentifier) == 0)) && (FindIdentifier(g_pLocalRegistry, pCntx->Pc.pToken->Obj.pIdentifier, eSymType::LOCALARRAY) == 0)){
				return ValueNewError(pValue, UNDECLARED);
			}
		}
		if(pCntx->Pc.pToken->Obj.pIdentifier->pSym->SymbolType == eSymType::USERFUNCTION || pCntx->Pc.pToken->Obj.pIdentifier->pSym->SymbolType == eSymType::BUILTINFUNCTION){
			Function(pValue);
			if(ValueRetype(pValue, eVType::V_VOID)->Type == eVType::V_ERROR) return pValue;
			ValueDestroy(pValue);
		}
		else{
			if(pCntx->Pc.pToken->Obj.pIdentifier->pSym->SymbolType == eSymType::SUBPROGRAMME){
				SubProgram(pValue);
				if(ValueRetype(pValue, eVType::V_VOID)->Type == eVType::V_ERROR) return pValue;
				ValueDestroy(pValue);
			}
			else{
				if(AssignValue(pValue)->Type == eVType::V_ERROR) return pValue;
				if(pCntx->Pass != ePass::INTERPRET) ValueDestroy(pValue);
			}
		}
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////

pValue_s stmt_IF_ELSEIFIF(pValue_s pValue)
{
PC_s EntryPc = pCntx->Pc;

	++pCntx->Pc.pToken;
	if(Evaluate(pValue, _T("condition"))->Type == eVType::V_ERROR) return pValue;
	if(pCntx->Pc.pToken->Type != T_THEN){
		ValueDestroy(pValue);
		return ValueNewError(pValue, MISSINGTHEN);
	}
	++pCntx->Pc.pToken;
	if(pCntx->Pass == ePass::INTERPRET){
		if(ValueIsNull(pValue)) pCntx->Pc = EntryPc.pToken->Obj.ElsePc;
		ValueDestroy(pValue);
	}
	else{
		ValueDestroy(pValue);
		if(pCntx->Pc.pToken->Type == T_EOL || pCntx->Pc.pToken->Type == T_REM) PushMarker(eMType::IF, &EntryPc);
		else{																																											// compile single line IF THEN ELSE recursively 
			if(Statements(pValue)->Type == eVType::V_ERROR) return pValue;
			ValueDestroy(pValue);
			if(pCntx->Pc.pToken->Type == T_ELSE){
				PC_s ElsePc = pCntx->Pc;
				++pCntx->Pc.pToken;
				EntryPc.pToken->Obj.ElsePc = pCntx->Pc;
				if(EntryPc.pToken->Type == T_ELSEIFIF) (EntryPc.pToken - 1)->Obj.ElsePc = pCntx->Pc;
				if(Statements(pValue)->Type == eVType::V_ERROR) return pValue;
				ValueDestroy(pValue);
				ElsePc.pToken->Obj.EndIfPc = pCntx->Pc;
			}
			else{
				EntryPc.pToken->Obj.ElsePc = pCntx->Pc;
				if(EntryPc.pToken->Type == T_ELSEIFIF) (EntryPc.pToken - 1)->Obj.ElsePc = pCntx->Pc;
			}
		}
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////

pValue_s stmt_IMAGE(pValue_s pValue)
{
	++pCntx->Pc.pToken;
	if(pCntx->Pc.pToken->Type != T_STRING) return ValueNewError(pValue, MISSINGFMT);
	++pCntx->Pc.pToken;
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////

pValue_s stmt_INPUT(pValue_s pValue)
{
TokenType_e InputType = pCntx->Pc.pToken->Type;
Token_s *pInDataToken = nullptr, *pToken = nullptr;
CS45BasicDoc* pDoc = static_cast<CS45BasicDoc*>(g_pBasicDoc);
bool EntryGraphicsMode, Skip;
PC_s EntryPC = pCntx->Pc;

	++pCntx->Pc.pToken;
	if(pCntx->Pass == ePass::INTERPRET) EntryGraphicsMode = pDoc->SetGraphicsMode(false);
	while(1){
		PC_s LValuePc;
		bool DefaultPrompt = true;
		if(pCntx->Pc.pToken->Type == T_STRING){																												// prompt 
			if((pCntx->Pass == ePass::INTERPRET) && (pCntx->Pc.pToken->Obj.pString->GetLength())) g_pPrinter->PrintText(eDispArea::PROMPT,  pCntx->Pc.pToken->Obj.pString->GetBuffer());
			++pCntx->Pc.pToken;
			if(pCntx->Pc.pToken->Type == T_COMMA){
				++pCntx->Pc.pToken;
				DefaultPrompt = false;
			}
			else{
				if(pCntx->Pc.pToken->Type == T_SEMICOLON) ++pCntx->Pc.pToken;
				else DefaultPrompt = false;
			}
		}
		if((pCntx->Pass == ePass::INTERPRET) && DefaultPrompt)	g_pPrinter->PrintText(eDispArea::PROMPT,  _T("? "));
retry:
		if(pCntx->Pass == ePass::INTERPRET){																													// read input line and tokenise it 
			BString Str;
			Skip = false;
			if(!GetInput(&Str)){
				if((pCntx->RunMode == eRunMode::STOP) || (pCntx->RunMode == eRunMode::HALT)) pCntx->Pc = EntryPC;
				if(pCntx->RunMode == eRunMode::STOP) return ValueNewError(pValue, BREAK);
				pCntx->Stack.Resumeable = 1;																															// make it resumable
				return ValueNewError(pValue, PAUSE);
			}
			if(Str.GetLength() == 0) Skip = true;	
			else pInDataToken = pToken = TokeniseData(Str.GetBuffer());
		}
		pValue_s pVal;
		if(pCntx->Pc.pToken->Type != T_IDENTIFIER) return ValueNewError(pValue, MISSINGVARIDENT);
		if((pCntx->Pass == ePass::DECLARE) && (FindOrRegister(g_pLocalRegistry, pCntx->Pc.pToken->Obj.pIdentifier) == 0))	 return ValueNewError(pValue, REDECLARATION);	
		LValuePc = pCntx->Pc;
		if(((pVal = GetValue(pValue))->Type) == eVType::V_ERROR) return pValue;
		if((pCntx->Pass == ePass::COMPILE) && (pVal->Type != eVType::V_STRING) && (InputType == T_LINEINPUT)){
			pCntx->Pc = LValuePc;
			return ValueNewError(pValue, TYPEMISMATCH4);
		}
		if(pCntx->Pass == ePass::INTERPRET && !Skip){
			if(pToken->Type == T_EOL){
				enum eVType ltype = pVal->Type;
				ValueDestroy(pVal);
				ValueNewNull(pVal, ltype);
			}
			else{
				if(Convert(pValue, pVal, pToken)){
					pCntx->Pc = LValuePc;
					BString Str;
					ValueToString(pValue, &Str, ' ', -1, 0, 0, -1, 0, 0);
					g_pPrinter->PrintText(eDispArea::COMMENT, Str.GetBuffer());
					ValueDestroy(pValue);
					TokenDestroy(pInDataToken);
					goto retry;
				}
				++pToken;
			}
			if(pToken->Type != T_EOL) g_pPrinter->PrintText(eDispArea::COMMENT,  _T("Too much input data"));
			TokenDestroy(pInDataToken);
		}
		if(pCntx->Pc.pToken->Type == T_COMMA) ++pCntx->Pc.pToken;
		else break;
	}
	g_pPrinter->PrintText(eDispArea::PROMPT, _T(""));
	if(pCntx->Pass == ePass::INTERPRET) pDoc->SetGraphicsMode(EntryGraphicsMode);
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////

pValue_s stmt_LABEL(pValue_s pValue)
{
	if((pCntx->Pass == ePass::DECLARE) && (RegisterLabel(pCntx->pSubCntx->pLabels, pCntx->Pc.pToken->Obj.pLabel, &pCntx->Pc) == 0))	return ValueNewError(pValue, DUPLICATION);
	++pCntx->Pc.pToken;
	if(pCntx->Pc.pToken->Type != T_EOL) return &MoreStatements;
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////

pValue_s stmt_LET(pValue_s pValue)
{
	++pCntx->Pc.pToken;
	if(AssignValue(pValue)->Type == eVType::V_ERROR) return pValue;
	if(pCntx->Pass != ePass::INTERPRET) ValueDestroy(pValue);
	return nullptr;
}

///////////////////////////////////////////////////////////////////////////////////////////////

pValue_s stmt_LINK(pValue_s pValue)
{
Context_s	*pRunCntx = &SystemContexts[(int)eCntxType::RUN];
BString FileName;
int LoadLine = -1, RunLine = -1;
PC_s ErrorPc;

	++pCntx->Pc.pToken;
	PC_s EntryPc = pCntx->Pc;
	if(Evaluate(pValue, _T("file name"))->Type == eVType::V_ERROR || ValueRetype(pValue, eVType::V_STRING)->Type == eVType::V_ERROR){
		pCntx->Pc = EntryPc;
		return pValue;
	}
	FileName.AppendString(pValue->uData.pString);
	ValueDestroy(pValue);
	if(pCntx->Pc.pToken->Type == T_COMMA){																											// load point
		++pCntx->Pc.pToken;
		ErrorPc = pCntx->Pc;
		if(Evaluate(pValue, _T( "load point"))->Type == eVType::V_ERROR){
			pCntx->Pc = ErrorPc;
			return pValue;
		}
		ValueRetype(pValue, eVType::V_INT);
		LoadLine = pValue->uData.Integer;
		ValueDestroy(pValue);
	}
	if(pCntx->Pc.pToken->Type == T_COMMA){																											// run point
		++pCntx->Pc.pToken;
		ErrorPc = pCntx->Pc;
		if(Evaluate(pValue, _T( "run from"))->Type == eVType::V_ERROR){
			pCntx->Pc = ErrorPc;
			return pValue;
		}
		ValueRetype(pValue, eVType::V_INT);
		RunLine = pValue->uData.Integer;
		ValueDestroy(pValue);
	}
	if(pCntx->Pass == ePass::INTERPRET){
		int FileIndex;
		Program_s newProg;
		if((FileIndex = g_pMassStorage->ReadOpen(FileName.GetBuffer(), MT_BAS, true)) == -1){
			pCntx->Pc = EntryPc;
			return ValueNewError(pValue, MASSSTROREERR, g_SufixErrorMsg.GetBuffer());
		}
  	ProgramNew(&newProg);
		ProgramSetName(&newProg, FileName.GetBuffer());																						// set new program name
		if(ProgramGet(&newProg, FileIndex, pValue)){																							// save new statements into program space
			pCntx->Pc = EntryPc;
			return pValue;
		}
		g_pMassStorage->Close(FileIndex);
		if(LoadLine > 0){
			ProgramDelete(&pRunCntx->Program, LoadLine);																						// delete existing statements from load line onwards
			ProgramRenumber(&newProg, LoadLine, 10);																								// renumber link statements
			ProgramMerge(&pRunCntx->Program, &newProg, pValue);																			// merg the two
		}
		else{
			ProgramDestroy(&pRunCntx->Program);																											// destroy previous statements if any
			pRunCntx->Program = newProg;																														// make the new link statements current
		}
		g_pLocalRegistry = pBaseLocalRegistry;																										// get base table
		pRunCntx->pSubCntx = pMainCntx;																														// make sure the programme subcontext is set correctly 
		DestroyLabels(pRunCntx->pSubCntx->pLabels);																								// we don't need the labels
		CleanStack(&pRunCntx->Stack);
		pRunCntx->WaitingInput = false;
		pRunCntx->IsAssign = false;
		pRunCntx->Program.Unsaved = FALSE;
		SetMainTitle(FileName.GetBuffer()); 
		if(!InDirectMode()){																																			// at this point pCntx & pRunCntx will be the same
			if(RunLine > 0){																																				// run from line number
				if(ProgramGoLine(&pCntx->Program, RunLine, &EntryPc) == nullptr) return ValueNewError(pValue, NOSUCHLINE);
				if(ProgramBeginning(&pCntx->Program, &EntryPc) == nullptr)	return ValueNewError(pValue, NOPROGRAM);
			}
			if(CompileProgram(pValue, 1)->Type == eVType::V_ERROR) return pValue;										// we now compile the programme for the first time after loading
			pCntx->Pc = EntryPc;																																		// set the start line
			pCntx->pSubCntx->CurrentData = pCntx->pSubCntx->BeginData;															// initialise the data stack pointer
			ResetVariables(g_pLocalRegistry);																												// get ready to run the programme 
			g_pMassStorage->CloseAll();																															// close all files
		}
	}
	else ValueDestroy(pValue);
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////

pValue_s stmt_LIST(pValue_s pValue)
{
PC_s FromPc, ToPc;
int From = 0, To = 0;
pContext_s	pRunContext = &SystemContexts[(int)eCntxType::RUN];

//	DevIndex = (pCntx->Pc.pToken->Type == T_LLIST ? SC_EXTERN_PRINTER : SC_TEXT_DISPLAY);
	++pCntx->Pc.pToken;
	if(pCntx->Pc.pToken->Type == T_INTNUM){
		if(pCntx->Pass == ePass::INTERPRET){
			if(ProgramFromLine(&pRunContext->Program, pCntx->Pc.pToken->Obj.Integer, &FromPc) == nullptr) return ValueNewError(pValue, NOSUCHLINE);
			From = 1;
		}
		++pCntx->Pc.pToken;
	}
	else if(pCntx->Pc.pToken->Type != T_MINUS && pCntx->Pc.pToken->Type != T_COMMA){
		if(Evaluate(pValue, nullptr)){
			if(pValue->Type == eVType::V_ERROR || (pCntx->Pass != ePass::DECLARE && ValueRetype(pValue, eVType::V_INT)->Type == eVType::V_ERROR)) return pValue;
			if(pCntx->Pass == ePass::INTERPRET){
				if(ProgramFromLine(&pRunContext->Program, pValue->uData.Integer, &FromPc) == nullptr) return ValueNewError(pValue, NOSUCHLINE);
				From = 1;
			}
			ValueDestroy(pValue);
		}
	}
	if(pCntx->Pc.pToken->Type == T_MINUS || pCntx->Pc.pToken->Type == T_COMMA){
		++pCntx->Pc.pToken;
		if(Evaluate(pValue, nullptr)){
			if(pValue->Type == eVType::V_ERROR || (pCntx->Pass != ePass::DECLARE && ValueRetype(pValue, eVType::V_INT)->Type == eVType::V_ERROR)) return pValue;
			if(pCntx->Pass == ePass::INTERPRET){
				if(ProgramToLine(&pRunContext->Program, pValue->uData.Integer, &ToPc) == nullptr) return ValueNewError(pValue, NOSUCHLINE);
				To = 1;
			}
			ValueDestroy(pValue);
		}
	}
	else if(From == 1){
		ToPc = FromPc;
		To = 1;
	}
	if(pCntx->Pass == ePass::INTERPRET) if(ProgramList(&pRunContext->Program, -1, From ? &FromPc : nullptr, To ? &ToPc : nullptr, pValue)) return pValue;	
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////

pValue_s stmt_LOAD(pValue_s pValue)
{
Context_s	*pRunCntx = &SystemContexts[(int)eCntxType::RUN];
BString FileName;

	if(pCntx->Pass == ePass::INTERPRET && !InDirectMode()) return ValueNewError(pValue, NOTINPROGRAMMODE);
	++pCntx->Pc.pToken;
	PC_s EntryPc = pCntx->Pc;
	if(Evaluate(pValue, _T("file name"))->Type == eVType::V_ERROR || ValueRetype(pValue, eVType::V_STRING)->Type == eVType::V_ERROR){
		pCntx->Pc = EntryPc;
		return pValue;
	}
	FileName.AppendString(pValue->uData.pString);
	if(pCntx->Pass == ePass::INTERPRET){
		int FileIndex;
		BasicNew();
		ProgramSetName(&pRunCntx->Program, pValue->uData.pString->GetBuffer());
		if((FileIndex = g_pMassStorage->ReadOpen(FileName.GetBuffer(), MT_PROG, false)) == -1){
			pCntx->Pc = EntryPc;
			ValueDestroy(pValue);
			return ValueNewError(pValue, MASSSTROREERR, g_SufixErrorMsg.GetBuffer());
		}
		ValueDestroy(pValue);
		if(ProgramLoad(&pRunCntx->Program, FileIndex, pValue)){
			pCntx->Pc = EntryPc;
			return pValue;
		}
		g_pMassStorage->Close(FileIndex);
		pRunCntx->Program.Unsaved = FALSE;
		SetMainTitle(FileName.GetBuffer()); 
	}
	else ValueDestroy(pValue);
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////

pValue_s stmt_LOCAL(pValue_s pValue)
{
PC_s *pCurrentFunction = nullptr;

	if(pCntx->Pass == ePass::DECLARE || pCntx->Pass == ePass::COMPILE) if((pCurrentFunction = FindMarker(eMType::FUNC)) == nullptr) return ValueNewError(pValue, STRAYLOCAL);
	++pCntx->Pc.pToken;
	while(1){
		if(pCntx->Pc.pToken->Type != T_IDENTIFIER) return ValueNewError(pValue, MISSINGVARIDENT);
		if(pCntx->Pass == ePass::DECLARE || pCntx->Pass == ePass::COMPILE){
			Symbol_s *fnsym;
			if(StackVariable(&pCntx->Stack, pCntx->Pc.pToken->Obj.pIdentifier) == 0) return ValueNewError(pValue, ALREADYLOCAL);
			if(pCntx->Pass == ePass::DECLARE){
				assert((pCurrentFunction->pToken->Type == T_DEFFN) || (pCurrentFunction->pToken->Type == T_SUB));
				fnsym = (pCurrentFunction->pToken + 1)->Obj.pIdentifier->pSym;
				assert(fnsym);
				if((fnsym->uType.sSubrtn.uFunc.sUser.pLocalTypes = (eVType*)realloc(fnsym->uType.sSubrtn.uFunc.sUser.pLocalTypes, sizeof(enum eVType) * (fnsym->uType.sSubrtn.uFunc.sUser.LocalLength + 1))) == nullptr) return ValueNewError(pValue, OUTOFMEMORY);;
				fnsym->uType.sSubrtn.uFunc.sUser.pLocalTypes[fnsym->uType.sSubrtn.uFunc.sUser.LocalLength] = pCntx->Pc.pToken->Obj.pIdentifier->DefType;
				++fnsym->uType.sSubrtn.uFunc.sUser.LocalLength;
			}
		}
		++pCntx->Pc.pToken;
		if(pCntx->Pc.pToken->Type == T_COMMA) ++pCntx->Pc.pToken;
		else break;
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////

pValue_s stmt_LOCK_UNLOCK(pValue_s pValue)
{
int Lock = pCntx->Pc.pToken->Type == T_LOCK;
int File;

	++pCntx->Pc.pToken;
	if(pCntx->Pc.pToken->Type == T_CHANNEL) ++pCntx->Pc.pToken;
	if(Evaluate(pValue, _T("stream"))->Type == eVType::V_ERROR || ValueRetype(pValue, eVType::V_INT)->Type == eVType::V_ERROR) return pValue;
	File = pValue->uData.Integer;
	ValueDestroy(pValue);
	if((pCntx->Pass == ePass::INTERPRET) && (g_pMassStorage->Lock(File, 0, 0, Lock ? FS_LOCK_EXCLUSIVE : FS_LOCK_NONE, 1) == -1)) return ValueNewError(pValue, MASSSTROREERR, g_SufixErrorMsg.GetBuffer());
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////

pValue_s stmt_LOOP(pValue_s pValue)
{
PC_s LoopPc = pCntx->Pc;
PC_s *pMarkerPc;

	++pCntx->Pc.pToken;
	if(pCntx->Pass == ePass::INTERPRET)	pCntx->Pc = LoopPc.pToken->Obj.DoPc;
	if(pCntx->Pass == ePass::DECLARE || pCntx->Pass == ePass::COMPILE){
		if((pMarkerPc = PopMarker(eMType::DO)) == nullptr && (pMarkerPc = PopMarker(eMType::DO_CND)) == nullptr) return ValueNewError(pValue, STRAYLOOP);
		LoopPc.pToken->Obj.DoPc = *pMarkerPc;
		pMarkerPc->pToken->Obj.ExitDoPc = pCntx->Pc;
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////

pValue_s stmt_LOOPUNTIL(pValue_s pValue)
{
PC_s LoopUntilPc = pCntx->Pc;
PC_s *pMarkerPc;

	++pCntx->Pc.pToken;
	if(Evaluate(pValue, _T("condition"))->Type == eVType::V_ERROR) return pValue;
	if(pCntx->Pass == ePass::INTERPRET){
		if(ValueIsNull(pValue)) pCntx->Pc = LoopUntilPc.pToken->Obj.DoPc;
		ValueDestroy(pValue);
	}
	if(pCntx->Pass == ePass::DECLARE || pCntx->Pass == ePass::COMPILE){
		if((pMarkerPc = PopMarker(eMType::DO)) == nullptr) return ValueNewError(pValue, STRAYLOOPUNTIL);
		LoopUntilPc.pToken->Obj.UntilPc = *pMarkerPc;
		pMarkerPc->pToken->Obj.ExitDoPc = pCntx->Pc;
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////

pValue_s stmt_LSET_RSET(pValue_s pValue)
{
pValue_s pVal;
PC_s ErrorPc;
int lSet = (pCntx->Pc.pToken->Type == T_LSET);

	++pCntx->Pc.pToken;
	if((pCntx->Pass == ePass::DECLARE) && (FindOrRegister(g_pLocalRegistry, pCntx->Pc.pToken->Obj.pIdentifier) == 0))	 return ValueNewError(pValue, REDECLARATION);	
	ErrorPc = pCntx->Pc;
	if((pVal = GetValue(pValue))->Type == eVType::V_ERROR) return pValue;
	if(pCntx->Pass == ePass::COMPILE && pVal->Type != eVType::V_STRING){
		pCntx->Pc = ErrorPc;
		return ValueNewError(pValue, TYPEMISMATCH4);
	}
	if(pCntx->Pc.pToken->Type != T_EQ) return ValueNewError(pValue, MISSINGEQ);
	++pCntx->Pc.pToken;
	ErrorPc = pCntx->Pc;
	if(Evaluate(pValue, _T("rhs"))->Type == eVType::V_ERROR || (pCntx->Pass != ePass::DECLARE && ValueRetype(pValue, pVal->Type)->Type == eVType::V_ERROR)){
		pCntx->Pc = ErrorPc;
		return pValue;
	}
	if(pCntx->Pass == ePass::INTERPRET) (lSet) ? pVal->uData.pString->SetLeft(pValue->uData.pString) : pVal->uData.pString->SetRight(pValue->uData.pString);
	ValueDestroy(pValue);
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////

pValue_s stmt_MASSTORAGEIS(pValue_s pValue)
{
int Device;

	++pCntx->Pc.pToken;
  if((Evaluate(pValue, _T("device"))->Type == eVType::V_ERROR) || (ValueRetype(pValue, eVType::V_STRING)->Type == eVType::V_ERROR)) return pValue;
	if((Device = FindDevice(pValue->uData.pString, eDType::MASS_STORAGE)) == -1) return ValueNewError(pValue, INVALIDDEVICE);
  ValueDestroy(pValue);
	if(pCntx->Pass == ePass::INTERPRET && MassStorageIs(Device) == -1) return ValueNewError(pValue, MASSSTROREERR, g_SufixErrorMsg.GetBuffer());
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////

pValue_s stmt_MAT(pValue_s pValue)
{
Var_s *pVar1, *pVar2, *pVar3 = nullptr;
PC_s OperationPc;
enum TokenType_e TokenType = T_EOL;

	OperationPc.Index = -1;
	OperationPc.pToken = nullptr;
	++pCntx->Pc.pToken;
	if(pCntx->Pc.pToken->Type != T_IDENTIFIER) return ValueNewError(pValue, MISSINGMATIDENT);
	if(pCntx->Pass == ePass::DECLARE && FindIdentifier(g_pLocalRegistry, pCntx->Pc.pToken->Obj.pIdentifier, eSymType::LOCALARRAY) == 0) return ValueNewError(pValue, REDECLARATION);
	pVar1 = &pCntx->Pc.pToken->Obj.pIdentifier->pSym->uType.Var;
	if(pCntx->Pass == ePass::INTERPRET) g_Trace.pIdentifier = pCntx->Pc.pToken->Obj.pIdentifier;
	++pCntx->Pc.pToken;
	if(pCntx->Pc.pToken->Type != T_EQ) return ValueNewError(pValue, MISSINGEQ);
	++pCntx->Pc.pToken;
	if(pCntx->Pc.pToken->Type == T_IDENTIFIER){																																			// a = b [ +|-|* c ] 
		double Factor = 0;
		if(pCntx->Pass == ePass::COMPILE)	if(((pCntx->Pc.pToken + 1)->Type == T_OP || StackFind(&pCntx->Stack, pCntx->Pc.pToken->Obj.pIdentifier) == 0) && FindIdentifier(g_pLocalRegistry, pCntx->Pc.pToken->Obj.pIdentifier, eSymType::LOCALARRAY) == 0) return ValueNewError(pValue, UNDECLARED);
		pVar2 = &pCntx->Pc.pToken->Obj.pIdentifier->pSym->uType.Var;
		if((pCntx->Pass == ePass::INTERPRET) && (pVar2->Dimensions != 1) && (pVar2->Dimensions != 2)) return ValueNewError(pValue, NOMATRIX, pVar2->Dimensions);
		if(pCntx->Pass == ePass::COMPILE && ValueCommonType[(int)pVar1->Type][(int)pVar2->Type] == eVType::V_ERROR) return ValueNewTypeError(pValue, pVar2->Type, pVar1->Type);
		++pCntx->Pc.pToken;
		if(pCntx->Pc.pToken->Type == T_PLUS || pCntx->Pc.pToken->Type == T_MINUS || pCntx->Pc.pToken->Type == T_MULT){
			OperationPc = pCntx->Pc;
			TokenType = pCntx->Pc.pToken->Type;
			++pCntx->Pc.pToken;
			if(pCntx->Pc.pToken->Type == T_IDENTIFIER){
				if(pCntx->Pass == ePass::COMPILE) if(((pCntx->Pc.pToken + 1)->Type == T_OP || StackFind(&pCntx->Stack, pCntx->Pc.pToken->Obj.pIdentifier) == 0) && FindIdentifier(g_pLocalRegistry, pCntx->Pc.pToken->Obj.pIdentifier, eSymType::LOCALARRAY) == 0) return ValueNewError(pValue, UNDECLARED);
				pVar3 = &pCntx->Pc.pToken->Obj.pIdentifier->pSym->uType.Var;
				if((pCntx->Pass == ePass::INTERPRET) && (pVar3->Dimensions != 1) && (pVar3->Dimensions != 2)) return ValueNewError(pValue, NOMATRIX, pVar3->Dimensions);
				++pCntx->Pc.pToken;
			}
			else{
				if(pCntx->Pc.pToken->Type == T_OP){
					++pCntx->Pc.pToken;
					if((Evaluate(pValue, _T("factor"))->Type == eVType::V_ERROR) || (ValueRetype(pValue, eVType::V_REAL)->Type == eVType::V_ERROR))return pValue;
					if(pCntx->Pass == ePass::COMPILE && ValueCommonType[(int)pVar1->Type][(int)pValue->Type] == eVType::V_ERROR) return ValueNewTypeError(pValue, pVar1->Type, pValue->Type);
					if(pCntx->Pc.pToken->Type != T_CP){
						ValueDestroy(pValue);
						return ValueNewError(pValue, MISSINGCP);
					}
					OperationPc = pCntx->Pc;
					Factor = pValue->uData.Real;
					ValueDestroy(pValue);
					++pCntx->Pc.pToken;
				}
				else return ValueNewError(pValue, MISSINGARRIDENT);
			}
		}
		if(pCntx->Pass != ePass::DECLARE){
			if(pVar3 == nullptr){																																						// only 2 matracies?
				if(Factor == 0){																																							// check for 3rd argument
					if(VarMatAssign(pVar1, pVar2, pValue, pCntx->Pass == ePass::INTERPRET)){										// MAT assignment
						assert(OperationPc.Index != -1);
						pCntx->Pc = OperationPc;
						return pValue;
					}
				}
				else{
					Value_s	Val;
					NewRealValue(&Val, Factor);
					if(TokenType == T_MULT){
						if(VarMatScalarMultiply(pVar1, pVar2, &Val, pCntx->Pass == ePass::INTERPRET)){						// Scalar multiply
							assert(OperationPc.Index != -1);
							pCntx->Pc = OperationPc;
							return pValue;
						}
					}
					else{
						if(VarMatScalarAddSub(pVar1, pVar2, &Val, TokenType == T_PLUS, pValue, pCntx->Pass == ePass::INTERPRET)){
							assert(OperationPc.Index != -1);
							pCntx->Pc = OperationPc;
							return pValue;
						}
					}
					ValueDestroy(&Val);
				}
			}
			else{
				if(TokenType == T_MULT){
					if(VarMatMultiply(pVar1, pVar2, pVar3, pValue, pCntx->Pass == ePass::INTERPRET)){
						assert(OperationPc.Index != -1);
						pCntx->Pc = OperationPc;
						return pValue;
					}
				}
				else{
					if(VarMatAddSub(pVar1, pVar2, pVar3, TokenType == T_PLUS, pValue, pCntx->Pass == ePass::INTERPRET)){
						assert(OperationPc.Index != -1);
						pCntx->Pc = OperationPc;
						return pValue;
					}
				}
			}
			if(pCntx->Pass == ePass::INTERPRET) ProgramTraceMat(&pCntx->Program, nullptr);
		}
	}
	else if(pCntx->Pc.pToken->Type == T_OP){
		if((int)pVar1->Type == T_STRING) return ValueNewError(pValue, TYPEMISMATCH5);
		++pCntx->Pc.pToken;
		if(Evaluate(pValue, _T("factor"))->Type == eVType::V_ERROR) return pValue;
		if(pCntx->Pass == ePass::COMPILE && ValueCommonType[(int)pVar1->Type][(int)pValue->Type] == eVType::V_ERROR) return ValueNewTypeError(pValue, pVar1->Type, pValue->Type);
		if(pCntx->Pc.pToken->Type != T_CP){
			ValueDestroy(pValue);
			return ValueNewError(pValue, MISSINGCP);
		}
		OperationPc = pCntx->Pc;
		if(pCntx->Pass != ePass::DECLARE){
			VarMatScalarSet(pVar1, pValue, pCntx->Pass == ePass::INTERPRET);
		}
		ValueDestroy(pValue);
		++pCntx->Pc.pToken;
	}
	else if(pCntx->Pc.pToken->Type == T_CON || pCntx->Pc.pToken->Type == T_ZER || pCntx->Pc.pToken->Type == T_IDN){
		TokenType = pCntx->Pc.pToken->Type;
		if(pCntx->Pass == ePass::COMPILE && ValueCommonType[(int)pVar1->Type][(int)eVType::V_INT] == eVType::V_ERROR) return ValueNewTypeError(pValue, eVType::V_INT, pVar1->Type);
		++pCntx->Pc.pToken;
		if(pCntx->Pc.pToken->Type == T_OP){
			int Dim;
			Geometry_s Geometry[DEF_MAX_DIMENSIONS];
			enum eVType VarType = pVar1->Type;
			++pCntx->Pc.pToken;
			if(EvaluateGeometry(pValue, &Dim, Geometry)) return pValue;
			if(pCntx->Pass == ePass::INTERPRET){
				VarDestroy(pVar1);
				VarNew(pVar1, VarType, Dim, Geometry);
			}
		}
		if(pCntx->Pass == ePass::INTERPRET){
			int i, j, c = -1;
			if((TokenType == T_CON) || (TokenType == T_ZER)){																					// T_CON & T_ZER
				int c = (TokenType == T_CON) ? 1 : 0;
				for(i = 0; i < pVar1->Size; ++i){
					if(pVar1->Type == eVType::V_INT) pVar1->pValue[i].uData.Integer = c;
					else pVar1->pValue[i].uData.Real = (double)c;
				}
				if(pCntx->Pass == ePass::INTERPRET) ProgramTraceMat(&pCntx->Program, (TokenType == T_CON) ? "CON" : "ZER");
			}
			else{																																											// T_IDN
				if(pVar1->Dimensions == 1){
					for(i = 0; i < pVar1->pGeometry[0].Size; ++i){
						int c = (i == 0 ? 1 : 0); 
						if(pVar1->Type == eVType::V_INT) pVar1->pValue[i].uData.Integer = c;
						else pVar1->pValue[i].uData.Real = (double)c;
					}
				}
				else{
					if(pVar1->Dimensions != 2) return ValueNewError(pValue, NOMATRIX, pVar1->Dimensions);		// must be a vector matrix
					for(i = 0; i < pVar1->pGeometry[0].Size; ++i){
						for(j = 0; j < pVar1->pGeometry[1].Size; ++j){
							ValueDestroy(&(pVar1->pValue[i * pVar1->pGeometry[1].Size + j]));
							int c = (i == j ? 1 : 0);
							if(pVar1->Type == eVType::V_INT) pVar1->pValue[i * pVar1->pGeometry[1].Size + j].uData.Integer = c;
							else pVar1->pValue[i * pVar1->pGeometry[1].Size + j].uData.Real = (double)c;
						}
					}
				}
				if(pCntx->Pass == ePass::INTERPRET) ProgramTraceMat(&pCntx->Program, "IDN");
			}
		}
	}
	else if(pCntx->Pc.pToken->Type == T_TRN || pCntx->Pc.pToken->Type == T_INV){
		TokenType = pCntx->Pc.pToken->Type;
		++pCntx->Pc.pToken;
		if(pCntx->Pc.pToken->Type != T_OP) return ValueNewError(pValue, MISSINGOP);
		++pCntx->Pc.pToken;
		if(pCntx->Pc.pToken->Type != T_IDENTIFIER) return ValueNewError(pValue, MISSINGMATIDENT);
		if(pCntx->Pass == ePass::COMPILE)	if(((pCntx->Pc.pToken + 1)->Type == T_OP || StackFind(&pCntx->Stack, pCntx->Pc.pToken->Obj.pIdentifier) == 0) && FindIdentifier(g_pLocalRegistry, pCntx->Pc.pToken->Obj.pIdentifier, eSymType::LOCALARRAY) == 0) return ValueNewError(pValue, UNDECLARED);
		pVar2 = &pCntx->Pc.pToken->Obj.pIdentifier->pSym->uType.Var;
		if(pCntx->Pass == ePass::COMPILE && ValueCommonType[(int)pVar1->Type][(int)pVar2->Type] == eVType::V_ERROR) return ValueNewTypeError(pValue, pVar2->Type, pVar1->Type);
		if(pCntx->Pass == ePass::INTERPRET)	if(pVar2->Dimensions != 2) return ValueNewError(pValue, NOMATRIX, pVar2->Dimensions);
		switch(TokenType){
			case T_TRN: VarMatTranspose(pVar1, pVar2); break;
			case T_INV: if(VarMatInverse(pVar1, pVar2, &pCntx->Stack.LastDet, pValue)) return pValue;break;
			default: assert(0);
		}
		if(pCntx->Pass == ePass::INTERPRET) ProgramTraceMat(&pCntx->Program, (TokenType == T_TRN) ? "TRN" : "INV");
		++pCntx->Pc.pToken;
		if(pCntx->Pc.pToken->Type != T_CP) return ValueNewError(pValue, MISSINGCP);
		++pCntx->Pc.pToken;
	}
	else return ValueNewError(pValue, MISSINGEXPR, _T("matrix"));
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////

pValue_s stmt_MATINPUT(pValue_s pValue)
{
PC_s PausePC = pCntx->Pc, LValuePc;
int Dim;
Geometry_s Geometry[DEF_MAX_DIMENSIONS];
Var_s *pVar;

	++pCntx->Pc.pToken;
	while(1){
		LValuePc = pCntx->Pc;
		if(pCntx->Pc.pToken->Type != T_IDENTIFIER) return ValueNewError(pValue, MISSINGMATIDENT);
		Identifier_s *pIdentifier = pCntx->Pc.pToken->Obj.pIdentifier;
		if((pCntx->Pass == ePass::DECLARE) && (StackFind(&pCntx->Stack, pIdentifier) == 0) && (FindIdentifier(g_pLocalRegistry, pIdentifier, LibSearchType((pCntx->Pc.pToken + 1)->Type, pIdentifier->DefType)) == 0)) return ValueNewError(pValue, REDECLARATION);
		pVar = &pCntx->Pc.pToken->Obj.pIdentifier->pSym->uType.Var;
		++pCntx->Pc.pToken;
		if(pCntx->Pc.pToken->Type == T_OP){
			enum eVType VarType = pVar->Type;
 			++pCntx->Pc.pToken;
			if(EvaluateGeometry(pValue, &Dim, Geometry)) return pValue;
			if(pCntx->Pass == ePass::INTERPRET){
				VarDestroy(pVar);
				VarNew(pVar, VarType, Dim, Geometry);
			}
		}
		if(pCntx->Pass == ePass::INTERPRET){
			int i, j;
			if(pVar->Dimensions != 1 && pVar->Dimensions != 2) return ValueNewError(pValue, NOMATRIX, pVar->Dimensions);
			for(i = 0; i < pVar->pGeometry[0].Size; ++i){
				BString Str;
				Token_s *pToken, *pInDataToken;
				g_pPrinter->PrintText(eDispArea::PROMPT, _T("?"));
				if(!GetInput(&Str)){
					if((pCntx->RunMode == eRunMode::STOP) || (pCntx->RunMode == eRunMode::HALT)) pCntx->Pc = PausePC;
					if(pCntx->RunMode == eRunMode::STOP) return ValueNewError(pValue, BREAK);
					pCntx->Stack.Resumeable = 1;																// make it resumable
					return ValueNewError(pValue, PAUSE);
				}
				if(Str.GetLength() == 0) return ValueNewError(pValue, IOERROR, _T("No input"));	
				pInDataToken = pToken = TokeniseData(Str.GetBuffer());
				if(pVar->Dimensions == 1){
					if(pToken->Type == T_COMMA || pToken->Type == T_EOL){
						ValueDestroy(&(pVar->pValue[i]));
						ValueNewNull(&(pVar->pValue[i]), pVar->Type);
					}
					else if(Convert(pValue, &(pVar->pValue[i]), pToken)){
						TokenDestroy(pInDataToken);
						pCntx->Pc = LValuePc;
						return pValue;
					}
					else ++pToken;
					if(pToken->Type == T_COMMA) ++pToken;
				}
				else{
					for(j = 0; j<pVar->pGeometry[1].Size; ++j){
						if(pToken->Type == T_COMMA || pToken->Type == T_EOL){
							ValueDestroy(&(pVar->pValue[i * pVar->pGeometry[1].Size + j]));
							ValueNewNull(&(pVar->pValue[i * pVar->pGeometry[1].Size + j]), pVar->Type);
						}
						else if(Convert(pValue, &(pVar->pValue[i * pVar->pGeometry[1].Size + j]), pToken)){
							TokenDestroy(pInDataToken);
							pCntx->Pc = LValuePc;
							return pValue;
						}
						else ++pToken;
						if(pToken->Type == T_COMMA) ++pToken;
					}
				}
				TokenDestroy(pInDataToken);
			}
		}
		if(pCntx->Pc.pToken->Type == T_COMMA) ++pCntx->Pc.pToken;
		else break;
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////

pValue_s stmt_MATPRINT(pValue_s pValue)
{
int File = SC_TEXT_DISPLAY;
int PrintUsing = 0;
Value_s UsingValue;
BString *pUsingStr = nullptr;
UINT UsingPos = 0;
int NotFirst = 0;

	++pCntx->Pc.pToken;
	if(pCntx->Pc.pToken->Type == T_CHANNEL){
		++pCntx->Pc.pToken;
		if(Evaluate(pValue, _T("stream"))->Type == eVType::V_ERROR || ValueRetype(pValue, eVType::V_INT)->Type == eVType::V_ERROR) return pValue;
		File = pValue->uData.Integer;
		ValueDestroy(pValue);
		if(pCntx->Pc.pToken->Type == T_COMMA) ++pCntx->Pc.pToken;
	}
	if(pCntx->Pc.pToken->Type == T_USING){
		PC_s ImagePc;

		ImagePc = pCntx->Pc;
		PrintUsing = 1;
		++pCntx->Pc.pToken;
		if(pCntx->Pc.pToken->Type == T_INTNUM){
			if(pCntx->Pass == ePass::COMPILE && ProgramImageLine(&pCntx->Program, pCntx->Pc.pToken->Obj.Integer, &ImagePc.pToken->Obj.ImagePc) == nullptr) return ValueNewError(pValue, NOSUCHIMAGELINE);
			else if(pCntx->Pass == ePass::INTERPRET) pUsingStr = ImagePc.pToken->Obj.ImagePc.pToken->Obj.pString;
			ValueNewString(&UsingValue);
			++pCntx->Pc.pToken;
		}
		else{
			if(Evaluate(&UsingValue, _T("format string"))->Type == eVType::V_ERROR || ValueRetype(&UsingValue, eVType::V_STRING)->Type == eVType::V_ERROR){
				*pValue = UsingValue;
				return pValue;
			}
			pUsingStr = UsingValue.uData.pString;
		}
		if(pCntx->Pc.pToken->Type != T_SEMICOLON){
			ValueDestroy(&UsingValue);
			return ValueNewError(pValue, MISSINGSEMICOLON);
		}
		++pCntx->Pc.pToken;
	}
	else{
		ValueNewString(&UsingValue);
		pUsingStr = UsingValue.uData.pString;
	}
	while(1){
		Var_s *pVar;
		int zoned = 1;
		if(pCntx->Pc.pToken->Type != T_IDENTIFIER){
			if(NotFirst) break;
			ValueDestroy(&UsingValue);
			return ValueNewError(pValue, MISSINGMATIDENT);
		}
		Identifier_s *pIdentifier = pCntx->Pc.pToken->Obj.pIdentifier;
		if((pCntx->Pass == ePass::DECLARE) && (StackFind(&pCntx->Stack, pIdentifier) == 0) && (FindIdentifier(g_pLocalRegistry, pIdentifier, LibSearchType((pCntx->Pc.pToken + 1)->Type, pIdentifier->DefType)) == 0)){
			ValueDestroy(&UsingValue);
			return ValueNewError(pValue, REDECLARATION);
		}
		pVar = &pCntx->Pc.pToken->Obj.pIdentifier->pSym->uType.Var;
		++pCntx->Pc.pToken;
		if(pCntx->Pc.pToken->Type == T_SEMICOLON) zoned = 0;
		if(pCntx->Pass == ePass::INTERPRET){
			int i, j;
			int Unused = 0;
			int g0, g1;
			if((pVar->Dimensions != 1) && (pVar->Dimensions != 2)) return ValueNewError(pValue, NOMATRIX, pVar->Dimensions);
			if((NotFirst ? g_pPrinter->PrintChars("\n") : g_pPrinter->PrintChars("\n")) == -1){
				ValueDestroy(&UsingValue);
				return ValueNewError(pValue, PRINTERERROR);
			}
			g0 = pVar->pGeometry[0].Size;
			g1 = pVar->Dimensions == 1 ? Unused + 1 : pVar->pGeometry[1].Size;
			for(i = Unused; i < g0; ++i){
				for(j = Unused; j < g1; ++j){
					BString PrintStr;
					ValueClone(pValue, &(pVar->pValue[pVar->Dimensions == 1 ? i : i * g1 + j]));
					if(ValueToStringUsing(pValue, &PrintStr, pUsingStr, &UsingPos)->Type == eVType::V_ERROR){
						ValueDestroy(&UsingValue);
						return pValue;
					}
					ValueDestroy(pValue);
					if(g_pPrinter->PrintString(&PrintStr) == -1){
						ValueDestroy(&UsingValue);
						return ValueNewError(pValue, PRINTERERROR);
					}
					if(!PrintUsing && zoned) g_pPrinter->NextTab();
				}
				if(g_pPrinter->PrintChars("\n") == -1) return ValueNewError(pValue, PRINTERERROR);
			}
		}
		if(pCntx->Pc.pToken->Type == T_COMMA || pCntx->Pc.pToken->Type == T_SEMICOLON) ++pCntx->Pc.pToken;
		else break;
		NotFirst = 1;
	}
	ValueDestroy(&UsingValue);
	if(pCntx->Pass == ePass::INTERPRET) if(g_pPrinter->Flush() == -1) return ValueNewError(pValue, PRINTERERROR);
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////

pValue_s stmt_MATREAD(pValue_s pValue)
{
	++pCntx->Pc.pToken;
	while(1){
		PC_s LValuePc;
		Var_s *pVar;
		LValuePc = pCntx->Pc;
		if(pCntx->Pc.pToken->Type != T_IDENTIFIER) return ValueNewError(pValue, MISSINGMATIDENT);
		if((pCntx->Pass == ePass::INTERPRET) && FindIdentifier(g_pLocalRegistry, pCntx->Pc.pToken->Obj.pIdentifier, eSymType::LOCALARRAY) == 0) return ValueNewError(pValue, REDECLARATION);
		pVar = &pCntx->Pc.pToken->Obj.pIdentifier->pSym->uType.Var;
		if(pCntx->Pass == ePass::INTERPRET) g_Trace.pIdentifier = pCntx->Pc.pToken->Obj.pIdentifier;
		++pCntx->Pc.pToken;
		if(pCntx->Pc.pToken->Type == T_OP){
			int Dim;
			Geometry_s Geometry[DEF_MAX_DIMENSIONS];
			if(EvaluateGeometry(pValue, &Dim, Geometry)) return pValue;
			if(pCntx->Pass == ePass::INTERPRET && VarMatRedim(pVar, Dim, Geometry, pValue) != nullptr) return pValue;
		}
		if(pCntx->Pass == ePass::INTERPRET){
			for(int i = 0; i < pVar->Size; ++i){
				if(DataRead(pValue, &pVar->pValue[i])){
					pCntx->Pc = LValuePc;
					return pValue;
				}
			}
			ProgramTraceMat(&pCntx->Program, nullptr);
		}
		if(pCntx->Pc.pToken->Type == T_COMMA) ++pCntx->Pc.pToken;
		else break;
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////

pValue_s stmt_NAME(pValue_s pValue)
{
PC_s namepc = pCntx->Pc;
Value_s OldValue;
int Result = -1, ResultErrorNumber = -1;

	++pCntx->Pc.pToken;
	if(Evaluate(pValue, _T("file name"))->Type == eVType::V_ERROR || ValueRetype(pValue, eVType::V_STRING)->Type == eVType::V_ERROR) return pValue;
	if(pCntx->Pc.pToken->Type != T_AS){
		ValueDestroy(pValue);
		return ValueNewError(pValue, MISSINGAS);
	}
	OldValue = *pValue;
	++pCntx->Pc.pToken;
	if(Evaluate(pValue, _T("file name"))->Type == eVType::V_ERROR || ValueRetype(pValue, eVType::V_STRING)->Type == eVType::V_ERROR){
		ValueDestroy(&OldValue);
		return pValue;
	}
	if(pCntx->Pass == ePass::INTERPRET){
		Result = rename(OldValue.uData.pString->GetBuffer(), pValue->uData.pString->GetBuffer());
		ResultErrorNumber = errno;
	}
	ValueDestroy(&OldValue);
	ValueDestroy(pValue);
	if(pCntx->Pass == ePass::INTERPRET && Result == -1){
		pCntx->Pc = namepc;
		char Buff[MSG_BUFFER_SIZE];
		strerror_s(Buff, MSG_BUFFER_SIZE, ResultErrorNumber);
		g_SufixErrorMsg.AppendChars(Buff);
		return ValueNewError(pValue, IOERROR, g_SufixErrorMsg.GetBuffer());
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////

pValue_s stmt_NEXT(pValue_s pValue)
{
Next_s **ppNext = &pCntx->Pc.pToken->Obj.pNext;
int Level = 0;

	if(pCntx->Pass == ePass::INTERPRET){
		pValue_s pLValue;
		Value_s IncValue;
		PC_s SavePc;
		++pCntx->Pc.pToken;
		while(1){
			SavePc = pCntx->Pc;
			pCntx->Pc = (*ppNext)[Level].Var;			/* get variable GetValue */
			if((pLValue = GetValue(pValue))->Type == eVType::V_ERROR) return pValue;
			pCntx->Pc = SavePc;
			SavePc = pCntx->Pc;
			pCntx->Pc = (*ppNext)[Level].Limit;			/* get LimitVal pValue and increment */
			if(Evaluate(pValue, _T("limit"))->Type == eVType::V_ERROR) return pValue;
			ValueRetype(pValue, pLValue->Type);
			assert(pValue->Type != eVType::V_ERROR);
			if(pCntx->Pc.pToken->Type == T_STEP){
				++pCntx->Pc.pToken;
				if(Evaluate(&IncValue, _T("step"))->Type == eVType::V_ERROR){
					ValueDestroy(pValue);
					*pValue = IncValue;
					return pValue;
				}
			}
			else NewIntegerValue(&IncValue, 1);
			DoValueRetype(&IncValue, pLValue->Type);
			assert(IncValue.Type != eVType::V_ERROR);
			pCntx->Pc = SavePc;
			ValueAdd(pLValue, &IncValue, 1);
			if(ValueExitFor(pLValue, pValue, &IncValue)){
				ValueDestroy(pValue);
				ValueDestroy(&IncValue);
				if(pCntx->Pc.pToken->Type == T_IDENTIFIER){
					if(GetValue(pValue)->Type == eVType::V_ERROR) return pValue;
					if(pCntx->Pc.pToken->Type == T_COMMA){ ++pCntx->Pc.pToken; ++Level; }else break;
				}
				else break;
			}
			else{
				pCntx->Pc = (*ppNext)[Level].Body;
				ValueDestroy(pValue);
				ValueDestroy(&IncValue);
				break;
			}
		}
	}
	else{
		PC_s *pBody;
		++pCntx->Pc.pToken;
		while(1){
			if((pBody = PopMarker(eMType::FOR_BODY)) == nullptr) return ValueNewError(pValue, STRAYNEXT, TopMarkerDescription());
			if(Level) if((*ppNext = (Next_s*)realloc(*ppNext, sizeof(Next_s) * (Level + 1))) == nullptr) return ValueNewError(pValue, OUTOFMEMORY);;
			(*ppNext)[Level].Body = *pBody;
			(*ppNext)[Level].Limit = *PopMarker(eMType::FOR_LIMIT);
			(*ppNext)[Level].Var = *PopMarker(eMType::FOR_VAR);
			(*ppNext)[Level].Frame = *PopMarker(eMType::FOR);
			if(pCntx->Pc.pToken->Type == T_IDENTIFIER){
				if(strcmp(pCntx->Pc.pToken->Obj.pIdentifier->Name, (*ppNext)[Level].Var.pToken->Obj.pIdentifier->Name)) return ValueNewError(pValue, FORMISMATCH);
				Identifier_s *pIdentifier = pCntx->Pc.pToken->Obj.pIdentifier;
				if((pCntx->Pass == ePass::DECLARE) && (StackFind(&pCntx->Stack, pIdentifier) == 0) && (FindIdentifier(g_pLocalRegistry, pIdentifier, LibSearchType((pCntx->Pc.pToken + 1)->Type, pIdentifier->DefType)) == 0)) return ValueNewError(pValue, REDECLARATION);
				if(GetValue(pValue)->Type == eVType::V_ERROR) return pValue;
				if(pCntx->Pc.pToken->Type == T_COMMA){
					++pCntx->Pc.pToken;
					++Level;
				}
				else break;
			}
			else break;
		}
		while(Level >= 0) (*ppNext)[Level--].Frame.pToken->Obj.ExitForPc = pCntx->Pc;
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////

pValue_s stmt_NUMERIC(pValue_s pValue)
{
TokenType_e NumType = pCntx->Pc.pToken->Type;

	++pCntx->Pc.pToken;																																					// define a numeric variable. T_INTEGER, T_SHORT & T_REAL
	while(1){
		if(pCntx->Pc.pToken->Type != T_IDENTIFIER) return ValueNewError(pValue, MISSINGVARIDENT);
		if(pCntx->Pass == ePass::DECLARE) pCntx->Pc.pToken->Obj.pIdentifier->DefType = eVType::V_INT;
		if((pCntx->Pc.pToken + 1)->Type != T_OP){
			if(pCntx->Pass == ePass::DECLARE && RegisterVariable(g_pLocalRegistry, pCntx->Pc.pToken->Obj.pIdentifier, pCntx->Pc.pToken->Obj.pIdentifier->DefType, eSymType::LOCALVAR, 0) == 0)	return ValueNewError(pValue, REDECLARATION);
			++pCntx->Pc.pToken;
		}
		else{
			int BoundLow, BoundHigh;
			Geometry_s Geometry[DEF_MAX_DIMENSIONS];
			if(pCntx->Pass == ePass::DECLARE){
				if(pCntx->Pc.pToken->Obj.pIdentifier->DefType == eVType::V_STRING) return ValueNewError(pValue, IMPROPERVALTYPE);
				if(NumType == T_REAL)	pCntx->Pc.pToken->Obj.pIdentifier->DefType = eVType::V_REAL;
				else pCntx->Pc.pToken->Obj.pIdentifier->DefType = eVType::V_INT;												 // including T_SHORT
				if(RegisterVariable(g_pLocalRegistry, pCntx->Pc.pToken->Obj.pIdentifier, pCntx->Pc.pToken->Obj.pIdentifier->DefType, eSymType::LOCALARRAY, 0) == 0)	return ValueNewError(pValue, REDECLARATION);
			}
			Var_s *pVar = &pCntx->Pc.pToken->Obj.pIdentifier->pSym->uType.Var;
			eVType VarType = pCntx->Pc.pToken->Obj.pIdentifier->DefType;
			++pCntx->Pc.pToken;
			if(pCntx->Pc.pToken->Type != T_OP) return ValueNewError(pValue, MISSINGOP);
			++pCntx->Pc.pToken;
			unsigned int Dim = 0;
			while(Dim < DEF_MAX_DIMENSIONS){
				PC_s ErrorPc = pCntx->Pc;
				BoundLow = pCntx->pSubCntx->OptionBase;
				BoundHigh = 0;
				if(Evaluate(pValue, _T("dimension"))->Type == eVType::V_ERROR || (pCntx->Pass != ePass::DECLARE && ValueRetype(pValue, eVType::V_INT)->Type == eVType::V_ERROR)){
					return pValue;
				}
				if(pCntx->Pass == ePass::INTERPRET){
					if(((pValue->uData.Integer != -1) || (!pVar->IsDefined)) && pValue->uData.Integer < 1){ 																	 // the dimension must be greater than 1
						ValueDestroy(pValue);
						ValueNewError(pValue, OUTOFRANGE, _T("Dimension"));
					}
				}
				if(pValue->Type == eVType::V_ERROR){ 
					pCntx->Pc = ErrorPc;
					return pValue;
				}
				BoundHigh = pValue->uData.Integer;
				ValueDestroy(pValue);
				if(pCntx->Pc.pToken->Type == T_COLON){																																				// Get the upper bounds
					++pCntx->Pc.pToken;
					if(Evaluate(pValue, _T("subscript"))->Type == eVType::V_ERROR || (pCntx->Pass != ePass::DECLARE && ValueRetype(pValue, eVType::V_INT)->Type == eVType::V_ERROR)){
						return pValue;
					}
					if(pCntx->Pass == ePass::INTERPRET && pValue->uData.Integer <= BoundHigh){																				// must be grater than the lower dimension				
						ValueDestroy(pValue);
						ValueNewError(pValue, OUTOFRANGE, _T("Subscript"));
					}
 					if(pValue->Type == eVType::V_ERROR){																																							
						pCntx->Pc = ErrorPc;
						return pValue;
					}
					BoundLow = BoundHigh;
					BoundHigh = pValue->uData.Integer;
					ValueDestroy(pValue);
				}
				if(pCntx->Pass == ePass::INTERPRET){
					if(pVar->IsDefined){
						if((pValue->uData.Integer != -1) && ((BoundHigh - BoundLow + 1) != pVar->pGeometry[Dim].Size)) return ValueNewError(pValue, REDIM);		// check subscript for wildcard and equality
					}
					else{
 						Geometry[Dim].Size = BoundHigh - BoundLow + 1;
						Geometry[Dim].Base = BoundLow;
					}
					++Dim;
				}
				if(pCntx->Pc.pToken->Type == T_COMMA) ++pCntx->Pc.pToken;
				else break;
			}
			if(pCntx->Pc.pToken->Type != T_CP){																					// abort: improper dimention count or no closing parentheses
				return ValueNewError(pValue, MISSINGCP);
			}
			++pCntx->Pc.pToken;
			if((pCntx->Pass == ePass::INTERPRET) && !pVar->IsDefined){
				Var_s NewArray;
				if(VarNew(&NewArray, VarType, Dim, Geometry) == nullptr){
					return ValueNewError(pValue, OUTOFMEMORY);
				}
				VarDestroy(pVar);
				*pVar = NewArray;
				pVar->IsDefined = true;
			}
		}
		if(pCntx->Pc.pToken->Type == T_COMMA) ++pCntx->Pc.pToken;  /* advance to next var */
		else break;
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////

pValue_s stmt_OFFKEY(pValue_s pValue)
{
int KeyNumber = 0;
PC_s ErrorPc = pCntx->Pc;

	++pCntx->Pc.pToken;
	if(pCntx->Pc.pToken->Type != T_CHANNEL){									// must have a key number
		pCntx->Pc = ErrorPc;
		return ValueNewError(pValue, SYNTAX, _T("no key specified"));
	}
	++pCntx->Pc.pToken;
	ErrorPc = pCntx->Pc;
	if(Evaluate(pValue, _T("key #"))->Type == eVType::V_ERROR || ValueRetype(pValue, eVType::V_INT)->Type == eVType::V_ERROR){
		pCntx->Pc = ErrorPc;
		return pValue;
	}
	if((pValue->uData.Integer < 1) || (pValue->uData.Integer > MAX_SOFT_KEYS)){									
		pCntx->Pc = ErrorPc;
		return ValueNewError(pValue, INVALIDVALUE, _T("key number"));
	}
	KeyNumber = pValue->uData.Integer - 1;																
	ValueDestroy(pValue);
	if(pCntx->Pass == ePass::COMPILE){
		pCntx->EventKeys[KeyNumber].Pc.Index = -1;
		pCntx->EventKeys[KeyNumber].JumpType = T_NOTOKEN;
		pCntx->EventKeys[KeyNumber].Priority = 1;
		pCntx->EventKeys[KeyNumber].State = eOnKeyState::INACTIVE;
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////

pValue_s stmt_ON(pValue_s pValue)
{
On_s *pOn = &pCntx->Pc.pToken->Obj.On;
bool IsValid = false, IsEOL = false;

	++pCntx->Pc.pToken;
	if(Evaluate(pValue, _T("selector"))->Type == eVType::V_ERROR) return pValue;
	if(ValueRetype(pValue, eVType::V_INT)->Type == eVType::V_ERROR) return pValue;
	if(pCntx->Pass == ePass::INTERPRET){
		PC_s NewPc;
		g_Trace.pLastPc = &pCntx->Pc;
		if((pValue->uData.Integer > 0) && (pValue->uData.Integer < pOn->PcLength))	NewPc = pOn->pPc[pValue->uData.Integer];
		else NewPc = pOn->pPc[0];
		if(pCntx->Pc.pToken->Type == T_GOTO) pCntx->Pc = NewPc;
		else{
			pCntx->Pc = pOn->pPc[0];
			StackPushGosubRet(&pCntx->Stack, &pCntx->Pc, -1);
			pCntx->Pc = NewPc;
		}
		ProgramTraceFlow(&pCntx->Program, &pCntx->Pc);
	}
	else if(pCntx->Pass == ePass::DECLARE || pCntx->Pass == ePass::COMPILE){
		ValueDestroy(pValue);
		if(pCntx->Pc.pToken->Type != T_GOTO && pCntx->Pc.pToken->Type != T_GOSUB) return ValueNewError(pValue, MISSINGGOTOSUB);
		++pCntx->Pc.pToken;
		pOn->PcLength = 1;
		while(!IsEOL){
			switch(pCntx->Pc.pToken->Type){
				case T_IDENTIFIER:{
					if((pOn->pPc = (PC_s*)realloc(pOn->pPc, sizeof(PC_s) * ++pOn->PcLength)) == nullptr) return ValueNewError(pValue, OUTOFMEMORY);
					if(pCntx->Pass == ePass::COMPILE){
						if(FindLabel(pCntx->pSubCntx->pLabels, pCntx->Pc.pToken->Obj.pIdentifier) == 0) return ValueNewError(pValue, MISSINGIDENTIFIER);
						ProgramGetLabelLine(&pCntx->Program, pCntx->Pc.pToken->Obj.pLabel, &pOn->pPc[pOn->PcLength - 1]);
						if(ProgramScopeCheck(&pCntx->Program, &pOn->pPc[pOn->PcLength - 1], FindMarker(eMType::FUNC))) return ValueNewError(pValue, OUTOFSCOPE);
					}
					IsValid = true;
					break;
				}
				case T_INTNUM:{
					if((pOn->pPc = (PC_s*)realloc(pOn->pPc, sizeof(PC_s) * ++pOn->PcLength)) == nullptr) return ValueNewError(pValue, OUTOFMEMORY);
					if(ProgramGoLine(&pCntx->Program, pCntx->Pc.pToken->Obj.Integer, &pOn->pPc[pOn->PcLength - 1]) == nullptr) return ValueNewError(pValue, ONNOSUCHLINE);
					if((pCntx->Pass == ePass::COMPILE) && ProgramScopeCheck(&pCntx->Program, &pOn->pPc[pOn->PcLength - 1], FindMarker(eMType::FUNC))) return ValueNewError(pValue, OUTOFSCOPE);
					IsValid = true;
					break;
				}
				case T_COMMA:{
					break;
				}
				default:{
					if(!IsValid) return ValueNewError(pValue, MISSINGLINENUMBER);
					else IsEOL = true;
				}
			}
			if(!IsEOL) ++pCntx->Pc.pToken;
		}
		pOn->pPc[0] = pCntx->Pc;
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////

pValue_s stmt_ONERROR(pValue_s pValue)
{
PC_s OnErrorPc;
TokenType_e OnErrorType;

	if(InDirectMode()) return ValueNewError(pValue, NOTINDIRECTMODE);
	++pCntx->Pc.pToken;
	OnErrorType = pCntx->Pc.pToken->Type;
	++pCntx->Pc.pToken;
	OnErrorPc = pCntx->Pc;
	switch(OnErrorType){
		case T_GOTO:
		case T_GOSUB:{
			switch(pCntx->Pc.pToken->Type){
				case T_IDENTIFIER:{
					if(pCntx->Pass != ePass::DECLARE){
						if(FindLabel(pCntx->pSubCntx->pLabels, pCntx->Pc.pToken->Obj.pIdentifier) == 0) return ValueNewError(pValue, MISSINGIDENTIFIER);
						ProgramGetLabelLine(&pCntx->Program, pCntx->Pc.pToken->Obj.pLabel, &OnErrorPc);
						if(pCntx->Pass == ePass::COMPILE)	if(ProgramScopeCheck(&pCntx->Program, &OnErrorPc, FindMarker(eMType::FUNC))) return ValueNewError(pValue, OUTOFSCOPE);
					}
					pCntx->Stack.OnErrorPc = OnErrorPc;
					break;
				}
				case T_INTNUM:{
					if(ProgramGoLine(&pCntx->Program, pCntx->Pc.pToken->Obj.Integer, &OnErrorPc) == nullptr) return ValueNewError(pValue, NOSUCHLINE);
					if((pCntx->Pass == ePass::COMPILE) && ProgramScopeCheck(&pCntx->Program, &OnErrorPc, FindMarker(eMType::FUNC))) return ValueNewError(pValue, OUTOFSCOPE);
					pCntx->Stack.OnErrorPc = OnErrorPc;
					break;
				}
				default: return ValueNewError(pValue, MISSINGLINENUMBER);
			}
			break;
		}
 		case T_CALL:{
			if(pCntx->Pc.pToken->Type != T_IDENTIFIER) return ValueNewError(pValue, MISSINGPROCIDENT);
			if(pCntx->Pass == ePass::DECLARE){
				if(Function(pValue)->Type == eVType::V_ERROR) return pValue;
				else ValueDestroy(pValue);
			}
			else{
				if(pCntx->Pass == ePass::COMPILE)	if(GlobalFind(pCntx->Pc.pToken->Obj.pIdentifier, eSymType::SUBPROGRAMME) == 0) return ValueNewError(pValue, UNDECLARED);
				pCntx->Stack.OnErrorPc = OnErrorPc;
				++pCntx->Pc.pToken;
			}
			break;
		}
		default: return ValueNewError(pValue, MISSINGGOTOSUB);
	}
	if(pCntx->Pass == ePass::INTERPRET)	pCntx->Stack.OnErrorType = OnErrorType;
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////

pValue_s stmt_ONERROROFF(pValue_s pValue)
{
	if(InDirectMode()) return ValueNewError(pValue, NOTINDIRECTMODE);
	if(pCntx->Pass == ePass::INTERPRET) pCntx->Stack.OnErrorPc.Index = -1;
	++pCntx->Pc.pToken;
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////

pValue_s stmt_ONKEY(pValue_s pValue)
{
int KeyNumber = 0, Priority = 1;
PC_s ErrorPc = pCntx->Pc;
enum TokenType_e OnKeyType;

	++pCntx->Pc.pToken;
	if(pCntx->Pc.pToken->Type != T_CHANNEL){									// must have a key number
		pCntx->Pc = ErrorPc;
		return ValueNewError(pValue, SYNTAX, _T("no key specified"));
	}
	++pCntx->Pc.pToken;
	ErrorPc = pCntx->Pc;
	if(Evaluate(pValue, _T("key #"))->Type == eVType::V_ERROR || ValueRetype(pValue, eVType::V_INT)->Type == eVType::V_ERROR){
		pCntx->Pc = ErrorPc;
		return pValue;
	}
	if((pValue->uData.Integer < 0) || (pValue->uData.Integer > MAX_SOFT_KEYS)){									
		pCntx->Pc = ErrorPc;
		return ValueNewError(pValue, INVALIDVALUE, _T("key number"));
	}
	KeyNumber = pValue->uData.Integer;																
	ValueDestroy(pValue);
	if(pCntx->Pc.pToken->Type == T_COMMA){
		++pCntx->Pc.pToken;
		if(Evaluate(pValue, _T("priority"))->Type == eVType::V_ERROR || ValueRetype(pValue, eVType::V_INT)->Type == eVType::V_ERROR) return pValue;
		Priority = pValue->uData.Integer;																											 // although priority not used
		ValueDestroy(pValue);
	}
	OnKeyType = pCntx->Pc.pToken->Type;
	++pCntx->Pc.pToken;
	PC_s OnKeyPc = pCntx->Pc;
	switch(OnKeyType){
		case T_GOTO:
		case T_GOSUB:{
			switch(pCntx->Pc.pToken->Type){
				case T_IDENTIFIER:{
					if(pCntx->Pass != ePass::DECLARE){
						if(FindLabel(pCntx->pSubCntx->pLabels, pCntx->Pc.pToken->Obj.pIdentifier) == 0) return ValueNewError(pValue, MISSINGIDENTIFIER);
						ProgramGetLabelLine(&pCntx->Program, pCntx->Pc.pToken->Obj.pLabel, &OnKeyPc);
						if(pCntx->Pass == ePass::COMPILE)	if(ProgramScopeCheck(&pCntx->Program, &OnKeyPc, FindMarker(eMType::FUNC))) return ValueNewError(pValue, OUTOFSCOPE);
					}
					pCntx->EventKeys[KeyNumber].Pc = OnKeyPc;																																// OnKeyPc points to a line number
					break;
				}
				case T_INTNUM:{
					if(ProgramGoLine(&pCntx->Program, pCntx->Pc.pToken->Obj.Integer, &OnKeyPc) == nullptr) return ValueNewError(pValue, NOSUCHLINE);
					if((pCntx->Pass == ePass::COMPILE) && ProgramScopeCheck(&pCntx->Program, &OnKeyPc, FindMarker(eMType::FUNC))) return ValueNewError(pValue, OUTOFSCOPE);
					pCntx->EventKeys[KeyNumber].Pc = OnKeyPc;																																// OnKeyPc points to a line number
					break;
				}
				default: return ValueNewError(pValue, MISSINGLINENUMBER);
			}
			++pCntx->Pc.pToken;
			break;
		}
 		case T_CALL:{
			if(pCntx->Pc.pToken->Type != T_IDENTIFIER) return ValueNewError(pValue, MISSINGPROCIDENT);
			if(pCntx->Pass == ePass::DECLARE){
				if(Function(pValue)->Type == eVType::V_ERROR) return pValue;
				else ValueDestroy(pValue);
			}
			else{
				if(pCntx->Pass == ePass::COMPILE)	if(GlobalFind(pCntx->Pc.pToken->Obj.pIdentifier, eSymType::SUBPROGRAMME) == 0) return ValueNewError(pValue, UNDECLARED);
				pCntx->EventKeys[KeyNumber].Pc = OnKeyPc;																														// save the function identifier 
				++pCntx->Pc.pToken;
			}
			break;
		}
		default: return ValueNewError(pValue, MISSINGGOTOSUB);
	}
	if(pCntx->Pass == ePass::INTERPRET){	
		pCntx->EventKeys[KeyNumber].State = eOnKeyState::ACTIVE;
		pCntx->EventKeys[KeyNumber].JumpType = OnKeyType;
		pCntx->EventKeys[KeyNumber].Priority = Priority;
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////

pValue_s stmt_OPTIONBASE(pValue_s pValue)
{
	if(InDirectMode()) return ValueNewError(pValue, NOTINDIRECTMODE);
	++pCntx->Pc.pToken;
	if(Evaluate(pValue, _T("subscript base"))->Type == eVType::V_ERROR || (pCntx->Pass != ePass::DECLARE && ValueRetype(pValue, eVType::V_INT)->Type == eVType::V_ERROR)) return pValue;
	if(pCntx->Pass == ePass::INTERPRET) pCntx->pSubCntx->OptionBase = pValue->uData.Integer;
	ValueDestroy(pValue);
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////

pValue_s stmt_OUTPUT(pValue_s pValue)
{
int Out, Address, Value;
PC_s SavePc;

	Out = (pCntx->Pc.pToken->Type == T_OUT);
	SavePc = pCntx->Pc;
	++pCntx->Pc.pToken;
	if(Evaluate(pValue, _T("address"))->Type == eVType::V_ERROR || ValueRetype(pValue, eVType::V_INT)->Type == eVType::V_ERROR) return pValue;
	Address = pValue->uData.Integer;
	ValueDestroy(pValue);
	if(pCntx->Pc.pToken->Type != T_COMMA) return ValueNewError(pValue, MISSINGCOMMA);
	++pCntx->Pc.pToken;
	if(Evaluate(pValue, _T("output pValue"))->Type == eVType::V_ERROR || ValueRetype(pValue, eVType::V_INT)->Type == eVType::V_ERROR) return pValue;
	Value = pValue->uData.Integer;
	ValueDestroy(pValue);
	if(pCntx->Pass == ePass::INTERPRET){
		if(DevicePortOutput(Address, pValue->uData.Integer) == -1){
			pCntx->Pc = SavePc;
			return ValueNewError(pValue, IOERROR, g_SufixErrorMsg.GetBuffer());
		}
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////

pValue_s stmt_OVERLAP_SERIAL(pValue_s pValue)
{
	++pCntx->Pc.pToken;
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////
pValue_s stmt_PAUSE(pValue_s pValue)
{
	++pCntx->Pc.pToken;
	if(pCntx->Pass == ePass::INTERPRET)	pCntx->RunMode = eRunMode::HALT;
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////

pValue_s stmt_PRINT_LPRINT(pValue_s pValue)
{
PC_s StatementPc = pCntx->Pc;
bool IsFilePrint = false, LineFeedSuppress = false, NextTabPos = false;
int File;
eCaridgeSuppresion Suppression = eCaridgeSuppresion::NONE;
TokenType_e PrintType = pCntx->Pc.pToken->Type;
Value_s UsingValue;
BString PrintStr, UsingStr, DispStr;
UINT UsingPos = 0;

	++pCntx->Pc.pToken;
  if(pCntx->Pc.pToken->Type == T_CHANNEL){												// File identifier
		IsFilePrint = true;
    ++pCntx->Pc.pToken;
		long Record = -1;		// serial
		PC_s ErrorPc = pCntx->Pc;
		if(Evaluate(pValue, _T("file #"))->Type == eVType::V_ERROR || ValueRetype(pValue, eVType::V_INT)->Type == eVType::V_ERROR){
			pCntx->Pc = ErrorPc;
			return pValue;
		}
		File = pValue->uData.Integer;																	 // File number
		ValueDestroy(pValue);
		if(pCntx->Pass == ePass::INTERPRET){
			if(!g_pMassStorage->IsValid(File)) return ValueNewError(pValue, INVALIDFILENUM);
			if(!g_pMassStorage->IsAssigned(File)) return ValueNewError(pValue, FILEUNASSIGNED);
		}
		if(pCntx->Pc.pToken->Type == T_COMMA){													// Random Access record		
	 		++pCntx->Pc.pToken;
			if(Evaluate(pValue, _T("record"))->Type == eVType::V_ERROR || ValueRetype(pValue, eVType::V_INT)->Type == eVType::V_ERROR){
				pCntx->Pc = ErrorPc;
				return pValue;
			}
			Record = pValue->uData.Integer;																// Record Number
			ValueDestroy(pValue);
			if(g_pMassStorage->ReadRecord(File, Record) == -1) return ValueNewError(pValue, OUTOFRANGE, _T("Record #"));
		}
		if(pCntx->Pc.pToken->Type == T_SEMICOLON){											// Variable list
			int Result = -1;
 			++pCntx->Pc.pToken;
			while(1){
				pValue_s pVal;
				if(pCntx->Pc.pToken->Type != T_IDENTIFIER) return ValueNewError(pValue, MISSINGPROCIDENT);
				if(pCntx->Pass == ePass::DECLARE){
					Identifier_s *pIdentifier = pCntx->Pc.pToken->Obj.pIdentifier;
					if(((pCntx->Pc.pToken + 1)->Type == T_OP || StackFind(&pCntx->Stack, pIdentifier) == 0) && FindIdentifier(g_pLocalRegistry, pIdentifier, LibSearchType((pCntx->Pc.pToken + 1)->Type, pIdentifier->DefType)) == 0)	return ValueNewError(pValue, REDECLARATION);
				}
				if((pVal = GetValue(pValue))->Type == eVType::V_ERROR) return pValue;
				if(pCntx->Pass == ePass::INTERPRET){
					switch(pVal->Type){
						case eVType::V_INT: Result = g_pMassStorage->PrintIntegerData(File, (Record > 0), pVal->uData.Integer); break;
						case eVType::V_REAL: Result = g_pMassStorage->PrintRealData(File, (Record > 0), pVal->uData.Real); break;
						case eVType::V_STRING: Result =	g_pMassStorage->PrintStringData(File, (Record > 0), pVal->uData.pString); break;
						case eVType::V_ARRAY: Result = g_pMassStorage->PrintArrayData(File, (Record > 0), pVal->uData.pArray); break;
						default: assert(0);
					}
				}
				if(pCntx->Pc.pToken->Type == T_COMMA) ++pCntx->Pc.pToken;
				else break;
			}
			if((pCntx->Pass == ePass::INTERPRET) && (Result == -1)){
				pCntx->Pc = StatementPc;
				return ValueNewError(pValue, MASSSTROREERR, g_SufixErrorMsg.GetBuffer());
			}
		}
		else if(Record == -1) return ValueNewError(pValue, MISSINGSEMICOLON);					// just reposition 
  }
	if(pCntx->Pc.pToken->Type == T_USING){																					// Using format
		PC_s ImagePc = pCntx->Pc;
		++pCntx->Pc.pToken;
		switch(pCntx->Pc.pToken->Type){
			case T_IDENTIFIER:{
				if(pCntx->Pass != ePass::DECLARE){
					if(FindLabel(pCntx->pSubCntx->pLabels, pCntx->Pc.pToken->Obj.pIdentifier) == 0) return ValueNewError(pValue, MISSINGIDENTIFIER);
					long LineNumber = ProgramGetLabelLine(&pCntx->Program, pCntx->Pc.pToken->Obj.pLabel, &ImagePc);
					if(ProgramImageLine(&pCntx->Program, LineNumber, &ImagePc) == nullptr) return ValueNewError(pValue, NOSUCHIMAGELINE);		// Image format
				}
				if(pCntx->Pass == ePass::INTERPRET) UsingStr.Clone(ImagePc.pToken->Obj.pString);
				++pCntx->Pc.pToken;
				break;
			}
			case T_INTNUM:{
				if((pCntx->Pass != ePass::DECLARE) && ProgramImageLine(&pCntx->Program, pCntx->Pc.pToken->Obj.Integer, &ImagePc) == nullptr) return ValueNewError(pValue, NOSUCHIMAGELINE);		// Image format
				else if(pCntx->Pass == ePass::INTERPRET) UsingStr.Clone(ImagePc.pToken->Obj.pString);
				++pCntx->Pc.pToken;
				break;
			}
			default:{
				if(Evaluate(&UsingValue, _T("format string"))->Type == eVType::V_ERROR || ValueRetype(&UsingValue, eVType::V_STRING)->Type == eVType::V_ERROR){			// inline format
					*pValue = UsingValue;
					return pValue;
				}
				if(pCntx->Pass == ePass::INTERPRET) UsingStr.Clone(UsingValue.uData.pString);
				ValueDestroy(&UsingValue);
				break;
			}
		}
		if(pCntx->Pc.pToken->Type != T_EOL){
			if(pCntx->Pc.pToken->Type != T_SEMICOLON){
				return ValueNewError(pValue, MISSINGSEMICOLON);
			}
			++pCntx->Pc.pToken;
		}
		if(pCntx->Pass == ePass::INTERPRET){
			switch(UsingStr.GetAt(0)){
				case '#':	Suppression = eCaridgeSuppresion::BOTH;	break;
				case '+': Suppression = eCaridgeSuppresion::NOLF;	break;
				case '-': Suppression = eCaridgeSuppresion::NOCR;	break;
			}
		}
		while(1){
			PC_s ValuePc = pCntx->Pc;
			if(Evaluate(pValue, nullptr)){
				if(pValue->Type == eVType::V_ERROR){
					return pValue;
				}
				if(pCntx->Pass != ePass::INTERPRET) ValueDestroy(pValue);
				else{
					if(pValue->Type == eVType::V_ARRAY){
						if(BuildArrayStringUsing(pValue, &PrintStr, &UsingStr, &UsingPos)->Type == eVType::V_ERROR){
							pCntx->Pc = ValuePc;
							return pValue;
						}
					}
					else BiuldStringUsing(pValue, &PrintStr, &UsingStr, &UsingPos);
					ValueDestroy(pValue);
				}
			}
			if(pCntx->Pc.pToken->Type == T_COMMA)	++pCntx->Pc.pToken;
			else break;
		}
		if(PrintStr.GetLength() > 0){
			switch(PrintType){
				case T_DISP:{
					g_pPrinter->PrintText(eDispArea::PROMPT, PrintStr.GetBuffer());			// send it directly to the prompt line
					break;
				}
				case T_PRINT:{
					g_pPrinter->PrintString(&PrintStr);
					PrintStr.Clear();
					break;
				}
				case T_GLABEL:{
					g_pPlotter->Label(&PrintStr);
					break;
				}
			}
		}
	}
	else{																																			// plain print (no using)
		while(1){
			PC_s ValuePc = pCntx->Pc;
			if(Evaluate(pValue, nullptr)){
				if(pValue->Type == eVType::V_ERROR){
					return pValue;
				}
				Suppression = eCaridgeSuppresion::NONE;
				PrintStr.Clear();
				if(pCntx->Pass == ePass::INTERPRET){
					if(NextTabPos){
						if(PrintType ==  T_PRINT)	g_pPrinter->NextTab();
						else DispStr.AppendChars(_T("        "));
					}
					if(pValue->Type == eVType::V_ARRAY){
						if(PrintType == T_PRINT){																														// format directly using printer driver
							if(g_pPrinter->PrintArray(pValue, (pCntx->Pc.pToken->Type != T_SEMICOLON)) == -1){
								pCntx->Pc = ValuePc;
								return ValueNewError(pValue, IMPROPERVALTYPE);
							}
							PrintStr.Clear();
						}
						else return ValueNewError(pValue, IMPROPERVALTYPE);																		 // probably wouldn't work for big arrays
					}
					else{																																										 // simple values
						NextTabPos = true;
						if(ValueToStringUsing(pValue, &PrintStr, &UsingStr, &UsingPos)->Type == eVType::V_ERROR){
							pCntx->Pc = ValuePc;
							return pValue;
						}
					}
					if(PrintStr.GetLength() > 0){
						switch(PrintType){
							case T_DISP:
							case T_GLABEL:{
								DispStr.AppendString(&PrintStr);																										// build display or label string
								break;
							}
							case T_PRINT:{
								g_pPrinter->PrintString(&PrintStr);
								break;
							}
						}
					}
				}
				ValueDestroy(pValue);
			}
			else if(pCntx->Pc.pToken->Type == T_TAB || pCntx->Pc.pToken->Type == T_SPA || pCntx->Pc.pToken->Type == T_LIN){	// print control sequence
				Suppression = eCaridgeSuppresion::NONE;
				TokenType_e PrintFunc = pCntx->Pc.pToken->Type;
				++pCntx->Pc.pToken;
				if(pCntx->Pc.pToken->Type != T_OP){
					return ValueNewError(pValue, MISSINGOP);
				}
				++pCntx->Pc.pToken;
				if(Evaluate(pValue, _T("count"))->Type == eVType::V_ERROR || ValueRetype(pValue, eVType::V_INT)->Type == eVType::V_ERROR){
					return pValue;
				}
				NextTabPos = false;
				if(pCntx->Pass == ePass::INTERPRET){
					int Count = pValue->uData.Integer, Result = 0;
					switch(PrintType){
						case T_DISP:
						case T_GLABEL:{
							switch(PrintFunc){
								case T_TAB:{ DispStr.PadToColumn(Count); break;	}
								case T_SPA:{ while(Count-- > 0) DispStr.AppendChar(' '); break;	}
							}
							break;
						}
						case T_PRINT:{
							switch(PrintFunc){
								case T_TAB:{ Result = g_pPrinter->SetCursorColumn(Count);	break; }
								case T_SPA:{ while(Count-- > 0 && (Result = g_pPrinter->PrintChars(" ")) != -1); break; }
								case T_LIN:{ Result = g_pPrinter->PrintLIN(Count); break; }
							}
							if(Result == -1){
								ValueDestroy(pValue);
								return ValueNewError(pValue, PRINTERERROR);
							}
							break;
						}
					}
				}
				ValueDestroy(pValue);
				if(pCntx->Pc.pToken->Type != T_CP){
					return ValueNewError(pValue, MISSINGCP);
				}
				++pCntx->Pc.pToken;
			}
			else if(pCntx->Pc.pToken->Type == T_PAGE){																			// Print 'Page' (form feed)
				++pCntx->Pc.pToken;
				Suppression = eCaridgeSuppresion::BOTH;
				NextTabPos = false;
				if(pCntx->Pass == ePass::INTERPRET){
					if(g_pPrinter->PrintChars("\f") == -1){
						return ValueNewError(pValue, PRINTERERROR);
					}
				}
			}
			if(pCntx->Pc.pToken->Type == T_SEMICOLON){
				++pCntx->Pc.pToken;
				Suppression = eCaridgeSuppresion::BOTH;																				// continuation of line in next print statement
				NextTabPos = false;
			}
			else if(pCntx->Pc.pToken->Type == T_COMMA){
				++pCntx->Pc.pToken;
				Suppression = eCaridgeSuppresion::BOTH;																				// continuation of line in next print statement
			}
			else break;
		}
		if(pCntx->Pass == ePass::INTERPRET){
			switch(PrintType){
				case T_DISP:
					g_pPrinter->PrintText(eDispArea::PROMPT, DispStr.GetBuffer());		// send it directly to the prompt line
					break;
				case T_GLABEL:{
					g_pPlotter->Label(&DispStr);
					break;
				}
			}
		}
	}
	if((pCntx->Pass == ePass::INTERPRET) && (PrintType == T_PRINT)){
		switch((int)Suppression){
			case eCaridgeSuppresion::NONE: g_pPrinter->PrintChars("\r\n"); break;
			case eCaridgeSuppresion::NOLF: g_pPrinter->PrintChars("\r"); break;
			case eCaridgeSuppresion::NOCR: g_pPrinter->PrintChars("\n"); break;
		}
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////

pValue_s stmt_PRINTERIS(pValue_s pValue)
{
int Device, Width = DEF_LINE_WIDTH;

	++pCntx->Pc.pToken;
  if((Evaluate(pValue, _T("device"))->Type == eVType::V_ERROR) || (ValueRetype(pValue, eVType::V_INT)->Type == eVType::V_ERROR)) return pValue;
	Device = pValue->uData.Integer;
  ValueDestroy(pValue);
	if(pCntx->Pc.pToken->Type == T_COMMA){
		++pCntx->Pc.pToken;
		while(1){
			switch(pCntx->Pc.pToken->Type){
				case T_WIDTH:{
					++pCntx->Pc.pToken;
					if(Evaluate(pValue, nullptr)){
						if(pValue->Type == eVType::V_ERROR || ValueRetype(pValue, eVType::V_INT)->Type == eVType::V_ERROR) return pValue;
						Width = pValue->uData.Integer;
						ValueDestroy(pValue);
					}
					break;
				}
			}
			if(pCntx->Pc.pToken->Type == T_COMMA) ++pCntx->Pc.pToken;
			else break;
		}
	}
	if(pCntx->Pass == ePass::INTERPRET && SetPrinterIs(Device, Width) == -1) return ValueNewError(pValue, PRINTERERROR);
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////

pValue_s stmt_PURGE(pValue_s pValue)
{
PC_s StatementPc = pCntx->Pc;

	++pCntx->Pc.pToken;
	if(Evaluate(pValue, _T("file name"))->Type == eVType::V_ERROR || (pCntx->Pass != ePass::DECLARE && ValueRetype(pValue, eVType::V_STRING)->Type == eVType::V_ERROR)) return pValue;
	if(pCntx->Pass == ePass::INTERPRET && _unlink(pValue->uData.pString->GetBuffer()) == -1){
		ValueDestroy(pValue);
		pCntx->Pc = StatementPc;
		char Buff[MSG_BUFFER_SIZE];
		strerror_s(Buff, MSG_BUFFER_SIZE, errno);
		g_SufixErrorMsg.AppendChars(Buff);
		return ValueNewError(pValue, MASSSTROREERR, g_SufixErrorMsg.GetBuffer());
	}
	else ValueDestroy(pValue);
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////

pValue_s stmt_RANDOMIZE(pValue_s pValue)
{
PC_s SavePc;

	++pCntx->Pc.pToken;
	SavePc = pCntx->Pc;
	if(pCntx->Pc.pToken->Type != T_EOL){
		if(Evaluate(pValue, nullptr)){
			ValueRetype(pValue, eVType::V_INT);
			if(pValue->Type == eVType::V_ERROR){
				pCntx->Pc = SavePc;
				ValueDestroy(pValue);
				return ValueNewError(pValue, MISSINGEXPR, _T("random number generator seed"));
			}
			if(pCntx->Pass == ePass::INTERPRET) srand((UINT)pCntx->Pc.pToken->Obj.Integer);
			ValueDestroy(pValue);
		}
	}
	else if(pCntx->Pass == ePass::INTERPRET) srand((UINT)time(nullptr));
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////

pValue_s stmt_READ(pValue_s pValue)
{
PC_s EntryPc = pCntx->Pc, ErrorPc;
long Record = -1;		// serial

	++pCntx->Pc.pToken;
	if(pCntx->Pc.pToken->Type == T_CHANNEL){														// Read Mass Storage
		long File = 1;
		++pCntx->Pc.pToken;
		ErrorPc = pCntx->Pc;
		if(Evaluate(pValue, _T("file #"))->Type == eVType::V_ERROR || ValueRetype(pValue, eVType::V_INT)->Type == eVType::V_ERROR){
			pCntx->Pc = ErrorPc;
			return pValue;
		}
		File = pValue->uData.Integer;																	 // File number
		ValueDestroy(pValue);
		if(pCntx->Pass == ePass::INTERPRET){
			if(!g_pMassStorage->IsValid(File)) return ValueNewError(pValue, INVALIDFILENUM);
			if(!g_pMassStorage->IsAssigned(File)) return ValueNewError(pValue, FILEUNASSIGNED);
		}
		if(pCntx->Pc.pToken->Type == T_COMMA){													// Random Access record		
	 		++pCntx->Pc.pToken;
			if(Evaluate(pValue, _T("record"))->Type == eVType::V_ERROR || ValueRetype(pValue, eVType::V_INT)->Type == eVType::V_ERROR){
				pCntx->Pc = ErrorPc;
				return pValue;
			}
			Record = pValue->uData.Integer;																// Record Number
			ValueDestroy(pValue);
			if(pCntx->Pass == ePass::INTERPRET){
				if(g_pMassStorage->ReadRecord(File, Record) == -1) return ValueNewError(pValue, OUTOFRANGE, _T("Record #"));
			}
		}
		if(pCntx->Pc.pToken->Type == T_SEMICOLON){											// Variable list
			int Result = 0;
 			++pCntx->Pc.pToken;
			while(1){
				pValue_s pVal;
				if(pCntx->Pc.pToken->Type != T_IDENTIFIER) return ValueNewError(pValue, MISSINGPROCIDENT);
				if(pCntx->Pass == ePass::INTERPRET) g_Trace.pIdentifier = pCntx->Pc.pToken->Obj.pIdentifier;
				if((pCntx->Pass == ePass::DECLARE) && (FindOrRegister(g_pLocalRegistry, pCntx->Pc.pToken->Obj.pIdentifier) == 0)) return ValueNewError(pValue, UNDECLARED);
				if((pVal = GetValue(pValue))->Type == eVType::V_ERROR) return pValue;
				if((pCntx->Pass == ePass::INTERPRET) && (Result != -1)){
					switch(pVal->Type){
						case eVType::V_INT: Result = g_pMassStorage->ReadIntegerData(File, (Record > 0), &pVal->uData.Integer); break;
						case eVType::V_REAL: Result = g_pMassStorage->ReadRealData(File, (Record > 0), &pVal->uData.Real); break;
						case eVType::V_STRING: Result = g_pMassStorage->ReadStringData(File, (Record > 0), pVal->uData.pString); break;
						case eVType::V_ARRAY: Result = g_pMassStorage->ReadArrayData(File, (Record > 0), pVal->uData.pArray); break;
						default: assert(0);
					}
					ProgramTraceVar(&pCntx->Program, pVal);
				}
				if(pCntx->Pc.pToken->Type == T_COMMA) ++pCntx->Pc.pToken;
				else break;
			}
			if((pCntx->Pass == ePass::INTERPRET) && (Result == -1)){
				pCntx->Pc = EntryPc;
				return ValueNewError(pValue, MASSSTROREERR, g_SufixErrorMsg.GetBuffer());
			}
		}
		else if(Record == -1) return ValueNewError(pValue, MISSINGSEMICOLON);				 // just reposition 
	}
	else{																															// Read DATA statement
		while(1){
			pValue_s pVal;
			ErrorPc = pCntx->Pc;
			if(pCntx->Pc.pToken->Type != T_IDENTIFIER) return ValueNewError(pValue, MISSINGREADIDENT);
			if(pCntx->Pass == ePass::INTERPRET) g_Trace.pIdentifier = pCntx->Pc.pToken->Obj.pIdentifier;
			if((pCntx->Pass == ePass::DECLARE) && (FindOrRegister(g_pLocalRegistry, pCntx->Pc.pToken->Obj.pIdentifier) == 0)) return ValueNewError(pValue, UNDECLARED);
			if((pVal = GetValue(pValue))->Type == eVType::V_ERROR) return pValue;
			if(pCntx->Pass == ePass::INTERPRET){
				switch(pVal->Type){
					case eVType::V_INT:
					case eVType::V_REAL:
					case eVType::V_STRING:{
						if(DataRead(pValue, pVal)){
							pCntx->Pc = ErrorPc;
							return pValue;
						}
						break;
					}
					case eVType::V_ARRAY:{
						int Size, i;
						Var_s *pVar = pVal->uData.pArray;
						for(Size = 1, i = 0; i < pVar->Dimensions; ++i) Size *= pVar->pGeometry[i].Size;
						for(i = 0; i < Size; ++i){
							if(DataRead(pValue, &pVar->pValue[i])){
								pCntx->Pc = ErrorPc;
								return pValue;
							}
						}
						break;
					}
				}
				ProgramTraceVar(&pCntx->Program, pVal);
			}
			if(pCntx->Pc.pToken->Type == T_COMMA) ++pCntx->Pc.pToken;
			else break;
		}
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////

pValue_s stmt_REDIM(pValue_s pValue)
{
	++pCntx->Pc.pToken;
	while(1){
		Var_s *pVar;
		int Dim;
		Geometry_s Geometry[DEF_MAX_DIMENSIONS];
		if(pCntx->Pc.pToken->Type != T_IDENTIFIER) return ValueNewError(pValue, MISSINGMATIDENT);
		if(pCntx->Pass == ePass::DECLARE && RegisterVariable(g_pLocalRegistry, pCntx->Pc.pToken->Obj.pIdentifier, pCntx->Pc.pToken->Obj.pIdentifier->DefType, eSymType::LOCALARRAY, 0) == 0) return ValueNewError(pValue, REDECLARATION);
		pVar = &pCntx->Pc.pToken->Obj.pIdentifier->pSym->uType.Var;
		++pCntx->Pc.pToken;
		if(pCntx->Pc.pToken->Type != T_OP) return ValueNewError(pValue, MISSINGOP);
		++pCntx->Pc.pToken;
		if(EvaluateGeometry(pValue, &Dim, Geometry)) return pValue;
		if(pCntx->Pass == ePass::INTERPRET && VarMatRedim(pVar, Dim, Geometry, pValue) != nullptr) return pValue;
		if(pCntx->Pc.pToken->Type == T_COMMA) ++pCntx->Pc.pToken;
		else break;
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////

pValue_s stmt_COPY_RENAME(pValue_s pValue)
{
PC_s ErrorPc, EntryPc = pCntx->Pc;
Value_s FromValue;

	++pCntx->Pc.pToken;
	ErrorPc = pCntx->Pc;
	if(Evaluate(&FromValue, _T("source file"))->Type == eVType::V_ERROR || (pCntx->Pass != ePass::DECLARE && ValueRetype(&FromValue, eVType::V_STRING)->Type == eVType::V_ERROR)){
		pCntx->Pc = ErrorPc;
		*pValue = FromValue;
		return pValue;
	}
	if(pCntx->Pc.pToken->Type != T_TO){
		ValueDestroy(&FromValue);
		return ValueNewError(pValue, MISSINGTO);
	}
	++pCntx->Pc.pToken;
	ErrorPc = pCntx->Pc;
	if(Evaluate(pValue, _T("destination file"))->Type == eVType::V_ERROR || (pCntx->Pass != ePass::DECLARE && ValueRetype(pValue, eVType::V_STRING)->Type == eVType::V_ERROR)){
		pCntx->Pc = ErrorPc;
		return pValue;
	}
	if(pCntx->Pass == ePass::INTERPRET){
		int Result;
		if(EntryPc.pToken->Type == T_RENAME) Result = g_pMassStorage->Rename(FromValue.uData.pString->GetBuffer(), pValue->uData.pString->GetBuffer());
		else Result = g_pMassStorage->Copy(FromValue.uData.pString->GetBuffer(), pValue->uData.pString->GetBuffer());
		if(Result == -1){
			ValueDestroy(&FromValue);
			ValueDestroy(pValue);
			pCntx->Pc = EntryPc;
			return ValueNewError(pValue, MASSSTROREERR, g_SufixErrorMsg.GetBuffer());
		}
	}
	ValueDestroy(&FromValue);
	ValueDestroy(pValue);
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////

pValue_s stmt_RENUM(pValue_s pValue)
{
int first = 10, Inc = 10;

	++pCntx->Pc.pToken;
	if(pCntx->Pc.pToken->Type == T_INTNUM){
		first = pCntx->Pc.pToken->Obj.Integer;
		++pCntx->Pc.pToken;
		if(pCntx->Pc.pToken->Type == T_COMMA){
			++pCntx->Pc.pToken;
			if(pCntx->Pc.pToken->Type != T_INTNUM) return ValueNewError(pValue, MISSINGINCREMENT);
			Inc = pCntx->Pc.pToken->Obj.Integer;
			++pCntx->Pc.pToken;
		}
	}
	if(pCntx->Pass == ePass::INTERPRET){
		if(!InDirectMode()) return ValueNewError(pValue, NOTINPROGRAMMODE);
		ProgramRenumber(&pCntx->Program, first, Inc);
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////

pValue_s stmt_REPEAT(pValue_s pValue)
{
	if(pCntx->Pass == ePass::DECLARE || pCntx->Pass == ePass::COMPILE) PushMarker(eMType::REPEAT, &pCntx->Pc);
	++pCntx->Pc.pToken;
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////

pValue_s stmt_RESTORE(pValue_s pValue)
{
Token_s *pRestorePc = pCntx->Pc.pToken;

	if(pCntx->Pass == ePass::INTERPRET) pCntx->pSubCntx->CurrentData = pCntx->Pc.pToken->Obj.RestorePc;
	++pCntx->Pc.pToken;
	switch(pCntx->Pc.pToken->Type){
		case T_IDENTIFIER:{
			if(pCntx->Pass == ePass::COMPILE){
				if(FindLabel(pCntx->pSubCntx->pLabels, pCntx->Pc.pToken->Obj.pIdentifier) == 0) return ValueNewError(pValue, MISSINGIDENTIFIER);
				if(ProgramGetLabelLine(&pCntx->Program, pCntx->Pc.pToken->Obj.pLabel, &pRestorePc->Obj.RestorePc) < 1) return ValueNewError(pValue, NOSUCHDATALINE);
			}
			++pCntx->Pc.pToken;
			break;
		}
		case T_INTNUM:{
			if(pCntx->Pass == ePass::COMPILE && ProgramDataLine(&pCntx->Program, pCntx->Pc.pToken->Obj.Integer, &pRestorePc->Obj.RestorePc) == nullptr) return ValueNewError(pValue, NOSUCHDATALINE);
			++pCntx->Pc.pToken;
			break;
		}
		default:
			if(pCntx->Pass == ePass::COMPILE) pRestorePc->Obj.RestorePc = pCntx->pSubCntx->BeginData;
			break;
	}
	if(pCntx->Pc.pToken->Type == T_INTNUM){
		++pCntx->Pc.pToken;
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////

pValue_s stmt_RETURN(pValue_s pValue)
{
PC_s EntryPc = pCntx->Pc;

	++pCntx->Pc.pToken;
	if((pCntx->Pc.pToken->Type != T_EOL) && (pCntx->Pc.pToken->Type != T_REM)){
		if((pCntx->Pass == ePass::DECLARE) || (pCntx->Pass == ePass::COMPILE)){
			PC_s *pMarkerPc = nullptr;
			if((pMarkerPc = FindMarker(eMType::FUNC)) == nullptr) return ValueNewError(pValue, STRAYFNRETURN);
			if((EntryPc.pToken->Obj.Type = (pMarkerPc->pToken + 1)->Obj.pIdentifier->DefType) == eVType::V_VOID) return ValueNewError(pValue, STRAYFNRETURN);
		}
		if(Evaluate(pValue, _T("return"))->Type == eVType::V_ERROR || ValueRetype(pValue, EntryPc.pToken->Obj.Type)->Type == eVType::V_ERROR){
			if(pCntx->Pass != ePass::INTERPRET)	StackFuncEnd(&pCntx->Stack);
			pCntx->Pc = EntryPc;
		}
		if(pCntx->Pass == ePass::INTERPRET) return pValue;
		else ValueDestroy(pValue);
	}
	else{
		if(pCntx->Pass == ePass::INTERPRET){
			g_Trace.pLastPc = &pCntx->Pc;
			if(StackGosubReturn(&pCntx->Stack, &pCntx->Pc)) ProgramTraceFlow(&pCntx->Program, &pCntx->Pc);
			else return ValueNewError(pValue, STRAYRETURN);
		}
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////

pValue_s stmt_RESULT(pValue_s pValue)
{
Value_s UsingValue;
BString *pUsingStr = nullptr;
UINT UsingPos = 0;

	++pCntx->Pc.pToken;
	PC_s ValuePc = pCntx->Pc;
	ValueClone(pValue, &g_ResultValue);
	BString PrintStr;
	ValueNewString(&UsingValue);																						// dummy using string
	pUsingStr = UsingValue.uData.pString;
	if(pCntx->Pass == ePass::INTERPRET){
		if(pValue->Type == eVType::V_ARRAY){
			if(ValueArrayToStringUsing(pValue, &PrintStr, pUsingStr, &UsingPos)->Type == eVType::V_ERROR){
				ValueDestroy(&UsingValue);
				pCntx->Pc = ValuePc;
				return pValue;
			}
		}
		else{
			if(ValueToStringUsing(pValue, &PrintStr, pUsingStr, &UsingPos)->Type == eVType::V_ERROR){
				ValueDestroy(&UsingValue);
				pCntx->Pc = ValuePc;
				return pValue;
			}
		}
	}
	if(PrintStr.GetLength() > 0)	g_pPrinter->PrintText(eDispArea::COMMENT, PrintStr.GetBuffer());			// send it directley to the system line
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////

pValue_s stmt_RUN(pValue_s pValue)
{
PC_s BeginPc, ErrorPc;

	pCntx->Stack.Resumeable = 0;
	++pCntx->Pc.pToken;
	ErrorPc = pCntx->Pc;
	if(pCntx->Pc.pToken->Type == T_INTNUM){																											 // run from line number
		if(ProgramGoLine(&pCntx->Program, pCntx->Pc.pToken->Obj.Integer, &BeginPc) == nullptr) return ValueNewError(pValue, NOSUCHLINE);
		if((pCntx->Pass == ePass::COMPILE) && ProgramScopeCheck(&pCntx->Program, &BeginPc, FindMarker(eMType::FUNC))) return ValueNewError(pValue, OUTOFSCOPE);
		++pCntx->Pc.pToken;
	}
	else{
		if(Evaluate(pValue, nullptr)){																														// run a file name from an expression
			if(pValue->Type == eVType::V_ERROR || ValueRetype(pValue, eVType::V_STRING)->Type == eVType::V_ERROR){
				pCntx->Pc = ErrorPc;
				return pValue;
			}
			else{
				if(pCntx->Pass == ePass::INTERPRET){
					int Handle;
					Program_s newContext;
					if((Handle = g_pMassStorage->ReadOpen(pValue->uData.pString->GetBuffer(), MT_PROG, true)) == -1){
						pCntx->Pc = ErrorPc;
						ValueDestroy(pValue);
						return ValueNewError(pValue, MASSSTROREERR, g_SufixErrorMsg.GetBuffer());
					}
					ValueDestroy(pValue);
					ProgramNew(&newContext);
					if(ProgramGet(&newContext, Handle, pValue)){
						pCntx->Pc = ErrorPc;
						ProgramDestroy(&newContext);
						return pValue;
					}
					g_pMassStorage->Close(Handle);
					BasicNew();
					ProgramDestroy(&pCntx->Program);
					pCntx->Program = newContext;
					if(ProgramBeginning(&pCntx->Program, &BeginPc) == nullptr)	return ValueNewError(pValue, NOPROGRAM);
				}
				else ValueDestroy(pValue);
			}
		}
		else if(ProgramBeginning(&pCntx->Program, &BeginPc) == nullptr) return ValueNewError(pValue, NOPROGRAM);
	}
	if(pCntx->Pass == ePass::INTERPRET){
		if(CompileProgram(pValue, 1)->Type == eVType::V_ERROR) return pValue;											// we now compile the programme for the first time after loading
		pCntx->Pc = BeginPc;																																			// set the start line
		pCntx->pSubCntx->CurrentData = pCntx->pSubCntx->BeginData;																// initialise the data stack pointer
		ResetVariables(g_pLocalRegistry);																													// get ready to run the programme 
		g_pMassStorage->CloseAll();																																// close all files
	}				
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////

pValue_s stmt_SAVE(pValue_s pValue)
{
BString FileName;
int DevIndex = 15;
Context_s	*pRunCntx = &SystemContexts[(int)eCntxType::RUN];

	if(pCntx->Pass == ePass::INTERPRET && !InDirectMode()) return ValueNewError(pValue, NOTINPROGRAMMODE);
	++pCntx->Pc.pToken;
	PC_s EntryPc = pCntx->Pc;
	if(pCntx->Pc.pToken->Type == T_EOL && pCntx->Program.Name.GetLength()) FileName.AppendString(&pCntx->Program.Name);
	else{
		if(Evaluate(pValue, _T("file name"))->Type == eVType::V_ERROR || ValueRetype(pValue, eVType::V_STRING)->Type == eVType::V_ERROR){
			pCntx->Pc = EntryPc;
			return pValue;
		}
		FileName.AppendString(pValue->uData.pString);
	  ValueDestroy(pValue);
	}
	if(pCntx->Pass == ePass::INTERPRET){
		int FileIndex;
		if((FileIndex = g_pMassStorage->WriteOpen(FileName.GetBuffer(), MT_BAS, true)) == -1){ // save to context name or specified name
			pCntx->Pc = EntryPc;
			return ValueNewError(pValue, MASSSTROREERR, g_SufixErrorMsg.GetBuffer());
		}
		if(ProgramList(&pRunCntx->Program, FileIndex, nullptr, nullptr, pValue)){
			pCntx->Pc = EntryPc;
			return pValue;
		}
		g_pMassStorage->Close(FileIndex);
		pRunCntx->Program.Unsaved = FALSE;
		SetMainTitle(FileName.GetBuffer()); 
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////

pValue_s stmt_SCRATCH(pValue_s pValue)
{
	TokenType_e TokType = pCntx->Pc.pToken->Type;
	++pCntx->Pc.pToken;
	if(pCntx->Pass == ePass::INTERPRET){
		if(!InDirectMode()) return ValueNewError(pValue, NOTINPROGRAMMODE);
		CS45BasicDoc* pDoc = static_cast<CS45BasicDoc*>(g_pBasicDoc);
		SystemContexts[(int)eCntxType::RUN].RunState = eRunState::STOPPED;
		SystemContexts[(int)eCntxType::RUN].RunMode = eRunMode::STOP;
		switch(TokType){
			case T_SCRATCH:
				DestroyAllIdentifiers(g_pLocalRegistry);
				DestroyLabels(pCntx->pSubCntx->pLabels);
				DestroyGlobals(g_pGlobalRegistry);
				ProgramDestroy(&SystemContexts[(int)eCntxType::RUN].Program);
				ProgramNew(&SystemContexts[(int)eCntxType::RUN].Program);
				SetPrinterIs(SC_TEXT_DISPLAY, DEF_LINE_WIDTH);
				SetPlotterIs(SC_GRAPHICS, 0);																	
				MassStorageIs(SC_STANDARD_TAPE);
				pDoc->SetGraphicsMode(false);
				ProgramTraceClear();
				SetMainTitle(nullptr); 
				break;
			case T_SCRATCH_A:
				DestroyAllIdentifiers(g_pLocalRegistry);
				DestroyLabels(pCntx->pSubCntx->pLabels);
				DestroyGlobals(g_pGlobalRegistry);
				ProgramDestroy(&SystemContexts[(int)eCntxType::RUN].Program);
				DestroyStack(&SystemContexts[(int)eCntxType::RUN].Stack);
				ProgramNew(&SystemContexts[(int)eCntxType::RUN].Program);
				NewStack(&SystemContexts[(int)eCntxType::RUN].Stack);
				SetPrinterIs(SC_TEXT_DISPLAY, DEF_LINE_WIDTH);
				SetPlotterIs(SC_GRAPHICS, 0);																	
				MassStorageIs(SC_STANDARD_TAPE);														
				pDoc->SetGraphicsMode(false);
				ProgramTraceClear();
				SetMainTitle(nullptr); 
				break;
			case T_SCRATCH_C:
				DestroyAllIdentifiers(g_pLocalRegistry);
				DestroyCommon();
				DestroyStack(&SystemContexts[(int)eCntxType::RUN].Stack);
				NewStack(&SystemContexts[(int)eCntxType::RUN].Stack);
				pDoc->SetGraphicsMode(false);
				g_Trace.Mode = eTraceMode::NONE;
				break;
			case T_SCRATCH_K:
				if(pCntx->Pc.pToken->Type == T_INTNUM){
					if(Evaluate(pValue, _T("Key #"))->Type == eVType::V_ERROR || ValueRetype(pValue, eVType::V_INT)->Type == eVType::V_ERROR)	return pValue;
					UINT KeyNumber = pValue->uData.Integer;																	 
					ValueDestroy(pValue);
					pDoc->ResetSoftKey(KeyNumber);
				}
				break;
			case T_SCRATCH_P:
				DestroyAllIdentifiers(g_pLocalRegistry);
				DestroyLabels(pCntx->pSubCntx->pLabels);
				DestroyGlobals(g_pGlobalRegistry);
				ProgramDestroy(&SystemContexts[(int)eCntxType::RUN].Program);
				DestroyStack(&SystemContexts[(int)eCntxType::RUN].Stack);
				ProgramNew(&SystemContexts[(int)eCntxType::RUN].Program);
				NewStack(&SystemContexts[(int)eCntxType::RUN].Stack);
				pDoc->SetGraphicsMode(false);
				ProgramTraceClear();
				SetMainTitle(nullptr); 
				break;
			case T_SCRATCH_V:
				ResetVariables(g_pLocalRegistry);
				ProgramTraceClear();
				break;
		}
	}
	else if((TokType == T_SCRATCH_K) && (pCntx->Pc.pToken->Type == T_INTNUM)) ++pCntx->Pc.pToken;	// the only	SCRATCH statement with a parameter
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////

pValue_s stmt_SELECT(pValue_s pValue)
{
PC_s SelectPc = pCntx->Pc, CasePc;

	if(pCntx->Pass == ePass::DECLARE || pCntx->Pass == ePass::COMPILE) PushMarker(eMType::SELECT, &pCntx->Pc);
	++pCntx->Pc.pToken;
	if(Evaluate(pValue, _T("selector"))->Type == eVType::V_ERROR) return pValue;
	if(pCntx->Pass == ePass::DECLARE || pCntx->Pass == ePass::COMPILE){
		SelectPc.pToken->Obj.pSelectCase->ValueType = pValue->Type;
		SelectPc.pToken->Obj.pSelectCase->NextCaseValue.Index = -1;
	}
	else{
		int Match = 0;
		pCntx->Pc = CasePc = SelectPc.pToken->Obj.pSelectCase->NextCaseValue;
		do{
			++pCntx->Pc.pToken;
			switch(CasePc.pToken->Type){
				case T_CASE:{
					do {
						Value_s CaseValueA;
						switch(pCntx->Pc.pToken->Type){
							case T_LT:
							case T_LE:
							case T_EQ:
							case T_GE:
							case T_GT:
							case T_NE:{
								enum TokenType_e RelativeOp = pCntx->Pc.pToken->Type;
								++pCntx->Pc.pToken;
								if(Evaluate(&CaseValueA, _T("`is'"))->Type == eVType::V_ERROR){
									ValueDestroy(pValue);
									*pValue = CaseValueA;
									return pValue;
								}
								ValueRetype(&CaseValueA, SelectPc.pToken->Obj.pSelectCase->ValueType);
								assert(CaseValueA.Type != eVType::V_ERROR);
								if(!Match){
									Value_s CmpValue;
									ValueClone(&CmpValue, pValue);
									switch(RelativeOp){
										case T_LT:
											ValueLt(&CmpValue, &CaseValueA, 1);
											break;
										case T_LE:
											ValueLe(&CmpValue, &CaseValueA, 1);
											break;
										case T_EQ:
											ValueEqu(&CmpValue, &CaseValueA, 1);
											break;
										case T_GE:
											ValueGe(&CmpValue, &CaseValueA, 1);
											break;
										case T_GT:
											ValueGt(&CmpValue, &CaseValueA, 1);
											break;
										case T_NE:
											ValueNe(&CmpValue, &CaseValueA, 1);
											break;
										default:
											assert(0);
									}
									assert(CmpValue.Type == eVType::V_INT);
									Match = CmpValue.uData.Integer;
									ValueDestroy(&CmpValue);
								}
								ValueDestroy(&CaseValueA);
								break;
							}
							default:{
								if(Evaluate(&CaseValueA, _T("`case'"))->Type == eVType::V_ERROR){
									ValueDestroy(pValue);
									*pValue = CaseValueA;
									return pValue;
								}
								ValueRetype(&CaseValueA, SelectPc.pToken->Obj.pSelectCase->ValueType);
								assert(CaseValueA.Type != eVType::V_ERROR);
								if(pCntx->Pc.pToken->Type == T_TO){ /* Match range */
									Value_s CaseValueB;
									++pCntx->Pc.pToken;
									if(Evaluate(&CaseValueB, _T("`case'"))->Type == eVType::V_ERROR){
										ValueDestroy(&CaseValueA);
										ValueDestroy(pValue);
										*pValue = CaseValueB;
										return pValue;
									}
									ValueRetype(&CaseValueB, SelectPc.pToken->Obj.pSelectCase->ValueType);
									assert(CaseValueB.Type != eVType::V_ERROR);
									if(!Match){
										Value_s CmpValueA, CmpValueB;
										ValueClone(&CmpValueA, pValue);
										ValueClone(&CmpValueB, pValue);
										ValueGe(&CmpValueA, &CaseValueA, 1);
										assert(CmpValueA.Type == eVType::V_INT);
										ValueLe(&CmpValueB, &CaseValueB, 1);
										assert(CmpValueB.Type == eVType::V_INT);
										Match = CmpValueA.uData.Integer && CmpValueB.uData.Integer;
										ValueDestroy(&CmpValueA);
										ValueDestroy(&CmpValueB);
									}
									ValueDestroy(&CaseValueB);
								}
								else{ 					/* Match pValue */
									if(!Match){
										Value_s CmpValue;
										ValueClone(&CmpValue, pValue);
										ValueEqu(&CmpValue, &CaseValueA, 1);
										assert(CmpValue.Type == eVType::V_INT);
										Match = CmpValue.uData.Integer;
										ValueDestroy(&CmpValue);
									}
									ValueDestroy(&CaseValueA);
								}
								break;
							}
						}
						if(pCntx->Pc.pToken->Type == T_COMMA) ++pCntx->Pc.pToken;
						else break;
					}
					while(1);
					break;
				}
				case T_CASEELSE:{
					Match = 1;
					break;
				}
				default: assert(0);
			}
			if(!Match){
				if(CasePc.pToken->Obj.pCaseValue->NextCaseValue.Index != -1) pCntx->Pc = CasePc = CasePc.pToken->Obj.pCaseValue->NextCaseValue;
				else{
					pCntx->Pc = SelectPc.pToken->Obj.pSelectCase->EndSelect;
					break;
				}
			}
		}
		while(!Match);
	}
	ValueDestroy(pValue);
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////

pValue_s stmt_STANDARD(pValue_s pValue)
{
	++pCntx->Pc.pToken;
	if(pCntx->Pass == ePass::INTERPRET){
		pCntx->pSubCntx->RoundingPrecision = 12;
		pCntx->pSubCntx->RoundingMode = eRounding::STANDARD;
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////

pValue_s stmt_STOP(pValue_s pValue)
{
	if(pCntx->Pass == ePass::INTERPRET) pCntx->RunMode = eRunMode::STOP;
	else ++pCntx->Pc.pToken;
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////

pValue_s stmt_STORE(pValue_s pValue)
{
BString FileName;
Context_s	*pRunCntx = &SystemContexts[(int)eCntxType::RUN];

	if(pCntx->Pass == ePass::INTERPRET && !InDirectMode()) return ValueNewError(pValue, NOTINPROGRAMMODE);
	++pCntx->Pc.pToken;
	PC_s ErrorPc = pCntx->Pc;
	if(pCntx->Pc.pToken->Type == T_EOL && pCntx->Program.Name.GetLength()) FileName.AppendString(&pCntx->Program.Name);
	else{
		if(Evaluate(pValue, _T("file name"))->Type == eVType::V_ERROR || ValueRetype(pValue, eVType::V_STRING)->Type == eVType::V_ERROR){
			pCntx->Pc = ErrorPc;
			return pValue;
		}
		FileName.AppendString(pValue->uData.pString);
		ValueDestroy(pValue);
	}
	if(pCntx->Pass == ePass::INTERPRET){
		int FileIndex;
		if((FileIndex = g_pMassStorage->WriteOpen(FileName.GetBuffer(), MT_PROG, false)) == -1){ // save to context name or specified name
			pCntx->Pc = ErrorPc;
			return ValueNewError(pValue, MASSSTROREERR, g_SufixErrorMsg.GetBuffer());
		}
		if(ProgramStore(&pRunCntx->Program, FileIndex, nullptr, nullptr, pValue)){
			pCntx->Pc = ErrorPc;
			return pValue;
		}
		g_pMassStorage->Close(FileIndex);
		pRunCntx->Program.Unsaved = FALSE;
		SetMainTitle(FileName.GetBuffer()); 
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////

pValue_s stmt_SUBDEF(pValue_s pValue)
{
	if(pCntx->Pass == ePass::DECLARE || pCntx->Pass == ePass::COMPILE){
		PC_s ErrorPc = pCntx->Pc;
		Identifier_s *pFn;
		int Args = 0;
		if(InDirectMode()) return ValueNewError(pValue, NOTINDIRECTMODE);
		++pCntx->Pc.pToken;
		if(pCntx->Pc.pToken->Type != T_IDENTIFIER) return ValueNewError(pValue, MISSINGPROCIDENT);
		pFn = pCntx->Pc.pToken->Obj.pIdentifier;
		pFn->DefType = eVType::V_VOID;
		++pCntx->Pc.pToken;
		if(FindMarker(eMType::FUNC)){																																								// should not be on marker stack
			pCntx->Pc = ErrorPc;
			return ValueNewError(pValue, NESTEDDEFINITION);
		}
		if(pCntx->Pc.pToken->Type == T_OP){																																 					// if parentheses...
			if(GetFormalArgs(pValue, &Args) != nullptr) return pValue;																								// ...must specify at least 1 formal parameter
		}
		if(pCntx->Pass == ePass::DECLARE){																																			
			enum eVType *pValType = (Args) ? (eVType*)malloc(Args * sizeof(enum eVType)) : nullptr;										// get a chunk of memory for the types
			for(int i = 0; i < Args; ++i) pValType[i] = StackArgType(&pCntx->Stack, i);																// now biuld a list of the parameter types
			if(RegisterSubProg(g_pGlobalRegistry, pFn, &pCntx->Pc, &ErrorPc, Args, pValType) == 0){										// save the sub-programme	along with the formal parameter list
				free(pValType);
				StackFuncEnd(&pCntx->Stack);
				pCntx->Pc = ErrorPc;
				return ValueNewError(pValue, REDECLARATION);
			}
			pFn->pSym->SymbolType	= eSymType::SUBPROGRAMME;																																			// mark the type as a sub-programme
			ProgramAddScope(&pCntx->Program, &pFn->pSym->uType.sSubrtn.uFunc.sUser.Scope);														// and add the scope details
			pFn->pSym->uType.sSubrtn.uFunc.sUser.pSubCntx = NewSubCntx();																							// create and initialise the sub-programme context
			pFn->pSym->uType.sSubrtn.uFunc.sUser.pSubIdents = NewIdentifiers();																				// create and initialise the sub-programme identifier container
			pCntx->pDataListEnd = &pFn->pSym->uType.sSubrtn.uFunc.sUser.pSubCntx->BeginData;													// set the start of the data list
		}
		pFn->pSym->uType.sSubrtn.uFunc.sUser.pCallerCntx = pCntx->pSubCntx;																					// save calling state
		pCntx->pSubCntx = pFn->pSym->uType.sSubrtn.uFunc.sUser.pSubCntx;																						// new labels will be stored to this context
		pCntx->pSubCntx->CurrentData = pCntx->pSubCntx->BeginData;																									// set the first line of data as current
		pFn->pSym->uType.sSubrtn.uFunc.sUser.pCallerIdents = g_pLocalRegistry;																			// save calling identifiers
		g_pLocalRegistry = pFn->pSym->uType.sSubrtn.uFunc.sUser.pSubIdents;																					// new identifiers will be stored to this context
		PushMarker(eMType::FUNC, &ErrorPc);																																					// save this source index as the start of the sub-programme
	}
	else pCntx->Pc = (pCntx->Pc.pToken + 1)->Obj.pIdentifier->pSym->uType.sSubrtn.uFunc.sUser.Scope.End;
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////

pValue_s stmt_SUBEND(pValue_s pValue)
{
PC_s *pThisPc = nullptr;

	if(pCntx->Pass == ePass::DECLARE || pCntx->Pass == ePass::COMPILE){
		if((pThisPc = PopMarker(eMType::FUNC)) == nullptr || (pThisPc->pToken + 1)->Obj.pIdentifier->DefType != eVType::V_VOID){
			if(pThisPc != nullptr) PushMarker(eMType::FUNC, pThisPc);
			return ValueNewError(pValue, STRAYSUBEND, TopMarkerDescription());
		}
		pCntx->pSubCntx = (pThisPc->pToken + 1)->Obj.pIdentifier->pSym->uType.sSubrtn.uFunc.sUser.pCallerCntx;	// return memory space back to calling state
		g_pLocalRegistry = (pThisPc->pToken + 1)->Obj.pIdentifier->pSym->uType.sSubrtn.uFunc.sUser.pCallerIdents;// return identifiers back to calling state
	}
	++pCntx->Pc.pToken;
	if(pCntx->Pass == ePass::INTERPRET) return ValueNewVoid(pValue);
	else{
		if(pCntx->Pass == ePass::DECLARE) RegisterSubProgEnd(g_pGlobalRegistry, (pThisPc->pToken + 1)->Obj.pIdentifier, &pCntx->Pc);
		StackFuncEnd(&pCntx->Stack);
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////

pValue_s stmt_SUBEXIT(pValue_s pValue)
{
PC_s *pThisPc = nullptr;

	if(pCntx->Pass == ePass::DECLARE || pCntx->Pass == ePass::COMPILE){
		if((pThisPc = FindMarker(eMType::FUNC)) == nullptr || (pThisPc->pToken + 1)->Obj.pIdentifier->DefType != eVType::V_VOID)	return ValueNewError(pValue, STRAYSUBEXIT);
	}
	++pCntx->Pc.pToken;
	if(pCntx->Pass == ePass::INTERPRET) return ValueNewVoid(pValue);
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////

pValue_s stmt_SYSTEM(pValue_s pValue)
{
	++pCntx->Pc.pToken;
	if(pCntx->Pass == ePass::INTERPRET){
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////

pValue_s stmt_TRIGMODE(pValue_s pValue)
{
eTrigMode TrigMode = eTrigMode::DEG;

	switch(pCntx->Pc.pToken->Type){
		case T_DEG: TrigMode = eTrigMode::DEG; break;
		case T_GRAD: TrigMode = eTrigMode::GRAD; break;
		case T_RAD: TrigMode = eTrigMode::RAD; break;
		default: assert(0);
	}
	if(pCntx->Pass == ePass::INTERPRET){
		pCntx->pSubCntx->TrigMode = TrigMode;
	}
	++pCntx->Pc.pToken;
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////

pValue_s stmt_TRACE(pValue_s pValue)
{
TokenType_e TokType = pCntx->Pc.pToken->Type;

	++pCntx->Pc.pToken;
	switch(TokType){
		case T_NORMAL:{
			if(pCntx->Pass == ePass::INTERPRET) ProgramTraceClear();
			break;
		}
		case T_TRACE:{
			if(pCntx->Pc.pToken->Type == T_INTNUM){
				g_Trace.From = (pCntx->Pc.pToken->Obj.Integer > 0) ? pCntx->Pc.pToken->Obj.Integer : 0;
				++pCntx->Pc.pToken;
				if(pCntx->Pc.pToken->Type == T_COMMA){
					++pCntx->Pc.pToken;
					if(pCntx->Pc.pToken->Type != T_INTNUM){
						g_Trace.To = (pCntx->Pc.pToken->Obj.Integer > 0) ? pCntx->Pc.pToken->Obj.Integer : 0;
						++pCntx->Pc.pToken;
					}
					else g_Trace.To = 0;
				}
			}
			else g_Trace.From = 0;
			if(pCntx->Pass == ePass::INTERPRET)	g_Trace.Mode = eTraceMode::FLOW;
			break;
		}
		case T_TRALL:{
			g_Trace.Mode = eTraceMode::ALL;
			break;
		}
		case T_TRPAUSE:{
			if(pCntx->Pc.pToken->Type == T_INTNUM){
				g_Trace.PauseLine = (pCntx->Pc.pToken->Obj.Integer > 0) ? pCntx->Pc.pToken->Obj.Integer : 0;
				++pCntx->Pc.pToken;
				if(pCntx->Pc.pToken->Type == T_COMMA){
					++pCntx->Pc.pToken;
					if(pCntx->Pc.pToken->Type != T_INTNUM){
						g_Trace.PauseCount = (pCntx->Pc.pToken->Obj.Integer > 0) ? pCntx->Pc.pToken->Obj.Integer : 0;
						++pCntx->Pc.pToken;
					}
					else g_Trace.PauseCount = 0;
				}
			}
			else g_Trace.PauseLine = 0;
			if(pCntx->Pass == ePass::INTERPRET)	g_Trace.Pause = true;
			break;
		}
		case T_TRVARS:{
			StringListNode *pNode;
			g_Trace.VarList.RemoveAll();
			while(1){
				if((pCntx->Pc.pToken->Type != T_IDENTIFIER) || (g_Trace.VarList.GetCount() > 5)) break;
				if(pCntx->Pass == ePass::INTERPRET){	
					pNode = g_Trace.VarList.Add();
					pNode->GetString()->AppendChars(pCntx->Pc.pToken->Obj.pIdentifier->Name);
				}
				++pCntx->Pc.pToken;
				if(pCntx->Pc.pToken->Type == T_COMMA) ++pCntx->Pc.pToken;
				else break;
			}
			if(pCntx->Pass == ePass::INTERPRET) g_Trace.Mode = eTraceMode::VARS;
			break;
		}
		case T_TRALLVARS:{
			if(pCntx->Pass == ePass::INTERPRET) g_Trace.Mode = eTraceMode::ALLVARS;
			break;
		}
		case T_TRWAIT:{
			if(pCntx->Pc.pToken->Type == T_INTNUM){
				g_Trace.WaitPeriod = (pCntx->Pc.pToken->Obj.Integer > 0) ? pCntx->Pc.pToken->Obj.Integer : 0;
				++pCntx->Pc.pToken;
			}
			else g_Trace.WaitPeriod = 0;
			if(pCntx->Pass == ePass::INTERPRET){
				if(g_Trace.WaitPeriod > 0) g_Trace.Wait = true;
			}
			break;
		}
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////

pValue_s stmt_TRUNCATE(pValue_s pValue)
{
PC_s EntryPc = pCntx->Pc;
int Handle;

	++pCntx->Pc.pToken;
	if(pCntx->Pc.pToken->Type == T_CHANNEL) ++pCntx->Pc.pToken;
	if(Evaluate(pValue, nullptr) == nullptr) return ValueNewError(pValue, MISSINGEXPR, _T("stream"));
	if(pValue->Type == eVType::V_ERROR || ValueRetype(pValue, eVType::V_INT)->Type == eVType::V_ERROR) return pValue;
	Handle = pValue->uData.Integer;
	ValueDestroy(pValue);
	if(pCntx->Pass == ePass::INTERPRET && g_pMassStorage->Truncate(Handle) == -1){
		pCntx->Pc = EntryPc;
		return ValueNewError(pValue, MASSSTROREERR, g_SufixErrorMsg.GetBuffer());
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////

pValue_s stmt_UNNUM(pValue_s pValue)
{
	++pCntx->Pc.pToken;
	if(pCntx->Pass == ePass::INTERPRET){
		if(!InDirectMode()) return ValueNewError(pValue, NOTINPROGRAMMODE);
		ProgramUnnumber(&pCntx->Program);
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////

pValue_s stmt_UNTIL(pValue_s pValue)
{
PC_s untilpc = pCntx->Pc;
PC_s *pRepeatPc;

	++pCntx->Pc.pToken;
	if(Evaluate(pValue, _T("condition"))->Type == eVType::V_ERROR) return pValue;
	if(pCntx->Pass == ePass::INTERPRET){
		if(ValueIsNull(pValue)) pCntx->Pc = untilpc.pToken->Obj.UntilPc;
		ValueDestroy(pValue);
	}
	if(pCntx->Pass == ePass::DECLARE || pCntx->Pass == ePass::COMPILE){
		if((pRepeatPc = PopMarker(eMType::REPEAT)) == nullptr) return ValueNewError(pValue, STRAYUNTIL);
		untilpc.pToken->Obj.UntilPc = *pRepeatPc;
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////

pValue_s stmt_WAIT(pValue_s pValue)
{
int Address, Mask, Select = -1, UseSelect;
PC_s EntryPc = pCntx->Pc;

	++pCntx->Pc.pToken;
	if(pCntx->Pc.pToken->Type != T_FOR){
		if(Evaluate(pValue, _T("wait"))->Type == eVType::V_ERROR || ValueRetype(pValue, eVType::V_REAL)->Type == eVType::V_ERROR) return pValue;
		double SleepValue = pValue->uData.Real * WAIT_CALIBRATION;
		ValueDestroy(pValue);
		if(pCntx->Pass == ePass::INTERPRET){
			if(SleepValue < 0.0) return ValueNewError(pValue, OUTOFRANGE, _T("Wait value"));
			Sleep((DWORD)SleepValue);
		}
	}
	else{
		if(Evaluate(pValue, _T("address"))->Type == eVType::V_ERROR || ValueRetype(pValue, eVType::V_INT)->Type == eVType::V_ERROR) return pValue;
		Address = pValue->uData.Integer;
		ValueDestroy(pValue);
		if(pCntx->Pc.pToken->Type != T_COMMA) return ValueNewError(pValue, MISSINGCOMMA);
		++pCntx->Pc.pToken;
		if(Evaluate(pValue, _T("mask"))->Type == eVType::V_ERROR || ValueRetype(pValue, eVType::V_INT)->Type == eVType::V_ERROR) return pValue;
		Mask = pValue->uData.Integer;
		ValueDestroy(pValue);
		if(pCntx->Pc.pToken->Type == T_COMMA){
			++pCntx->Pc.pToken;
			if(Evaluate(pValue, _T("select"))->Type == eVType::V_ERROR || ValueRetype(pValue, eVType::V_INT)->Type == eVType::V_ERROR) return pValue;
			Select = pValue->uData.Integer;
			UseSelect = 1;
			ValueDestroy(pValue);
		}
		else UseSelect = 0;
		if(pCntx->Pass == ePass::INTERPRET){
			int PortVal;
			do{
				if((PortVal = DevicePortInput(Address)) == -1){
					pCntx->Pc = EntryPc;
					return ValueNewError(pValue, IOERROR, g_SufixErrorMsg.GetBuffer());
				}
			}
			while((UseSelect ? (PortVal ^ Select) & Mask : PortVal ^ Mask) == 0);
		}
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////

pValue_s stmt_WEND(pValue_s pValue)
{
	if((pCntx->Pass == ePass::DECLARE) || (pCntx->Pass == ePass::COMPILE)){
		PC_s *pWhilePc;
		if((pWhilePc = PopMarker(eMType::WHILE)) == nullptr) return ValueNewError(pValue, STRAYWEND, TopMarkerDescription());
		*pCntx->Pc.pToken->Obj.pWhilePc = *pWhilePc;
		++pCntx->Pc.pToken;
		*(pWhilePc->pToken->Obj.pAfterWendPc) = pCntx->Pc;
	}
	else pCntx->Pc = *pCntx->Pc.pToken->Obj.pWhilePc;
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////

pValue_s stmt_WHILE(pValue_s pValue)
{
PC_s EntryPc = pCntx->Pc;

	if(pCntx->Pass == ePass::DECLARE || pCntx->Pass == ePass::COMPILE) PushMarker(eMType::WHILE, &pCntx->Pc);
	++pCntx->Pc.pToken;
	if(Evaluate(pValue, _T("condition"))->Type == eVType::V_ERROR) return pValue;
	if(pCntx->Pass == ePass::INTERPRET){
		if(ValueIsNull(pValue)) pCntx->Pc = *EntryPc.pToken->Obj.pAfterWendPc;
		ValueDestroy(pValue);
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////

pValue_s stmt_WRITE(pValue_s pValue)
{
int Device = SC_TEXT_DISPLAY;
int Comma = 0;

	++pCntx->Pc.pToken;
	if(pCntx->Pc.pToken->Type == T_CHANNEL){
		++pCntx->Pc.pToken;
		if(Evaluate(pValue, _T("stream"))->Type == eVType::V_ERROR || ValueRetype(pValue, eVType::V_INT)->Type == eVType::V_ERROR) return pValue;
		Device = pValue->uData.Integer;
		ValueDestroy(pValue);
		if(pCntx->Pc.pToken->Type == T_COMMA) ++pCntx->Pc.pToken;
	}

	while(1){
		if(Evaluate(pValue, nullptr)){
			if(pValue->Type == eVType::V_ERROR) return pValue;
			if(pCntx->Pass == ePass::INTERPRET){
				BString Str;
				if(Comma) Str.AppendChar(',');
				if(g_pMassStorage->PrintString(Device, ValueToWrite(pValue, &Str)) == -1){
					ValueDestroy(pValue);
					return ValueNewError(pValue, MASSSTROREERR, g_SufixErrorMsg.GetBuffer());
				}
				if(g_pMassStorage->Flush(Device) == -1) return ValueNewError(pValue, MASSSTROREERR, g_SufixErrorMsg.GetBuffer());
			}
			ValueDestroy(pValue);
			Comma = 1;
		}
		else if((pCntx->Pc.pToken->Type == T_COMMA) || (pCntx->Pc.pToken->Type == T_SEMICOLON)) ++pCntx->Pc.pToken;
		else break;
	}
	if(pCntx->Pass == ePass::INTERPRET){
		g_pMassStorage->Put(Device, '\n');
		if(g_pMassStorage->Flush(Device) == -1) return ValueNewError(pValue, MASSSTROREERR, g_SufixErrorMsg.GetBuffer());
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////

