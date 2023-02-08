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

/////////////////////////////////////////////////////////////////////////////////////////////////

int CommonFind(Identifier_s *pIdent, eSymType SymbolType)
{
Symbol_s **ppSym;

	for(ppSym = &g_CommonVars.pTable[Hash(pIdent->Name)]; *ppSym != nullptr; ppSym = &((*ppSym)->pNext)){
		if(((*ppSym)->SymbolType == SymbolType)	&& !strcmp((*ppSym)->pName, pIdent->Name)) break;
	}
	if(*ppSym == nullptr) return 0;
	pIdent->pSym = (*ppSym);
	return 1;
}

/////////////////////////////////////////////////////////////////////////////////////////////////

int CommonVariable(Identifier_s *pIdent, enum eVType Type, enum eSymType SymbolType)
{
Symbol_s **ppSym;

	for(ppSym = &g_CommonVars.pTable[Hash(pIdent->Name)]; *ppSym != nullptr; ppSym = &((*ppSym)->pNext)) if(((*ppSym)->SymbolType == SymbolType) && !_stricmp((*ppSym)->pName, pIdent->Name)) break;
	if(*ppSym == nullptr){
		*ppSym = (Symbol_s*)malloc(sizeof(Symbol_s));
		assert(*ppSym != nullptr);
		UINT Len = strlen(pIdent->Name) + 1;
		(*ppSym)->pName = (char*)malloc(Len);
		assert((*ppSym)->pName != nullptr);
		strcpy_s((*ppSym)->pName, Len, pIdent->Name);
		(*ppSym)->pNext = nullptr;
		(*ppSym)->SymbolType = SymbolType;
		VarNew(&((*ppSym)->uType.Var), Type, 0, nullptr);
	}
	switch((*ppSym)->SymbolType){
		case eSymType::COMNVAR:
		case eSymType::COMNARRAY:{
			pIdent->pSym = (*ppSym);
			break;
		}
		default: assert(0);
	}
	return 1;
}

/////////////////////////////////////////////////////////////////////////////////////////////////

void CommonClear(void)
{
	for(int i = 0; i < IDENT_HASHSIZE; ++i){
		Symbol_s *pSym;
		for(pSym = g_CommonVars.pTable[i]; pSym; pSym = pSym->pNext) if(pSym->SymbolType == eSymType::COMNVAR || pSym->SymbolType == eSymType::COMNARRAY) VarReset(&(pSym->uType.Var));
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////////

void DestroyCommon(void)
{
	for(int i = 0; i < IDENT_HASHSIZE; ++i){
		Symbol_s *pSymCurrent = g_CommonVars.pTable[i];
		while(pSymCurrent){
			Symbol_s *pSym = pSymCurrent;
			Symbol_s *pSymNext = pSymCurrent->pNext;
			switch(pSym->SymbolType){
				case eSymType::COMNVAR:
				case eSymType::COMNARRAY:
					VarDestroy(&(pSym->uType.Var));
					break;
				default: assert(0);
			}
			free(pSymCurrent->pName);
			free(pSymCurrent);
			pSymCurrent = pSymNext;
		}
		g_CommonVars.pTable[i] = NULL;
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////////



