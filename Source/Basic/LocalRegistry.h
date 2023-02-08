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

#include "BasicTypes.h"

 /////////////////////////////////////////////////////////////////////////////////////////////////

extern inline eSymType  LibSearchType(TokenType_e TokType, eVType DefType);
Registry_s*			NewIdentifiers(void);
void						DeleteIdentifiers(Registry_s *pRegistry);
int 						LocalFind(Registry_s *pRegistry, Identifier_s *pIdent, eSymType SymbolType);
void						RegisterBuiltIn(Registry_s *pRegistry, const char *pIdent, eVType ValueType, pValue_s (* func)(pValue_s pValue, Stack_s *pStack), int argLength, ...);
int							RegisterVariable(Registry_s *pRegistry, Identifier_s *pIdent, eVType Type, eSymType SymbolType, int Redeclare);
int							RegisterLocalFunc(Registry_s *pRegistry, Identifier_s *pIdent, eVType Type, PC_s *pFunc, PC_s *pBegin, int ArgLength, eVType *pArgTypes);
void 						RegisterLocalFuncEnd(Registry_s *pRegistry, Identifier_s *pIdent, PC_s *pEnd);
void						ResetVariables(Registry_s *pRegistry);
bool						CloneVar(Registry_s *pRegistry, Symbol_s *pSym);
void						DestroyVarIdentifiers(Registry_s *pRegistry);
void						DestroyFuncIdentifiers(Registry_s *pRegistry);
void						DestroyAllIdentifiers(Registry_s *pRegistry);
void						DestroyIdentifiers(Registry_s *pRegistry, eSymType Type);



