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

/////////////////////////////////////////////////////////////////////////////////////////////////

SubCntx_s*			NewSubCntx(void);
void						DeleteSubCntx(pSubCntx_s pSub);
Registry_s*			NewGlobalRegistry(void);
void 						DeleteGlobals(Registry_s *pRegistry);
int							GlobalFind(Identifier_s *pIdent, eSymType SymbolType);
int							RegisterSubProg(Registry_s *pRegistry, Identifier_s *pIdent, PC_s *pFunc, PC_s *pBegin, int ArgLength, eVType *pArgTypes);
void 						RegisterSubProgEnd(Registry_s *pRegistry, Identifier_s *pIdent, PC_s *pEnd);
int							RegisterGlobalFunc(Registry_s *pRegistry, Identifier_s *pIdent, eVType Type, PC_s *pFunc, PC_s *pBegin, int ArgLength, eVType *pArgTypes);
void 						RegisterGlobalFuncEnd(Registry_s *pRegistry, Identifier_s *pIdent, PC_s *pEnd);
void						DestroyGlobals(Registry_s *pRegistry);



