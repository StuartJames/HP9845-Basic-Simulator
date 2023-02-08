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

UINT GetArraySize(Var_s	*pVar)
{
UINT Count = 1;

	if((pVar->Dimensions <= 0) || (pVar->Dimensions > 6)) return -1;
	for(int i = 0; i < pVar->Dimensions; ++i) Count *= pVar->pGeometry[i].Size;
	return Count;
}

/////////////////////////////////////////////////////////////////////////////

void IndexToSubscript(int Dim, const Geometry_s *pGeometry, int Index, int Subscript[])
{
div_t d;
int i = Dim - 1;
int Quot = Index;

  for(int j = 0; j < Dim; ++j, --i){
		d = div(Quot, pGeometry[i].Size);
		Subscript[i] = (d.rem + pGeometry[i].Base);
		Quot = d.quot;
	}
}

/////////////////////////////////////////////////////////////////////////////

int SubscriptToIndex(int Dim, const Geometry_s *pGeometry, int Subscript[], int *Index)
{
int Idx = 0;
int Factor = 1, i = Dim - 1;

  for(int j = 0; j < Dim; ++j, --i){
		if((Subscript[i] < pGeometry[i].Base) || ((Subscript[i] - pGeometry[i].Base) >= pGeometry[i].Size)) return -1;
		Idx += (Subscript[i] - pGeometry[i].Base) * Factor;
		Factor = pGeometry[i].Size;
	}
	return *Index = Idx;
}

/////////////////////////////////////////////////////////////////////////////

struct Var_s *VarNew(Var_s *pVar, enum eVType Type, int Dim, const Geometry_s *pGeometry)
{
int i, Size = 1;

  if(pVar == nullptr) return nullptr;
	pVar->IsDefined = false;
	pVar->Type = Type;
	pVar->Dimensions = Dim;
	pVar->Size = Size;
	pVar->IsReference = false;																									// default to value
	for(Dim = 0; Dim < pVar->Dimensions; ++Dim){																// calculate the number of elements
		if((Size *= pGeometry[Dim].Size) < pVar->Size) return nullptr;
		pVar->Size = Size;
	}
	if((Size *= sizeof(struct Value_s)) < pVar->Size) return nullptr;			
	if((pVar->pValue = (pValue_s)malloc(Size)) == nullptr) return nullptr;			// reserve space
	if(Dim){
		pVar->pGeometry = (Geometry_s*)malloc(sizeof(Geometry_s) * Dim);					// define the dimensions
		for(i = 0; i < Dim; ++i){
			pVar->pGeometry[i].Size = pGeometry[i].Size;
			pVar->pGeometry[i].Base = pGeometry[i].Base;
		}
	}
	else pVar->pGeometry = nullptr;
	for(i = 0; i < pVar->Size; ++i){
		ValueNewNull(&(pVar->pValue[i]), Type);																		// plug in the new elements
		pVar->pValue[i].Index = i;																								// set the index in the array
	}
	return pVar;
}

/////////////////////////////////////////////////////////////////////////////

struct Var_s *VarNewScalar(Var_s *pVar)
{
	pVar->Dimensions = 0;
	pVar->Size = 1;
	pVar->IsReference = false;																								// default to value
	pVar->pGeometry = nullptr;
	pVar->pValue = (Value_s*)malloc(sizeof(struct Value_s));
	ValueNewNil(pVar->pValue);
	return pVar;
}

/////////////////////////////////////////////////////////////////////////////

void VarDestroy(Var_s *pVar)
{
	while(pVar->Size--) ValueDestroy(&(pVar->pValue[pVar->Size]));
	free(pVar->pValue);
	pVar->pValue = nullptr;
	pVar->Size = 0;
	pVar->Dimensions = 0;
	pVar->IsReference = false;
	pVar->IsDefined = false;
	if(pVar->pGeometry){
		free(pVar->pGeometry);
		pVar->pGeometry = nullptr;
	}
}

/////////////////////////////////////////////////////////////////////////////

