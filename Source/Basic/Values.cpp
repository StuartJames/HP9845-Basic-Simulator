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

static BString NullStr;

/////////////////////////////////////////////////////////////////////////////

static const char *typestr[] =
{
	NULL,
	NULL,
	_T("integer"),
	NULL,
	_T("real"),
	_T("string"),
	_T("void")
};

/////////////////////////////////////////////////////////////////////////////

const enum eVType ValueCommonType[eVType::V_END][eVType::V_END] =
{
	{ eVType::V_NONE, eVType::V_NONE,  eVType::V_NONE,    eVType::V_NONE,  eVType::V_NONE,  eVType::V_NONE,   eVType::V_NONE  },
	{ eVType::V_NONE, eVType::V_ERROR, eVType::V_ERROR,   eVType::V_ERROR, eVType::V_ERROR, eVType::V_ERROR,  eVType::V_ERROR },
	{ eVType::V_NONE, eVType::V_ERROR, eVType::V_INT,			eVType::V_ERROR, eVType::V_REAL,  eVType::V_ERROR,  eVType::V_ERROR },
	{ eVType::V_NONE, eVType::V_ERROR, eVType::V_ERROR,   eVType::V_ERROR, eVType::V_ERROR, eVType::V_ERROR,  eVType::V_ERROR },
	{ eVType::V_NONE, eVType::V_ERROR, eVType::V_REAL,    eVType::V_ERROR, eVType::V_REAL,  eVType::V_ERROR,  eVType::V_ERROR },
	{ eVType::V_NONE, eVType::V_ERROR, eVType::V_ERROR,   eVType::V_ERROR, eVType::V_ERROR, eVType::V_STRING, eVType::V_ERROR },
	{ eVType::V_NONE, eVType::V_ERROR, eVType::V_ERROR,   eVType::V_ERROR, eVType::V_ERROR, eVType::V_ERROR,  eVType::V_ERROR }
};

/////////////////////////////////////////////////////////////////////////////

inline pValue_s  NewIntegerValue(pValue_s pVal, long int n){ pVal->Type = eVType::V_INT; pVal->uData.Integer = n; return pVal;}

inline pValue_s  NewRealValue(pValue_s pVal, double n){ pVal->Type = eVType::V_REAL; pVal->uData.Real = n; return pVal;}

inline pValue_s  NewArrayValue(pValue_s pVal, long int n){ pVal->Type = eVType::V_ARRAY; pVal->uData.Integer = n; return pVal;}

inline pValue_s  DoValueRetype(pValue_s pVal, enum eVType Type){ pVal->Type == Type ? pVal : ValueRetype(pVal, Type); return pVal;}

/////////////////////////////////////////////////////////////////////////////

pValue_s ValueInit(pValue_s pVal, eVType Type)
{
	assert(pVal != NULL);
	pVal->uData.Integer = 0;
	pVal->uData.Real = 0.0;
	pVal->uData.pString = nullptr;
	pVal->Type = Type;
	pVal->Index = 0;
	return pVal;
}

/////////////////////////////////////////////////////////////////////////////

int ValueIsNull(const pValue_s pVal)
{
	switch(pVal->Type){
		case eVType::V_INT: return (pVal->uData.Integer == 0);
		case eVType::V_REAL: return (pVal->uData.Real == 0.0);
		case eVType::V_STRING: return (pVal->uData.pString->GetLength() == 0);
		default: assert(0);
	}
	return -1;
}

/////////////////////////////////////////////////////////////////////////////

void ValueDestroy(pValue_s pVal)
{
	assert(pVal != NULL);
	switch(pVal->Type){
		case eVType::V_ERROR:{ free(pVal->uData.Error.pStr); break; }
		case eVType::V_INT:
		case eVType::V_NIL:
		case eVType::V_REAL: break;
		case eVType::V_STRING:{ delete pVal->uData.pString; break; }
		case eVType::V_ARRAY:
		case eVType::V_VOID:
		case eVType::V_NONE: break;
		default: assert(0);
	}
	pVal->Type = eVType::V_NONE;
}

/////////////////////////////////////////////////////////////////////////////

pValue_s ValueNewNil(pValue_s pVal)
{
	assert(pVal != NULL);
	ValueInit(pVal, eVType::V_NIL);
	return pVal;
}

/////////////////////////////////////////////////////////////////////////////

pValue_s ValueNewError(pValue_s pVal, int code, int Num, const char *error, ...)
{
va_list ap;
char Buffer[MSG_BUFFER_SIZE];

	assert(pVal != NULL);
	ValueInit(pVal, eVType::V_ERROR);
  va_start(ap, error);
	int Len = vsprintf_s(Buffer, error, ap);
	va_end(ap);
	pVal->uData.Error.Code = code;
	pVal->uData.Error.Number = Num;
	if(Len > 0){
		pVal->uData.Error.pStr = (char*)malloc(Len + 1);
		strcpy_s(pVal->uData.Error.pStr, Len + 1, Buffer);
	}
	return pVal;
}

/////////////////////////////////////////////////////////////////////////////

pValue_s ValueNewInteger(pValue_s pVal, int n)
{
	assert(pVal != NULL);
	ValueInit(pVal, eVType::V_INT);
	pVal->uData.Integer = n;
	return pVal;
}

/////////////////////////////////////////////////////////////////////////////

pValue_s ValueNewReal(pValue_s pVal, double n)
{
	assert(pVal != NULL);
	ValueInit(pVal, eVType::V_REAL);
	pVal->uData.Real = n;
	return pVal;
}

/////////////////////////////////////////////////////////////////////////////

pValue_s ValueNewString(pValue_s pVal)
{
	assert(pVal != NULL);
	ValueInit(pVal, eVType::V_STRING);
	pVal->uData.pString = new BString;
	return pVal;
}

/////////////////////////////////////////////////////////////////////////////

