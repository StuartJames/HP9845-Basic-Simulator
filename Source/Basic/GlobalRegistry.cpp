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

inline bool IsGlobal(eSymType SymType, eSymType Type){ return (((Type == eSymType::SUBPROGRAMME)	&& (SymType == eSymType::SUBPROGRAMME)) || (((Type == eSymType::LOCALARRAY) || (Type == eSymType::USERFUNCTION)) && (SymType == eSymType::USERFUNCTION))) ? true : false; }

//////////////////////////////////////////////////////////////////////

pSubCntx_s NewSubCntx(void)
{
	pSubCntx_s pSub = (SubCntx_s*)malloc(sizeof(SubCntx_s));
	assert(pSub != nullptr);
	pSub->OptionBase = 0;
	pSub->RoundingPrecision = 12;
  pSub->RoundingMode = eRounding::STANDARD;
	pSub->TrigMode = eTrigMode::RAD;
	pSub->pLabels = (Labels_s*)malloc(sizeof(Labels_s));
	assert(pSub->pLabels != nullptr);
	memset(pSub->pLabels, 0, sizeof(Labels_s));
	pSub->BeginData.Index = -1;
	pSub->NextData.Index = -1;
	return pSub;
}

//////////////////////////////////////////////////////////////////////

void DeleteSubCntx(pSubCntx_s pSub)
{
	DestroyLabels(pSub->pLabels);
	free(pSub->pLabels);
	free(pSub);
}

//////////////////////////////////////////////////////////////////////

Registry_s* NewGlobalRegistry(void)
{
Registry_s *pRegistry;

	pRegistry = (Registry_s*)malloc(sizeof(Registry_s));
	assert(pRegistry != nullptr);
	if(pRegistry != nullptr) memset(pRegistry, 0, sizeof(Registry_s));
	return pRegistry;
}

/////////////////////////////////////////////////////////////////////////////////////////////////

void DeleteGlobals(Registry_s *pRegistry)
{
	DestroyGlobals(pRegistry);
	free(pRegistry);
}

/////////////////////////////////////////////////////////////////////////////////////////////////

int GlobalFind(Identifier_s *pIdent, eSymType SymbolType)
{
Symbol_s **ppSym;

	for(ppSym = &g_pGlobalRegistry->pTable[Hash(pIdent->Name)]; *ppSym != nullptr; ppSym = &((*ppSym)->pNext)){
		if(IsGlobal((*ppSym)->SymbolType, SymbolType) && (!strcmp((*ppSym)->pName, pIdent->Name))) break;
	}
	if(*ppSym == nullptr) return 0;
	pIdent->pSym = (*ppSym);
	return 1;
}

/////////////////////////////////////////////////////////////////////////////////////////////////