void VarReset(struct Var_s *pVar)
{
	for(int i = 0; i < pVar->Size; ++i){
		ValueDestroy(&(pVar->pValue[i]));
		ValueNewNull(&(pVar->pValue[i]), pVar->Type);
	}
}

/////////////////////////////////////////////////////////////////////////////

void VarRetype(Var_s *pVar, enum eVType Type)
{
	for(int i = 0; i < pVar->Size; ++i){
		ValueDestroy(&(pVar->pValue[i]));
		ValueNewNull(&(pVar->pValue[i]), Type);
	}
}

/////////////////////////////////////////////////////////////////////////////

Value_s *VarValue(Var_s *pVar, int Dim, int Subscript[], Value_s *pValue)										// Geometry is in reverse order, ie. [0] is hightest order and [n] is lowest order (row)
{																																														// subscripts are in reverse order, ie. (a,b,c) where 'a' = is highest order and 'c' = lowest order (row)
int Index = 0;

	assert(pVar->pValue);																												
	if(Dim != pVar->Dimensions) return ValueNewError(pValue, DIMENSION);
	if(Dim && (pVar->Dimensions > 0) && (Subscript != nullptr)){
		if((SubscriptToIndex(pVar->Dimensions, pVar->pGeometry, reinterpret_cast<int*>(Subscript), &Index) == -1) || (Index >= (int)pVar->Size)) return ValueNewError(pValue, OUTOFRANGE, "Subscript");	// calculate the element offset into the array
	}
	return pVar->pValue + Index;
}

/////////////////////////////////////////////////////////////////////////////

Value_s *VarValue(Var_s *pVar, int Index, Value_s *pValue)																	// Geometry is in reverse order, ie. [0] is hightest order and [n] is lowest order (row)
{																																														// subscripts are in reverse order, ie. (a,b,c) where 'a' = is highest order and 'c' = lowest order (row)
	assert(pVar->pValue);																												
	if(Index >= (int)pVar->Size) return ValueNewError(pValue, OUTOFRANGE, "Index");						// calculate the element offset into the array
	return pVar->pValue + Index;
}

/////////////////////////////////////////////////////////////////////////////

Value_s *VarVar(Var_s *pVar, Value_s *pValue)
{
	assert(pVar->pValue);
	if(pVar->Dimensions < 1) return ValueNewError(pValue, DIMENSION);
	pValue->uData.pArray = pVar;
	pValue->Type = eVType::V_ARRAY;
	return pValue;
}

/////////////////////////////////////////////////////////////////////////////

Value_s *VarMatAssign(Var_s *pVarX, Var_s *pVarY, Value_s *pErr, int Pass)
{
enum eVType xType = pVarX->Type;
int i, Size = 1;

	if(Pass){
		if(pVarX == pVarY) return nullptr;
		for(i = 0; i < pVarY->Dimensions; ++i) Size *= pVarY->pGeometry[i].Size;				// get the number of elements
		VarDestroy(pVarX);
		VarNew(pVarX, xType, pVarY->Dimensions, pVarY->pGeometry);
		for(i = 0; i < Size; ++i){
			ValueDestroy(&(pVarX->pValue[i]));
			ValueClone(&(pVarX->pValue[i]), &(pVarY->pValue[i]));												// set the new value
			ValueRetype(&(pVarX->pValue[i]), xType);
		}
	}
	else if(ValueCommonType[(int)pVarX->Type][(int)pVarY->Type] == eVType::V_ERROR) return ValueNewTypeError(pErr, pVarX->Type, pVarY->Type);
	return nullptr;
}

/////////////////////////////////////////////////////////////////////////////

void VarMatScalarSet(Var_s *pVar, Value_s *pValue, int Pass)
{
enum eVType thisType = pVar->Type;
int i, Size;

	ValueRetype(pValue, thisType);
	if(Pass){
		for(Size = 1, i = 0; i < pVar->Dimensions; ++i) Size *= pVar->pGeometry[i].Size;
		for(i = 0; i < Size; ++i){
			ValueDestroy(&(pVar->pValue[i]));
			ValueClone(&(pVar->pValue[i]), pValue);
		}
	}
}

