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


#include "BasicDefs.h"

double ValueTruncate(double d);

double ValueRound(double d);

long int ValueToI(double d, int *overflow);

long int ValueStringToInteger(const char *s, char **end, int *overflow);

double ValueStringToReal(const char *s, char **end, int *overflow);

extern inline pValue_s  NewIntegerValue(pValue_s pVal, long int n);
extern inline pValue_s  NewRealValue(pValue_s pVal, double n);
extern inline pValue_s  NewArrayValue(pValue_s pVal, long int n);
extern inline pValue_s  DoValueRetype(pValue_s pVal, eVType Type);

pValue_s	ValueInit(pValue_s pVal, eVType Type);
void			ValueDestroy(pValue_s pVal);
int				ValueIsNull(const pValue_s pVal);
pValue_s	ValueNewNil(pValue_s pVal);
pValue_s	ValueNewError(pValue_s pVal, int Code, int Num, const char *pError, ...);
pValue_s	ValueNewInteger(pValue_s pVal, int n);
pValue_s	ValueNewReal(pValue_s pVal, double n);
pValue_s	ValueNewString(pValue_s pVal);
pValue_s	ValueNewVoid(pValue_s pVal);
pValue_s	ValueNewNull(pValue_s pVal, eVType ValueType);
void			ValueErrorPrefix(pValue_s pVal, const char *pPrefix);
void			ValueErrorSuffix(pValue_s pVal, const char *pSuffix);
pValue_s	ValueNewTypeError(pValue_s pVal, enum eVType ValueType1, eVType ValueType2);
pValue_s	ValueRetype(pValue_s pVal, enum eVType ValueType);
pValue_s	ValueAssign(pValue_s pVal, pValue_s pValue, int Execute);
pValue_s	ValueClone(pValue_s pVal, const pValue_s pOriginal);
pValue_s	ValuePlus(pValue_s pVal, int Calc);
pValue_s	ValueNegate(pValue_s pVal, int Calc);
pValue_s	ValueNot(pValue_s pVal, int Calc);
pValue_s	ValueAdd(pValue_s pVal, pValue_s pValue, int Calc);
pValue_s	ValueSub(pValue_s pVal, pValue_s pValue, int Calc);
pValue_s	ValueMultiply(pValue_s pVal, pValue_s pValue, int Calc);
pValue_s	ValueDiv(pValue_s pVal, pValue_s pValue, int Calc);
pValue_s	ValueDivide(pValue_s pVal, pValue_s pValue, int Calc);
pValue_s	ValueDivideI(pValue_s pVal, pValue_s pValue, int Calc);
pValue_s	ValueMod(pValue_s pVal, pValue_s pValue, int Calc);
pValue_s	ValuePow(pValue_s pVal, pValue_s pValue, int Calc);
pValue_s	ValueAnd(pValue_s pVal, pValue_s pValue, int Calc);
pValue_s	ValueAmpersand(pValue_s pVal, pValue_s pValue, int Calc);
pValue_s	ValueOr(pValue_s pVal, pValue_s pValue, int Calc);
pValue_s	ValueXor(pValue_s pVal, pValue_s pValue, int Calc);
pValue_s	ValueEqv(pValue_s pVal, pValue_s pValue, int Calc);
pValue_s	ValueImp(pValue_s pVal, pValue_s pValue, int Calc);
pValue_s	ValueLt(pValue_s pVal, pValue_s pValue, int Calc);
pValue_s	ValueLe(pValue_s pVal, pValue_s pValue, int Calc);
pValue_s	ValueEqu(pValue_s pVal, pValue_s s, int Calc);
pValue_s	ValueGe(pValue_s pVal, pValue_s pValue, int Calc);
pValue_s	ValueGt(pValue_s pVal, pValue_s pValue, int Calc);
pValue_s	ValueNe(pValue_s pVal, pValue_s pValue, int Calc);
int				ValueExitFor(pValue_s pVal, pValue_s Limit, pValue_s Step);
BString		*ValueToString(pValue_s pVal, BString *pStr, char Pad, int HeadingSign, UINT Width, int Commas, int Precision, int Exponent, int TrailingSign);
pValue_s	ValueToStringUsing(pValue_s pVal, BString *pStr, BString *StrObj, UINT *pUsingPos);
pValue_s  ValueArrayToStringUsing(pValue_s pValue, BString *pPrintStr, BString *pUsingStr, UINT *pUsingPos);
BString		*ValueToWrite(pValue_s pVal, BString *pStr);
pValue_s	ValueNullValue(eVType ValueType);

