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

#define INCREASE_STACK 16

//////////////////////////////////////////////////////////////////////

Stack_s *NewStack(struct Stack_s *pStack)
{
	pStack->Position = 0;
	pStack->Size = 0;
	pStack->FramePosition = 0;
	pStack->FrameSize = 0;
	pStack->OnErrorPc.Index = -1;
	pStack->ErrorLine = 0;
	ValueNewNil(&pStack->Error);
	ValueNewNil(&pStack->LastDet);
	pStack->pSlot = nullptr;
	pStack->pLocalSymbols = pStack->pAllSymbols = nullptr;
	return pStack;
}

//////////////////////////////////////////////////////////////////////

void DestroyStack(Stack_s *pStack)
{
Symbol_s *pSym = nullptr;

	ValueDestroy(&pStack->Error);
	ValueDestroy(&pStack->LastDet);
	if(pStack->Size) free(pStack->pSlot);
	for(pSym = pStack->pAllSymbols; pSym != nullptr;){
		Symbol_s *pToFree = pSym;
		pSym = pSym->pNext;
		free(pToFree->pName);
		free(pToFree);
	}
}

//////////////////////////////////////////////////////////////////////

void CleanStack(Stack_s *pStack)
{
	DestroyStack(pStack);
	NewStack(pStack);
}

//////////////////////////////////////////////////////////////////////

int StackFind(Stack_s *pStack, Identifier_s *pIdent)
{
	for(Symbol_s *pStackSym = pStack->pLocalSymbols; pStackSym != nullptr; pStackSym = pStackSym->pNext){
		if(!strcmp(pStackSym->pName, pIdent->Name)){
			pIdent->pSym = pStackSym;
			return 1;
		}
	}
	return 0;
}

//////////////////////////////////////////////////////////////////////

inline bool StackIncSize(Stack_s *pStack, int inc)
{
union StackSlot *pSlot;

	if((pStack->Position + inc) >= pStack->Size){
		if((pSlot = (StackSlot*)realloc(pStack->pSlot, sizeof(pStack->pSlot[0]) * (pStack->Size += INCREASE_STACK))) == nullptr) return false;
		pStack->pSlot = pSlot;
	}
	return true;
}

//////////////////////////////////////////////////////////////////////

void StackPushFuncRet(Stack_s *pStack, int FirstArg, PC_s *pPc)
{
	if(!StackIncSize(pStack, 2)) return;
	pStack->pSlot[pStack->Position].ReturnException.OnError = pStack->OnErrorPc;
	pStack->pSlot[pStack->Position].ReturnException.Resumeable = pStack->Resumeable;
	++pStack->Position;
	pStack->pSlot[pStack->Position].ReturnFrame.Pc = *pPc;
	pStack->pSlot[pStack->Position].ReturnFrame.FramePosition = pStack->FramePosition;
	pStack->pSlot[pStack->Position].ReturnFrame.FrameSize = pStack->FrameSize;
	++pStack->Position;
	pStack->FramePosition = FirstArg;
	pStack->FrameSize = pStack->Position - FirstArg;
	pStack->OnErrorPc.Index = -1;
}

//////////////////////////////////////////////////////////////////////

int StackFuncReturn(Stack_s *pStack, PC_s *pPc)
{
int i, RetFrame, RetException;

	if(pStack->Position == 0) return 0;
	assert(pStack->FrameSize);
	RetFrame = pStack->FramePosition + pStack->FrameSize - 1;
	RetException = pStack->FramePosition + pStack->FrameSize - 2;
	assert((RetException >= 0) && (RetFrame < pStack->Position));
	for(i = 0; i < pStack->FrameSize - 2; ++i) VarDestroy(&pStack->pSlot[pStack->FramePosition + i].Var);
	pStack->Position = pStack->FramePosition;
	if(pPc != nullptr) *pPc = pStack->pSlot[RetFrame].ReturnFrame.Pc;
	pStack->FrameSize = pStack->pSlot[RetFrame].ReturnFrame.FrameSize;
	pStack->FramePosition = pStack->pSlot[RetFrame].ReturnFrame.FramePosition;
	pStack->OnErrorPc = pStack->pSlot[RetException].ReturnException.OnError;
	return 1;
}

//////////////////////////////////////////////////////////////////////

void StackFuncEnd(Stack_s *pStack)
{
Symbol_s **ppTail;

	for(ppTail = &pStack->pAllSymbols; *ppTail != nullptr; ppTail = &(*ppTail)->pNext);
	*ppTail = pStack->pLocalSymbols;
	pStack->pLocalSymbols = nullptr;
}

//////////////////////////////////////////////////////////////////////

void StackPushGosubRet(Stack_s *pStack, PC_s *pPc, int KeyIndex)
{
	if(!StackIncSize(pStack, 1)) return;
	pStack->pSlot[pStack->Position].ReturnFrame.Pc = *pPc;
	pStack->pSlot[pStack->Position].ReturnFrame.KeyIndex = KeyIndex;
	++pStack->Position;
}

//////////////////////////////////////////////////////////////////////