struct Value_s *ValueNewVoid(pValue_s pVal)
{
	assert(pVal != NULL);
	ValueInit(pVal, eVType::V_VOID);
	return pVal;
}

/////////////////////////////////////////////////////////////////////////////

pValue_s ValueNewNull(pValue_s pVal, enum eVType Type)
{
	assert(pVal != NULL);
	ValueInit(pVal, Type);
	switch(Type){
		case eVType::V_VOID:
		case eVType::V_INT:
		case eVType::V_REAL: break;
		case eVType::V_STRING:{	pVal->uData.pString = new BString; break;	}
		default: assert(0);
	}
	return pVal;
}

/////////////////////////////////////////////////////////////////////////////

pValue_s ValueClone(pValue_s pVal, const pValue_s pOriginal)
{
	assert(pVal != nullptr);
	assert(pOriginal != nullptr);
	switch(pOriginal->Type){
		case eVType::V_ERROR:{
			int Len = strlen(pOriginal->uData.Error.pStr) + 1;
			pVal->uData.Error.pStr = (char*)malloc(Len);
			strcpy_s(pVal->uData.Error.pStr, Len, pOriginal->uData.Error.pStr);
			pVal->uData.Error.Code = pOriginal->uData.Error.Code;
			break;
		}
		case eVType::V_INT:
			pVal->uData.Integer = pOriginal->uData.Integer;
			break;
		case eVType::V_NIL:
			break;
		case eVType::V_REAL:
			pVal->uData.Real = pOriginal->uData.Real;
			break;
		case eVType::V_STRING:
			pVal->uData.pString = new BString;
			pVal->uData.pString->GetSubStr(pOriginal->uData.pString);														// may only get the selected part of the string
			if(pOriginal->uData.pString->GetSSType() != eSSType::NONE) pCntx->IsPassVar = false;	// if so it cannot be a reference
			break;
		case eVType::V_ARRAY:
			pVal->uData.pArray = pOriginal->uData.pArray;											// we don't actually clone this just copy a reference
			break;
		default: assert(0);
	}
	pVal->Index = pOriginal->Index;
	pVal->Type = pOriginal->Type;
	return pVal;
}

/////////////////////////////////////////////////////////////////////////////

pValue_s ValueAssign(pValue_s pVal, pValue_s pValue, int Execute)
{
	switch(pVal->Type){
		case eVType::V_INT:{
			DoValueRetype(pValue, eVType::V_INT);
			if(Execute) pVal->uData.Integer = pValue->uData.Integer;
			break;
		}
		case eVType::V_REAL:{
			DoValueRetype(pValue, eVType::V_REAL);
			if(Execute) pVal->uData.Real = pValue->uData.Real;
			break;
		}
		case eVType::V_STRING:{
			if(Execute) pVal->uData.pString->SubString(pValue->uData.pString);
			break;
		}
		default: assert(0);
	}
	if(Execute) ProgramTraceVar(&pCntx->Program, pVal);
	return pVal;
}

/////////////////////////////////////////////////////////////////////////////

double ValueTruncate(double d)
{
	return (d < 0.0 ? ceil(d) : floor(d));
}

/////////////////////////////////////////////////////////////////////////////

double ValueRound(double d)
{
	return (d < 0.0 ? ceil(d - 0.5) : floor(d + 0.5));
}

/////////////////////////////////////////////////////////////////////////////

long int ValueToI(double d, int *overflow)
{
	d = ValueRound(d);
	*overflow = (d < LONG_MIN || d > LONG_MAX);
	return lrint(d);
}

/////////////////////////////////////////////////////////////////////////////

long int ValueStringToInteger(const char *pStr, char **ppEnd, int *pOverflow)
{
long int n;

	errno = 0;
	if((*pStr == '&') && (tolower((int)*(pStr + 1)) == 'h')) n = strtoul(pStr + 2, ppEnd, 16);
	else if((*pStr == '&') && (tolower((int)*(pStr + 1)) == 'o')) n = strtoul(pStr + 2, ppEnd, 8);
	else	n = strtoul(pStr, ppEnd, 10);
	*pOverflow = (errno == ERANGE);
	return n;
}

/////////////////////////////////////////////////////////////////////////////

double ValueStringToReal(const char *pStr, char **ppEnd, int *pOverflow)
{
double d;

	errno = 0;
	d = strtod(pStr, ppEnd);
	*pOverflow = (errno == ERANGE);
	return d;
}

/////////////////////////////////////////////////////////////////////////////

pValue_s ValuePlus(pValue_s pVal, int Calc)
{
	switch(pVal->Type){
		case eVType::V_INT:
		case eVType::V_REAL:{
			break;
		}
		case eVType::V_STRING:{
			ValueDestroy(pVal);
			ValueNewError(pVal, INVALIDUOPERAND);
			break;
		}
		default: assert(0);
	}
	return pVal;
}

/////////////////////////////////////////////////////////////////////////////

pValue_s ValueNegate(pValue_s pVal, int Calc)
{
	switch(pVal->Type){
		case eVType::V_INT:{
			if(Calc) pVal->uData.Integer = -pVal->uData.Integer;
			break;
		}
		case eVType::V_REAL:{
			if(Calc) pVal->uData.Real = -pVal->uData.Real;
			break;
		}
		case eVType::V_STRING:{
			ValueDestroy(pVal);
			ValueNewError(pVal, INVALIDUOPERAND);
			break;
		}
		default: assert(0);
	}
	return pVal;
}

/////////////////////////////////////////////////////////////////////////////

pValue_s ValueNot(pValue_s pVal, int Calc)
{
	switch(pVal->Type){
		case eVType::V_REAL: ValueRetype(pVal, eVType::V_INT);
		case eVType::V_INT:{
			if(Calc) pVal->uData.Integer = (pVal->uData.Integer == 0) ? 1 : 0;
			break;
		}
		case eVType::V_STRING:{
			ValueDestroy(pVal);
			ValueNewError(pVal, INVALIDUOPERAND);
			break;
		}
		default: assert(0);
	}
	return pVal;
}

