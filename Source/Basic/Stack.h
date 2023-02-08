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

#pragma once

Stack_s*			NewStack(Stack_s *pStack);
void 					DestroyStack(Stack_s *pStack);
void					CleanStack(Stack_s *pStack);
int 					StackFind(Stack_s *pStack, Identifier_s *pIdent);
inline bool		StackIncSize(Stack_s *pStack, int inc);
void 					StackPushFuncRet(Stack_s *pStack, int firstarg, PC_s *pPc);
int 					StackFuncReturn(Stack_s *pStack, PC_s *pPc);
void 					StackFuncEnd(Stack_s *pStack);
void 					StackPushGosubRet(Stack_s *pStack, PC_s *pPc, int KeyIndex);
int 					StackGosubReturn(Stack_s *pStack, PC_s *pPc);
int 					StackVariable(Stack_s *pStack, const Identifier_s *pIdent);
Var_s*				StackPushArg(Stack_s *pStack);
Var_s*				StackGetAt(Stack_s *pStack, int Offset);
Var_s*				StackGetVar(Stack_s *pStack, int Offset);
eVType				StackArgType(const Stack_s *pStack, int Offset);
eVType				StackVarType(const Stack_s *pStack, Symbol_s *pSym);
void 					StackFrameToError(Stack_s *pStack, Program_s *pProg, Value_s *pVal);
void 					StackSetError(Stack_s *pStack, long line, struct PC_s *pc, Value_s *pVal);