int StackGosubReturn(Stack_s *pStack, PC_s *pPc)
{
int KeyIndex; 

	if(pStack->Position <= pStack->FramePosition + pStack->FrameSize) return 0;
	--pStack->Position;
	if(pPc) *pPc = pStack->pSlot[pStack->Position].ReturnFrame.Pc;
	KeyIndex = pStack->pSlot[pStack->Position].ReturnFrame.KeyIndex;
	if((KeyIndex >= 0) && (KeyIndex < MAX_SOFT_KEYS)){ 
		if(pCntx->EventKeys[KeyIndex].State == eOnKeyState::BLOCKED) pCntx->EventKeys[KeyIndex].State = eOnKeyState::ACTIVE;	// this may have been changed in the subroutine
	}
	return 1;
}

//////////////////////////////////////////////////////////////////////

int StackVariable(Stack_s *pStack, const Identifier_s *pIdent)
{
Symbol_s **ppTail;
int Offset;

	for(Offset = 0, ppTail = &pStack->pLocalSymbols; *ppTail != nullptr; ppTail = &(*ppTail)->pNext, ++Offset) if(!strcmp((*ppTail)->pName, pIdent->Name)) return 0; 	 // return if found
	(*ppTail) = (Symbol_s*)malloc(sizeof(Symbol_s));
	memset((*ppTail), 0, sizeof(Symbol_s));
	(*ppTail)->pNext = nullptr;
	int len = strlen(pIdent->Name) + 1;
	(*ppTail)->pName = (char*)malloc(len);
	strcpy_s((*ppTail)->pName, len, pIdent->Name);
	(*ppTail)->SymbolType = eSymType::STACKVAR;
	(*ppTail)->uType.Stack.ValType = pIdent->DefType;
	/* the offset -1 of the eVType::V_VOID procedure return symbol is ok, it is not used */
	(*ppTail)->uType.Stack.Offset = Offset - (pStack->pLocalSymbols->uType.Stack.ValType == eVType::V_VOID ? 1 : 0);
	return 1;
}

//////////////////////////////////////////////////////////////////////

Var_s *StackPushArg(Stack_s *pStack)
{
	if(!StackIncSize(pStack, 1)) return nullptr;
	return &pStack->pSlot[pStack->Position++].Var;
}

//////////////////////////////////////////////////////////////////////

Var_s *StackGetAt(Stack_s *pStack, int Offset)
{
	assert(pStack->FrameSize > (Offset + 2));
	Var_s *pVar = &(pStack->pSlot[pStack->FramePosition + Offset].Var);
	if(pVar->IsReference) return pVar->pRefVar;													 // return reference Var pointer
	return pVar;
}

//////////////////////////////////////////////////////////////////////

Var_s *StackGetVar(Stack_s *pStack, int Offset)
{
	assert(pStack->FrameSize > (Offset + 2));
	Var_s *pVar = &(pStack->pSlot[pStack->FramePosition + Offset].Var);
	return pVar;
}

//////////////////////////////////////////////////////////////////////

enum eVType StackArgType(const Stack_s *pStack, int Index)													 // get the type of the local argument
{
Symbol_s *pStackSym;
int Offset;

	if(pStack->pLocalSymbols->uType.Stack.ValType == eVType::V_VOID) ++Index;
	for(Offset = 0, pStackSym = pStack->pLocalSymbols; Offset != Index; pStackSym = pStackSym->pNext, ++Offset) assert(pStackSym != nullptr);
	assert(pStackSym != nullptr);
	return pStackSym->uType.Stack.ValType;
}

//////////////////////////////////////////////////////////////////////

enum eVType StackVarType(const Stack_s *pStack, Symbol_s *pSym)
{
Symbol_s *pStackSym;

	for(pStackSym = pStack->pLocalSymbols; pStackSym->uType.Stack.Offset != pSym->uType.Stack.Offset; pStackSym = pStackSym->pNext) assert(pStackSym != nullptr);
	assert(pStackSym != nullptr);
	return pStackSym->uType.Stack.ValType;
}

//////////////////////////////////////////////////////////////////////

void StackFrameToError(Stack_s *pStack, Program_s *pProg, pValue_s pValue)
{
int i = pStack->Position, FramePointer, FrameSize, RetFrame;
PC_s p;

	FramePointer = pStack->FramePosition;
	FrameSize = pStack->FrameSize;
	if(i > FramePointer + FrameSize){
		p = pStack->pSlot[--i].ReturnFrame.Pc;
		ValueErrorSuffix(pValue, _T(", Called"));
		ProgramPCtoError(pProg, &p, pValue);
	}
	else{
		if(i){
			RetFrame = FramePointer + FrameSize - 1;
			i = FramePointer;
			p = pStack->pSlot[RetFrame].ReturnFrame.Pc;
			FrameSize = pStack->pSlot[RetFrame].ReturnFrame.FrameSize;
			FramePointer = pStack->pSlot[RetFrame].ReturnFrame.FramePosition;
			ValueErrorSuffix(pValue, _T(", Proc Called"));
			ProgramPCtoError(pProg, &p, pValue);
		}
	}
}

//////////////////////////////////////////////////////////////////////

void StackSetError(Stack_s *pStack, long int Line, PC_s *pPc, pValue_s pValue)
{
	pStack->ErrorPc = *pPc;
	pStack->ErrorLine = Line;
	ValueDestroy(&pStack->Error);
	ValueClone(&pStack->Error, pValue);
}

//////////////////////////////////////////////////////////////////////