/////////////////////////////////////////////////////////////////////////////

pValue_s ValueAdd(pValue_s pVal, pValue_s pValue, int Calc)
{
	switch(ValueCommonType[(int)pVal->Type][(int)pValue->Type]){
		case eVType::V_INT:{
			DoValueRetype(pVal, eVType::V_INT);
			DoValueRetype(pValue, eVType::V_INT);
			if(Calc) pVal->uData.Integer += pValue->uData.Integer;
			break;
		}
		case eVType::V_REAL:{
			DoValueRetype(pVal, eVType::V_REAL);
			DoValueRetype(pValue, eVType::V_REAL);
			if(Calc) pVal->uData.Real += pValue->uData.Real;
			break;
		}
		case eVType::V_STRING:{
			if(Calc) pVal->uData.pString->AppendString(pValue->uData.pString);
			break;
		}
		default: assert(0);
	}
	return pVal;
}

/////////////////////////////////////////////////////////////////////////////

pValue_s ValueSub(pValue_s pVal, pValue_s pValue, int Calc)
{
	switch(ValueCommonType[(int)pVal->Type][(int)pValue->Type]){
		case eVType::V_INT:{
			DoValueRetype(pVal, eVType::V_INT);
			DoValueRetype(pValue, eVType::V_INT);
			if(Calc) pVal->uData.Integer -= pValue->uData.Integer;
			break;
		}
		case eVType::V_REAL:{
			DoValueRetype(pVal, eVType::V_REAL);
			DoValueRetype(pValue, eVType::V_REAL);
			if(Calc) pVal->uData.Real -= pValue->uData.Real;
			break;
		}
		case eVType::V_STRING:{
			ValueDestroy(pVal);
			ValueNewError(pVal, INVALIDOPERAND);
			break;
		}
		default: assert(0);
	}
	return pVal;
}

/////////////////////////////////////////////////////////////////////////////

pValue_s ValueMultiply(pValue_s pVal, pValue_s pValue, int Calc)
{
	switch(ValueCommonType[(int)pVal->Type][(int)pValue->Type]){
		case eVType::V_INT:{
			DoValueRetype(pVal, eVType::V_INT);
			DoValueRetype(pValue, eVType::V_INT);
			if(Calc) pVal->uData.Integer *= pValue->uData.Integer;
			break;
		}
		case eVType::V_REAL:{
			DoValueRetype(pVal, eVType::V_REAL);
			DoValueRetype(pValue, eVType::V_REAL);
			if(Calc) pVal->uData.Real *= pValue->uData.Real;
			break;
		}
		case eVType::V_STRING:{
			ValueDestroy(pVal);
			ValueNewError(pVal, INVALIDOPERAND);
			break;
		}
		default: assert(0);
	}
	return pVal;
}

/////////////////////////////////////////////////////////////////////////////

pValue_s ValueDiv(pValue_s pVal, pValue_s pValue, int Calc)
{
	switch(ValueCommonType[(int)pVal->Type][(int)pValue->Type]){
		case eVType::V_INT:
		case eVType::V_REAL:{
			ldiv_t Result;
			DoValueRetype(pVal, eVType::V_INT);
			DoValueRetype(pValue, eVType::V_INT);
			if(Calc){
				if(pValue->uData.Real == 0.0){
					ValueDestroy(pVal);
					ValueNewError(pVal, UNDEFINED, _T("Division by zero"));
				}
				else{
					Result = ldiv(pVal->uData.Integer,	pValue->uData.Integer);
					pVal->uData.Integer = Result.quot;
				}
			}
			break;
		}
		case eVType::V_STRING:{
			ValueDestroy(pVal);
			ValueNewError(pVal, INVALIDOPERAND);
			break;
		}
		default: assert(0);
	}
	return pVal;
}

/////////////////////////////////////////////////////////////////////////////

pValue_s ValueDivide(pValue_s pVal, pValue_s pValue, int Calc)
{
	switch(ValueCommonType[(int)pVal->Type][(int)pValue->Type]){
		case eVType::V_INT:{
			DoValueRetype(pVal, eVType::V_REAL);
			DoValueRetype(pValue, eVType::V_REAL);
			if(Calc){
				if(pValue->uData.Real == 0){
					ValueDestroy(pVal);
					ValueNewError(pVal, UNDEFINED, _T("Division by zero"));
				}
				else pVal->uData.Real /= pValue->uData.Real;
			}
			break;
		}
		case eVType::V_REAL:{
			DoValueRetype(pVal, eVType::V_REAL);
			DoValueRetype(pValue, eVType::V_REAL);
			if(Calc){
				if(pValue->uData.Real == 0.0){
					ValueDestroy(pVal);
					ValueNewError(pVal, UNDEFINED, _T("Division by zero"));
				}
				else pVal->uData.Real /= pValue->uData.Real;
			}
			break;
		}
		case eVType::V_STRING:{
			ValueDestroy(pVal);
			ValueNewError(pVal, INVALIDOPERAND);
			break;
		}
		default: assert(0);
	}
	return pVal;
}

/////////////////////////////////////////////////////////////////////////////