/////////////////////////////////////////////////////////////////////////////

Value_s *VarMatAddSub(Var_s *pVarX, Var_s *pVarY, Var_s *pVarZ, int Add, Value_s *pErr, int Pass) 
{
int i, Count = 1;
enum eVType Type = pVarX->Type;
Value_s yVal, zVal;

	if(Pass){
		assert((pVarY->Dimensions > 0) && (pVarY->Dimensions == pVarZ->Dimensions));
		for(i = 0; i < pVarY->Dimensions; ++i) if((pVarY->pGeometry[i].Size) != (pVarZ->pGeometry[i].Size)) return ValueNewError(pErr, DIMENSION);		 // check source arrays have equivilent number of elements
		for(i = 0; i < pVarZ->Dimensions; ++i) Count *= pVarZ->pGeometry[i].Size;		 // we work the whole array  
		if(pVarX != pVarY && pVarX != pVarZ){																				 // check for destination not source
			VarDestroy(pVarX);
			VarNew(pVarX, Type, pVarY->Dimensions, pVarY->pGeometry);
		}
		for(i = 0; i < Count; ++i){																									 // sequence through each element
			ValueClone(&yVal, &(pVarY->pValue[i]));
			ValueClone(&zVal, &(pVarZ->pValue[i]));
			if(Add) ValueAdd(&yVal, &zVal, 1);
			else ValueSub(&yVal, &zVal, 1);
			if(yVal.Type == eVType::V_ERROR){
				*pErr = yVal;
				ValueDestroy(&zVal);
				return pErr;
			}
			ValueDestroy(&zVal);
			ValueDestroy(&(pVarX->pValue[i]));
			pVarX->pValue[i] = *ValueRetype(&yVal, Type);
		}
	}
	else{
		ValueClone(pErr, pVarY->pValue);
		if(Add) ValueAdd(pErr, pVarZ->pValue, 0);
		else ValueSub(pErr, pVarZ->pValue, 0);
		if(pErr->Type == eVType::V_ERROR) return pErr;
		ValueDestroy(pErr);
	}
	return nullptr;
}

/////////////////////////////////////////////////////////////////////////////

Value_s	*VarMatScalarAddSub(Var_s *pVarX, Var_s *pVarY, Value_s *pFactor, int Add, Value_s *pErr, int Pass)
{
int i, ySize;
enum eVType Type = pVarX->Type;
Value_s yVal;

	if(Pass){
		assert(pVarY->Dimensions >= 1);
		for(ySize = 1, i = 0; i < pVarY->Dimensions; ++i) ySize *= pVarY->pGeometry[i].Size;		// check source arrays have equivilent number of elements
		if(pVarX != pVarY){																																			// check for destination not source
			VarDestroy(pVarX);
			VarNew(pVarX, Type, pVarY->Dimensions, pVarY->pGeometry);
		}
		for(i = 0; i < ySize; ++i){																									 // sequence through each element
			ValueClone(&yVal, &(pVarY->pValue[i]));
			if(Add) ValueAdd(&yVal, pFactor, 1);
			else ValueSub(&yVal, pFactor, 1);
			if(yVal.Type == eVType::V_ERROR){
				*pErr = yVal;
				return pErr;
			}
			ValueDestroy(&(pVarX->pValue[i]));
			pVarX->pValue[i] = *ValueRetype(&yVal, Type);
		}
	}
	else{
		ValueClone(pErr, pVarY->pValue);
		if(Add) ValueAdd(pErr, pFactor, 0);
		else ValueSub(pErr, pFactor, 0);
		if(pErr->Type == eVType::V_ERROR) return pErr;
		ValueDestroy(pErr);
	}
	return nullptr;
}

/////////////////////////////////////////////////////////////////////////////

