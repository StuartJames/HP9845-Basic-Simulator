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

#include <fcntl.h>      /* Needed only for _O_RDWR definition */
#include <io.h>

static Value_s *SubProgram(Value_s *pValue, bool NoArgs = false);
Value_s *ProcessErrorEvent(Value_s *pValue);
Value_s *ProcessKeyEvents(Value_s *pValue);

const ePass PassTable[ePass::END] = { ePass::COMPILE, ePass::INTERPRET,	ePass::DECLARE };	 // pass look up table
static inline void NextPass(void){ pCntx->Pass = PassTable[(int)pCntx->Pass]; };					 // simple function to lookup next pass enum

/////////////////////////////////////////////////////////////////////////////////////////////////

unsigned int Hash(const char *pStr)
{
unsigned int HashCode = 0;

	while(*pStr){
		HashCode = HashCode * 256 + tolower((int)*pStr);
		++pStr;
	}
	return HashCode % IDENT_HASHSIZE;
}

/////////////////////////////////////////////////////////////////////////////////////////////////

int FindIdentifier(Registry_s *pRegistry, Identifier_s *pIdent, eSymType SymbolType)
{
Symbol_s **ppSym;
unsigned int HashVal = Hash(pIdent->Name); 

	for(ppSym = &BuiltInLibrary.pTable[HashVal]; *ppSym != nullptr; ppSym = &((*ppSym)->pNext)) if(!strcmp((*ppSym)->pName, pIdent->Name)) break;					// first search for builtin functions
	if(*ppSym == nullptr){
		if(LocalFind(pRegistry, pIdent, SymbolType)) return 1;
		if(GlobalFind(pIdent, SymbolType)) return 1;
		return CommonFind(pIdent, (SymbolType == eSymType::LOCALARRAY) ? eSymType::COMNARRAY : eSymType::COMNVAR);																		// lastly search the COM registry, modifying the VAR type
	}
	pIdent->pSym = (*ppSym);
	return 1;
}

//////////////////////////////////////////////////////////////////////

int FindOrRegister(Registry_s *pRegistry, Identifier_s *pIdent)			// Find an identifier or register it if not found providing it's a string or simple type
{
	if((StackFind(&pCntx->Stack, pIdent) == 0) && (FindIdentifier(g_pLocalRegistry, pIdent, LibSearchType((pCntx->Pc.pToken + 1)->Type, pIdent->DefType)) == 0)){
		if((pCntx->Pc.pToken + 1)->Type == T_OP) return 0;
		if(RegisterVariable(g_pLocalRegistry, pIdent,	pIdent->DefType, ((pIdent->DefType == eVType::V_STRING) ? eSymType::LOCALARRAY : eSymType::LOCALVAR), 0) == 0) return 0;
	}
	return 1;
}

//////////////////////////////////////////////////////////////////////

static void PushMarker(enum eMType type, PC_s *pPc)
{
MarkerItem_s *pNewStack = nullptr;

	if(pCntx->Marker.Index == pCntx->Marker.Size){
		if((pNewStack = (MarkerItem_s*)realloc(pCntx->Marker.pStack, sizeof(MarkerItem_s) * ++pCntx->Marker.Size )) == nullptr) return;
		pCntx->Marker.pStack = pNewStack;
	}
	pCntx->Marker.pStack[pCntx->Marker.Index].Type = type;
	pCntx->Marker.pStack[pCntx->Marker.Index++].Pc = *pPc;
}

//////////////////////////////////////////////////////////////////////

static PC_s *PopMarker(enum eMType Type)
{
	if((pCntx->Marker.Index == 0) || (pCntx->Marker.pStack[pCntx->Marker.Index - 1].Type != Type)) return nullptr;
	else return &pCntx->Marker.pStack[--pCntx->Marker.Index].Pc;
}

//////////////////////////////////////////////////////////////////////

static PC_s *FindMarker(enum eMType Type)
{
int i;

	for(i = pCntx->Marker.Index - 1; i >= 0; --i)	if(pCntx->Marker.pStack[i].Type == Type) return &pCntx->Marker.pStack[i].Pc;
	return nullptr;
}

//////////////////////////////////////////////////////////////////////

static void MarkerStackError(Value_s *v)
{
	assert(pCntx->Marker.Index);
	pCntx->Pc = pCntx->Marker.pStack[pCntx->Marker.Index - 1].Pc;
	switch(pCntx->Marker.pStack[pCntx->Marker.Index - 1].Type){
		case eMType::IF:
			ValueNewError(v, STRAYIF);
			break;
		case eMType::DO:
			ValueNewError(v, STRAYDO);
			break;
		case eMType::DO_CND:
			ValueNewError(v, STRAYDOcondition);
			break;
		case eMType::ELSE:
			ValueNewError(v, STRAYELSE2);
			break;
		case eMType::FOR_BODY:{
			ValueNewError(v, STRAYFOR);
			pCntx->Pc = *FindMarker(eMType::FOR);
			break;
		}
		case eMType::WHILE:
			ValueNewError(v, STRAYWHILE);
			break;
		case eMType::REPEAT:
			ValueNewError(v, STRAYREPEAT);
			break;
		case eMType::SELECT:
			ValueNewError(v, STRAYSELECTCASE);
			break;
		case eMType::FUNC:
			ValueNewError(v, STRAYFUNC);
			break;
		default: assert(0);
	}
}

//////////////////////////////////////////////////////////////////////

static const char *TopMarkerDescription(void)
{
	if(pCntx->Marker.Index == 0) return _T("program");
	switch(pCntx->Marker.pStack[pCntx->Marker.Index - 1].Type){
		case eMType::IF:
			return _T("'if' branch");
		case eMType::DO:
			return _T("'do' loop");
		case eMType::DO_CND:
			return _T("'do while' or `do until' loop");
		case eMType::ELSE:
			return _T("'else' branch");
		case eMType::FOR_BODY:
			return _T("'for' loop");
		case eMType::WHILE:
			return _T("'while' loop");
		case eMType::REPEAT:
			return _T("'repeat' loop");
		case eMType::SELECT:
			return _T("'select' control structure");
		case eMType::FUNC:
			return _T("function or sub-programme");
		default:
			assert(0);
	}
	return nullptr;		/* NOTREACHED */
}

//////////////////////////////////////////////////////////////////////

bool BiuldStringUsing(pValue_s pVal, BString *pDestStr, BString *pUsingStr, UINT *pUsingPos)
{
char Pad = ' ';
int HeadingSign;
int Width = 0;
int Commas = 0;
int Precision = -1;
int Exponent = 0;
int TrailingSign = 0;
bool ValValid = true;

	HeadingSign = (pUsingStr->GetLength() > 0) ? 0 : -1;
	if(*pUsingPos == pUsingStr->GetLength()) *pUsingPos = 0;
	while(*pUsingPos < pUsingStr->GetLength()){
		switch(pUsingStr->GetAt(*pUsingPos)){
			case '"':{
				++(*pUsingPos);
				while((pUsingStr->GetAt(*pUsingPos) != '"') && (*pUsingPos < pUsingStr->GetLength())) pDestStr->AppendChar(pUsingStr->GetAt((*pUsingPos)++));
				++(*pUsingPos);
				break;
			}
			case '#':
			case ',':{
				++(*pUsingPos);
				break;
			}
			case '/':{
				++(*pUsingPos);
				pDestStr->AppendChar('\n');
				break;
			}
			case '@':{
				++(*pUsingPos);
				pDestStr->AppendChar('\f');
				break;
			}
			case '+':{
				if(*pUsingPos > 0) HeadingSign = 1;
				++(*pUsingPos);
				break;
			}
			case '-':{
				if(*pUsingPos > 0){
					if(HeadingSign == 0) HeadingSign = 2;
					TrailingSign = -1;
				}
				++(*pUsingPos);
				break;
			}
			case '*':
			case 'D':
			case 'Z':{ /* output numeric */
				if(!ValValid) return true;			 // exit if no more items
				ValValid = false;
				switch(pUsingStr->GetAt(*pUsingPos)){
					case '*': Pad = '*'; break;
					case 'Z': Pad = '0'; break;
					case 'D': Pad = ' '; break;
				}
				++(*pUsingPos);
				if(Width < 1) for(Width = 1; pUsingStr->GetAt(*pUsingPos) == 'D'; ++Width) (*pUsingPos)++;
				if(pUsingStr->GetAt(*pUsingPos) == '.'){
					(*pUsingPos)++;
					for(Precision = 0; pUsingStr->GetAt(*pUsingPos) == 'D'; ++Precision) (*pUsingPos)++;
				}
				else Precision = -1;
				ValueToString(pVal, pDestStr, Pad, HeadingSign, Width, Commas, Precision, Exponent, TrailingSign);
				Width = 0;
				break;
			}
			case 'A':{ /* output alpha */
				if(!ValValid) return true;			 // exit if no more items
				ValValid = false;
				++(*pUsingPos);
				ValueToString(pVal, pDestStr, Pad, HeadingSign, Width, Commas, Precision, Exponent, TrailingSign);
				Width = 0;
				break;
			}
			case 'K':{																											// output alpha 
				if(!ValValid) return true;			 // exit if no more items
				ValValid = false;
				++(*pUsingPos);
				ValueToString(pVal, pDestStr, Pad, HeadingSign, Width, Commas, Precision, Exponent, TrailingSign);
				break;
			}
			case 'M':{
				HeadingSign = -1;
				++(*pUsingPos);
				break;
			}
			case 'S':{
				HeadingSign = 1;
				++(*pUsingPos);
				break;
			}
			case 'X':		/* blank spaces */
				Width = (Width > 0) ? Width : 1;
				while(Width--) pDestStr->AppendChar(Pad);
				++(*pUsingPos);
				Width = 0;
				break;
			default:{
				if(isdigit(pUsingStr->GetAt(*pUsingPos))){		 /* look for count value */
					Width = atoi(pUsingStr->GetBuffer(*pUsingPos));
					while(isdigit(pUsingStr->GetAt(*pUsingPos))) (*pUsingPos)++;
				}
				else (*pUsingPos)++;
			}
		}
	}
	return false;
}