pValue_s ValueDivideI(pValue_s pVal, pValue_s pValue, int Calc)
{
	switch(ValueCommonType[(int)pVal->Type][(int)pValue->Type]){
		case eVType::V_INT:{
			DoValueRetype(pVal, eVType::V_INT);
			DoValueRetype(pValue, eVType::V_INT);
			if(Calc){
				if(pValue->uData.Integer == 0){
					ValueDestroy(pVal);
					ValueNewError(pVal, UNDEFINED, _T("Division by zero"));
				}
				else pVal->uData.Integer /= pValue->uData.Integer;
			}
			break;
		}
		case eVType::V_REAL:{
			DoValueRetype(pVal, eVType::V_REAL);
			DoValueRetype(pValue, eVType::V_REAL);
			if(Calc){
				if(pValue->uData.Real == 0.0){
					ValueDestroy(pVal);
					ValueNewError(pVal, UNDEFINED, _T("Division by zero"));
				}
				else pVal->uData.Real = ValueTruncate(pVal->uData.Real / pValue->uData.Real);
			}
			break;
		}
		case eVType::V_STRING:{
			ValueDestroy(pVal);
			ValueNewError(pVal, INVALIDOPERAND);
			break;
		}
		default: assert(0);
	}
	return pVal;
}

/////////////////////////////////////////////////////////////////////////////

pValue_s ValueMod(pValue_s pVal, pValue_s pValue, int Calc)
{
	switch(ValueCommonType[(int)pVal->Type][(int)pValue->Type]){
		case eVType::V_INT:{
			DoValueRetype(pVal, eVType::V_INT);
			DoValueRetype(pValue, eVType::V_INT);
			if(Calc){
				if(pValue->uData.Integer == 0){
					ValueDestroy(pVal);
					ValueNewError(pVal, UNDEFINED, _T("Modulo by zero"));
				}
				else{
					double X = (double)pVal->uData.Integer, Y = (double)pValue->uData.Integer;
					pVal->uData.Integer = (int)(X - (Y * floor(X / Y)));
				}
			}
			break;
		}
		case eVType::V_REAL:{
			DoValueRetype(pVal, eVType::V_REAL);
			DoValueRetype(pValue, eVType::V_REAL);
			if(Calc){
				if(pValue->uData.Real == 0.0){
					ValueDestroy(pVal);
					ValueNewError(pVal, UNDEFINED, _T("Modulo by zero"));
				}
				else{
					double X = pVal->uData.Real, Y = pValue->uData.Real;
					pVal->uData.Real = X - (Y * floor(X / Y));
				}
			}
			break;
		}
		case eVType::V_STRING:{
			ValueDestroy(pVal);
			ValueNewError(pVal, INVALIDOPERAND);
			break;
		}
		default: assert(0);
	}
	return pVal;
}

/////////////////////////////////////////////////////////////////////////////

pValue_s ValuePow(pValue_s pVal, pValue_s pValue, int Calc)
{
	switch(ValueCommonType[(int)pVal->Type][(int)pValue->Type]){
		case eVType::V_INT:{
			DoValueRetype(pVal, eVType::V_INT);
			DoValueRetype(pValue, eVType::V_INT);
			if(Calc){
				if(pVal->uData.Integer == 0 && pValue->uData.Integer <= 0){
					ValueDestroy(pVal);
					ValueNewError(pVal, ZERONEGPOWER);
				}
				else if(pValue->uData.Integer > 0) pVal->uData.Integer = (long)pow(pVal->uData.Integer, pValue->uData.Integer);
				else{
					long int thisi = pVal->uData.Integer;
					ValueDestroy(pVal);
					ValueNewReal(pVal, pow(thisi, pValue->uData.Integer));
				}
			}
			break;
		}
		case eVType::V_REAL:{
			DoValueRetype(pVal, eVType::V_REAL);
			DoValueRetype(pValue, eVType::V_REAL);
			if(Calc){
				if(pVal->uData.Real == 0.0 && pValue->uData.Real <= 0.0){
					ValueDestroy(pVal);
					ValueNewError(pVal, ZERONEGPOWER);
				}
				else pVal->uData.Real = pow(pVal->uData.Real, pValue->uData.Real);
			}
			break;
		}
		case eVType::V_STRING:{
			ValueDestroy(pVal);
			ValueNewError(pVal, INVALIDOPERAND);
			break;
		}
		default: assert(0);
	}
	return pVal;
}

/////////////////////////////////////////////////////////////////////////////

pValue_s ValueAnd(pValue_s pVal, pValue_s pValue, int Calc)
{
	switch(ValueCommonType[(int)pVal->Type][(int)pValue->Type]){
		case eVType::V_INT:
		case eVType::V_REAL:{
			DoValueRetype(pVal, eVType::V_INT);
			DoValueRetype(pValue, eVType::V_INT);
			if(Calc) pVal->uData.Integer &= pValue->uData.Integer;
			break;
		}
		case eVType::V_STRING:{
			ValueDestroy(pVal);
			ValueNewError(pVal, INVALIDOPERAND);
			break;
		}
		default: assert(0);
	}
	return pVal;
}

/////////////////////////////////////////////////////////////////////////////

pValue_s ValueAmpersand(pValue_s pVal, pValue_s pValue, int Calc)
{
	switch(ValueCommonType[(int)pVal->Type][(int)pValue->Type]){
		case eVType::V_INT:
		case eVType::V_REAL:{
			ValueDestroy(pVal);
			ValueNewError(pVal, INVALIDOPERAND);
			break;
		}
		case eVType::V_STRING:{
			if(Calc) pVal->uData.pString->AppendString(pValue->uData.pString);
			break;
		}
		default: assert(0);
	}
	return pVal;
}

/////////////////////////////////////////////////////////////////////////////

pValue_s ValueOr(pValue_s pVal, pValue_s pValue, int Calc)
{
	switch(ValueCommonType[(int)pVal->Type][(int)pValue->Type]){
		case eVType::V_INT:
		case eVType::V_REAL:{
			DoValueRetype(pVal, eVType::V_INT);
			DoValueRetype(pValue, eVType::V_INT);
			if(Calc) pVal->uData.Integer |= pValue->uData.Integer;
			break;
		}
		case eVType::V_STRING:{
			ValueDestroy(pVal);
			ValueNewError(pVal, INVALIDOPERAND);
			break;
		}
		default: assert(0);
	}
	return pVal;
}