Value_s *VarMatMultiply(Var_s *pVarX, Var_s *pVarY, Var_s *pVarZ, Value_s *pError, int Pass)							// only 2 dimensions
{
int i, j, k, Dim;
Geometry_s Geometry[DEF_MAX_DIMENSIONS];
eVType xType = pVarX->Type;
Var_s ResultVar;

	if(Pass){
		if((pVarY->Dimensions < 1) || (pVarY->Dimensions > 2) || (pVarZ->Dimensions < 1) || (pVarZ->Dimensions > 2) || (pVarY->pGeometry[1].Size != pVarZ->pGeometry[0].Size)) return ValueNewError(pError, DIMENSION);
		Dim = ((pVarY->Dimensions > 1) && (pVarZ->Dimensions > 1)) ? 2 : 1;												 
		Geometry[0] = pVarY->pGeometry[0];
		Geometry[1] = pVarZ->pGeometry[1];
		VarNew(&ResultVar, xType, Dim, Geometry);
		for(i = 0; i < Geometry[0].Size; ++i){
			for(j = 0; j < Geometry[1].Size; ++j){
				pValue_s pValRes = &ResultVar.pValue[i * Geometry[1].Size + j];
				ValueNewNull(pValRes, xType);
				for(k = 0; k < pVarY->pGeometry[1].Size; ++k){
					Value_s ValProduct;
					ValueClone(&ValProduct, &(pVarY->pValue[i * pVarY->pGeometry[1].Size + k]));
					ValueMultiply(&ValProduct, &(pVarZ->pValue[k * pVarZ->pGeometry[1].Size + j]), 1);
					if(ValProduct.Type == eVType::V_ERROR){
						*pError = ValProduct;
						VarDestroy(&ResultVar);
						return pError;
					}
					ValueAdd(pValRes, &ValProduct, 1);
					ValueDestroy(&ValProduct);
				}
				ValueRetype(pValRes, xType);
			}
		}
		VarDestroy(pVarX);
		*pVarX = ResultVar;
	}
	else{
		ValueClone(pError, pVarY->pValue);
		ValueMultiply(pError, pVarZ->pValue, 0);
		if(pError->Type == eVType::V_ERROR) return pError;
		ValueDestroy(pError);
	}
	return nullptr;
}

/////////////////////////////////////////////////////////////////////////////

Value_s *VarMatScalarMultiply(Var_s *pVarX,  Var_s *pVarY, Value_s *pFactor,int Pass)
{
eVType xType = pVarX->Type;
int i, Size = 1;

	if(Pass){
		assert(pVarY->Dimensions >= 1);
		if(pVarX != pVarY){
			VarDestroy(pVarX);
			VarNew(pVarX, xType, pVarY->Dimensions, pVarY->pGeometry);
		}
		for(i = 0; i < pVarX->Dimensions; ++i) Size *= pVarY->pGeometry[i].Size;
		for(i = 0; i < Size; ++i){
			Value_s ValX;
			ValueClone(&ValX, &(pVarY->pValue[i]));
			ValueMultiply(&ValX, pFactor, 1);
			if(ValX.Type == eVType::V_ERROR){
				ValueDestroy(pFactor);
				*pFactor = ValX;
				return pFactor;
			}
			ValueDestroy(&(pVarX->pValue[i]));
			pVarX->pValue[i] = *ValueRetype(&ValX, xType);
		}
	}
	else if(ValueMultiply(pVarX->pValue, pFactor, 0)->Type == eVType::V_ERROR) return pFactor;	// just check type
	return nullptr;
}

/////////////////////////////////////////////////////////////////////////////

