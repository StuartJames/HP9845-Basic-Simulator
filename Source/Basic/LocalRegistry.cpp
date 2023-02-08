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


/////////////////////////////////////////////////////////////////////////////

inline eSymType  LibSearchType(TokenType_e TokType, eVType DefType){ return ((TokType == T_OP) || (DefType == eVType::V_STRING)) ? eSymType::LOCALARRAY : eSymType::LOCALVAR; }

/////////////////////////////////////////////////////////////////////////////

inline bool IsIdent(eSymType SymType, eSymType Type){ return (((Type == eSymType::LOCALVAR)	&& (SymType == eSymType::LOCALVAR)) || ((Type == eSymType::LOCALARRAY) && ((SymType == eSymType::LOCALARRAY) || (SymType == eSymType::USERFUNCTION)))) ? true : false; }


/////////////////////////////////////////////////////////////////////////////

Registry_s* NewIdentifiers(void)
{
	Registry_s *pRegistry = (Registry_s*)malloc(sizeof(Registry_s));
	assert(pRegistry != nullptr);
	memset(pRegistry, 0, sizeof(Registry_s));
	return pRegistry;
}

/////////////////////////////////////////////////////////////////////////////

void DeleteIdentifiers(Registry_s *pRegistry)
{
	DestroyAllIdentifiers(pRegistry);
	free(pRegistry);
}

/////////////////////////////////////////////////////////////////////////////////////////////////

int LocalFind(Registry_s *pRegistry, Identifier_s *pIdent, eSymType SymbolType)
{
Symbol_s **ppSym;
unsigned int HashVal = Hash(pIdent->Name); 

	for(ppSym = &pRegistry->pTable[HashVal]; *ppSym != nullptr; ppSym = &((*ppSym)->pNext)){					// search for localy defined functions and variables
		if(IsIdent((*ppSym)->SymbolType, SymbolType) && (!strcmp((*ppSym)->pName, pIdent->Name))) break;
	}
	if(*ppSym == nullptr) return 0;																		
	pIdent->pSym = (*ppSym);
	return 1;
}

/////////////////////////////////////////////////////////////////////////////////////////////////

int RegisterVariable(Registry_s *pRegistry, Identifier_s *pIdent, enum eVType Type, enum eSymType SymbolType, int Redeclare)
{
Symbol_s **ppSym;

	for(ppSym = &pRegistry->pTable[Hash(pIdent->Name)]; *ppSym != nullptr; ppSym = &((*ppSym)->pNext)) if(((*ppSym)->SymbolType == SymbolType) && !strcmp((*ppSym)->pName, pIdent->Name)) break;
	if(*ppSym == nullptr){
		*ppSym = (Symbol_s*)malloc(sizeof(Symbol_s));
		memset((*ppSym), 0, sizeof(Symbol_s));
		UINT Len = strlen(pIdent->Name) + 1;
		(*ppSym)->pName = (char*)malloc(Len);
		strcpy_s((*ppSym)->pName, Len, pIdent->Name);
		(*ppSym)->pNext = nullptr;
		(*ppSym)->SymbolType = SymbolType;
		VarNew(&((*ppSym)->uType.Var), Type, 0, nullptr);
	}
	else if(Redeclare) VarRetype(&((*ppSym)->uType.Var), Type);
	switch((*ppSym)->SymbolType){
		case eSymType::LOCALVAR:
		case eSymType::LOCALARRAY:{
			pIdent->pSym = (*ppSym);
			break;
		}
		case eSymType::USERFUNCTION:{
			return 0;
		}
		default: assert(0);
	}
	return 1;
}

/////////////////////////////////////////////////////////////////////////////////////////////////

int RegisterLocalFunc(Registry_s *pRegistry, Identifier_s *pIdent, enum eVType Type, PC_s *pFunc, PC_s *pBegin, int ArgLength, enum eVType *pArgTypes)
{
	Symbol_s **ppSym;
	for(ppSym = &pRegistry->pTable[Hash(pIdent->Name)]; *ppSym != nullptr && strcmp((*ppSym)->pName, pIdent->Name); ppSym = &((*ppSym)->pNext));
	if(*ppSym != nullptr) return 0;
	*ppSym = (Symbol_s*)malloc(sizeof(Symbol_s));
	(*ppSym)->pName = (char*)malloc(strlen(pIdent->Name) + 1);
	 strcpy_s((*ppSym)->pName, strlen(pIdent->Name) + 1, pIdent->Name);
	(*ppSym)->pNext = nullptr;
	(*ppSym)->SymbolType = eSymType::USERFUNCTION;
	(*ppSym)->uType.sSubrtn.uFunc.sUser.Scope.Start = *pFunc;
	(*ppSym)->uType.sSubrtn.uFunc.sUser.Scope.Begin = *pBegin;
	(*ppSym)->uType.sSubrtn.uFunc.sUser.LocalLength = 0;
	(*ppSym)->uType.sSubrtn.uFunc.sUser.pLocalTypes = nullptr;
	(*ppSym)->uType.sSubrtn.ArgLength = ArgLength;
	(*ppSym)->uType.sSubrtn.pArgTypes = pArgTypes;
	(*ppSym)->uType.sSubrtn.RetType = Type;
	pIdent->pSym = (*ppSym);
	return 1;
}