/////////////////////////////////////////////////////////////////////////////

pValue_s ValueXor(pValue_s pVal, pValue_s pValue, int Calc)
{
	switch(ValueCommonType[(int)pVal->Type][(int)pValue->Type]){
		case eVType::V_INT:
		case eVType::V_REAL:{
			DoValueRetype(pVal, eVType::V_INT);
			DoValueRetype(pValue, eVType::V_INT);
			if(Calc) pVal->uData.Integer ^= pValue->uData.Integer;
			break;
		}
		case eVType::V_STRING:{
			ValueDestroy(pVal);
			ValueNewError(pVal, INVALIDOPERAND);
			break;
		}
		default: assert(0);
	}
	return pVal;
}

/////////////////////////////////////////////////////////////////////////////

pValue_s ValueEqv(pValue_s pVal, pValue_s pValue, int Calc)
{
	switch(ValueCommonType[(int)pVal->Type][(int)pValue->Type]){
		case eVType::V_INT:
		case eVType::V_REAL:{
			DoValueRetype(pVal, eVType::V_INT);
			DoValueRetype(pValue, eVType::V_INT);
			if(Calc) pVal->uData.Integer = ~(pVal->uData.Integer ^ pValue->uData.Integer);
			break;
		}
		case eVType::V_STRING:{
			ValueDestroy(pVal);
			ValueNewError(pVal, INVALIDOPERAND);
			break;
		}
		default: assert(0);
	}
	return pVal;
}

/////////////////////////////////////////////////////////////////////////////

pValue_s ValueImp(pValue_s pVal, pValue_s pValue, int Calc)
{
	switch(ValueCommonType[(int)pVal->Type][(int)pValue->Type]){
		case eVType::V_INT:
		case eVType::V_REAL:{
			DoValueRetype(pVal, eVType::V_INT);
			DoValueRetype(pValue, eVType::V_INT);
			if(Calc) pVal->uData.Integer = (~pVal->uData.Integer) | pValue->uData.Integer;
			break;
		}
		case eVType::V_STRING:{
			ValueDestroy(pVal);
			ValueNewError(pVal, INVALIDOPERAND);
			break;
		}
		default: assert(0);
	}
	return pVal;
}

/////////////////////////////////////////////////////////////////////////////

pValue_s ValueLt(pValue_s pVal, pValue_s pValue, int Calc)
{
	switch(ValueCommonType[(int)pVal->Type][(int)pValue->Type]){
		case eVType::V_INT:{
			DoValueRetype(pVal, eVType::V_INT);
			DoValueRetype(pValue, eVType::V_INT);
			if(Calc) pVal->uData.Integer = (pVal->uData.Integer < pValue->uData.Integer) ? TRUE : FALSE;
			break;
		}
		case eVType::V_REAL:{
			int v;

			DoValueRetype(pVal, eVType::V_REAL);
			DoValueRetype(pValue, eVType::V_REAL);
			if(Calc) v = (pVal->uData.Real < pValue->uData.Real) ? TRUE : FALSE;
			else v = 0;
			ValueDestroy(pVal);
			ValueNewInteger(pVal, v);
			break;
		}
		case eVType::V_STRING:{
			int v;
			if(Calc) v = (pVal->uData.pString->Compare(pValue->uData.pString) < 0) ? TRUE : FALSE;
			else v = 0;
			ValueDestroy(pVal);
			ValueNewInteger(pVal, v);
			break;
		}
		default: assert(0);
	}
	return pVal;
}

/////////////////////////////////////////////////////////////////////////////

pValue_s ValueLe(pValue_s pVal, pValue_s pValue, int Calc)
{
	switch(ValueCommonType[(int)pVal->Type][(int)pValue->Type]){
		case eVType::V_INT:{
			DoValueRetype(pVal, eVType::V_INT);
			DoValueRetype(pValue, eVType::V_INT);
			if(Calc) pVal->uData.Integer = (pVal->uData.Integer <= pValue->uData.Integer) ? TRUE : FALSE;
			break;
		}
		case eVType::V_REAL:{
			int v;
			DoValueRetype(pVal, eVType::V_REAL);
			DoValueRetype(pValue, eVType::V_REAL);
			if(Calc) v = (pVal->uData.Real <= pValue->uData.Real) ? TRUE : FALSE;
			else v = 0;
			ValueDestroy(pVal);
			ValueNewInteger(pVal, v);
			break;
		}
		case eVType::V_STRING:{
			int v;
			if(Calc) v = (pVal->uData.pString->Compare(pValue->uData.pString) <= 0) ? TRUE : FALSE;
			else v = 0;
			ValueDestroy(pVal);
			ValueNewInteger(pVal, v);
			break;
		}
		default: assert(0);
	}
	return pVal;
}

/////////////////////////////////////////////////////////////////////////////

pValue_s ValueEqu(pValue_s pVal, pValue_s pValue, int Calc)
{
	switch(ValueCommonType[(int)pVal->Type][(int)pValue->Type]){
		case eVType::V_INT:{
			DoValueRetype(pVal, eVType::V_INT);
			DoValueRetype(pValue, eVType::V_INT);
			if(Calc) pVal->uData.Integer = (pVal->uData.Integer == pValue->uData.Integer) ? TRUE : FALSE;
			break;
		}
		case eVType::V_REAL:{
			int v;
			DoValueRetype(pVal, eVType::V_REAL);
			DoValueRetype(pValue, eVType::V_REAL);
			if(Calc) v = (pVal->uData.Real == pValue->uData.Real) ? TRUE : FALSE;
			else v = 0;
			ValueDestroy(pVal);
			ValueNewInteger(pVal, v);
			break;
		}
		case eVType::V_STRING:{
			int v;
			if(Calc) v = (pVal->uData.pString->Compare(pValue->uData.pString) == 0) ? TRUE : FALSE;
			else v = 0;
			ValueDestroy(pVal);
			ValueNewInteger(pVal, v);
			break;
		}
		default: assert(0);
	}
	return pVal;
}