//////////////////////////////////////////////////////////////////////

pValue_s  BuildArrayStringUsing(pValue_s pValue, BString *pPrintStr, BString *pUsingStr, UINT *pUsingPos)
{
Var_s	*pVar = pValue->uData.pArray;
UINT i, Count = 1, UsingPos = 0;

	if((Count = GetArraySize(pVar)) < 1) return ValueNewError(pValue, NOMATRIX, pVar->Dimensions);
	for(i = 0; i < Count; ++i) BiuldStringUsing(&(pVar->pValue[i]), pPrintStr, pUsingStr, &UsingPos);
	return nullptr;
}

/////////////////////////////////////////////////////////////////////////////

static Value_s *GetValue(pValue_s pValue)
{
	Symbol_s *pSym = pCntx->Pc.pToken->Obj.pIdentifier->pSym;
	assert(pCntx->Pass == ePass::DECLARE || pSym->SymbolType == eSymType::LOCALVAR || pSym->SymbolType == eSymType::LOCALARRAY || pSym->SymbolType == eSymType::STACKVAR || pSym->SymbolType == eSymType::COMNVAR || pSym->SymbolType == eSymType::COMNARRAY);
	if(((pCntx->Pc.pToken + 1)->Type == T_OP) || (pCntx->Pc.pToken->Obj.pIdentifier->DefType == eVType::V_STRING) || (pSym->SymbolType == eSymType::LOCALARRAY)){		// array and string variables
		unsigned int Dim = 0;
		int SubScripts[DEF_MAX_DIMENSIONS] = {0}, n = 0, m = 0;
		eSSType SubStrType = eSSType::NONE;
		PC_s SavePc;
		
		++pCntx->Pc.pToken;
		if(pCntx->Pc.pToken->Type == T_OP){
			++pCntx->Pc.pToken;
			while(Dim < DEF_MAX_DIMENSIONS){
				SavePc = pCntx->Pc;
				if(Evaluate(pValue, _T("index"))->Type == eVType::V_ERROR || DoValueRetype(pValue, eVType::V_INT)->Type == eVType::V_ERROR){
					pCntx->Pc = SavePc;
					return pValue;
				}
				if(pCntx->Pass == ePass::INTERPRET){
					if(pValue->uData.Integer < -1){
						pCntx->Pc = SavePc;
						return ValueNewError(pValue, INVALIDMATDIM);
					}
					SubScripts[Dim] = pValue->uData.Integer;
					++Dim;
				}
				ValueDestroy(pValue);
				if(pCntx->Pc.pToken->Type == T_COMMA) ++pCntx->Pc.pToken;
				else break;
			}
			if(pCntx->Pc.pToken->Type != T_CP){
				assert(pCntx->Pass != ePass::INTERPRET);
				return ValueNewError(pValue, MISSINGCP);
			}
			else ++pCntx->Pc.pToken;
		}
		if(pCntx->Pc.pToken->Type == T_OSB){
			++pCntx->Pc.pToken;
			SavePc = pCntx->Pc;
			if((Evaluate(pValue, _T("position"))->Type == eVType::V_ERROR) || (DoValueRetype(pValue, eVType::V_INT)->Type == eVType::V_ERROR)){
				pCntx->Pc = SavePc;
				return pValue;
			}
			SubStrType = eSSType::POSITN;
			if((pCntx->Pass == ePass::INTERPRET) && (pValue->uData.Integer < 1)){
				pCntx->Pc = SavePc;
				return ValueNewError(pValue, SUBSCRIPTVALUE);
			}
			n = pValue->uData.Integer;
			ValueDestroy(pValue);
			if((pCntx->Pc.pToken->Type == T_COMMA) || (pCntx->Pc.pToken->Type == T_SEMICOLON)){
				SubStrType = (pCntx->Pc.pToken->Type == T_COMMA) ? eSSType::RANGE : eSSType::LENGTH;			
				++pCntx->Pc.pToken;
				if(pSym->uType.Var.pValue->uData.pString->SSType() == eSSType::RANGE){						// Substring is a start and an end character position. A$[n; m] {m >= n - 1}
					if((Evaluate(pValue, _T("end pos"))->Type == eVType::V_ERROR) || (DoValueRetype(pValue, eVType::V_INT)->Type == eVType::V_ERROR)){
						pCntx->Pc = SavePc;
						return pValue;
					}
					m = pValue->uData.Integer;
					ValueDestroy(pValue);
					if((pCntx->Pass == ePass::INTERPRET) && ((n - 1) > m)){
						pCntx->Pc = SavePc;
						return ValueNewError(pValue, SUBSCRIPTVALUE);
					}
				}
				else{																																							// Substring is a starting character position and a length. A$[n, m] {m >= 0}
					if((Evaluate(pValue, _T("length"))->Type == eVType::V_ERROR) || (DoValueRetype(pValue, eVType::V_INT)->Type == eVType::V_ERROR)){
						pCntx->Pc = SavePc;
						return pValue;
					}
					m = pValue->uData.Integer;
					ValueDestroy(pValue);
					if((pCntx->Pass == ePass::INTERPRET) && (m < 1)){
						pCntx->Pc = SavePc;
						return ValueNewError(pValue, SUBSCRIPTVALUE);
					}
				}
			}
			if(pCntx->Pc.pToken->Type != T_CSB) return ValueNewError(pValue, MISSINGCSB);
			++pCntx->Pc.pToken;
		}
		else SubStrType = eSSType::NONE;			
		SavePc = pCntx->Pc;
		switch(pCntx->Pass){
			case ePass::INTERPRET:{																								// if not a stack variable and no index or the index is negative using the '*' character, the whole array was specified 
				if((SubScripts[0] < 0) || ((Dim == 0) && (pSym->uType.Var.Dimensions > 0) && (pSym->SymbolType != eSymType::STACKVAR))){	
					if((pValue = VarVar((pSym->SymbolType != eSymType::STACKVAR) ? &(pSym->uType.Var) : StackGetAt(&pCntx->Stack, pSym->uType.Stack.Offset), pValue))->Type == eVType::V_ERROR) pCntx->Pc = SavePc;							// take a reference to the array
				}
				else{																															 // descrete values, either array or strings 
					if((pValue = VarValue((pSym->SymbolType != eSymType::STACKVAR) ? &(pSym->uType.Var) : StackGetAt(&pCntx->Stack, pSym->uType.Stack.Offset), Dim, SubScripts, pValue))->Type == eVType::V_ERROR) pCntx->Pc = SavePc;
					else{
						if(pValue->Type == eVType::V_STRING) pValue->uData.pString->SetSSType(SubStrType, n, m);
					}
				}
				return pValue;
			}
			case ePass::DECLARE:{
				return ValueNullValue(eVType::V_INT);
			}
			case ePass::COMPILE:{
				return ValueNullValue((pSym->SymbolType != eSymType::STACKVAR) ? pSym->uType.Var.Type : StackVarType(&pCntx->Stack, pSym));
			}
			default:
				assert(0);
		}
		return nullptr;
	}
	else{																																								// simple variable
		++pCntx->Pc.pToken;
		switch(pCntx->Pass){
			case ePass::DECLARE:
				return ValueNullValue(eVType::V_INT);
			case ePass::COMPILE:
				return ValueNullValue(((pSym->SymbolType == eSymType::LOCALVAR) || (pSym->SymbolType == eSymType::COMNVAR)) ? pSym->uType.Var.Type : StackVarType(&pCntx->Stack, pSym));
			case ePass::INTERPRET:
				return VAR_SCALAR_VALUE(((pSym->SymbolType == eSymType::LOCALVAR) || (pSym->SymbolType == eSymType::COMNVAR)) ? &(pSym->uType.Var) : StackGetAt(&pCntx->Stack, pSym->uType.Stack.Offset));
			default:
				assert(0);
		}
		return nullptr;
	}
}

//////////////////////////////////////////////////////////////////////

