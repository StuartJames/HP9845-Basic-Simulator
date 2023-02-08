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


#define VAR_SCALAR_VALUE(pVar) ((pVar)->pValue)

UINT			GetArraySize(Var_s	*pVar);
void			IndexToSubscript(int Dim, const Geometry_s *pGeometry, int Index, int Subscript[]);
int				SubscriptToIndex(int Dim, const Geometry_s *pGeometry, int Subscript[], int *Index);
Var_s 		*VarNew(Var_s *pVar, eVType Type, int Dim, const Geometry_s *pGeometry);
Var_s 		*VarNewScalar(Var_s *pVar);
void 			VarDestroy(Var_s *pVar);
void			VarRetype(Var_s *pVar, eVType Type);
Value_s		*VarValue(Var_s *pVar, int Dim, int Subscript[], Value_s *pValue);
Value_s		*VarValue(Var_s *pVar, int Index, Value_s *pValue);
Value_s		*VarVar(Var_s *pVar, Value_s *pValue);
void			VarReset(Var_s *pVar);
Value_s		*VarMatAssign(Var_s *pVarX, Var_s *pVarY, Value_s *pErr, int Work);
Value_s		*VarMatAddSub(Var_s *pVarX, Var_s *pVarY, Var_s *pVarZ, int Add, Value_s *pErr, int Work);
Value_s		*VarMatMultiply(Var_s *pVarX, Var_s *pVarY, Var_s *pVarZ, Value_s *pErr, int Work);
void			VarMatScalarSet(Var_s *pVarX, Value_s *pValue, int Work);
Value_s		*VarMatScalarMultiply(Var_s *pVarX, Var_s *pVarY, Value_s *pFactor, int Work);
Value_s		*VarMatScalarAddSub(Var_s *pVarX, Var_s *pVarY, Value_s *pFactor, int Add, Value_s *pErr, int Work);
void 			VarMatTranspose(Var_s *pVarX, Var_s *pVaB);
Value_s		*VarMatInverse(Var_s *pVarX, Var_s *pVarY, Value_s *pValue, Value_s *pErr);
Value_s		*VarMatRedim(Var_s *pVar, int Dim, const Geometry_s *pGeometry, Value_s *pErr);
bool			IsValidReference(Symbol_s *pSymbol);
Value_s		*SaveResult(Value_s *pValue);