/////////////////////////////////////////////////////////////////////////////

pValue_s ValueGe(pValue_s pVal, pValue_s pValue, int Calc)
{
	switch(ValueCommonType[(int)pVal->Type][(int)pValue->Type]){
		case eVType::V_INT:{
			DoValueRetype(pVal, eVType::V_INT);
			DoValueRetype(pValue, eVType::V_INT);
			if(Calc) pVal->uData.Integer = (pVal->uData.Integer >= pValue->uData.Integer) ? TRUE : FALSE;
			break;
		}
		case eVType::V_REAL:{
			int v;
			DoValueRetype(pVal, eVType::V_REAL);
			DoValueRetype(pValue, eVType::V_REAL);
			if(Calc) v = (pVal->uData.Real >= pValue->uData.Real) ? TRUE : FALSE;
			else v = 0;
			ValueDestroy(pVal);
			ValueNewInteger(pVal, v);
			break;
		}
		case eVType::V_STRING:{
			int v;
			if(Calc) v = (pVal->uData.pString->Compare(pValue->uData.pString) >= 0) ? TRUE : FALSE;
			else v = 0;
			ValueDestroy(pVal);
			ValueNewInteger(pVal, v);
			break;
		}
		default: assert(0);
	}
	return pVal;
}

/////////////////////////////////////////////////////////////////////////////

pValue_s ValueGt(pValue_s pVal, pValue_s pValue, int Calc)
{
	switch(ValueCommonType[(int)pVal->Type][(int)pValue->Type]){
		case eVType::V_INT:{
			DoValueRetype(pVal, eVType::V_INT);
			DoValueRetype(pValue, eVType::V_INT);
			if(Calc) pVal->uData.Integer = (pVal->uData.Integer>pValue->uData.Integer) ? TRUE : FALSE;
			break;
		}
		case eVType::V_REAL:{
			int v;
			DoValueRetype(pVal, eVType::V_REAL);
			DoValueRetype(pValue, eVType::V_REAL);
			if(Calc) v = (pVal->uData.Real>pValue->uData.Real) ? TRUE : FALSE;
			else v = 0;
			ValueDestroy(pVal);
			ValueNewInteger(pVal, v);
			break;
		}
		case eVType::V_STRING:{
			int v;
			if(Calc) v = (pVal->uData.pString->Compare(pValue->uData.pString) > 0) ? TRUE : FALSE;
			else v = 0;
			ValueDestroy(pVal);
			ValueNewInteger(pVal, v);
			break;
		}
		default: assert(0);
	}
	return pVal;
}

/////////////////////////////////////////////////////////////////////////////

pValue_s ValueNe(pValue_s pVal, pValue_s pValue, int Calc)
{
	switch(ValueCommonType[(int)pVal->Type][(int)pValue->Type]){
		case eVType::V_INT:{
			DoValueRetype(pVal, eVType::V_INT);
			DoValueRetype(pValue, eVType::V_INT);
			if(Calc) pVal->uData.Integer = (pVal->uData.Integer != pValue->uData.Integer) ? TRUE : FALSE;
			break;
		}
		case eVType::V_REAL:{
			int v;
			DoValueRetype(pVal, eVType::V_REAL);
			DoValueRetype(pValue, eVType::V_REAL);
			if(Calc) v = (pVal->uData.Real != pValue->uData.Real) ? TRUE : FALSE;
			else v = 0;
			ValueDestroy(pVal);
			ValueNewInteger(pVal, v);
			break;
		}
		case eVType::V_STRING:{
			int v;
			if(Calc) v = pVal->uData.pString->Compare(pValue->uData.pString) ? TRUE : FALSE;
			else v = 0;
			ValueDestroy(pVal);
			ValueNewInteger(pVal, v);
			break;
		}
		default: assert(0);
	}
	return pVal;
}

/////////////////////////////////////////////////////////////////////////////

int ValueExitFor(pValue_s pVal, pValue_s limit, pValue_s step)
{
	switch(pVal->Type){
		case eVType::V_INT: return (step->uData.Integer < 0 ? (pVal->uData.Integer < limit->uData.Integer) : (pVal->uData.Integer>limit->uData.Integer));
		case eVType::V_REAL:    return (step->uData.Real < 0.0 ? (pVal->uData.Real < limit->uData.Real) : (pVal->uData.Real>limit->uData.Real));
		case eVType::V_STRING:  return (pVal->uData.pString->Compare(limit->uData.pString) > 0);
		default:        assert(0);
	}
	return -1;
}

/////////////////////////////////////////////////////////////////////////////

void ValueErrorPrefix(pValue_s pVal, const char *pPrefix)
{
size_t prefixlen, msglen;

	assert(pVal->Type == eVType::V_ERROR);
	prefixlen = strlen(pPrefix);
	msglen = strlen(pVal->uData.Error.pStr);
	pVal->uData.Error.pStr = (char*)realloc(pVal->uData.Error.pStr, (prefixlen + msglen + 1) * 2);
	memmove(pVal->uData.Error.pStr + prefixlen, pVal->uData.Error.pStr, msglen);
	memcpy(pVal->uData.Error.pStr, pPrefix, prefixlen);
}

/////////////////////////////////////////////////////////////////////////////

void ValueErrorSuffix(pValue_s pVal, const char *pSuffix)
{
size_t suffixlen, msglen;

	assert(pVal->Type == eVType::V_ERROR);
	suffixlen = strlen(pSuffix);
	msglen = strlen(pVal->uData.Error.pStr);
	pVal->uData.Error.pStr = (char*)realloc(pVal->uData.Error.pStr, (suffixlen + msglen + 1) * 2);
	memcpy(pVal->uData.Error.pStr + msglen, pSuffix, suffixlen + 1);
}