static Value_s *AssignValue(Value_s *pValue)
{
pValue_s pVal;

	Identifier_s *pIdentifier = pCntx->Pc.pToken->Obj.pIdentifier;
	if(pCntx->Pass == ePass::DECLARE){
		if((StackFind(&pCntx->Stack, pIdentifier) == 0) && (FindIdentifier(g_pLocalRegistry, pIdentifier, LibSearchType((pCntx->Pc.pToken + 1)->Type, pIdentifier->DefType)) == 0) 
			&& (RegisterVariable(g_pLocalRegistry, pIdentifier,	pIdentifier->DefType, (((pCntx->Pc.pToken + 1)->Type == T_OP) || (pIdentifier->DefType == eVType::V_STRING)) ? eSymType::LOCALARRAY : eSymType::LOCALVAR, 0) == 0)
		)	return ValueNewError(pValue, REDECLARATION);
	}
	g_Trace.pIdentifier = pIdentifier;
	if((pVal = GetValue(pValue))->Type == eVType::V_ERROR) return pValue;
	if(pCntx->Pc.pToken->Type != T_EQ) return ValueNewError(pValue, MISSINGEQ);
	++pCntx->Pc.pToken;
	PC_s Pc = pCntx->Pc;
	if(Evaluate(pValue, _T("rhs"))->Type == eVType::V_ERROR) return pValue;
	if(pCntx->Pass != ePass::DECLARE && DoValueRetype(pValue, pVal->Type)->Type == eVType::V_ERROR){
		pCntx->Pc = Pc;
		return pValue;
	}
	return ValueAssign(pVal, pValue, pCntx->Pass == ePass::INTERPRET);
}

//////////////////////////////////////////////////////////////////////

static Value_s *GetPassParameters(Value_s *pValue, Identifier_s *pIdentifier,	int *pFirstSlot, int *pArgs, bool IsSubProg)
{
Symbol_s *pSymbol = nullptr;

	++pCntx->Pc.pToken;
	if(pCntx->Pc.pToken->Type != T_CP){
		while(1){																																																			// push arguments to Stack
			if(pCntx->Pass == ePass::DECLARE){																																					// check parameter syntax
				if(Evaluate(pValue, _T("parameter"))->Type == eVType::V_ERROR) return pValue;
				ValueDestroy(pValue);
			}
			else{
				if(pCntx->Pc.pToken->Type == T_IDENTIFIER) pSymbol = pCntx->Pc.pToken->Obj.pIdentifier->pSym;							// save symbol for variable as reference 
				Var_s *pVar = StackPushArg(&pCntx->Stack);																																// get a new stack slot
				VarNewScalar(pVar);																																												// create new value on the stack
				pCntx->IsPassVar = IsSubProg;																																							// this will get reset if the evaluation is a nemeric expression or substring
				if(Evaluate(pVar->pValue, nullptr)->Type == eVType::V_ERROR){																							// recover the parameter details
					ValueClone(pValue, pVar->pValue);																																				// make a copy
					while(pCntx->Stack.Position > *pFirstSlot) VarDestroy(&pCntx->Stack.pSlot[--pCntx->Stack.Position].Var);// error: remove values previously pushed onto the stack
					return pValue;
				}
				if((pCntx->Pass == ePass::INTERPRET) && pCntx->IsPassVar){																								// references are only valid in the interpret pass
					pVar->IsReference = true;																																								// mark as pass by reference 
					if(pSymbol->SymbolType == eSymType::STACKVAR){																																		// if source variable is a stack item 
						Var_s *pStackVar = StackGetVar(&pCntx->Stack, pSymbol->uType.Stack.Offset);														// it could be a reference
						if(pStackVar->IsReference){
							pVar->pRefVar = pStackVar->pRefVar;																																	// plugin source reference
							pVar->Type = pVar->pRefVar->Type;																																		// make the variable the same type			
						}
						else{
							pVar->pRefVar = pStackVar;																																					// plugin source reference
							pVar->Type = pStackVar->Type;																																				// make the variable the same type			
						}
					}
					else{
						pVar->pRefVar = &pSymbol->uType.Var;																																	// plugin source reference
						pVar->Type = pSymbol->uType.Var.Type;																																	// make the variable the same type			
					}
					pVar->pValue->Type = pVar->Type;																																				// make the value the same type	
				}
				if(!pVar->IsReference){
					pVar->Type = pVar->pValue->Type;																																				// make the value same type	
				}
				pCntx->IsPassVar = false;																																									// safe to reset this as can't reference a function
				if(pIdentifier->pSym->uType.sSubrtn.ArgLength < 0){																												// variable argument function, expand arg list as nessesary
					pIdentifier->pSym->uType.sSubrtn.pArgTypes = (eVType*)realloc(pIdentifier->pSym->uType.sSubrtn.pArgTypes, sizeof(enum eVType) * (*pArgs + 1));
					pIdentifier->pSym->uType.sSubrtn.pArgTypes[*pArgs] = pVar->Type;																				// save the type info to the list
				}
			}
			++(*pArgs);
			if(pCntx->Pc.pToken->Type == T_COMMA) ++pCntx->Pc.pToken;																									// loop until all parameters evaluated
			else break;																																																
		}
	}
	if(pCntx->Pc.pToken->Type != T_CP){
		if(pCntx->Pass != ePass::DECLARE) while(pCntx->Stack.Position > *pFirstSlot) VarDestroy(&pCntx->Stack.pSlot[--pCntx->Stack.Position].Var);
		return ValueNewError(pValue, MISSINGCP);
	}
	++pCntx->Pc.pToken;
	return pValue;
}

//////////////////////////////////////////////////////////////////////