void VarMatTranspose(Var_s *pVarX, Var_s *pVarY)
{
int i, j;
Geometry_s pGeometry[DEF_MAX_DIMENSIONS];																			
eVType xType = pVarX->Type;
Var_s VarTrn;

	if((pVarY->Dimensions != 2)) return;
	pGeometry[0] = pVarY->pGeometry[1];
	pGeometry[1] = pVarY->pGeometry[0];
	VarNew(&VarTrn, xType, 2, pGeometry);
	for(i = 0; i < pVarY->pGeometry[0].Size; ++i){
		for(j = 0; j < pVarY->pGeometry[1].Size; ++j){
			ValueDestroy(&VarTrn.pValue[j * pVarY->pGeometry[0].Size + i]);
			ValueClone(&VarTrn.pValue[j * pVarY->pGeometry[0].Size + i], &(pVarY->pValue[i * pVarY->pGeometry[1].Size + j]));
			ValueRetype(&VarTrn.pValue[j * pVarY->pGeometry[0].Size + i], xType);
		}
	}
	VarDestroy(pVarX);
	*pVarX = VarTrn;
}

/////////////////////////////////////////////////////////////////////////////

Value_s *VarMatInverse(Var_s *pVarX, Var_s *pVarY, Value_s *pValue, Value_s *pErr)
{
enum eVType Type = pVarX->Type;																									 // THIS NEEDS WORK
int n, i, j, k, max;
double t, *a, *u, d;

	if(pVarY->Type != eVType::V_INT && pVarY->Type != eVType::V_REAL) return ValueNewError(pErr, TYPEMISMATCH5);						// must be numeric ...
	if(pVarY->pGeometry[0].Size != pVarY->pGeometry[1].Size) return ValueNewError(pErr, MATRIXNOTSQR);					// ... and square
	n = pVarY->pGeometry[0].Size;
	a = (double*)malloc(sizeof(double) * n * n);
	u = (double*)malloc(sizeof(double) * n * n);
	for(i = 0; i < n; ++i){
		for(j = 0; j < n; ++j){
			if(pVarY->Type == eVType::V_INT) a[i * n + j] = pVarY->pValue[i * n + j].uData.Integer;
			else a[i * n + j] = pVarY->pValue[i * n + j].uData.Real;
			u[i * n + j] = (i == j) ? 1.0 : 0.0;
		}
	}
	d = 1.0;
	for(i = 0; i < n; ++i){ /* get zeroes in column i below the main diagonale */
		max = i;
		for(j = i + 1; j < n; ++j) if(fabs(a[j * n + i]) > fabs(a[max * n + i])) max = j;
		/* exchanging row i against row max */
		if(i != max) d = -d;
		for(k = i; k < n; ++k){
			t = a[i * n + k];
			a[i * n + k] = a[max * n + k];
			a[max * n + k] = t;
		}
		for(k = 0; k < n; ++k){
			t = u[i * n + k];
			u[i * n + k] = u[max * n + k];
			u[max * n + k] = t;
		}
		if(a[i * n + i] == 0.0){
			free(a);
			free(u);
			return ValueNewError(pErr, SINGULAR);
		}
		for(j = i + 1; j < n; ++j){
			t = a[j * n + i] / a[i * n + i];
			/* substract row i*t from row j */
			for(k = i; k < n; ++k) a[j * n + k] -= a[i * n + k] * t;
			for(k = 0; k < n; ++k) u[j * n + k] -= u[i * n + k] * t;
		}
	}
	for(i = 0; i < n; ++i) d *= a[i * n + i];											/* compute determinant */
	for(i = n - 1; i >= 0; --i){																	/* get zeroes in column i above the main diagonal */
		for(j = 0; j < i; ++j){
			t = a[j * n + i] / a[i * n + i];
			a[j * n + i] = 0.0;																				/* a[j * n + i] -= a[i * n + i] * t; */
			for(k = 0; k < n; ++k) u[j * n + k] -= u[i * n + k] * t; 	/* subtract row i * t from row j */
		}
		t = a[i * n + i];
		a[i * n + i] = 1.0;																					/* a[i * n + i] /= t; */
		for(k = 0; k < n; ++k) u[i * n + k] /= t;
	}
	free(a);
	if(pVarX != pVarY){
		VarDestroy(pVarX);
		VarNew(pVarX, Type, 2, pVarY->pGeometry);
	}
	for(i = 0; i < n; ++i){
		for(j = 0; j < n; ++j){
			ValueDestroy(&pVarX->pValue[i * n + j]);
			if(Type == eVType::V_INT) ValueNewInteger(&pVarX->pValue[i * n + j], (int)u[i * n + j]);
			else ValueNewReal(&pVarX->pValue[i * n + j], u[i * n + j]);
		}
	}
	free(u);
	ValueDestroy(pValue);
	if(Type == eVType::V_INT) ValueNewInteger(pValue, (int)d);
	else ValueNewReal(pValue, d);
	return nullptr;
}