/////////////////////////////////////////////////////////////////////////////

pValue_s ValueNewTypeError(pValue_s pVal, enum eVType t1, enum eVType t2)
{
	assert(typestr[(int)t1]);
	assert(typestr[(int)t2]);
	return ValueNewError(pVal, TYPEMISMATCH1, typestr[(int)t1], typestr[(int)t2]);
}

/////////////////////////////////////////////////////////////////////////////

static void RetypeError(pValue_s pVal, enum eVType ToType)
{
enum eVType ThisType = pVal->Type;

	assert(typestr[(int)ThisType]);
	assert(typestr[(int)ToType]);
	ValueDestroy(pVal);
	ValueNewError(pVal, TYPEMISMATCH1, typestr[(int)ThisType], typestr[(int)ToType]);
}

/////////////////////////////////////////////////////////////////////////////

pValue_s ValueRetype(pValue_s pVal, enum eVType type)
{
	switch(pVal->Type){
		case eVType::V_INT:{
			switch(type){
				case eVType::V_INT: break;
				case eVType::V_REAL: pVal->uData.Real = pVal->uData.Integer; pVal->Type = type; break;
				case eVType::V_VOID: ValueDestroy(pVal); ValueNewVoid(pVal); break;
				default: RetypeError(pVal, type); break;
			}
			break;
		}
		case eVType::V_REAL:{
			int overflow;
			switch(type){
				case eVType::V_INT:{
					pVal->uData.Integer = ValueToI(pVal->uData.Real, &overflow);
					pVal->Type = eVType::V_INT;
					if(overflow){
						ValueDestroy(pVal);
						ValueNewError(pVal, INTEGEROVERFLOW, typestr[(int)eVType::V_INT]);
					}
					break;
				}
				case eVType::V_REAL:
					break;
				case eVType::V_VOID:
					ValueDestroy(pVal);
					ValueNewVoid(pVal);
					break;
				default:
					RetypeError(pVal, type);
					break;
			}
			break;
		}
		case eVType::V_STRING:{
			switch(type){
				case eVType::V_STRING:
					break;
				case eVType::V_VOID:
					ValueDestroy(pVal);
					ValueNewVoid(pVal);
					break;
				default:
					RetypeError(pVal, type);
					break;
			}
			break;
		}
		case eVType::V_VOID:{
			switch(type){
				case eVType::V_VOID:
					break;
				default:
					RetypeError(pVal, type);
			}
			break;
		}
		case eVType::V_ARRAY:{
			switch(type){
				case eVType::V_INT:
				case eVType::V_REAL:
					break;
				case eVType::V_VOID:
				default:
					RetypeError(pVal, type);
					break;
			}
			break;
		}
		case eVType::V_ERROR:
			break;
		default:
			assert(0);
	}
	return pVal;
}

/////////////////////////////////////////////////////////////////////////////

BString *ValueToString(pValue_s pVal, BString *pStr, char Pad, int HeadingSign, UINT Width, int Commas, int Precision, int Exponent, int TrailingSign)
{
size_t oldlength = pStr->GetLength();

	switch(pVal->Type){
		case eVType::V_ERROR: pStr->AppendChars(pVal->uData.Error.pStr); break;
		case eVType::V_REAL:
		case eVType::V_INT:{
			int sign;
			BString TmpStr;
			UINT TotalWidth = Width;
			if(pVal->Type == eVType::V_INT){
				if(pVal->uData.Integer < 0){
					sign = -1;
					pVal->uData.Integer = -pVal->uData.Integer;
				}
				else if(pVal->uData.Integer == 0) sign = 0;
				else sign = 1;
			}
			else{
				if(pVal->uData.Real < 0.0){
					sign = -1;
					pVal->uData.Real = -pVal->uData.Real;
				}
				else if(pVal->uData.Real == 0.0) sign = 0;
				else sign = 1;
			}
			switch(HeadingSign){
				case -1:{
					++TotalWidth;
					TmpStr.AppendChar(sign == -1 ? '-' : ' ');
					break;
				}
				case 0:{
					if(sign == -1) TmpStr.AppendChar('-');
					break;
				}
				case 1:{
					++TotalWidth;
					TmpStr.AppendChar(sign == -1 ? '-' : '+');
					break;
				}
				case 2:
					break;
				default:
					assert(0);
			}
			TotalWidth += Exponent;
			if(pVal->Type == eVType::V_INT){
				if(Precision > 0 || Exponent) TmpStr.FormatDouble((double)pVal->uData.Integer, Width, Precision, Exponent);
				else if(Precision == 0) TmpStr.AppendPrintf(_T("%lu."), pVal->uData.Integer);
				else TmpStr.AppendPrintf(_T("%lu"), pVal->uData.Integer);
			}
			else TmpStr.FormatDouble(pVal->uData.Real, Width, Precision, Exponent);
			if(Commas){
				size_t digits;
				size_t first;
				first = (HeadingSign) ? 1 : 0;
				for(digits = first; digits < TmpStr.GetLength() && TmpStr.GetAt(digits) >= '0' && TmpStr.GetAt(digits) <= '9'; ++digits);
				while(digits > first + 3){
					digits -= 3;
					TmpStr.Insert(digits, ',');
				}
			}
			if(TrailingSign == -1){
				++TotalWidth;
				TmpStr.AppendChar(sign == -1 ? '-' : ' ');
			}
			else if(TrailingSign == 1){
				++TotalWidth;
				TmpStr.AppendChar(sign == -1 ? '-' : '+');
			}
			UINT Len = (TotalWidth > TmpStr.GetLength()) ? TotalWidth : TmpStr.GetLength();
			pStr->Resize(oldlength + Len);
			if(TotalWidth > TmpStr.GetLength()) memset(pStr->GetBuffer() + oldlength, Pad, TotalWidth - TmpStr.GetLength());
			memcpy(pStr->GetBuffer() + oldlength + (TotalWidth > TmpStr.GetLength() ? (TotalWidth - TmpStr.GetLength()) : 0), TmpStr.GetBuffer(), TmpStr.GetLength());
			pStr->SetLength(oldlength + Len);
			break;
		}
		case eVType::V_STRING:{
			if(Width > 0){
				UINT Len = oldlength + Width;
				size_t blanks = (pVal->uData.pString->GetLength() < Width ? (Width - pVal->uData.pString->GetLength()) : 0);
				pStr->Resize(Len);
				memcpy(pStr->GetBuffer() + oldlength, pVal->uData.pString->GetBuffer(), blanks ? pVal->uData.pString->GetLength() : Width);
				if(blanks) memset(pStr->GetBuffer() + oldlength + pVal->uData.pString->GetLength(), ' ', blanks);
				pStr->SetLength(Len);
			}
			else pStr->AppendString(pVal->uData.pString);
			break;
		}
		default:
			assert(0);
			return 0;
	}
	return pStr;
}