static Value_s *Function(Value_s *pValue)
{
Identifier_s *pIdentifier;
PC_s SavePc = pCntx->Pc;
int FirstSlot = -99;
int Args = 0;
int ArgError;
int OverLoaded;
bool IsRange = false;
Symbol_s *pSymbol = nullptr;

	assert(pCntx->Pc.pToken->Type == T_IDENTIFIER);
	/* Evaluating a function in direct mode may start a program, so it needs to	be compiled.  If in direct mode, programs will be compiled after the
	   direct mode pass ePass::DECLARE, but errors are ignored at that point, because	the program may not be needed.  If the program is fine, its symbols will
	   be available during the compile phase already.  If not and we need it at this point, compile it again to get the error and abort. */
	if(InDirectMode() && !pCntx->Program.Runable && (pCntx->Pass != ePass::DECLARE)){
		if(CompileProgram(pValue, 0)->Type == eVType::V_ERROR) return pValue;
		ValueDestroy(pValue);
	}
	pIdentifier = pCntx->Pc.pToken->Obj.pIdentifier;
	assert((pCntx->Pass == ePass::DECLARE) || (pIdentifier->pSym->SymbolType == eSymType::BUILTINFUNCTION) || (pIdentifier->pSym->SymbolType == eSymType::USERFUNCTION) || (pIdentifier->pSym->SymbolType == eSymType::SUBPROGRAMME));
	++pCntx->Pc.pToken;
	if(pCntx->Pass != ePass::DECLARE){
		FirstSlot = pCntx->Stack.Position;																																						// get the head of the stack frame for return address
		if((pIdentifier->pSym->SymbolType == eSymType::USERFUNCTION) && (pIdentifier->pSym->uType.sSubrtn.RetType != eVType::V_VOID)){	// check for a return value
			Var_s *pVar = StackPushArg(&pCntx->Stack);																																	// get a new stack slot
			VarNew(pVar, pIdentifier->pSym->uType.sSubrtn.RetType, 0, nullptr);																					// create a new variable of the return type
		}
	}
	if(pCntx->Pc.pToken->Type == T_OP){ 
		if(GetPassParameters(pValue, pIdentifier, &FirstSlot, &Args, false)->Type == eVType::V_ERROR) return pValue;					// functions must have parentheses
	}
	if(pCntx->Pc.pToken->Type == T_OSB){																																						// check for string variables in declare pass only
		if(pIdentifier->DefType != eVType::V_STRING)	return ValueNewError(pValue, TYPEMISMATCH4);
		++pCntx->Pc.pToken;
		if(pCntx->Pass == ePass::DECLARE){																																						// just check substring declaration
			if(Evaluate(pValue, _T("position"))->Type == eVType::V_ERROR) return pValue;
			ValueDestroy(pValue);
		}
		if((pCntx->Pc.pToken->Type == T_COMMA) || (pCntx->Pc.pToken->Type == T_SEMICOLON)){
			if(pCntx->Pc.pToken->Type == T_SEMICOLON) IsRange = true;																										// Substring is a start and end character position. A$[n; m] {m >= n - 1}
			++pCntx->Pc.pToken;
			if(pCntx->Pass == ePass::DECLARE){
				if(IsRange){
					if(Evaluate(pValue, _T("end pos"))->Type == eVType::V_ERROR) return pValue;
				}
				else if(Evaluate(pValue, _T("length"))->Type == eVType::V_ERROR) return pValue;
				ValueDestroy(pValue);
			}
		}
		if(pCntx->Pc.pToken->Type != T_CSB) return ValueNewError(pValue, MISSINGCSB);
		++pCntx->Pc.pToken;
	}
	if(pCntx->Pass == ePass::DECLARE) ValueNewNull(pValue, pIdentifier->DefType);
	else{
		int NoMore;
		pCntx->IsPassVar = false;																																												// cannot take a reference of a function
		if((pCntx->Pass == ePass::INTERPRET) && (pIdentifier->pSym->SymbolType == eSymType::USERFUNCTION)){
			for(int i = 0; i < pIdentifier->pSym->uType.sSubrtn.uFunc.sUser.LocalLength; ++i){
				Var_s *pVal = StackPushArg(&pCntx->Stack);
				VarNew(pVal, pIdentifier->pSym->uType.sSubrtn.uFunc.sUser.pLocalTypes[i], 0, nullptr);
			}
		}
		StackPushFuncRet(&pCntx->Stack, FirstSlot, &pCntx->Pc);																												// push the return address onto the stack at the head of the frame
		pSymbol = pIdentifier->pSym;
		OverLoaded = (pCntx->Pass == ePass::COMPILE && pSymbol->SymbolType == eSymType::BUILTINFUNCTION && pSymbol->uType.sSubrtn.uFunc.uBuiltIn.pNext);	 // Overload will point to next function with same name
		do{
			NoMore = (pCntx->Pass != ePass::DECLARE && !(pSymbol->SymbolType == eSymType::BUILTINFUNCTION && pSymbol->uType.sSubrtn.uFunc.uBuiltIn.pNext));
			ArgError = 0;
			if(pSymbol->uType.sSubrtn.ArgLength >= 0){																																 // check pushed args against function requirements
				if(Args < pSymbol->uType.sSubrtn.ArgLength){																														 // too few
					if(NoMore) ValueNewError(pValue, TOOFEW);
					ArgError = 1;
				}
				if(Args > pSymbol->uType.sSubrtn.ArgLength){																															// to many
					if(NoMore) ValueNewError(pValue, TOOMANY);
					ArgError = 1;
				}
				if(!ArgError){
					for(int i = 0; i < Args; ++i){
						Var_s *pVar = StackGetAt(&pCntx->Stack, i);																														// get the root container
						Value_s *pVal = VarValue(pVar, pVar->Dimensions, nullptr, pValue);																		// value should be the same as Var type 
						assert(pVal->Type != eVType::V_ERROR);
						if(OverLoaded){
							if(pVal->Type != pSymbol->uType.sSubrtn.pArgTypes[i]){
								if(NoMore) ValueNewError(pValue, TYPEMISMATCH2, i + 1);
								ArgError = 1;
								break;
							}
						}
						else if(ValueRetype(pVal, pSymbol->uType.sSubrtn.pArgTypes[i])->Type == eVType::V_ERROR){							// will it re-type?
							if(NoMore) ValueNewError(pValue, TYPEMISMATCH3, pVal->uData.Error.pStr, i + 1);
							ArgError = 1;
							break;
						}
					}
				}
			}
			if(ArgError){
				if(NoMore){
					StackFuncReturn(&pCntx->Stack, nullptr);
					pCntx->Pc = SavePc;
					return pValue;
				}
				else pSymbol = pSymbol->uType.sSubrtn.uFunc.uBuiltIn.pNext;
			}
		}
		while(ArgError);
		pIdentifier->pSym = pSymbol;
		if(pSymbol->SymbolType == eSymType::BUILTINFUNCTION){
			if(pCntx->Pass == ePass::INTERPRET){
				if(pSymbol->uType.sSubrtn.uFunc.uBuiltIn.call(pValue, &pCntx->Stack)->Type == eVType::V_ERROR) pCntx->Pc = SavePc;
			}
			else ValueNewNull(pValue, pSymbol->uType.sSubrtn.RetType);
		}
		else if(pSymbol->SymbolType == eSymType::USERFUNCTION){
			if(pCntx->Pass == ePass::INTERPRET){																																							// here we process function statement similar to the main program loop
				int r = 1;
				pCntx->Pc = pSymbol->uType.sSubrtn.uFunc.sUser.Scope.Start;
				ProgramSkipEOL(&pCntx->Program, &pCntx->Pc);
				do{
					if(pCntx->OnEventsEnabled && g_EventPending) ProcessKeyEvents(pValue);																				// check for asynchronous key events
					else Statements(pValue);																																											// process each statement
					if(pValue->Type == eVType::V_ERROR){																																					// check for returned errors
						if(strchr(pValue->uData.Error.pStr, '\n') == nullptr){
							StackSetError(&pCntx->Stack, ProgramLineNumber(&pCntx->Program, &pCntx->Pc), &pCntx->Pc, pValue);					// record the error on the stack
							ProgramPCtoError(&pCntx->Program, &pCntx->Pc, pValue);
						}
						if(pCntx->Stack.OnErrorPc.Index != -1) ProcessErrorEvent(pValue);
						else{
							StackFrameToError(&pCntx->Stack, &pCntx->Program, pValue);
							break;
						}
					}
					else if(pValue->Type != eVType::V_NIL) break;
					ValueDestroy(pValue);
				}
				while((r = ProgramSkipEOL(&pCntx->Program, &pCntx->Pc)));
				if(!r) ValueNewVoid(pValue);
			}
			else ValueNewNull(pValue, pSymbol->uType.sSubrtn.RetType);
		}
		StackFuncReturn(&pCntx->Stack, pCntx->Pass == ePass::INTERPRET && pValue->Type != eVType::V_ERROR ? &pCntx->Pc : nullptr);
	}
	return pValue;
}

//////////////////////////////////////////////////////////////////////