/////////////////////////////////////////////////////////////////////////////////////////////////

void RegisterLocalFuncEnd(Registry_s *pRegistry, Identifier_s *pIdent, PC_s *pEnd)
{
	Symbol_s **ppSym;
	for(ppSym = &pRegistry->pTable[Hash(pIdent->Name)]; *ppSym != NULL && strcmp((*ppSym)->pName, pIdent->Name); ppSym = &((*ppSym)->pNext));
	assert(*ppSym != NULL);
	(*ppSym)->uType.sSubrtn.uFunc.sUser.Scope.End = *pEnd;
}

/////////////////////////////////////////////////////////////////////////////////////////////////

void ResetVariables(Registry_s *pRegistry)
{
	for(int i = 0; i < IDENT_HASHSIZE; ++i){
		Symbol_s *pSym;
		for(pSym = pRegistry->pTable[i]; pSym; pSym = pSym->pNext) if(pSym->SymbolType == eSymType::LOCALVAR || pSym->SymbolType == eSymType::LOCALARRAY) VarReset(&(pSym->uType.Var));
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////////

bool CloneVar(Registry_s *pRegistry, Symbol_s *pSym)
{
Symbol_s **ppSym;

	for(ppSym = &pRegistry->pTable[Hash(pSym->pName)]; *ppSym != nullptr; ppSym = &((*ppSym)->pNext)) if(((*ppSym)->SymbolType == pSym->SymbolType) && !_stricmp((*ppSym)->pName, pSym->pName)) break;
	if(*ppSym == nullptr){
		*ppSym = pSym;
		return true;
	}
	return false;
}

/////////////////////////////////////////////////////////////////////////////////////////////////

void DestroyVarIdentifiers(Registry_s *pRegistry)
{
	DestroyIdentifiers(pRegistry, eSymType::LOCALVAR);
	DestroyIdentifiers(pRegistry, eSymType::LOCALARRAY);
}

/////////////////////////////////////////////////////////////////////////////////////////////////

void DestroyFuncIdentifiers(Registry_s *pRegistry)
{
	DestroyIdentifiers(pRegistry, eSymType::USERFUNCTION);
}

/////////////////////////////////////////////////////////////////////////////////////////////////

void DestroyAllIdentifiers(Registry_s *pRegistry)
{
	DestroyIdentifiers(pRegistry, eSymType::USERFUNCTION);
	DestroyIdentifiers(pRegistry, eSymType::LOCALVAR);
	DestroyIdentifiers(pRegistry, eSymType::LOCALARRAY);
}

/////////////////////////////////////////////////////////////////////////////////////////////////

void DestroyIdentifiers(Registry_s *pRegistry, enum eSymType Type)
{
int i;
Symbol_s **ppSymCurrent, *pSym, *pSymNext;

	for(i = 0; i < IDENT_HASHSIZE; ++i){
		ppSymCurrent = &pRegistry->pTable[i];
		while(*ppSymCurrent){
			pSym = *ppSymCurrent;
			pSymNext = pSym->pNext;
			if(pSym->SymbolType == Type){
				switch(pSym->SymbolType){
					case eSymType::LOCALVAR:
					case eSymType::LOCALARRAY:
						VarDestroy(&(pSym->uType.Var));
						break;
					case eSymType::USERFUNCTION:{
						if(pSym->uType.sSubrtn.uFunc.sUser.pLocalTypes) free(pSym->uType.sSubrtn.uFunc.sUser.pLocalTypes);
						if(pSym->uType.sSubrtn.pArgTypes) free(pSym->uType.sSubrtn.pArgTypes);
						break;
					}
					default: assert(0);
				}
				free(pSym->pName);
				free(pSym);
				*ppSymCurrent = pSymNext;
			}
			else ppSymCurrent = &pSym->pNext;
		}
	}
}