/////////////////////////////////////////////////////////////////////////////

Value_s *VarMatRedim(Var_s *pVar, int NewDim, const Geometry_s *pGeometry, Value_s *pError)
{
int NewSize = 1;
pValue_s pValNew;																																		
int Subscript[DEF_MAX_DIMENSIONS];																			

	if((pVar->Dimensions > 0) && (pVar->Dimensions != NewDim)) return ValueNewError(pError, DIMENSION);
	for(int i = 0; i < NewDim; ++i) NewSize *= pGeometry[i].Size;																							// calculate new size
	pValNew = (pValue_s)malloc(sizeof(Value_s) * NewSize);																											// make space for the new matrix
	for(int i = 0; i < NewSize; ++i){																																					// for each new value
		IndexToSubscript(NewDim, pGeometry, i, Subscript);																												// get the subscript
		bool IsNewVal = false;
		for(int j = 0; j < pVar->Dimensions; ++j) if(Subscript[j] >= pVar->pGeometry[j].Size) IsNewVal = true;		// does it exist in the old matrix
		if(IsNewVal) ValueNewNull(&(pValNew[i]), pVar->Type);																											// if no, generate a new value
		else{
			int k;
			if(SubscriptToIndex(NewDim, pVar->pGeometry, Subscript, &k) < 0) k = 0;																	// else get the index to the original value
			ValueClone(&pValNew[i], &pVar->pValue[k]);																															// and plug it in
		}
		pValNew[i].Index = i;
	}
	for(int i = 0; i < pVar->Size; ++i) ValueDestroy(&pVar->pValue[i]);																				// clean out the original
	free(pVar->pValue);
	if(pVar->pGeometry == nullptr) pVar->pGeometry = (Geometry_s*)malloc(sizeof(Geometry_s) * NewDim);					// rebuild the geometry
	for(int i = 0; i < NewDim; ++i){
		pVar->pGeometry[i].Size = pGeometry[i].Size;
		pVar->pGeometry[i].Base = pGeometry[i].Base;
	}
	pVar->Dimensions = NewDim;																																									// assign the new values
	pVar->Size = NewSize;
	pVar->pValue = pValNew;
	return nullptr;
}

/////////////////////////////////////////////////////////////////////////////

bool IsValidReference(Symbol_s *pSymbol)
{
	if((pSymbol->SymbolType != eSymType::LOCALVAR) && (pSymbol->SymbolType != eSymType::LOCALARRAY)) return false;
	if((pSymbol->uType.Var.Type == eVType::V_STRING) && (pSymbol->uType.Var.pValue->uData.pString->SSType() != eSSType::NONE)) return false;
	return true;
}

/////////////////////////////////////////////////////////////////////////////

Value_s *SaveResult(Value_s *pValue)
{
	switch(pValue->Type){
		case eVType::V_INT:
			ValueRetype(&g_ResultValue, pValue->Type);
			g_ResultValue.uData.Integer = pValue->uData.Integer;
			break;
		case eVType::V_REAL:
			ValueRetype(&g_ResultValue, pValue->Type);
			g_ResultValue.uData.Real = pValue->uData.Real;
			break;
		case eVType::V_STRING:
//			ValueRetype(&g_ResultValue, pValue->Type);
//			StringClone(&g_ResultValue.uData.String, &pValue->uData.String);
			break;
		case eVType::V_ERROR:
		case eVType::V_NIL:
		case eVType::V_ARRAY:
			break;
		default: assert(0);
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////