static Value_s *SubProgram(Value_s *pValue, bool NoArgs/*=false*/)
{
Identifier_s *pIdentifier;
Registry_s *pSubIdetifiers = nullptr;
PC_s SavePc = pCntx->Pc;
int FirstSlot = -99;
int Args = 0;
int ArgError = 0;
Symbol_s *pSymbol = nullptr;

	assert(pCntx->Pc.pToken->Type == T_IDENTIFIER);
	pIdentifier = pCntx->Pc.pToken->Obj.pIdentifier;
	assert((pCntx->Pass == ePass::DECLARE) || (pIdentifier->pSym->SymbolType == eSymType::SUBPROGRAMME));
	g_Trace.pLastPc = &pCntx->Pc;
	++pCntx->Pc.pToken;
	if(pCntx->Pass != ePass::DECLARE){
		FirstSlot = pCntx->Stack.Position;																																						// get the head of the stack frame for return address
		if(pIdentifier->pSym->uType.sSubrtn.RetType != eVType::V_VOID){																								// check for a return value, must be void
			return ValueNewError(pValue, VOIDVALUE);
		}
	}
	if(pCntx->Pc.pToken->Type == T_OP){ 																																						// parentheses for formal arguments
		if(NoArgs) return ValueNewError(pValue, PASSINGPARAMETERS);
		if(GetPassParameters(pValue, pIdentifier, &FirstSlot, &Args, true)->Type == eVType::V_ERROR) return pValue;					
	}
	if(pCntx->Pass == ePass::DECLARE) ValueNewNull(pValue, pIdentifier->DefType);
	else{
		if(pCntx->Pass == ePass::INTERPRET){
			for(int i = 0; i < pIdentifier->pSym->uType.sSubrtn.uFunc.sUser.LocalLength; ++i){													// generate the local stack variables
				Var_s *pVar = StackPushArg(&pCntx->Stack);
				VarNew(pVar, pIdentifier->pSym->uType.sSubrtn.uFunc.sUser.pLocalTypes[i], 0, nullptr);
			}
		}
		StackPushFuncRet(&pCntx->Stack, FirstSlot, &pCntx->Pc);																												// push the return address onto the stack at the head of the frame
		pSymbol = pIdentifier->pSym;
		if(pSymbol->uType.sSubrtn.ArgLength >= 0){																																		// check pushed args against function requirements
			if(Args < pSymbol->uType.sSubrtn.ArgLength){																																// too few
				ValueNewError(pValue, TOOFEW);
				ArgError = 1;
			}
			if(Args > pSymbol->uType.sSubrtn.ArgLength){																																// to many
				ValueNewError(pValue, TOOMANY);
				ArgError = 1;
			}
			if(!ArgError){
				for(int i = 0; i < Args; ++i){
					assert(pCntx->Stack.FrameSize > (i + 2));
					Var_s *pVar = StackGetAt(&pCntx->Stack, i);																															// get the root container
					Value_s *pVal = VarValue(pVar, pVar->Dimensions, nullptr, pValue);																			// value should be the same as Var type 
					assert(pVal->Type != eVType::V_ERROR);
					if(ValueRetype(pVal, pSymbol->uType.sSubrtn.pArgTypes[i])->Type == eVType::V_ERROR){										// will it re-type?
						ValueNewError(pValue, TYPEMISMATCH3, pVal->uData.Error.pStr, i + 1);
						ArgError = 1;
						break;
					}
				}
			}
		}
		if(ArgError){
			StackFuncReturn(&pCntx->Stack, nullptr);
			pCntx->Pc = SavePc;
			return pValue;
		}
		pIdentifier->pSym = pSymbol;
		if(pCntx->Pass == ePass::INTERPRET){																																			// here we process statements similar to the main program loop
			int Res = 1;
			pCntx->Pc = pSymbol->uType.sSubrtn.uFunc.sUser.Scope.Start;
			pCntx->pSubCntx->ppFileTable = g_pMassStorage->NewFCB();																								// create new FCB and save old one
			pSymbol->uType.sSubrtn.uFunc.sUser.pCallerCntx = pCntx->pSubCntx;																				// sub-programs work on their own memory space. save labels
			pCntx->pSubCntx = pSymbol->uType.sSubrtn.uFunc.sUser.pSubCntx;																					// new labels and parameters defaults will be stored here
			pCntx->pSubCntx->CurrentData = pCntx->pSubCntx->BeginData;																							// reset the data pointer
			pSymbol->uType.sSubrtn.uFunc.sUser.pCallerIdents = g_pLocalRegistry;																		// save calling memory
			g_pLocalRegistry = pSymbol->uType.sSubrtn.uFunc.sUser.pSubIdents;																				// new identifiers will be stored here
			ProgramSkipEOL(&pCntx->Program, &pCntx->Pc);
	 		ProgramTraceFlow(&pCntx->Program, &pCntx->Pc);
			do{
				if(pCntx->OnEventsEnabled && g_EventPending) ProcessKeyEvents(pValue);																// check for interrupt events
				else Statements(pValue);																																							// process each statement
				if(pValue->Type == eVType::V_ERROR){																																	// check for returned errors
					if(strchr(pValue->uData.Error.pStr, '\n') == nullptr){
						if((pCntx->RunState == eRunState::STOPPED) || (pCntx->RunState == eRunState::PAUSED))	ProgramSkipEOL(&pCntx->Program, &pCntx->Pc);
						StackSetError(&pCntx->Stack, ProgramLineNumber(&pCntx->Program, &pCntx->Pc), &pCntx->Pc, pValue);
						ProgramPCtoError(&pCntx->Program, &pCntx->Pc, pValue);
					}
					if((pCntx->RunState == eRunState::RUNNING) && (pCntx->Stack.OnErrorPc.Index != -1)) ProcessErrorEvent(pValue);
					else{
						if(pCntx->RunState == eRunState::PAUSED){
							BString Str;
							Str.AppendChars(pValue->uData.Error.pStr);
							TokenToString(pCntx->Pc.pToken, nullptr, &Str, nullptr, -1);
							g_pSysPrinter->PrintText(eDispArea::COMMENT, Str.GetBuffer());
							PauseLoop();																																										// pause waits here for user action
							ProgramState(pCntx->RunMode);																	
						}
						else break;																																												// return to caller
					}
				}
				else if(pValue->Type != eVType::V_NIL) break;
				ValueDestroy(pValue);
			}
			while((Res = ProgramSkipEOL(&pCntx->Program, &pCntx->Pc)));
			if(!Res) ValueNewVoid(pValue);
			pCntx->pSubCntx = pSymbol->uType.sSubrtn.uFunc.sUser.pCallerCntx;																				// return the caller context state
			g_pLocalRegistry = pSymbol->uType.sSubrtn.uFunc.sUser.pCallerIdents;																		// return the caller variables
			ResetVariables(pSymbol->uType.sSubrtn.uFunc.sUser.pSubIdents);																					// clear all variables in this sub-programmes local area
			g_pMassStorage->DestroyFCB(pCntx->pSubCntx->ppFileTable);																								// close all files and restore FCB
		}
		else ValueNewNull(pValue, pSymbol->uType.sSubrtn.RetType);
		g_Trace.pLastPc = &pCntx->Pc;
		StackFuncReturn(&pCntx->Stack, pCntx->Pass == ePass::INTERPRET && pValue->Type != eVType::V_ERROR ? &pCntx->Pc : nullptr);
 		ProgramTraceFlow(&pCntx->Program, &pCntx->Pc);
	}
	return pValue;
}

//////////////////////////////////////////////////////////////////////

static inline Value_s *BinaryLevel(Value_s *pValue, Value_s *(Level)(Value_s *pValue), const int Priority)
{
enum TokenType_e TokenType;
PC_s SavePc;

	if(Level(pValue) == nullptr) return nullptr;
	if(pValue->Type == eVType::V_ERROR) return pValue;
	do{
		TokenType = pCntx->Pc.pToken->Type;
		if(!TOKEN_ISBINARYOPERATOR(TokenType) || TOKEN_BINARYPRIORITY(TokenType) != Priority) return pValue;
		SavePc = pCntx->Pc;
		++pCntx->Pc.pToken;
		Value_s Val;
		if(Level(&Val) == nullptr){
			ValueDestroy(pValue);
			return ValueNewError(pValue, MISSINGEXPR, _T("binary operand"));
		}
		if(Val.Type == eVType::V_ERROR){
			ValueDestroy(pValue);
			*pValue = Val;
			return pValue;
		}
		if(ValueCommonType[(int)pValue->Type][(int)Val.Type] == eVType::V_ERROR){
			ValueDestroy(pValue);
			ValueDestroy(&Val);
			return ValueNewError(pValue, INVALIDOPERAND);
		}
		pCntx->IsPassVar = false;																																		// cannot take a reference to an operator
		switch(TokenType){
			case T_LT:				ValueLt(pValue, &Val, pCntx->Pass == ePass::INTERPRET);					break;
			case T_LE:				ValueLe(pValue, &Val, pCntx->Pass == ePass::INTERPRET);					break;
			case T_EQ:				ValueEqu(pValue, &Val, pCntx->Pass == ePass::INTERPRET); 				break;
			case T_GE:				ValueGe(pValue, &Val, pCntx->Pass == ePass::INTERPRET);					break;
			case T_GT:				ValueGt(pValue, &Val, pCntx->Pass == ePass::INTERPRET);					break;
			case T_NE:				ValueNe(pValue, &Val, pCntx->Pass == ePass::INTERPRET);					break;
			case T_PLUS:			ValueAdd(pValue, &Val, pCntx->Pass == ePass::INTERPRET);				break;
			case T_MINUS:			ValueSub(pValue, &Val, pCntx->Pass == ePass::INTERPRET);				break;
			case T_MULT:			ValueMultiply(pValue, &Val, pCntx->Pass == ePass::INTERPRET);		break;
			case T_DIV:				ValueDiv(pValue, &Val, pCntx->Pass == ePass::INTERPRET);				break;
			case T_DIVIDE:		ValueDivide(pValue, &Val, pCntx->Pass == ePass::INTERPRET);			break;
			case T_MOD:				ValueMod(pValue, &Val, pCntx->Pass == ePass::INTERPRET);				break;
			case T_POW:				ValuePow(pValue, &Val, pCntx->Pass == ePass::INTERPRET);				break;
			case T_AND:				ValueAnd(pValue, &Val, pCntx->Pass == ePass::INTERPRET);				break;
			case T_AMPERSAND:	ValueAmpersand(pValue, &Val, pCntx->Pass == ePass::INTERPRET);	break;
			case T_OR:				ValueOr(pValue, &Val, pCntx->Pass == ePass::INTERPRET);					break;
			case T_XOR:				ValueXor(pValue, &Val, pCntx->Pass == ePass::INTERPRET);				break;
			case T_EQV:				ValueEqv(pValue, &Val, pCntx->Pass == ePass::INTERPRET);				break;
			case T_IMP:				ValueImp(pValue, &Val, pCntx->Pass == ePass::INTERPRET);				break;
			default:					assert(0);
		}
		ValueDestroy(&Val);
	}
	while(pValue->Type != eVType::V_ERROR);
	if(pValue->Type == eVType::V_ERROR) pCntx->Pc = SavePc;
	return pValue;
}

//////////////////////////////////////////////////////////////////////

static inline Value_s *UnaryLevel(Value_s *pValue, Value_s *(Level)(Value_s *pValue), const int Priority)
{
enum TokenType_e TokenType;
PC_s SavePc;

	TokenType = pCntx->Pc.pToken->Type;
	if(!TOKEN_ISUNARYOPERATOR(TokenType) || TOKEN_UNARYPRIORITY(TokenType) != Priority) return Level(pValue);
	SavePc = pCntx->Pc;
	++pCntx->Pc.pToken;
	if(UnaryLevel(pValue, Level, Priority) == nullptr) return ValueNewError(pValue, MISSINGEXPR, _T("unary operand"));
	if(pValue->Type == eVType::V_ERROR) return pValue;
	switch(TokenType){
		case T_PLUS:
			ValuePlus(pValue, pCntx->Pass == ePass::INTERPRET);
			break;
		case T_MINUS:
			ValueNegate(pValue, pCntx->Pass == ePass::INTERPRET);
			break;
		case T_NOT:
			ValueNot(pValue, pCntx->Pass == ePass::INTERPRET);
			break;
		default:
			assert(0);
	}
	if(pValue->Type == eVType::V_ERROR) pCntx->Pc = SavePc;
	return pValue;
}

//////////////////////////////////////////////////////////////////////

