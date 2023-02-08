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

typedef struct Common_s{
	Symbol_s *pTable[IDENT_HASHSIZE];
} Common_s;

/////////////////////////////////////////////////////////////////////////////////////////////////

void 						DestroyCommon(void);
void						CommonClear(void);
int 						CommonFind(Identifier_s *pIdent, eSymType SymbolType);
int							CommonVariable(Identifier_s *pIdent, eVType Type, eSymType SymbolType);



