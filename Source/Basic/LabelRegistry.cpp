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

int FindLabel(Labels_s *pLabels, Identifier_s *pIdent)
{
Symbol_s **ppSym;

	for(ppSym = &pLabels->pTable[Hash(pIdent->Name)]; *ppSym != nullptr; ppSym = &((*ppSym)->pNext)) if((((*ppSym)->SymbolType == eSymType::LOCALLABEL)) && !_stricmp((*ppSym)->pName, pIdent->Name)) break;	
	if(*ppSym == nullptr) return 0;
	pIdent->pSym = (*ppSym);
	return 1;
}

/////////////////////////////////////////////////////////////////////////////////////////////////

int RegisterLabel(Labels_s *pLabels, Label_s *pIdent, PC_s *pBegin)
{
Symbol_s **ppSym;

	for(ppSym = &pLabels->pTable[Hash(pIdent->pName)]; *ppSym != nullptr; ppSym = &((*ppSym)->pNext)) if((((*ppSym)->SymbolType == eSymType::LOCALLABEL)) && !_stricmp((*ppSym)->pName, pIdent->pName)) break;
	if(*ppSym == nullptr){
		*ppSym = (Symbol_s*)malloc(sizeof(Symbol_s));
		assert(*ppSym != nullptr);
		UINT Len = strlen(pIdent->pName) + 1;
		(*ppSym)->pName = (char*)malloc(Len);
		assert((*ppSym)->pName != nullptr);
		strcpy_s((*ppSym)->pName, Len, pIdent->pName);
		(*ppSym)->pNext = nullptr;
		(*ppSym)->SymbolType = eSymType::LOCALLABEL;
		(*ppSym)->uType.sLabel.Line = *pBegin;
	}
	else return 0;
	pIdent->pSym = (*ppSym);
	return 1;
}

/////////////////////////////////////////////////////////////////////////////////////////////////

void DestroyLabels(Labels_s *pLabels)
{
	for(int i = 0; i < IDENT_HASHSIZE; ++i){
		Symbol_s *pSymCurrent = pLabels->pTable[i];
		while(pSymCurrent){
			Symbol_s *pSym = pSymCurrent;
			Symbol_s *pSymNext = pSymCurrent->pNext;
			switch(pSym->SymbolType){
				case eSymType::LOCALLABEL:{
					break;
				}
				default: assert(0);
			}
			free(pSymCurrent->pName);
			free(pSymCurrent);
			pSymCurrent = pSymNext;
		}
		pLabels->pTable[i] = NULL;
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////////