static Value_s *EvalLevel8(Value_s *pValue)
{
	switch(pCntx->Pc.pToken->Type){
		case T_IDENTIFIER:{
			PC_s SavPc;
			Value_s *pVal;
			SavPc = pCntx->Pc;
			Identifier_s *pIdentifier = pCntx->Pc.pToken->Obj.pIdentifier;
			if(pCntx->Pass == ePass::COMPILE){
				if((StackFind(&pCntx->Stack, pIdentifier) == 0)	&& (FindIdentifier(g_pLocalRegistry, pIdentifier, LibSearchType((pCntx->Pc.pToken + 1)->Type, pIdentifier->DefType)) == 0)) return ValueNewError(pValue, UNDECLARED);
			}
			assert(pCntx->Pass == ePass::DECLARE || pIdentifier->pSym);
			if((pCntx->Pass != ePass::DECLARE) && ((pIdentifier->pSym->SymbolType == eSymType::LOCALVAR) || (pIdentifier->pSym->SymbolType == eSymType::LOCALARRAY) || 
				(pIdentifier->pSym->SymbolType == eSymType::COMNVAR) || (pIdentifier->pSym->SymbolType == eSymType::COMNARRAY) ||
				(pIdentifier->pSym->SymbolType == eSymType::STACKVAR))){
				if((pVal = GetValue(pValue))->Type == eVType::V_ERROR) return pValue;
				if((pCntx->Pc.pToken->Type != T_EQ) || (!pCntx->IsAssign)){																							  // conditional expression
					ValueClone(pValue, pVal);
					pCntx->IsAssign = false;
				}
				else{																																																			// consecutive assignments
					++pCntx->Pc.pToken;
					PC_s Pc = pCntx->Pc;
					if(Evaluate(pValue, _T("rhs"))->Type == eVType::V_ERROR) return pValue;
					ValueAssign(pVal, pValue, pCntx->Pass == ePass::INTERPRET);																							// assign new value and...
					ValueClone(pValue, pVal);																																								// ...swap over
					pCntx->IsAssign = false;																																								// no more assignments allowed
				}
 				break;
			}
			pCntx->IsPassVar = false;																																										// would be a complex evaluation
			if(pCntx->Pass != ePass::DECLARE) Function(pValue);																													// only functions come here during interpret pass
			else{
				if(((pCntx->Pc.pToken + 1)->Type == T_OP) || ((pCntx->Pc.pToken + 1)->Type == T_OSB)) Function(pValue);		// functions or arrays must be define explicitly
				else{
					eSymType VarType = LibSearchType((pCntx->Pc.pToken + 1)->Type, pIdentifier->DefType);
					if((StackFind(&pCntx->Stack, pIdentifier) == 0)	&& (FindIdentifier(g_pLocalRegistry, pIdentifier, VarType) == 0) && (RegisterVariable(g_pLocalRegistry, pIdentifier,	pIdentifier->DefType, VarType, 0) == 0))	return ValueNewError(pValue, REDECLARATION);
					ValueNewNull(pValue, pIdentifier->DefType);																															// simple variable can be implicitly defined
					++pCntx->Pc.pToken;
				}
			}
			if(pValue->Type == eVType::V_VOID){
				ValueDestroy(pValue);
				pCntx->Pc = SavPc;
				return ValueNewError(pValue, VOIDVALUE);
			}
			break;
		}
		case T_INTNUM:{
			NewIntegerValue(pValue, pCntx->Pc.pToken->Obj.Integer);
			pCntx->IsPassVar = false;
			++pCntx->Pc.pToken;
			break;
		}
		case T_REALNUM:{
			NewRealValue(pValue, pCntx->Pc.pToken->Obj.Real);
			pCntx->IsPassVar = false;
			++pCntx->Pc.pToken;
			break;
		}
		case T_STRING:{
			ValueNewString(pValue);
			pValue->uData.pString->Clone(pCntx->Pc.pToken->Obj.pString);
			pCntx->IsPassVar = false;
			++pCntx->Pc.pToken;
			break;
		}
		case T_HEXNUM:{
			NewIntegerValue(pValue, pCntx->Pc.pToken->Obj.HexInteger);
			pCntx->IsPassVar = false;
			++pCntx->Pc.pToken;
			break;
		}
		case T_OCTNUM:{
			NewIntegerValue(pValue, pCntx->Pc.pToken->Obj.OctInteger);
			pCntx->IsPassVar = false;
			++pCntx->Pc.pToken;
			break;
		}
		case T_OP:{
			pCntx->IsPassVar = false;
			++pCntx->Pc.pToken;
			if(Evaluate(pValue, _T("parenthetic"))->Type == eVType::V_ERROR) return pValue;
			if(pCntx->Pc.pToken->Type != T_CP){
				ValueDestroy(pValue);
				return ValueNewError(pValue, MISSINGCP);
			}
			++pCntx->Pc.pToken;
			break;
		}
		case T_MULT:{
			NewIntegerValue(pValue, -1);
			++pCntx->Pc.pToken;
			break;
		}
		default:{
			return nullptr;
		}
	}
	return pValue;
}

//////////////////////////////////////////////////////////////////////

static Value_s *EvalLevel7(Value_s *pValue)
{
	return BinaryLevel(pValue, EvalLevel8, 7);
}

//////////////////////////////////////////////////////////////////////

static Value_s *EvalLevel6(Value_s *pValue)
{
	return UnaryLevel(pValue, EvalLevel7, 6);
}

//////////////////////////////////////////////////////////////////////

static Value_s *EvalLevel5(Value_s *pValue)
{
	return BinaryLevel(pValue, EvalLevel6, 5);
}

//////////////////////////////////////////////////////////////////////

static Value_s *EvalLevel4(Value_s *pValue)
{
	return BinaryLevel(pValue, EvalLevel5, 4);
}

//////////////////////////////////////////////////////////////////////

static Value_s *EvalLevel3(Value_s *pValue)
{
	return BinaryLevel(pValue, EvalLevel4, 3);
}

//////////////////////////////////////////////////////////////////////

static Value_s *EvalLevel2(Value_s *pValue)
{
	return UnaryLevel(pValue, EvalLevel3, 2);
}

//////////////////////////////////////////////////////////////////////

static Value_s *EvalLevel1(Value_s *pValue)
{
	return BinaryLevel(pValue, EvalLevel2, 1);
}

//////////////////////////////////////////////////////////////////////

static Value_s *Evaluate(Value_s *pValue, const char *Description)
{
	/* avoid function calls for atomic expression */
	switch(pCntx->Pc.pToken->Type){
		case T_STRING:
		case T_REALNUM:
		case T_INTNUM:
		case T_HEXNUM:
		case T_OCTNUM:
		case T_IDENTIFIER:
			if(!TOKEN_ISBINARYOPERATOR((pCntx->Pc.pToken + 1)->Type) && ((pCntx->Pc.pToken + 1)->Type != T_OP) && ((pCntx->Pc.pToken + 1)->Type != T_OSB))	return EvalLevel7(pValue);
		default:
			break;
	}
	if(BinaryLevel(pValue, EvalLevel1, 0) == nullptr){
		if(Description) return ValueNewError(pValue, MISSINGEXPR, Description);
		else return nullptr;
	}
	if(pCntx->Pass == ePass::INTERPRET) SaveResult(pValue);
	return pValue;
}

//////////////////////////////////////////////////////////////////////

static Value_s *EvaluateGeometry(Value_s *pValue, int *pDim, Geometry_s Geometry[])
{
PC_s ExprPc = pCntx->Pc;
int Dim = 0, BoundLow, BoundHigh;

  do{
		BoundLow = pCntx->pSubCntx->OptionBase;
		if(Evaluate(pValue, _T("dimension"))->Type == eVType::V_ERROR || (pCntx->Pass != ePass::DECLARE && ValueRetype(pValue, eVType::V_INT)->Type == eVType::V_ERROR)) return pValue;
		if(pCntx->Pass == ePass::INTERPRET && pValue->uData.Integer < 1){
			ValueDestroy(pValue);
			pCntx->Pc = ExprPc;
			return ValueNewError(pValue, OUTOFRANGE, _T("Dimension"));
		}
		BoundHigh = pValue->uData.Integer;
		Geometry[Dim].Size = pValue->uData.Integer;
		ValueDestroy(pValue);
		if(pCntx->Pc.pToken->Type == T_COLON){																																				// Get the upper bounds
			++pCntx->Pc.pToken;
			ExprPc = pCntx->Pc;
			if(Evaluate(pValue, _T("subscript"))->Type == eVType::V_ERROR || (pCntx->Pass != ePass::DECLARE && ValueRetype(pValue, eVType::V_INT)->Type == eVType::V_ERROR)){
				return pValue;
			}
			if(pCntx->Pass == ePass::INTERPRET && pValue->uData.Integer <= BoundHigh){																				// must be grater than the lower dimension				
				ValueDestroy(pValue);
				pCntx->Pc = ExprPc;
				return ValueNewError(pValue, OUTOFRANGE, _T("Subscript"));
			}
 			BoundLow = BoundHigh;
			BoundHigh = pValue->uData.Integer;
			ValueDestroy(pValue);
		}
		if(pCntx->Pass == ePass::INTERPRET){
			Geometry[Dim].Size = BoundHigh - BoundLow + 1;
			Geometry[Dim].Base = BoundLow;
			++Dim;
		}
		if(pCntx->Pc.pToken->Type == T_COMMA)	++pCntx->Pc.pToken;
		else if(pCntx->Pc.pToken->Type == T_CP){
			++pCntx->Pc.pToken;
			*pDim = Dim;
			return nullptr;
		}
		else return ValueNewError(pValue, MISSINGCP);
	}
	while(Dim < DEF_MAX_DIMENSIONS);
	return ValueNewError(pValue, TOOMANY);
}