/////////////////////////////////////////////////////////////////////////////

pValue_s ValueToStringUsing(pValue_s pVal, BString *pDestStr, BString *pUsingStr, UINT *pUsingPos)
{
char Pad = ' ';
int HeadingSign;
int Width = 0;
int Commas = 0;
int Precision = -1;
int Exponent = 0;
int TrailingSign = 0;

	HeadingSign = (pUsingStr->GetLength()) ? 0 : -1;
	if(*pUsingPos == pUsingStr->GetLength()) *pUsingPos = 0;
	ValueToString(pVal, pDestStr, Pad, HeadingSign, Width, Commas, Precision, Exponent, TrailingSign);
	if((pVal->Type == eVType::V_INT || pVal->Type == eVType::V_REAL) && Width == 0 && Precision == -1) pDestStr->AppendChar(' ');
	while(*pUsingPos < pUsingStr->GetLength()){
		switch(pUsingStr->GetAt(*pUsingPos)){
			case '_':{ /* output next char */
				++(*pUsingPos);
				if(*pUsingPos < pUsingStr->GetLength()) pDestStr->AppendChar(pUsingStr->GetAt((*pUsingPos)++));
				else{
					ValueDestroy(pVal);
					return ValueNewError(pVal, MISSINGCHARACTER);
				}
				break;
			}
			case '!':
			case '\\':
			case '&':
			case '*':
			case '0':
			case '+':
			case '#':
			case '.':
				return pVal;
			default:{
				pDestStr->AppendChar(pUsingStr->GetAt((*pUsingPos)++));
			}
		}
	}
	return pVal;
}

//////////////////////////////////////////////////////////////////////

pValue_s  ValueArrayToStringUsing(pValue_s pValue, BString *pPrintStr, BString *pUsingStr, UINT *pUsingPos)
{
Var_s	*pVar = pValue->uData.pArray;
int j = 0;
UINT UsingPos = 0;

	if((pVar->Dimensions < 1)) return ValueNewError(pValue, NOMATRIX, pVar->Dimensions);
	int RowSize = pVar->pGeometry[pVar->Dimensions - 1].Size;
	do{
		if(ValueToStringUsing(&pVar->pValue[j++], pPrintStr, pUsingStr, &UsingPos)->Type == eVType::V_ERROR)	return ValueNewError(pValue, MISSINGCHARACTER);
		if((j % RowSize) == 0) pPrintStr->AppendChar('\n');
		else pPrintStr->AppendChars("  "); 
	}
	while(j < pVar->Size);
	return pValue;
}

/////////////////////////////////////////////////////////////////////////////

BString *ValueToWrite(pValue_s pVal, BString *pStr)
{
	switch(pVal->Type){
		case eVType::V_INT: pStr->AppendPrintf(_T("%ld"), pVal->uData.Integer); break;
		case eVType::V_REAL:{
			double pValue;
			int p = DBL_DIG;
			int n, o;

			pValue = (pVal->uData.Real < 0.0) ? -pVal->uData.Real : pVal->uData.Real;
			while((pValue > 1.0) && (p > 0)){
				pValue /= 10.0; --p;
			}
			o = pStr->GetLength();
			pStr->AppendPrintf(_T("%.*f"), p, pVal->uData.Real);
			n = pStr->GetLength();
			if(memchr(pStr->GetBuffer() + o, '.', n - o)){
				while(pStr->GetAt(pStr->GetLength() - 1) == '0') pStr->Truncate();
				if(pStr->GetAt(pStr->GetLength() - 1) == '.') pStr->Truncate();
			}
			break;
		}
		case eVType::V_STRING:{
			size_t l = pVal->uData.pString->GetLength();
			char *data = pVal->uData.pString->GetBuffer();

			pStr->AppendChar('"');
			while(l--){
				if(*data == '"') pStr->AppendChar('"');
				pStr->AppendChar(*data);
				++data;
			}
			pStr->AppendChar('"');
			break;
		}
		default: assert(0);
	}
	return pStr;
}

/////////////////////////////////////////////////////////////////////////////

pValue_s ValueNullValue(enum eVType Type)
{
static Value_s integer = { eVType::V_INT };
static Value_s real = { eVType::V_REAL };
static Value_s string = { eVType::V_STRING };
static int init = 0;

	if(!init){
		integer.uData.Integer = 0;
		real.uData.Real = 0.0;
		string.uData.pString = &NullStr;
		init = 1;
	}
	switch(Type){
		case eVType::V_INT: return &integer;
		case eVType::V_REAL: return &real;
		case eVType::V_STRING: return &string;
		default: assert(0);
	}
	return NULL;
}

/////////////////////////////////////////////////////////////////////////////