int RegisterGlobalFunc(Registry_s *pRegistry, Identifier_s *pIdent, enum eVType Type, PC_s *pFunc, PC_s *pBegin, int ArgLength, enum eVType *pArgTypes)
{
	Symbol_s **ppSym;
	for(ppSym = &pRegistry->pTable[Hash(pIdent->Name)]; *ppSym != nullptr && strcmp((*ppSym)->pName, pIdent->Name); ppSym = &((*ppSym)->pNext));
	if(*ppSym != nullptr) return 0;
	*ppSym = (Symbol_s*)malloc(sizeof(Symbol_s));
	assert(*ppSym != nullptr);
	(*ppSym)->pName = (char*)malloc(strlen(pIdent->Name) + 1);
	assert((*ppSym)->pName != nullptr);
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

void RegisterGlobalFuncEnd(Registry_s *pRegistry, Identifier_s *pIdent, PC_s *pEnd)
{
	Symbol_s **ppSym;
	for(ppSym = &pRegistry->pTable[Hash(pIdent->Name)]; *ppSym != NULL && strcmp((*ppSym)->pName, pIdent->Name); ppSym = &((*ppSym)->pNext));
	assert(*ppSym != NULL);
	(*ppSym)->uType.sSubrtn.uFunc.sUser.Scope.End = *pEnd;
}

/////////////////////////////////////////////////////////////////////////////////////////////////

void DestroyGlobals(Registry_s *pRegistry)
{
	for(int i = 0; i < IDENT_HASHSIZE; ++i){
		Symbol_s *pSymCurrent = pRegistry->pTable[i];
		while(pSymCurrent){
			Symbol_s *pSym = pSymCurrent;
			Symbol_s *pSymNext = pSymCurrent->pNext;
			switch(pSym->SymbolType){
				case eSymType::SUBPROGRAMME:{
					DeleteSubCntx(pSym->uType.sSubrtn.uFunc.sUser.pSubCntx);
					DeleteIdentifiers(pSym->uType.sSubrtn.uFunc.sUser.pSubIdents);
					if(pSym->uType.sSubrtn.uFunc.sUser.pLocalTypes) free(pSym->uType.sSubrtn.uFunc.sUser.pLocalTypes);
					if(pSym->uType.sSubrtn.pArgTypes) free(pSym->uType.sSubrtn.pArgTypes);
					break;
				}
				case eSymType::USERFUNCTION:{
					if(pSym->uType.sSubrtn.uFunc.sUser.pLocalTypes) free(pSym->uType.sSubrtn.uFunc.sUser.pLocalTypes);
					if(pSym->uType.sSubrtn.pArgTypes) free(pSym->uType.sSubrtn.pArgTypes);
					break;
				}
				default: assert(0);
			}
			free(pSymCurrent->pName);
			free(pSymCurrent);
			pSymCurrent = pSymNext;
		}
		pRegistry->pTable[i] = NULL;
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////////

int RegisterSubProg(Registry_s *pRegistry, Identifier_s *pIdent, PC_s *pFunc, PC_s *pBegin, int ArgLength, enum eVType *pArgTypes)
{
	Symbol_s **ppSym;
	for(ppSym = &pRegistry->pTable[Hash(pIdent->Name)]; *ppSym != nullptr && strcmp((*ppSym)->pName, pIdent->Name); ppSym = &((*ppSym)->pNext));
	if(*ppSym != nullptr) return 0;
	*ppSym = (Symbol_s*)malloc(sizeof(Symbol_s));
	assert(*ppSym != nullptr);
	(*ppSym)->pName = (char*)malloc(strlen(pIdent->Name) + 1);
	assert((*ppSym)->pName != nullptr);
	strcpy_s((*ppSym)->pName, strlen(pIdent->Name) + 1, pIdent->Name);
	(*ppSym)->pNext = nullptr;
	(*ppSym)->SymbolType = eSymType::SUBPROGRAMME;
	(*ppSym)->uType.sSubrtn.uFunc.sUser.Scope.Start = *pFunc;
	(*ppSym)->uType.sSubrtn.uFunc.sUser.Scope.Begin = *pBegin;
	(*ppSym)->uType.sSubrtn.uFunc.sUser.LocalLength = 0;
	(*ppSym)->uType.sSubrtn.uFunc.sUser.pLocalTypes = nullptr;
	(*ppSym)->uType.sSubrtn.ArgLength = ArgLength;
	(*ppSym)->uType.sSubrtn.pArgTypes = pArgTypes;
	(*ppSym)->uType.sSubrtn.RetType = eVType::V_VOID;			               // sub-programmes don't return a value
	pIdent->pSym = (*ppSym);
	return 1;
}

/////////////////////////////////////////////////////////////////////////////////////////////////

void RegisterSubProgEnd(Registry_s *pRegistry, Identifier_s *pIdent, PC_s *pEnd)
{
	Symbol_s **ppSym;
	for(ppSym = &pRegistry->pTable[Hash(pIdent->Name)]; *ppSym != NULL && _stricmp((*ppSym)->pName, pIdent->Name); ppSym = &((*ppSym)->pNext));
	assert(*ppSym != NULL);
	(*ppSym)->uType.sSubrtn.uFunc.sUser.Scope.End = *pEnd;
}

/////////////////////////////////////////////////////////////////////////////////////////////////