//////////////////////////////////////////////////////////////////////

static Value_s *Convert(Value_s *pValue, Value_s *pVal, Token_s *pToken)
{
	switch(pVal->Type){
		case eVType::V_INT:{
			char *pDataInput;
			char *pEnd = nullptr;
			long int v;
			int overflow;
			if(pToken->Type != T_DATAINPUT) return ValueNewError(pValue, BADCONVERSION, _T("integer"));
			pDataInput = pToken->Obj.pDataInput;
			v = ValueStringToInteger(pDataInput, &pEnd, &overflow);
			if(pEnd == pDataInput || (*pEnd != '\0' && *pEnd != ' ' && *pEnd != '\t')) return ValueNewError(pValue, BADCONVERSION, _T("integer"));
			if(overflow) return ValueNewError(pValue, INTEGEROVERFLOW, _T("converted value"));
			ValueDestroy(pVal);
			NewIntegerValue(pVal, v);
			break;
		}
		case eVType::V_REAL:{
			char *pDataInput;
			char *pEnd = nullptr;
			double v;
			int overflow;
			if(pToken->Type != T_DATAINPUT) return ValueNewError(pValue, BADCONVERSION, _T("real"));
			pDataInput = pToken->Obj.pDataInput;
			v = ValueStringToReal(pDataInput, &pEnd, &overflow);
			if(pEnd == pDataInput || (*pEnd != '\0' && *pEnd != ' ' && *pEnd != '\t')) return ValueNewError(pValue, BADCONVERSION, _T("real"));
			if(overflow) return ValueNewError(pValue, INTEGEROVERFLOW, _T("converted value"));
			ValueDestroy(pVal);
			NewRealValue(pVal, v);
			break;
		}
		case eVType::V_STRING:{
			ValueDestroy(pVal);
			ValueNewString(pVal);
			if(pToken->Type == T_STRING) pVal->uData.pString->AppendString(pToken->Obj.pString);
			else pVal->uData.pString->AppendChars(pToken->Obj.pDataInput);
			break;
		}
		default: assert(0);
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////

static Value_s *DataRead(Value_s *pValue, Value_s *pVal)
{
	if(pCntx->pSubCntx->CurrentData.Index == -1) return ValueNewError(pValue, ENDOFDATA);
	if(pCntx->pSubCntx->CurrentData.pToken->Type == T_DATA){
		pCntx->pSubCntx->NextData = pCntx->pSubCntx->CurrentData.pToken->Obj.NextDataPc;
		++pCntx->pSubCntx->CurrentData.pToken;
	}
	if(Convert(pValue, pVal, pCntx->pSubCntx->CurrentData.pToken)) return pValue;
	++pCntx->pSubCntx->CurrentData.pToken;
	if(pCntx->pSubCntx->CurrentData.pToken->Type == T_COMMA) ++pCntx->pSubCntx->CurrentData.pToken;
	else pCntx->pSubCntx->CurrentData = pCntx->pSubCntx->NextData;
	return nullptr;
}

//////////////////////////////////////////////////////////////////////

Value_s *GetFormalArgs(Value_s *pValue, int *pArgs)
{
eVType TypeSpec;

	++pCntx->Pc.pToken;																																												
	while(1){																																																	// ...must specify at least 1 formal parameter
		switch(pCntx->Pc.pToken->Type){
			case T_SHORT:
			case T_INTEGER:
				TypeSpec = eVType::V_INT;
				++pCntx->Pc.pToken;
				continue;
			case T_REAL:
				TypeSpec = eVType::V_REAL;
				++pCntx->Pc.pToken;
				continue;
			case T_STRING:
				TypeSpec = eVType::V_STRING;
				++pCntx->Pc.pToken;
				continue;
			case T_IDENTIFIER:{																																										// can only be a formal parameter 
				if(StackVariable(&pCntx->Stack, pCntx->Pc.pToken->Obj.pIdentifier) == 0){														// place it on the stack, used to get type and attach to function body
					StackFuncEnd(&pCntx->Stack);
					return ValueNewError(pValue, ALREADYDECLARED);
				}
				++*pArgs;
				++pCntx->Pc.pToken;
				if(pCntx->Pc.pToken->Type == T_OP){																																	// parentheses full array specification, (*)
					++pCntx->Pc.pToken;
					if((pCntx->Pc.pToken->Type != T_MULT)	|| ((pCntx->Pc.pToken + 1)->Type != T_CP)){
						StackFuncEnd(&pCntx->Stack);
						return ValueNewError(pValue, MISSINGFORMIDENT);
					}
					pCntx->Pc.pToken += 2;
				}
				break;
			}
			default:{
				StackFuncEnd(&pCntx->Stack);
				return ValueNewError(pValue, MISSINGFORMIDENT);
			}
		}
		if(pCntx->Pc.pToken->Type == T_COMMA) ++pCntx->Pc.pToken;
		else break;
	}
	if(pCntx->Pc.pToken->Type != T_CP){
		StackFuncEnd(&pCntx->Stack);
		return ValueNewError(pValue, MISSINGCP);
	}
	++pCntx->Pc.pToken;
	return nullptr;
}

//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////

static Value_s MoreStatements;

#include "statement.cpp"
#include "graphics.cpp"

//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////

static void ProcessTokens(Token_s *pToken)
{
Value_s Value;

	ValueNewVoid(&Value);
	ProgramState(eRunMode::GO);																	
	for(pCntx->Pass = ePass::DECLARE; pCntx->Pass != ePass::INTERPRET; NextPass()){		// main pass loop
		pCntx->pSubCntx->CurrentData.Index = -1;
		pCntx->Pc.Index = -1;																														// place in direct mode
		pCntx->Pc.pToken = pToken;
		pCntx->pSubCntx->OptionBase = 0;
		Statements(&Value);																															// check the command syntax
		if(Value.Type != eVType::V_ERROR && pCntx->Pc.pToken->Type != T_EOL){
			ValueDestroy(&Value);
			ValueNewError(&Value, SYNTAX);
		}
		if(Value.Type != eVType::V_ERROR && pCntx->Marker.Index > 0){
			ValueDestroy(&Value);
			MarkerStackError(&Value);
		}
		if((Value.Type != eVType::V_ERROR) && !pCntx->Program.Runable && (pCntx->Pass == ePass::COMPILE)){
			ValueDestroy(&Value);
			CompileProgram(&Value, !InDirectMode());																  				// compile the command
		}
		if(Value.Type == eVType::V_ERROR){
			BString Str;
			StackSetError(&pCntx->Stack, ProgramLineNumber(&pCntx->Program, &pCntx->Pc), &pCntx->Pc, &Value);
			ProgramPCtoError(&pCntx->Program, &pCntx->Pc, &Value);
			pCntx->Marker.Index = 0;
			Str.AppendChars(_T("Error: "));
			ValueToString(&Value, &Str, ' ', -1, 0, 0, -1, 0, 0);
			ValueDestroy(&Value);
			g_pPrinter->PrintText(eDispArea::COMMENT, Str.GetBuffer());
			ProgramState(eRunMode::STOP);																	
			return;
		}
	}		
	pCntx->Pc.Index = -1;
	pCntx->Pc.pToken = pToken;
	pCntx->pSubCntx->OptionBase = 0;
	pCntx->pSubCntx->CurrentData = pCntx->pSubCntx->BeginData;
	pCntx->pSubCntx->NextData.Index = -1;
	Interpret();																																			// now execute the programme
}

//////////////////////////////////////////////////////////////////////

static Value_s *CompileProgram(Value_s *pValue, int ClearGlobals)
{
PC_s Begin;

	pCntx->Stack.Resumeable = 0;
	if(ClearGlobals){
		g_pLocalRegistry = pBaseLocalRegistry;
		pCntx->pSubCntx = pMainCntx;
		DestroyGlobals(g_pGlobalRegistry);
		DestroyAllIdentifiers(g_pLocalRegistry);
		DestroyLabels(pCntx->pSubCntx->pLabels);
		CleanStack(&pCntx->Stack);
	}
	else{
		DestroyFuncIdentifiers(g_pGlobalRegistry);
	}
	if(ProgramBeginning(&pCntx->Program, &Begin)){
		PC_s SavePc = pCntx->Pc;
		ePass SaveMode = pCntx->Pass;
		ProgramNoRun(&pCntx->Program);
		for(pCntx->Pass = ePass::DECLARE; pCntx->Pass != ePass::INTERPRET; NextPass()){
			if(pCntx->Pass == ePass::DECLARE){
				pCntx->pSubCntx->BeginData.Index = -1;
				pCntx->pDataListEnd = &pCntx->pSubCntx->BeginData;
			}
			pCntx->pSubCntx->OptionBase = 0;
			pCntx->Program.Runable = 1;
			pCntx->Pc = Begin;
			while(1){
				Statements(pValue);
				if(pValue->Type == eVType::V_ERROR) break;
				ValueDestroy(pValue);
				if(!ProgramSkipEOL(&pCntx->Program, &pCntx->Pc)){
					ValueNewNil(pValue);
					break;
				}
			}
			if(pValue->Type != eVType::V_ERROR && pCntx->Marker.Index > 0){
				ValueDestroy(pValue);
				MarkerStackError(pValue);
			}
			if(pValue->Type == eVType::V_ERROR){
				pCntx->Marker.Index = 0;
				ProgramNoRun(&pCntx->Program);
				if(pCntx->Stack.pLocalSymbols) StackFuncEnd(&pCntx->Stack);  /* Always correct? */
				pCntx->Pass = SaveMode;
				return pValue;
			}
		}
		pCntx->Pc = Begin;
		if(ProgramAnalyse(&pCntx->Program, &pCntx->Pc, pValue)){
			pCntx->Marker.Index = 0;
			ProgramNoRun(&pCntx->Program);
			if(pCntx->Stack.pLocalSymbols) StackFuncEnd(&pCntx->Stack);  /* Always correct? */
			pCntx->Pass = SaveMode;
			return pValue;
		}
		pCntx->pSubCntx->CurrentData = pCntx->pSubCntx->BeginData;
		pCntx->Pc = SavePc;
		pCntx->Pass = SaveMode;
	}
	return ValueNewNil(pValue);
}

//////////////////////////////////////////////////////////////////////

static void Interpret(void)
{
Value_s Value;

	ValueNewVoid(&Value);
	pCntx->Pass = ePass::INTERPRET;
	if(pCntx->Step) pCntx->RunMode = eRunMode::HALT;
	ProgramState(pCntx->RunMode);																	
	do{
		assert(pCntx->Pass == ePass::INTERPRET);
		if(pCntx->OnEventsEnabled && g_EventPending) ProcessKeyEvents(&Value);
		else Statements(&Value);
		assert(pCntx->Pass == ePass::INTERPRET);
		if(Value.Type == eVType::V_ERROR){
			if(strchr(Value.uData.Error.pStr, '\n') == nullptr){
				if((pCntx->RunState == eRunState::STOPPED) || (pCntx->RunState == eRunState::PAUSED))	ProgramSkipEOL(&pCntx->Program, &pCntx->Pc);
				StackSetError(&pCntx->Stack, ProgramLineNumber(&pCntx->Program, &pCntx->Pc), &pCntx->Pc, &Value);
				ProgramPCtoError(&pCntx->Program, &pCntx->Pc, &Value);
			}
			if((pCntx->RunState == eRunState::RUNNING) && (pCntx->Stack.OnErrorPc.Index != -1)) ProcessErrorEvent(&Value);
			if(pCntx->RunState != eRunState::SUSPENDED){
				BString Str;
				if((pCntx->RunState != eRunState::STOPPED) && (pCntx->RunState != eRunState::PAUSED)){
					g_pPrinter->PrintText(eDispArea::COMMENT, _T("Error: "));
					ProgramState(eRunMode::STOP);																	
				}
				if(pCntx->RunState == eRunState::PAUSED){
					Str.AppendChars(Value.uData.Error.pStr);
					TokenToString(pCntx->Pc.pToken, nullptr, &Str, nullptr, -1);
					g_pSysPrinter->PrintText(eDispArea::COMMENT, Str.GetBuffer());
					PauseLoop();																																											// pause waits here for user action
					ProgramState(pCntx->RunMode);
					ValueDestroy(&Value);
					continue;
				}
				StackFrameToError(&pCntx->Stack, &pCntx->Program, &Value);
				Str.AppendChars(Value.uData.Error.pStr);
				while(StackGosubReturn(&pCntx->Stack, nullptr));
				g_pSysPrinter->PrintText(eDispArea::COMMENT, Str.GetBuffer());
				ValueDestroy(&Value);
				break;
			}
			else break;
		}
		ValueDestroy(&Value);
	}
	while(pCntx->Pc.pToken->Type != T_EOL || ProgramSkipEOL(&pCntx->Program, &pCntx->Pc));
	if(pCntx->RunState == eRunState::STOPPED) g_pPrinter->EndPrint();
	if(pCntx->RunState == eRunState::RUNNING) ProgramState(eRunMode::STOP);																	
}

//////////////////////////////////////////////////////////////////////

static Value_s *ProcessErrorEvent(Value_s *pValue)
{
PC_s HerePc = pCntx->Pc;

	pCntx->Stack.Resumeable = 1;
	g_Trace.pLastPc = &pCntx->Pc;
	ValueDestroy(pValue);																										// destroy error report
	switch(pCntx->Stack.OnErrorType){
		case T_GOTO:{
			pCntx->Pc = pCntx->Stack.OnErrorPc;
			ProgramTraceFlow(&pCntx->Program, &pCntx->Pc);
			return ValueNewNil(pValue);
		}
		case T_GOSUB:{
			StackPushGosubRet(&pCntx->Stack, &pCntx->Pc, -1);
			pCntx->Pc = pCntx->Stack.OnErrorPc;
			ProgramTraceFlow(&pCntx->Program, &pCntx->Pc);
			return ValueNewNil(pValue);
		}
		case T_CALL:{
			pCntx->Pc = pCntx->Stack.OnErrorPc;
			SubProgram(pValue, true);																						// call the subroutine
			pCntx->Pc	= HerePc;																									// reinstate the PC
			if(ValueRetype(pValue, eVType::V_VOID)->Type == eVType::V_ERROR) return pValue;
			ValueDestroy(pValue);
			return ValueNewNil(pValue);
		}
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////

static Value_s *ProcessKeyEvents(Value_s *pValue)
{
PC_s HerePc = pCntx->Pc;

	g_Trace.pLastPc = &pCntx->Pc;
	for(int i = 0; i < MAX_SOFT_KEYS; ++i){
		if(pCntx->EventKeys[i].State == eOnKeyState::FIRED){
			switch(pCntx->EventKeys[i].JumpType){
				case T_GOTO:{
					pCntx->EventKeys[i].State = eOnKeyState::BLOCKED;
					pCntx->Pc = pCntx->EventKeys[i].Pc;															// set jump destination
					ProgramTraceFlow(&pCntx->Program, &pCntx->Pc);
					return ValueNewNil(pValue);
				}
				case T_GOSUB:{
					pCntx->EventKeys[i].State = eOnKeyState::BLOCKED;
					StackPushGosubRet(&pCntx->Stack, &pCntx->Pc, i);				// set return address
					pCntx->Pc = pCntx->EventKeys[i].Pc;											// set jump destination
					ProgramTraceFlow(&pCntx->Program, &pCntx->Pc);
					return ValueNewNil(pValue);
				}
				case T_CALL:{
					pCntx->EventKeys[i].State = eOnKeyState::BLOCKED;
					pCntx->Pc = pCntx->EventKeys[i].Pc;
					SubProgram(pValue, true);																// call the subroutine
					pCntx->Pc	= HerePc;																		 	// reinstate the PC
					if(pCntx->EventKeys[i].State == eOnKeyState::BLOCKED) pCntx->EventKeys[i].State = eOnKeyState::ACTIVE;	// this may return as inactive
					if(ValueRetype(pValue, eVType::V_VOID)->Type == eVType::V_ERROR) return pValue;
					ValueDestroy(pValue);
					return ValueNewNil(pValue);
				}
			}
		}
	}
	g_EventPending = false;
	return ValueNewNil(pValue);
}

//////////////////////////////////////////////////////////////////////

static Value_s *Statements(Value_s *pValue)
{
loop:
	if(pCntx->Pc.pToken->pStatement){
		Value_s *pVal;
		if((pVal = pCntx->Pc.pToken->pStatement(pValue))){
			if(pVal == &MoreStatements) goto loop;
			else return pValue;
		}
	}
	else return ValueNewError(pValue, MISSINGSTATEMENT);
	if(pCntx->RunMode != eRunMode::GO){
		ProgramState(pCntx->RunMode);																						// IsStopped and IsPaused get set here
		if(pCntx->RunMode == eRunMode::STOP) return ValueNewError(pValue, BREAK);
		else{
			pCntx->Stack.Resumeable = 1;																					// make it resumable
			return ValueNewError(pValue, PAUSE);
		}
	}
	else if(pCntx->Pc.pToken->Type == T_REM){
		++pCntx->Pc.pToken;
		goto loop;
	}
	else if((pCntx->Pass == ePass::DECLARE || pCntx->Pass == ePass::COMPILE) && pCntx->Pc.pToken->Type != T_EOL && pCntx->Pc.pToken->Type != T_ELSE) return ValueNewError(pValue, UNRECOGNISEDTOKS);
	return ValueNewNil(pValue);
}


