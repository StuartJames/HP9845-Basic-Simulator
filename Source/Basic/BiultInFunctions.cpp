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


#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif


char							**Environ;
Registry_s				BuiltInLibrary;

/////////////////////////////////////////////////////////////////////////////////////////////////

inline double ToRadians(const double angle)
{
	switch(pCntx->pSubCntx->TrigMode){
		case eTrigMode::DEG: return (angle * M_PI) / 180.0;
		case eTrigMode::GRAD: return (angle * M_PI) / 200.0;
		case eTrigMode::RAD: return angle;
	}
	return 0.0;
}

/////////////////////////////////////////////////////////////////////////////////////////////////

inline double FromRadians(const double angle)
{
	switch(pCntx->pSubCntx->TrigMode){
		case eTrigMode::DEG: return (angle * 180.0) / M_PI;
		case eTrigMode::GRAD: return (angle * 200.0) / M_PI;
		case eTrigMode::RAD: return angle;
	}
	return 0.0;
}

/////////////////////////////////////////////////////////////////////////////////////////////////

static int WildCardMatch(const char *pPtr, const char *pPattern)
{
	while(*pPattern){
		switch(*pPattern){
			case '*':{
				++pPattern;
				while(*pPtr) if(WildCardMatch(pPtr, pPattern)) return 1;else ++pPtr;
				break;
			}
			case '?':{
				if(*pPtr){ ++pPtr; ++pPattern; }else return 0;
				break;
			}
			default: if(*pPtr  ==  *pPattern){ ++pPtr; ++pPattern; }else return 0;
		}
	}
	return (*pPattern  ==  '\0' && *pPtr  ==  '\0');
}

/////////////////////////////////////////////////////////////////////////////////////////////////

static long IntValue(Stack_s *pStack, int l)
{
Value_s value;
pValue_s arg = VarValue(StackGetAt(pStack, l), 0, NULL, &value);

	assert(arg->Type  ==  eVType::V_INT);
	return arg->uData.Integer;
}

/////////////////////////////////////////////////////////////////////////////////////////////////

static double RealValue(Stack_s *pStack, int l)
{
Value_s value;
pValue_s arg = VarValue(StackGetAt(pStack, l), 0, NULL, &value);

	assert(arg->Type  ==  eVType::V_REAL);
	return arg->uData.Real;
}

/////////////////////////////////////////////////////////////////////////////////////////////////

static BString *StringValue(Stack_s *pStack, int l)
{
Value_s value;
pValue_s arg = VarValue(StackGetAt(pStack, l), 0, NULL, &value);

	assert(arg->Type  ==  eVType::V_STRING);
	return arg->uData.pString;
}

/////////////////////////////////////////////////////////////////////////////////////////////////

static pValue_s Bin(pValue_s pValue, unsigned long Value, long Digits)
{
char Buffer[sizeof(long) * 8 + 1];
char *pStr;

	ValueNewString(pValue);
	pStr = Buffer + sizeof(Buffer);
	*--pStr = '\0';
	if(Digits  ==  0) Digits = 1;
	while(Digits || Value){
		*--pStr = Value & 1 ? '1' : '0';
		if(Digits) --Digits;
		Value >>=  1;
	}
	pValue->uData.pString->AppendChars(pStr);
	return pValue;
}

/////////////////////////////////////////////////////////////////////////////////////////////////

static pValue_s Hex(pValue_s pValue, long value, long digits)
{
char buf[sizeof(long)*2 + 1];

	sprintf_s(buf, _T("%0*lx"), (int)digits, value);
	ValueNewString(pValue);
	pValue->uData.pString->AppendChars( buf);
	return pValue;
}

/////////////////////////////////////////////////////////////////////////////////////////////////

static pValue_s Find(pValue_s pValue, BString *pattern, UINT occurence)
{
	return pValue;
}

/////////////////////////////////////////////////////////////////////////////////////////////////

static pValue_s InStr(pValue_s pValue, UINT StartPos, UINT Length, BString *pHaystackStr, BString *pNeedleStr)
{
const char *pHaystackChars = pHaystackStr->GetBuffer();
UINT HaystackLength = pHaystackStr->GetLength();
const char *pNeedleChars = pNeedleStr->GetBuffer();
UINT NeedleLength = pNeedleStr->GetLength();
int Found;

	--StartPos;
	if(StartPos < 0) return ValueNewError(pValue, OUTOFRANGE, _T("Position"));
	if(Length < 0) return ValueNewError(pValue, OUTOFRANGE, _T("Length"));
	if(((UINT)StartPos) >= HaystackLength) return ValueNewInteger(pValue, 0);
	pHaystackChars += StartPos; HaystackLength -= StartPos;
	if(HaystackLength > (UINT)Length) HaystackLength = Length;
	Found = 1 + StartPos;
	while(NeedleLength <= HaystackLength){
		if(memcmp(pHaystackChars, pNeedleChars, NeedleLength) == 0) return ValueNewInteger(pValue, Found);
		++pHaystackChars; --HaystackLength;
		++Found;
	}
	return ValueNewInteger(pValue, 0);
}

/////////////////////////////////////////////////////////////////////////////////////////////////

static pValue_s String(pValue_s pValue, UINT Length, int c)
{
	if(Length < 0) return ValueNewError(pValue, OUTOFRANGE, _T("Length"));
	if(c < 0 || c > 255) return ValueNewError(pValue, OUTOFRANGE, _T("Number"));
	ValueNewString(pValue);
	pValue->uData.pString->Resize(Length);
	if(Length) memset(pValue->uData.pString->GetBuffer(), (char)c, Length);
	return pValue;
}

/////////////////////////////////////////////////////////////////////////////////////////////////

static pValue_s Mid(pValue_s pValue, BString *pStr, UINT Position, UINT Length)
{
	--Position;
	if(Position < 0) return ValueNewError(pValue, OUTOFRANGE, _T("Position"));
	if(Length < 0) return ValueNewError(pValue, OUTOFRANGE, _T("Length"));
	if((Position) + Length > pStr->GetLength()){
		Length = pStr->GetLength() - Position;
		if(Length < 0) Length = 0;
	}
	ValueNewString(pValue);
	pValue->uData.pString->Resize(Length);
	if(Length > 0) memcpy(pValue->uData.pString->GetBuffer(), pStr->GetBuffer() + Position, Length);
	return pValue;
}

/////////////////////////////////////////////////////////////////////////////////////////////////

static pValue_s Env(pValue_s pValue, long n)
{
int i;

	--n;
	if(n < 0) return ValueNewError(pValue, OUTOFRANGE, _T("Variable"));
	for(i = 0; Environ[i] && i < n; ++i) ;
	ValueNewString(pValue);
	if(i == n && Environ[i]) pValue->uData.pString->AppendChars(Environ[i]);
	return pValue;
}

/////////////////////////////////////////////////////////////////////////////////////////////////

static pValue_s fnAbs(pValue_s pValue, Stack_s *pStack)
{
	return ValueNewReal(pValue, fabs(RealValue(pStack, 0)));
}

/////////////////////////////////////////////////////////////////////////////////////////////////

static pValue_s fnAcs(pValue_s pValue, Stack_s *pStack)
{
	double d = FromRadians(acos(RealValue(pStack, 0)));
	return ValueNewReal(pValue, d);
}

/////////////////////////////////////////////////////////////////////////////////////////////////

static pValue_s fnAsn(pValue_s pValue, Stack_s *pStack)
{
	double d = FromRadians(asin(RealValue(pStack, 0)));
	return ValueNewReal(pValue, d);
}

/////////////////////////////////////////////////////////////////////////////////////////////////

static pValue_s fnAtn(pValue_s pValue, Stack_s *pStack)
{
	double d = FromRadians(atan(RealValue(pStack, 0)));
	return ValueNewReal(pValue, d);
}

/////////////////////////////////////////////////////////////////////////////////////////////////

static pValue_s fnBinI(pValue_s pValue, Stack_s *pStack)
{
	return Bin(pValue, IntValue(pStack, 0), 0);
}

/////////////////////////////////////////////////////////////////////////////////////////////////

static pValue_s fnBinD(pValue_s pValue, Stack_s *pStack)
{
int overflow;
long n;

	n = ValueToI(RealValue(pStack, 0), &overflow);
	if(overflow) return ValueNewError(pValue, INTEGEROVERFLOW, _T("number"));
	return Bin(pValue, n, 0);
}

/////////////////////////////////////////////////////////////////////////////////////////////////

static pValue_s fnBinII(pValue_s pValue, Stack_s *pStack)
{
	return Bin(pValue, IntValue(pStack, 0), IntValue(pStack, 1));
}

/////////////////////////////////////////////////////////////////////////////////////////////////

static pValue_s fnBinDI(pValue_s pValue, Stack_s *pStack)
{
int overflow;
long n;

	n = ValueToI(RealValue(pStack, 0), &overflow);
	if(overflow) return ValueNewError(pValue, INTEGEROVERFLOW, _T("number"));
	return Bin(pValue, n, IntValue(pStack, 1));
}

/////////////////////////////////////////////////////////////////////////////////////////////////

static pValue_s fnBinID(pValue_s pValue, Stack_s *pStack)
{
int overflow;
long digits;

	digits = ValueToI(RealValue(pStack, 1), &overflow);
	if(overflow) return ValueNewError(pValue, INTEGEROVERFLOW, _T("digits"));
	return Bin(pValue, IntValue(pStack, 0), digits);
}

/////////////////////////////////////////////////////////////////////////////////////////////////

static pValue_s fnBinDD(pValue_s pValue, Stack_s *pStack)
{
int overflow;
long n, digits;

	n = ValueToI(RealValue(pStack, 0), &overflow);
	if(overflow) return ValueNewError(pValue, INTEGEROVERFLOW, _T("number"));
	digits = ValueToI(RealValue(pStack, 1), &overflow);
	if(overflow) return ValueNewError(pValue, INTEGEROVERFLOW, _T("digits"));
	return Bin(pValue, n, digits);
}

/////////////////////////////////////////////////////////////////////////////////////////////////

static pValue_s fnChr(pValue_s pValue, Stack_s *pStack)
{
	long chr = IntValue(pStack, 0);
	if(chr < 0 || chr > 255) return ValueNewError(pValue, IMPROPERCHRARG, _T("character code"));
	ValueNewString(pValue);
	pValue->uData.pString->AppendChar((BYTE)chr);
	return pValue;
}

/////////////////////////////////////////////////////////////////////////////////////////////////

static pValue_s fnCint(pValue_s pValue, Stack_s *pStack)
{
	return ValueNewReal(pValue, ceil(RealValue(pStack, 0)));
}

/////////////////////////////////////////////////////////////////////////////////////////////////

static pValue_s fnCol(pValue_s pValue, Stack_s *pStack)
{
int Args = pStack->FrameSize - 2;

	if(Args > 0){ 
		pValue_s pArg = VarValue(StackGetAt(pStack, 0), 0, NULL, pValue);															// pop the variable off the stack
		if(pArg->Type	!= eVType::V_ARRAY) return  ValueNewError(pValue, TYPEMISMATCH2, 1);							// check that it's an array
		Var_s *pVar = pArg->uData.pArray;
		if(pVar->Dimensions > 0) return ValueNewInteger(pValue, pVar->pGeometry[pVar->Dimensions - 1].Size);
		return ValueNewError(pValue, INVALIDMATDIM, pVar->Dimensions);
	}
	return ValueNewError(pValue, TOOFEW);
}

/////////////////////////////////////////////////////////////////////////////////////////////////

static pValue_s fnCos(pValue_s pValue, Stack_s *pStack)
{
	double d = ToRadians(RealValue(pStack, 0));
	return ValueNewReal(pValue, cos(d));
}

/////////////////////////////////////////////////////////////////////////////////////////////////

static pValue_s fnCvi(pValue_s pValue, Stack_s *pStack)
{
int i;

	BString *s = StringValue(pStack, 0);
	long n = (s->GetLength() && s->GetAt(s->GetLength() - 1) < 0) ? -1 : 0;
	for(i = s->GetLength() - 1; i >= 0; --i) n = (n << 8) | (s->GetAt(i) & 0xff);
	return ValueNewInteger(pValue, n);
}

/////////////////////////////////////////////////////////////////////////////////////////////////

static pValue_s fnCvs(pValue_s pValue, Stack_s *pStack)
{
float n;

	BString *s = StringValue(pStack, 0);
	if(s->GetLength() != sizeof(float)) return ValueNewError(pValue, BADCONVERSION, _T("number"));
	memcpy(&n, s->GetBuffer(), sizeof(float));
	return ValueNewReal(pValue, (double)n);
}

/////////////////////////////////////////////////////////////////////////////////////////////////

static pValue_s fnCvd(pValue_s pValue, Stack_s *pStack)
{
double n;

	BString *s = StringValue(pStack, 0);
	if(s->GetLength() != sizeof(double)) return ValueNewError(pValue, BADCONVERSION, _T("number"));
	memcpy(&n, s->GetBuffer(), sizeof(double));
	return ValueNewReal(pValue, n);
}

/////////////////////////////////////////////////////////////////////////////////////////////////

static pValue_s fnDate(pValue_s pValue, Stack_s *pStack)
{
time_t tme;
struct tm LocTime;

	ValueNewString(pValue);
	pValue->uData.pString->Resize(15);
	time(&tme);
	localtime_s(&LocTime, &tme);
	sprintf_s(pValue->uData.pString->GetBuffer(), 15, _T("%02d-%02d-%04d"), LocTime.tm_mon + 1, LocTime.tm_mday, LocTime.tm_year + 1900);
	return pValue;
}

/////////////////////////////////////////////////////////////////////////////////////////////////

static pValue_s fnDec(pValue_s pValue, Stack_s *pStack)
{
Value_s value, *arg;
UINT upos;

	ValueNewString(pValue);
	arg = VarValue(StackGetAt(pStack, 0), 0, NULL, &value);
	upos = 0;
	ValueToStringUsing(arg, pValue->uData.pString, StringValue(pStack, 1), &upos);
	return pValue;
}

/////////////////////////////////////////////////////////////////////////////////////////////////

static pValue_s fnDeg(pValue_s pValue, Stack_s *pStack)
{
	return ValueNewReal(pValue, RealValue(pStack, 0) * (180.0 / M_PI));
}

/////////////////////////////////////////////////////////////////////////////////////////////////

static pValue_s fnDet(pValue_s pValue, Stack_s *pStack)
{
	return ValueNewReal(pValue, pStack->LastDet.Type == eVType::V_NIL ? 0.0 : (pStack->LastDet.Type == eVType::V_REAL ? pStack->LastDet.uData.Real : pStack->LastDet.uData.Integer));
}

/////////////////////////////////////////////////////////////////////////////////////////////////

static pValue_s fnDot(pValue_s pValue, Stack_s *pStack)
{
Value_s value;
int Args = pStack->FrameSize - 2;
int SubscriptA[2] = {0}, SubscriptB[2] = {0};																			
int IndexA, IndexB;																			

	if(Args > 1){ 
		pValue_s pArgA = VarValue(StackGetAt(pStack, 0), 0, NULL, &value);														// pop the 2 variables of the stack
		pValue_s pArgB = VarValue(StackGetAt(pStack, 0), 0, NULL, &value);
		if(pArgA->Type	!= eVType::V_ARRAY) return  ValueNewError(pValue, TYPEMISMATCH2, 1);					// check that they are arrays...
		if(pArgB->Type	!= eVType::V_ARRAY) return  ValueNewError(pValue, TYPEMISMATCH2, 2);
		Var_s *pVarA = pArgA->uData.pArray;
		Var_s *pVarB = pArgB->uData.pArray;																														// ...and that they are of the same dimensions and size
		if((pVarA->Dimensions != pVarB->Dimensions) || (pVarA->Dimensions != 2) || (pVarA->pGeometry[0].Size != pVarB->pGeometry[1].Size)) return  ValueNewError(pValue, TYPEMISMATCH2, 1);
		eVType TypeA = pVarA->Type;
		ValueNewNull(pValue, TypeA);
		for(int i = 0; i < pVarA->pGeometry[0].Size; ++i){																						// iterate for row size of A
			SubscriptA[0] = i;																																					
			SubscriptB[1] = i;																																					
			SubscriptToIndex(pVarA->Dimensions, pVarA->pGeometry, SubscriptA, &IndexA);									// get index in row of array A
			SubscriptToIndex(pVarA->Dimensions, pVarA->pGeometry, SubscriptB, &IndexB);									// and index in column of array B
			Value_s ValProduct;
			ValueClone(&ValProduct, &(pVarA->pValue[IndexA]));
			ValueMultiply(&ValProduct, &(pVarB->pValue[IndexB]), 1);
			if(ValProduct.Type == eVType::V_ERROR){
				*pValue = ValProduct;
				return pValue;
			}
			ValueAdd(pValue, &ValProduct, 1);
			ValueDestroy(&ValProduct);
		}
		return pValue;
	}
	return ValueNewError(pValue, TOOFEW);
}

/////////////////////////////////////////////////////////////////////////////////////////////////

static pValue_s fnDRound(pValue_s pValue, Stack_s *pStack)
{
double d;
long l;
int i = 0;

	d = RealValue(pStack, 0);
	l = IntValue(pStack, 1);
	if(l < 1) return ValueNewReal(pValue, 0.0);				// required significant digits must be greater than 1
	while(d >= 1){ d /= 10;	i++; }										// get the number of significant digits of the value
	if(l <= 12){																			// required significant digits must be less or equal to 12
		d *= pow(10, l);																// shift the value to the significant rounding digits
		d = round(d);																		// round up the value
		d *= pow(10, (long)i - l);											// shift the result to the orignal number of digits
	}
	return ValueNewReal(pValue, d);
}

/////////////////////////////////////////////////////////////////////////////////////////////////

static pValue_s fnEnvironI(pValue_s pValue, Stack_s *pStack)
{
	return Env(pValue, IntValue(pStack, 0));
}

/////////////////////////////////////////////////////////////////////////////////////////////////

static pValue_s fnEnvironD(pValue_s pValue, Stack_s *pStack)
{
int overflow;
long n;

	n = ValueToI(RealValue(pStack, 0), &overflow);
	if(overflow) return ValueNewError(pValue, INTEGEROVERFLOW, _T("number"));
	return Env(pValue, n);
}

/////////////////////////////////////////////////////////////////////////////////////////////////

static pValue_s fnEnvironS(pValue_s pValue, Stack_s *pStack)
{
char *pVar, *pVal;
size_t Count;

	ValueNewString(pValue);
	if((pVar = StringValue(pStack, 0)->GetBuffer())){
		_dupenv_s(&pVal, &Count, pVar);
		if(pVal) pValue->uData.pString->AppendChars(pVal);
	}
	return pValue;
}

/////////////////////////////////////////////////////////////////////////////////////////////////

static pValue_s fnEof(pValue_s pValue, Stack_s *pStack)
{
	return ValueNewInteger(pValue, 0);
}

/////////////////////////////////////////////////////////////////////////////////////////////////

static pValue_s fnErrC(pValue_s pValue, Stack_s *pStack)
{
	return ValueNewInteger(pValue, pStack->Error.Type == eVType::V_NIL ? 0 : pStack->Error.uData.Error.Code);
}

/////////////////////////////////////////////////////////////////////////////////////////////////

static pValue_s fnErrL(pValue_s pValue, Stack_s *pStack)
{
	return ValueNewInteger(pValue, pStack->ErrorLine);
}

/////////////////////////////////////////////////////////////////////////////////////////////////

static pValue_s fnErrM(pValue_s pValue, Stack_s *pStack)
{
	ValueNewString(pValue);
	pValue->uData.pString->AppendChars(pStack->Error.uData.Error.pStr);
	return pValue;
}

/////////////////////////////////////////////////////////////////////////////////////////////////

static pValue_s fnErrN(pValue_s pValue, Stack_s *pStack)
{
	return ValueNewInteger(pValue, pStack->Error.Type == eVType::V_NIL ? 0 : pStack->Error.uData.Error.Number);
}

/////////////////////////////////////////////////////////////////////////////////////////////////

static pValue_s fnExp(pValue_s pValue, Stack_s *pStack)
{
	return ValueNewReal(pValue, exp(RealValue(pStack, 0)));
}

/////////////////////////////////////////////////////////////////////////////////////////////////

static pValue_s fnFalse(pValue_s pValue, Stack_s *pStack)
{
	return ValueNewInteger(pValue, 0);
}

/////////////////////////////////////////////////////////////////////////////////////////////////

static pValue_s fnFind(pValue_s pValue, Stack_s *pStack)
{
	return Find(pValue, StringValue(pStack, 0), 0);
}

/////////////////////////////////////////////////////////////////////////////////////////////////

static pValue_s fnFindI(pValue_s pValue, Stack_s *pStack)
{
	return Find(pValue, StringValue(pStack, 0), IntValue(pStack, 1));
}

/////////////////////////////////////////////////////////////////////////////////////////////////

static pValue_s fnFindD(pValue_s pValue, Stack_s *pStack)
{
int overflow;
long n;

	n = ValueToI(RealValue(pStack, 1), &overflow);
	if(overflow) return ValueNewError(pValue, INTEGEROVERFLOW, _T("number"));
	return Find(pValue, StringValue(pStack, 0), n);
}

/////////////////////////////////////////////////////////////////////////////////////////////////

static pValue_s fnFix(pValue_s pValue, Stack_s *pStack)
{
	double x = RealValue(pStack, 0);
	return ValueNewReal(pValue, x < 0.0 ? ceil(x) : floor(x));
}

/////////////////////////////////////////////////////////////////////////////////////////////////

static pValue_s fnFract(pValue_s pValue, Stack_s *pStack)
{
int overflow;
	
	double d = RealValue(pStack, 0);
	long n = ValueToI(d, &overflow);
	return ValueNewReal(pValue, d - n);
}

/////////////////////////////////////////////////////////////////////////////////////////////////

static pValue_s fnFreeFile(pValue_s pValue, Stack_s *pStack)
{
	return ValueNewInteger(pValue, 0);
}

/////////////////////////////////////////////////////////////////////////////////////////////////

static pValue_s fnHexI(pValue_s pValue, Stack_s *pStack)
{
char buf[sizeof(long) * 2 + 1];

	sprintf_s(buf, strlen(buf), _T("%lx"), IntValue(pStack, 0));
	ValueNewString(pValue);
	pValue->uData.pString->AppendChars(buf);
	return pValue;
}

/////////////////////////////////////////////////////////////////////////////////////////////////

static pValue_s fnHexD(pValue_s pValue, Stack_s *pStack)
{
char buf[sizeof(long) * 2 + 1];
int overflow;
long n;

	n = ValueToI(RealValue(pStack, 0), &overflow);
	if(overflow) return ValueNewError(pValue, INTEGEROVERFLOW, _T("number"));
	sprintf_s(buf, strlen(buf), _T("%lx"), n);
	ValueNewString(pValue);
	pValue->uData.pString->AppendChars(buf);
	return pValue;
}

/////////////////////////////////////////////////////////////////////////////////////////////////

static pValue_s fnHexII(pValue_s pValue, Stack_s *pStack)
{
	return Hex(pValue, IntValue(pStack, 0), IntValue(pStack, 1));
}

/////////////////////////////////////////////////////////////////////////////////////////////////

static pValue_s fnHexDI(pValue_s pValue, Stack_s *pStack)
{
int overflow;
long n;

	n = ValueToI(RealValue(pStack, 0), &overflow);
	if(overflow) return ValueNewError(pValue, INTEGEROVERFLOW, _T("number"));
	return Hex(pValue, n, IntValue(pStack, 1));
}

/////////////////////////////////////////////////////////////////////////////////////////////////

static pValue_s fnHexID(pValue_s pValue, Stack_s *pStack)
{
int overflow;
long digits;

	digits = ValueToI(RealValue(pStack, 1), &overflow);
	if(overflow) return ValueNewError(pValue, INTEGEROVERFLOW, _T("digits"));
	return Hex(pValue, IntValue(pStack, 0), digits);
}

/////////////////////////////////////////////////////////////////////////////////////////////////

static pValue_s fnHexDD(pValue_s pValue, Stack_s *pStack)
{
int overflow;
long n, digits;

	n = ValueToI(RealValue(pStack, 0), &overflow);
	if(overflow) return ValueNewError(pValue, INTEGEROVERFLOW, _T("number"));
	digits = ValueToI(RealValue(pStack, 1), &overflow);
	if(overflow) return ValueNewError(pValue, INTEGEROVERFLOW, _T("digits"));
	return Hex(pValue, n, digits);
}

/////////////////////////////////////////////////////////////////////////////////////////////////

static pValue_s fnInt(pValue_s pValue, Stack_s *pStack)
{
	return ValueNewReal(pValue, floor(RealValue(pStack, 0)));
}

/////////////////////////////////////////////////////////////////////////////////////////////////

static pValue_s fnIntp(pValue_s pValue, Stack_s *pStack)
{
long l;

	errno = 0;
	l = lrint(floor(RealValue(pStack, 0)));
	if(errno == EDOM) return ValueNewError(pValue, OUTOFRANGE, _T("number"));
	return ValueNewInteger(pValue, l);
}

/////////////////////////////////////////////////////////////////////////////////////////////////

static pValue_s fnInp(pValue_s pValue, Stack_s *pStack)
{
	return ValueNewInteger(pValue, 0);
}

/////////////////////////////////////////////////////////////////////////////////////////////////

static pValue_s fnInPos(pValue_s pValue, Stack_s *pStack)
{
	BString *haystack = StringValue(pStack, 0);
	return InStr(pValue, 1, haystack->GetLength(), haystack, StringValue(pStack, 1));
}

/////////////////////////////////////////////////////////////////////////////////////////////////

static pValue_s fnLcase(pValue_s pValue, Stack_s *pStack)
{
	ValueNewString(pValue);
	pValue->uData.pString->AppendString(StringValue(pStack, 0));
	pValue->uData.pString->ToLowerCase();
	return pValue;
}

/////////////////////////////////////////////////////////////////////////////////////////////////

static pValue_s fnLen(pValue_s pValue, Stack_s *pStack)
{
	return ValueNewInteger(pValue, StringValue(pStack, 0)->GetLength());
}

/////////////////////////////////////////////////////////////////////////////////////////////////

static pValue_s fnLeft(pValue_s pValue, Stack_s *pStack)
{
	BString *pStr = StringValue(pStack, 0);
	long len = IntValue(pStack, 1);
	int left = (len < (long)pStr->GetLength()) ? (int)len : (int)pStr->GetLength();
	if(left < 0) return ValueNewError(pValue, SUBSCRIPTVALUE);
	ValueNewString(pValue);
	pValue->uData.pString->Resize(left);
	if(left) memcpy(pValue->uData.pString->GetBuffer(), pStr->GetBuffer(), left);
	return pValue;
}

/////////////////////////////////////////////////////////////////////////////////////////////////

static pValue_s fnLog(pValue_s pValue, Stack_s *pStack)
{
	if(RealValue(pStack, 0) <= 0.0) ValueNewError(pValue, NEGATIVELOGVALUE);
	else ValueNewReal(pValue, log(RealValue(pStack, 0)));
	return pValue;
}

/////////////////////////////////////////////////////////////////////////////////////////////////

static pValue_s fnLtrim(pValue_s pValue, Stack_s *pStack)
{
int spaces;

	BString *s = StringValue(pStack, 0);
	int len = s->GetLength();
	for(spaces = 0; spaces < len && s->GetAt(spaces) == ' '; ++spaces) ;
	ValueNewString(pValue);
	pValue->uData.pString->Resize(len - spaces);
	if(len - spaces) memcpy(pValue->uData.pString->GetBuffer(), s->GetBuffer() + spaces, len - spaces);
	return pValue;
}

/////////////////////////////////////////////////////////////////////////////////////////////////

static pValue_s fnMatch(pValue_s pValue, Stack_s *pStack)
{
long found;
const char *n, *h;

	BString *needle = StringValue(pStack, 0);
	const char *needleChars = needle->GetBuffer();
	const char *needleEnd = needle->GetBuffer() + needle->GetLength();
	BString *haystack = StringValue(pStack, 1);
	const char *haystackChars = haystack->GetBuffer();
	UINT haystackLength = haystack->GetLength();
	long start = IntValue(pStack, 2);
	if(start < 0) return ValueNewError(pValue, OUTOFRANGE, _T("Position"));
	if(((UINT)start) >= haystackLength) return ValueNewInteger(pValue, 0);
	haystackChars += start; haystackLength -= start;
	found = 1 + start;
	while(haystackLength){
		for(n = needleChars, h = haystackChars; n < needleEnd && h < (haystackChars + haystackLength); ++n, ++h){
			if(*n == '\\'){
				if(++n < needleEnd && *n != *h) break;
			}
			else if(*n == '!'){
				if(!isalpha((int)*h)) break;
			}
			else if(*n == '#'){
				if(!isdigit((int)*h)) break;
			}
			else if(*n != '?' && *n != *h) break;
		}
		if(n == needleEnd) return ValueNewInteger(pValue, found);
		++haystackChars;
		--haystackLength;
		++found;
	}
	return ValueNewInteger(pValue, 0);
}

/////////////////////////////////////////////////////////////////////////////////////////////////

static pValue_s fnMid2I(pValue_s pValue, Stack_s *pStack)
{
	return Mid(pValue, StringValue(pStack, 0), IntValue(pStack, 1), StringValue(pStack, 0)->GetLength());
}

/////////////////////////////////////////////////////////////////////////////////////////////////

static pValue_s fnMid2D(pValue_s pValue, Stack_s *pStack)
{
int overflow;
long start;

	start = ValueToI(RealValue(pStack, 1), &overflow);
	if(overflow) return ValueNewError(pValue, INTEGEROVERFLOW, _T("start"));
	return Mid(pValue, StringValue(pStack, 0), start, StringValue(pStack, 0)->GetLength());
}

/////////////////////////////////////////////////////////////////////////////////////////////////

static pValue_s fnMid3II(pValue_s pValue, Stack_s *pStack)
{
	return Mid(pValue, StringValue(pStack, 0), IntValue(pStack, 1), IntValue(pStack, 2));
}

/////////////////////////////////////////////////////////////////////////////////////////////////

static pValue_s fnMid3ID(pValue_s pValue, Stack_s *pStack)
{
int overflow;
long len;

	len = ValueToI(RealValue(pStack, 2), &overflow);
	if(overflow) return ValueNewError(pValue, INTEGEROVERFLOW, _T("Length"));
	return Mid(pValue, StringValue(pStack, 0), IntValue(pStack, 1), len);
}

/////////////////////////////////////////////////////////////////////////////////////////////////

static pValue_s fnMid3DI(pValue_s pValue, Stack_s *pStack)
{
int overflow;
long start;

	start = ValueToI(RealValue(pStack, 1), &overflow);
	if(overflow) return ValueNewError(pValue, INTEGEROVERFLOW, _T("start"));
	return Mid(pValue, StringValue(pStack, 0), start, IntValue(pStack, 2));
}

/////////////////////////////////////////////////////////////////////////////////////////////////

static pValue_s fnMid3DD(pValue_s pValue, Stack_s *pStack)
{
int overflow;
long start, len;

	start = ValueToI(RealValue(pStack, 1), &overflow);
	if(overflow) return ValueNewError(pValue, INTEGEROVERFLOW, _T("start"));
	len = ValueToI(RealValue(pStack, 2), &overflow);
	if(overflow) return ValueNewError(pValue, INTEGEROVERFLOW, _T("Length"));
	return Mid(pValue, StringValue(pStack, 0), start, len);
}

/////////////////////////////////////////////////////////////////////////////////////////////////

static pValue_s fnMki(pValue_s pValue, Stack_s *pStack)
{
UINT i;

	long x = IntValue(pStack, 0);
	ValueNewString(pValue);
	pValue->uData.pString->Resize(sizeof(long));
	for(i = 0; i < sizeof(long); ++i, x >>=  8) pValue->uData.pString->SetAt(i, (x & 0xff));
	return pValue;
}

/////////////////////////////////////////////////////////////////////////////////////////////////

static pValue_s fnMks(pValue_s pValue, Stack_s *pStack)
{
	float x = (float)RealValue(pStack, 0);
	ValueNewString(pValue);
	pValue->uData.pString->Resize(sizeof(float));
	memcpy(pValue->uData.pString->GetBuffer(), &x, sizeof(float));
	return pValue;
}

/////////////////////////////////////////////////////////////////////////////////////////////////

static pValue_s fnMkd(pValue_s pValue, Stack_s *pStack)
{
	double x = RealValue(pStack, 0);
	ValueNewString(pValue);
	pValue->uData.pString->Resize(sizeof(double));
	memcpy(pValue->uData.pString->GetBuffer(), &x, sizeof(double));
	return pValue;
}

/////////////////////////////////////////////////////////////////////////////////////////////////

static pValue_s fnMax(pValue_s pValue, Stack_s *pStack)
{
Value_s value;
double Max = DBL_MIN;
int Args = pStack->FrameSize - 2;

	if(Args > 0){ 
		for(int i = 0; i < Args; i++){
			pValue_s pArg = VarValue(StackGetAt(pStack, i), 0, NULL, &value);
			switch(pArg->Type){
				case  eVType::V_REAL:{
					Max = (pArg->uData.Real > Max) ? pArg->uData.Real : Max;
					break;
				}
				case eVType::V_INT:{
					Max = ((double)pArg->uData.Integer > Max) ? (double)pArg->uData.Integer : Max;
					break;
				}
				case eVType::V_ARRAY:{
					Var_s *pVar = pArg->uData.pArray;
					for(int j = 0; j < pVar->Size; ++j){
						switch(pValue->Type){
							case  eVType::V_REAL:{
								Max = (pVar->pValue[j].uData.Real > Max) ? pVar->pValue[j].uData.Real : Max;
								break;
							}
							case eVType::V_INT:{
								Max = ((double)pVar->pValue[j].uData.Integer > Max) ? (double)pVar->pValue[j].uData.Integer : Max;
								break;
							}
							default:{
								return  ValueNewError(pValue, TYPEMISMATCH2, i + 1);
							}
						}
					}
					break;
				}
				default:{
					return  ValueNewError(pValue, TYPEMISMATCH2, i + 1);
				}
			}
		}
		return ValueNewReal(pValue, Max);
	}
	return ValueNewError(pValue, TOOFEW);
}

/////////////////////////////////////////////////////////////////////////////////////////////////

static pValue_s fnMin(pValue_s pValue, Stack_s *pStack)
{
Value_s value;
double Min = DBL_MAX;
int Args = pStack->FrameSize - 2;

	if(Args > 0){ 
		for(int i = 0; i < Args; i++){
			pValue_s pArg = VarValue(StackGetAt(pStack, i), 0, NULL, &value);
			switch(pArg->Type){
				case  eVType::V_REAL:{
					Min = (pArg->uData.Real < Min) ? pArg->uData.Real : Min;
					break;
				}
				case eVType::V_INT:{
					Min = ((double)pArg->uData.Integer < Min) ? (double)pArg->uData.Integer : Min;
					break;
				}
				case eVType::V_ARRAY:{
					Var_s *pVar = pArg->uData.pArray;
					for(int j = 0; j < pVar->Size; ++j){
						switch(pVar->pValue[j].Type){
							case eVType::V_INT:{
								Min = ((double)pVar->pValue[j].uData.Integer < Min) ? (double)pVar->pValue[j].uData.Integer : Min;
								break;
							}
							case eVType::V_REAL:{
								Min = (pVar->pValue[j].uData.Real < Min) ? pVar->pValue[j].uData.Real : Min;
								break;
							}
							case eVType::V_STRING:
							default: return  ValueNewError(pValue, TYPEMISMATCH2, i + 1);
						}
					}
					break;
				}
				default:{
					return  ValueNewError(pValue, TYPEMISMATCH2, i + 1);
				}
			}
		}
		return ValueNewReal(pValue, Min);
	}
	return ValueNewError(pValue, TOOFEW);
}

/////////////////////////////////////////////////////////////////////////////////////////////////

static pValue_s fnNum(pValue_s pValue, Stack_s *pStack)
{
	BString *pStr = StringValue(pStack, 0);
	if(pStr->GetLength() == 0) return ValueNewError(pValue, IMPROPERCHRARG, _T("string"));
	return ValueNewInteger(pValue, pStr->GetAt(0) & 0xff);
}

/////////////////////////////////////////////////////////////////////////////////////////////////

static pValue_s fnOct(pValue_s pValue, Stack_s *pStack)
{
char buf[sizeof(long) * 3 + 1];

	sprintf_s(buf, strlen(buf), _T("%lo"), IntValue(pStack, 0));
	ValueNewString(pValue);
	pValue->uData.pString->AppendChars(buf);
	return pValue;
}

/////////////////////////////////////////////////////////////////////////////////////////////////

static pValue_s fnPI(pValue_s pValue, Stack_s *pStack)
{
	return ValueNewReal(pValue, M_PI);
}

/////////////////////////////////////////////////////////////////////////////////////////////////

static pValue_s fnPRound(pValue_s pValue, Stack_s *pStack)
{
	double p = pow(10, IntValue(pStack, 1));																			// will always be greater than 0
	double d = round(RealValue(pStack, 0) / p);																		// round to an integer
	return ValueNewReal(pValue, d * p);																						// recover precision
}

/////////////////////////////////////////////////////////////////////////////////////////////////

static pValue_s fnRad(pValue_s pValue, Stack_s *pStack)
{
	return ValueNewReal(pValue, (RealValue(pStack, 0) * M_PI) / 180.0);
}

/////////////////////////////////////////////////////////////////////////////////////////////////

static pValue_s fnRev(pValue_s pValue, Stack_s *pStack)
{
BString *pStr = StringValue(pStack, 0);
int Count = pStr->GetLength(), i, j;

	ValueNewString(pValue);
	if(Count > 1){
		pValue->uData.pString->Resize(Count);
		for(i = 0, j = Count - 1; i < Count; ++i, --j){
			pValue->uData.pString->SetAt(i, pStr->GetAt(j));
		}
	}
	else pValue->uData.pString->AppendString(pStr);
	return pValue;
}

/////////////////////////////////////////////////////////////////////////////////////////////////

static pValue_s fnRes(pValue_s pValue, Stack_s *pStack)
{
	ValueClone(pValue, &g_ResultValue);
	return pValue;
}

/////////////////////////////////////////////////////////////////////////////////////////////////

static pValue_s fnRight(pValue_s pValue, Stack_s *pStack)
{
	BString *s = StringValue(pStack, 0);
	int len = s->GetLength();
	int right = (IntValue(pStack, 1) < len) ? IntValue(pStack, 1) : len;
	if(right < 0) return ValueNewError(pValue, OUTOFRANGE, _T("Length"));
	ValueNewString(pValue);
	pValue->uData.pString->Resize(right);
	if(right) memcpy(pValue->uData.pString->GetBuffer(), s->GetBuffer() + len - right, right);
	return pValue;
}

/////////////////////////////////////////////////////////////////////////////////////////////////

static pValue_s fnRnd(pValue_s pValue, Stack_s *pStack)
{
	ValueNewReal(pValue, rand() / (double)RAND_MAX);
	return pValue;
}

/////////////////////////////////////////////////////////////////////////////////////////////////

static pValue_s fnRow(pValue_s pValue, Stack_s *pStack)
{
int Args = pStack->FrameSize - 2;

	if(Args > 0){ 
		pValue_s pArg = VarValue(StackGetAt(pStack, 0), 0, NULL, pValue);															// pop the variable off the stack
		if(pArg->Type	!= eVType::V_ARRAY) return  ValueNewError(pValue, TYPEMISMATCH2, 1);						// check that it's an array
		Var_s *pVar = pArg->uData.pArray;
		if(pVar->Dimensions > 1) return ValueNewInteger(pValue, pVar->pGeometry[pVar->Dimensions - 2].Size);
		return ValueNewError(pValue, INVALIDMATDIM, pVar->Dimensions);
	}
	return ValueNewError(pValue, TOOFEW);
}

/////////////////////////////////////////////////////////////////////////////////////////////////

static pValue_s fnRpt(pValue_s pValue, Stack_s *pStack)
{
int Count = IntValue(pStack, 1);
BString *pStr = StringValue(pStack, 0);

	ValueNewString(pValue);
	if((Count < 0) || (Count > 0x7FFF)) return ValueNewError(pValue, IMPROPERCHRARG, _T("length"));
	for(int i = 0; i < Count; ++i) pValue->uData.pString->AppendString(pStr);
	return pValue;
}

/////////////////////////////////////////////////////////////////////////////////////////////////

static pValue_s fnRtrim(pValue_s pValue, Stack_s *pStack)
{
int lastSpace;

	BString *s = StringValue(pStack, 0);
	int len = s->GetLength();
	for(lastSpace = len; lastSpace > 0 && s->GetAt(lastSpace - 1) == ' '; --lastSpace) ;
	ValueNewString(pValue);
	pValue->uData.pString->Resize(lastSpace);
	if(lastSpace) memcpy(pValue->uData.pString->GetBuffer(), s->GetBuffer(), lastSpace);
	return pValue;
}

/////////////////////////////////////////////////////////////////////////////////////////////////

static pValue_s fnSgn(pValue_s pValue, Stack_s *pStack)
{
	double x = RealValue(pStack, 0);
	return ValueNewInteger(pValue, (x < 0.0) ? -1 : ((x == 0.0) ? 0 : 1));
}

/////////////////////////////////////////////////////////////////////////////////////////////////

static pValue_s fnSin(pValue_s pValue, Stack_s *pStack)
{
	double d = ToRadians(RealValue(pStack, 0));
	return ValueNewReal(pValue, sin(d));
}

/////////////////////////////////////////////////////////////////////////////////////////////////

static pValue_s fnSpace(pValue_s pValue, Stack_s *pStack)
{
	long len = IntValue(pStack, 0);
	if(len < 0) return ValueNewError(pValue, OUTOFRANGE, _T("Length"));
	ValueNewString(pValue);
	pValue->uData.pString->Resize(len);
	if(len) memset(pValue->uData.pString->GetBuffer(), ' ', len);
	return pValue;
}

/////////////////////////////////////////////////////////////////////////////////////////////////

static pValue_s fnSqr(pValue_s pValue, Stack_s *pStack)
{
	if(RealValue(pStack, 0) < 0.0) ValueNewError(pValue, OUTOFRANGE, _T("Argument"));
	else ValueNewReal(pValue, sqrt(RealValue(pStack, 0)));
	return pValue;
}

/////////////////////////////////////////////////////////////////////////////////////////////////

static pValue_s fnStr(pValue_s pValue, Stack_s *pStack)
{
Value_s Value, *pArg;
BString String;

	pArg = VarValue(StackGetAt(pStack, 0), 0, NULL, &Value);
	assert(pArg->Type != eVType::V_ERROR);
	ValueToString(pArg, &String, ' ', -1, 0, 0, -1, 0, 0);
	pValue->Type = eVType::V_STRING;
	pValue->uData.pString = &String;
	return pValue;
}

/////////////////////////////////////////////////////////////////////////////////////////////////

static pValue_s fnStringII(pValue_s pValue, Stack_s *pStack)
{
	return String(pValue, IntValue(pStack, 0), IntValue(pStack, 1));
}

/////////////////////////////////////////////////////////////////////////////////////////////////

static pValue_s fnStringID(pValue_s pValue, Stack_s *pStack)
{
int overflow;
long chr;

	chr = ValueToI(RealValue(pStack, 1), &overflow);
	if(overflow) return ValueNewError(pValue, INTEGEROVERFLOW, _T("character code"));
	return String(pValue, IntValue(pStack, 0), chr);
}

/////////////////////////////////////////////////////////////////////////////////////////////////

static pValue_s fnStringDI(pValue_s pValue, Stack_s *pStack)
{
int overflow;
long len;

	len = ValueToI(RealValue(pStack, 0), &overflow);
	if(overflow) return ValueNewError(pValue, INTEGEROVERFLOW, _T("Length"));
	return String(pValue, len, IntValue(pStack, 1));
}

/////////////////////////////////////////////////////////////////////////////////////////////////

static pValue_s fnStringDD(pValue_s pValue, Stack_s *pStack)
{
int overflow;
long len, chr;

	len = ValueToI(RealValue(pStack, 0), &overflow);
	if(overflow) return ValueNewError(pValue, INTEGEROVERFLOW, _T("Length"));
	chr = ValueToI(RealValue(pStack, 1), &overflow);
	if(overflow) return ValueNewError(pValue, INTEGEROVERFLOW, _T("character code"));
	return String(pValue, len, chr);
}

/////////////////////////////////////////////////////////////////////////////////////////////////

static pValue_s fnStringIS(pValue_s pValue, Stack_s *pStack)
{
	if(StringValue(pStack, 1)->GetLength() == 0) return ValueNewError(pValue, UNDEFINED, _T("`string$' of empty string"));
	return String(pValue, IntValue(pStack, 0), StringValue(pStack, 1)->GetAt(0));
}

/////////////////////////////////////////////////////////////////////////////////////////////////

static pValue_s fnStringDS(pValue_s pValue, Stack_s *pStack)
{
int overflow;
long len;

	len = ValueToI(RealValue(pStack, 0), &overflow);
	if(overflow) return ValueNewError(pValue, INTEGEROVERFLOW, _T("Length"));
	if(StringValue(pStack, 1)->GetLength() == 0) return ValueNewError(pValue, UNDEFINED, _T("`string$' of empty string"));
	return String(pValue, len, StringValue(pStack, 1)->GetAt(0));
}

/////////////////////////////////////////////////////////////////////////////////////////////////

static pValue_s fnStrip(pValue_s pValue, Stack_s *pStack)
{
	ValueNewString(pValue);
	pValue->uData.pString->AppendString(StringValue(pStack, 0));
	for(UINT i = 0; i < pValue->uData.pString->GetLength(); ++i) pValue->uData.pString->AndChar(i, 0x7f);
	return pValue;
}

/////////////////////////////////////////////////////////////////////////////////////////////////

static pValue_s fnSum(pValue_s pValue, Stack_s *pStack)
{
int Args = pStack->FrameSize - 2;

	if(Args > 0){ 
		pValue_s pArg = VarValue(StackGetAt(pStack, 0), 0, NULL, pValue);															// pop the variable off the stack
		if(pArg->Type	!= eVType::V_ARRAY) return  ValueNewError(pValue, TYPEMISMATCH2, 1);							// check that it's an array
		Var_s *pVar = pArg->uData.pArray;
		if((pVar->Dimensions < 1)) return  ValueNewError(pValue, TYPEMISMATCH2, 1);
		eVType Type = pVar->Type;
		ValueNewNull(pValue, Type);
		for(int i = 0; i < pVar->Size; ++i) ValueAdd(pValue, &(pVar->pValue[i]), 1);
		return pValue;
	}
	return ValueNewError(pValue, TOOFEW);
}

/////////////////////////////////////////////////////////////////////////////////////////////////

static pValue_s fnTan(pValue_s pValue, Stack_s *pStack)
{
	double d = ToRadians(RealValue(pStack, 0));
	return ValueNewReal(pValue, tan(d));
}

/////////////////////////////////////////////////////////////////////////////////////////////////

static pValue_s fnTimei(pValue_s pValue, Stack_s *pStack)
{

	time_t t = time(NULL);
	return ValueNewInteger(pValue, (unsigned long)t);
}

/////////////////////////////////////////////////////////////////////////////////////////////////

static pValue_s fnTimes(pValue_s pValue, Stack_s *pStack)
{
time_t tme;
struct tm LocTime;

	ValueNewString(pValue);
	pValue->uData.pString->Resize(15);
	time(&tme);
	localtime_s(&LocTime, &tme);
	sprintf_s(pValue->uData.pString->GetBuffer(), 15, _T("%02d:%02d:%02d"), LocTime.tm_hour, LocTime.tm_min, LocTime.tm_sec);
	return pValue;
}

/////////////////////////////////////////////////////////////////////////////////////////////////

static pValue_s fnTimer(pValue_s pValue, Stack_s *pStack)
{
time_t tme;
struct tm LocTime;

	time(&tme);
	localtime_s(&LocTime, &tme);
	return ValueNewReal(pValue, LocTime.tm_hour * 3600 + LocTime.tm_min * 60 + LocTime.tm_sec);
}

/////////////////////////////////////////////////////////////////////////////////////////////////

static pValue_s fnTl(pValue_s pValue, Stack_s *pStack)
{
BString *s = StringValue(pStack, 0);

	ValueNewString(pValue);
	if(s->GetLength()){
		int tail = s->GetLength() - 1;
		pValue->uData.pString->Resize(tail);
		if(s->GetLength()) memcpy(pValue->uData.pString->GetBuffer(), s->GetBuffer() + 1, tail);
	}
	return pValue;
}

/////////////////////////////////////////////////////////////////////////////////////////////////

static pValue_s fnTrim(pValue_s pValue, Stack_s *pStack)
{
int First, Last;

	BString *pStr = StringValue(pStack, 0);
	int Length = pStr->GetLength();
	for(First = 0; First < Length && pStr->GetAt(First) == ' '; ++First);
	for(Last = Length; Last > 0 && pStr->GetAt(Last - 1) == ' '; --Last);
	Length = Last - First;
	if(Length){
		ValueNewString(pValue);
		pValue->uData.pString->Resize(Length);
		memcpy(pValue->uData.pString->GetBuffer(), pStr->GetBuffer() + First, Length);
	}
	return pValue;
}

/////////////////////////////////////////////////////////////////////////////////////////////////
static pValue_s fnTrue(pValue_s pValue, Stack_s *pStack)
{
	return ValueNewInteger(pValue, -1);
}

/////////////////////////////////////////////////////////////////////////////////////////////////

static pValue_s fnUcase(pValue_s pValue, Stack_s *pStack)
{
	ValueNewString(pValue);
	pValue->uData.pString->AppendString(StringValue(pStack, 0));
	pValue->uData.pString->ToUpperCase();
	return pValue;
}

/////////////////////////////////////////////////////////////////////////////////////////////////

static pValue_s fnVal(pValue_s pValue, Stack_s *pStack)
{
BString *pStr = StringValue(pStack, 0);
char *pEnd = pStr->GetBuffer(pStr->GetLength());														 // set default
long i;
int overflow;

	if(pStr->GetBuffer() == NULL) return ValueNewReal(pValue, 0.0);
	i = ValueStringToInteger(pStr->GetBuffer(), &pEnd, &overflow);
	if(overflow > 0) return  ValueNewError(pValue, IMPROPERVALARG);			// check for error
	if(*pEnd == '\0') return ValueNewInteger(pValue, i);
	else return ValueNewReal(pValue, ValueStringToReal(pStr->GetBuffer(), NULL, &overflow));
}

/////////////////////////////////////////////////////////////////////////////////////////////////

static pValue_s fnValn(pValue_s pValue, Stack_s *pStack)
{
Value_s value;
eRounding Mode = pCntx->pSubCntx->RoundingMode;

	pValue_s arg = VarValue(StackGetAt(pStack, 0), 0, NULL, &value);
	switch(arg->Type){
		case eVType::V_INT:{
			ValueNewString(pValue);
			pValue->uData.pString->AppendPrintf("%ld", arg->uData.Integer);
			return pValue;
		}
		case eVType::V_REAL:{
			ValueNewString(pValue);
			switch(Mode){
				case eRounding::FLOAT:{
					pValue->uData.pString->AppendPrintf("%0.*le", pCntx->pSubCntx->RoundingPrecision, arg->uData.Real);
					break;
				}
				case eRounding::FIXED:{
					pValue->uData.pString->AppendPrintf("%0.*lf", pCntx->pSubCntx->RoundingPrecision, arg->uData.Real);
					break;
				}
				default:
				case eRounding::STANDARD:{
					pValue->uData.pString->AppendPrintf("%0lg", arg->uData.Real);
					break;
				}
			}
			return pValue;
		}
		case eVType::V_STRING:{
			pValue->uData.pString->AppendString(arg->uData.pString);
			return pValue;
		}
	}
	return nullptr;
}

/////////////////////////////////////////////////////////////////////////////////////////////////

void BuiltIns(void)
{
	RegisterBuiltIn(_T("ABS"),			eVType::V_REAL,			fnAbs,				1,	eVType::V_REAL);
	RegisterBuiltIn(_T("ACS"),			eVType::V_REAL,			fnAcs,				1,	eVType::V_REAL);
	RegisterBuiltIn(_T("ASN"),			eVType::V_REAL,			fnAsn,				1,	eVType::V_REAL);
	RegisterBuiltIn(_T("ATN"),			eVType::V_REAL,			fnAtn,				1,	eVType::V_REAL);
	RegisterBuiltIn(_T("BIN$"),			eVType::V_STRING,		fnBinI,				1,	eVType::V_INT);
	RegisterBuiltIn(_T("BIN$"),			eVType::V_STRING,		fnBinD,				1,	eVType::V_REAL);
	RegisterBuiltIn(_T("BIN$"),			eVType::V_STRING,		fnBinII,			2,	eVType::V_INT,			eVType::V_INT);
	RegisterBuiltIn(_T("bin$"),			eVType::V_STRING,		fnBinDI,			2,	eVType::V_REAL,			eVType::V_INT);
	RegisterBuiltIn(_T("BIN$"),			eVType::V_STRING,		fnBinID,			2,	eVType::V_INT,			eVType::V_REAL);
	RegisterBuiltIn(_T("BIN$"),			eVType::V_STRING,		fnBinDD,			2,	eVType::V_REAL,			eVType::V_REAL);
	RegisterBuiltIn(_T("CHR$"),			eVType::V_STRING,		fnChr,				1,	eVType::V_INT);
	RegisterBuiltIn(_T("CINT"),			eVType::V_REAL,			fnCint,				1,	eVType::V_REAL);
	RegisterBuiltIn(_T("COS"),			eVType::V_REAL,			fnCos,				1,	eVType::V_REAL);
	RegisterBuiltIn(_T("COL"),			eVType::V_INT,			fnCol,				1,	eVType::V_INT);
	RegisterBuiltIn(_T("COL"),			eVType::V_INT,			fnCol,				1,	eVType::V_REAL);
	RegisterBuiltIn(_T("COL"),			eVType::V_INT,			fnCol,				1,	eVType::V_STRING);
	RegisterBuiltIn(_T("CVI"),			eVType::V_INT,			fnCvi,				1,	eVType::V_STRING);
	RegisterBuiltIn(_T("CVS"),			eVType::V_REAL,			fnCvs,				1,	eVType::V_STRING);
	RegisterBuiltIn(_T("CVD"),			eVType::V_REAL,			fnCvd,				1,	eVType::V_STRING);
	RegisterBuiltIn(_T("DATE$"),		eVType::V_STRING,		fnDate,				0);
	RegisterBuiltIn(_T("DEC$"),			eVType::V_STRING,		fnDec,				2,	eVType::V_REAL,			eVType::V_STRING);
	RegisterBuiltIn(_T("DEC$"),			eVType::V_STRING,		fnDec,				2,	eVType::V_INT,			eVType::V_STRING);
	RegisterBuiltIn(_T("DEC$"),			eVType::V_STRING,		fnDec,				2,	eVType::V_STRING,		eVType::V_STRING);
	RegisterBuiltIn(_T("DEG"),			eVType::V_REAL,			fnDeg,				1,	eVType::V_REAL);
	RegisterBuiltIn(_T("DET"),			eVType::V_REAL,			fnDet,				0);
	RegisterBuiltIn(_T("DOT"),			eVType::V_INT,			fnDot,				2,	eVType::V_INT,			eVType::V_INT);
	RegisterBuiltIn(_T("DOT"),			eVType::V_REAL,			fnDot,				2,	eVType::V_REAL,			eVType::V_REAL);
	RegisterBuiltIn(_T("DROUND"),		eVType::V_REAL,			fnDRound,			2,	eVType::V_REAL,			eVType::V_INT);
	RegisterBuiltIn(_T("ENVIRON$"),	eVType::V_STRING,		fnEnvironI,		1,	eVType::V_INT);
	RegisterBuiltIn(_T("ENVIRON$"),	eVType::V_STRING,		fnEnvironD,		1,	eVType::V_REAL);
	RegisterBuiltIn(_T("ENVIRON$"),	eVType::V_STRING,		fnEnvironS,		1,	eVType::V_STRING);
	RegisterBuiltIn(_T("FRACT"),		eVType::V_REAL,			fnFract,			1,	eVType::V_REAL);
	RegisterBuiltIn(_T("EOF"),			eVType::V_INT,			fnEof,				1,	eVType::V_INT);
	RegisterBuiltIn(_T("ERRC"),			eVType::V_INT,			fnErrC,				0);
	RegisterBuiltIn(_T("ERRL"),			eVType::V_INT,			fnErrL,				0);
	RegisterBuiltIn(_T("ERRM$"),		eVType::V_STRING,		fnErrM,				0);
	RegisterBuiltIn(_T("ERRN"),			eVType::V_INT,			fnErrN,				0);
	RegisterBuiltIn(_T("EXP"),			eVType::V_REAL,			fnExp,				1,	eVType::V_REAL);
	RegisterBuiltIn(_T("FALSE"),		eVType::V_INT,			fnFalse,			0);
	RegisterBuiltIn(_T("FIND$"),		eVType::V_STRING,		fnFind,				1,	eVType::V_STRING);
	RegisterBuiltIn(_T("FIND$"),		eVType::V_STRING,		fnFindI,			2,	eVType::V_STRING,		eVType::V_INT);
	RegisterBuiltIn(_T("FIND$"),		eVType::V_STRING,		fnFindD,			2,	eVType::V_STRING,		eVType::V_REAL);
	RegisterBuiltIn(_T("FIX"),			eVType::V_REAL,			fnFix,				1,	eVType::V_REAL);
	RegisterBuiltIn(_T("FREEFILE"),	eVType::V_INT,			fnFreeFile,		0);
	RegisterBuiltIn(_T("HEX$"),			eVType::V_STRING,		fnHexI,				1,	eVType::V_INT);
	RegisterBuiltIn(_T("HEX$"),			eVType::V_STRING,		fnHexD,				1,	eVType::V_REAL);
	RegisterBuiltIn(_T("HEX$"),			eVType::V_STRING,		fnHexII,			2,	eVType::V_INT,			eVType::V_INT);
	RegisterBuiltIn(_T("HEX$"),			eVType::V_STRING,		fnHexDI,			2,	eVType::V_REAL,			eVType::V_INT);
	RegisterBuiltIn(_T("HEX$"),			eVType::V_STRING,		fnHexID,			2,	eVType::V_INT,			eVType::V_REAL);
	RegisterBuiltIn(_T("HEX$"),			eVType::V_STRING,		fnHexDD,			2,	eVType::V_REAL,			eVType::V_REAL);
	RegisterBuiltIn(_T("INP"),			eVType::V_INT,			fnInp,				1,	eVType::V_INT);
	RegisterBuiltIn(_T("INT"),			eVType::V_REAL,			fnInt,				1,	eVType::V_REAL);
	RegisterBuiltIn(_T("INT%"),			eVType::V_INT,			fnIntp,				1,	eVType::V_REAL);
	RegisterBuiltIn(_T("LCASE$"),		eVType::V_STRING,		fnLcase,			1,	eVType::V_STRING);
	RegisterBuiltIn(_T("LWC$"),			eVType::V_STRING,		fnLcase,			1,	eVType::V_STRING);
	RegisterBuiltIn(_T("LEFT$"),		eVType::V_STRING,		fnLeft,				2,	eVType::V_STRING,		eVType::V_INT);
	RegisterBuiltIn(_T("LEN"),			eVType::V_INT,			fnLen,				1,	eVType::V_STRING);
	RegisterBuiltIn(_T("LOG"),			eVType::V_REAL,			fnLog,				1,	eVType::V_REAL);
	RegisterBuiltIn(_T("LTRIM$"),		eVType::V_STRING,		fnLtrim,			1,	eVType::V_STRING);
	RegisterBuiltIn(_T("MATCH"),		eVType::V_INT,			fnMatch,			3,	eVType::V_STRING,		eVType::V_STRING,		eVType::V_INT);
	RegisterBuiltIn(_T("MAX"),			eVType::V_INT,			fnMax,				-1);
	RegisterBuiltIn(_T("MIN"),			eVType::V_INT,			fnMin,				-1);
	RegisterBuiltIn(_T("MID$"),			eVType::V_STRING,		fnMid2I,			2,	eVType::V_STRING,		eVType::V_INT);
	RegisterBuiltIn(_T("MID$"),			eVType::V_STRING,		fnMid2D,			2,	eVType::V_STRING,		eVType::V_REAL);
	RegisterBuiltIn(_T("MID$"),			eVType::V_STRING,		fnMid3II,			3,	eVType::V_STRING,		eVType::V_INT,			eVType::V_INT);
	RegisterBuiltIn(_T("MID$"),			eVType::V_STRING,		fnMid3ID,			3,	eVType::V_STRING,		eVType::V_INT,			eVType::V_REAL);
	RegisterBuiltIn(_T("MID$"),			eVType::V_STRING,		fnMid3DI,			3,	eVType::V_STRING,		eVType::V_REAL,			eVType::V_INT);
	RegisterBuiltIn(_T("MID$"),			eVType::V_STRING,		fnMid3DD,			3,	eVType::V_STRING,		eVType::V_REAL,			eVType::V_REAL);
	RegisterBuiltIn(_T("MKI$"),			eVType::V_STRING,		fnMki,				1,	eVType::V_INT);
	RegisterBuiltIn(_T("MKS$"),			eVType::V_STRING,		fnMks,				1,	eVType::V_REAL);
	RegisterBuiltIn(_T("MKD$"),			eVType::V_STRING,		fnMkd,				1,	eVType::V_REAL);
	RegisterBuiltIn(_T("NUM"),			eVType::V_INT,			fnNum,				1,	eVType::V_STRING);
	RegisterBuiltIn(_T("OCT$"),			eVType::V_STRING,		fnOct,				1,	eVType::V_INT);
	RegisterBuiltIn(_T("PI"),				eVType::V_REAL,			fnPI,					0);
	RegisterBuiltIn(_T("POS"),			eVType::V_INT,			fnInPos,			2,	eVType::V_STRING,		eVType::V_STRING);
	RegisterBuiltIn(_T("PROUND"),		eVType::V_REAL,			fnPRound,			2,	eVType::V_REAL,			eVType::V_INT);
	RegisterBuiltIn(_T("RAD"),			eVType::V_REAL,			fnRad,				1,	eVType::V_REAL);
	RegisterBuiltIn(_T("RES"),			eVType::V_REAL,			fnRes,				0);
	RegisterBuiltIn(_T("REV$"),			eVType::V_STRING,		fnRev,				1,	eVType::V_STRING);
	RegisterBuiltIn(_T("RIGHT$"),		eVType::V_STRING,		fnRight,			2,	eVType::V_STRING,		eVType::V_INT);
	RegisterBuiltIn(_T("RND"),			eVType::V_REAL,			fnRnd,				0);
	RegisterBuiltIn(_T("ROW"),			eVType::V_INT,			fnRow,				1,	eVType::V_INT);
	RegisterBuiltIn(_T("ROW"),			eVType::V_INT,			fnRow,				1,	eVType::V_REAL);
	RegisterBuiltIn(_T("ROW"),			eVType::V_INT,			fnRow,				1,	eVType::V_STRING);
	RegisterBuiltIn(_T("RPT$"),			eVType::V_STRING,		fnRpt,				2,	eVType::V_STRING,		eVType::V_INT);
	RegisterBuiltIn(_T("RTRIM$"),		eVType::V_STRING,		fnRtrim,			1,	eVType::V_STRING);
	RegisterBuiltIn(_T("SEG$"),			eVType::V_STRING,		fnMid3II,			3,	eVType::V_STRING,		eVType::V_INT,			eVType::V_INT);
	RegisterBuiltIn(_T("SEG$"),			eVType::V_STRING,		fnMid3ID,			3,	eVType::V_STRING,		eVType::V_INT,			eVType::V_REAL);
	RegisterBuiltIn(_T("SEG$"),			eVType::V_STRING,		fnMid3DI,			3,	eVType::V_STRING,		eVType::V_REAL,			eVType::V_INT);
	RegisterBuiltIn(_T("SEG$"),			eVType::V_STRING,		fnMid3DD,			3,	eVType::V_STRING,		eVType::V_REAL,			eVType::V_REAL);
	RegisterBuiltIn(_T("SGN"),			eVType::V_INT,			fnSgn,				1,	eVType::V_REAL);
	RegisterBuiltIn(_T("SIN"),			eVType::V_REAL,			fnSin,				1,	eVType::V_REAL);
	RegisterBuiltIn(_T("SPACE$"),		eVType::V_STRING,		fnSpace,			1,	eVType::V_INT);
	RegisterBuiltIn(_T("SQR"),			eVType::V_REAL,			fnSqr,				1,	eVType::V_REAL);
	RegisterBuiltIn(_T("STR$"),			eVType::V_STRING,		fnStr,				1,	eVType::V_REAL);
	RegisterBuiltIn(_T("STR$"),			eVType::V_STRING,		fnStr,				1,	eVType::V_INT);
	RegisterBuiltIn(_T("STRING$"),	eVType::V_STRING,		fnStringII,		2,	eVType::V_INT,		eVType::V_INT);
	RegisterBuiltIn(_T("STRING$"),	eVType::V_STRING,		fnStringID,		2,	eVType::V_INT,		eVType::V_REAL);
	RegisterBuiltIn(_T("STRING$"),	eVType::V_STRING,		fnStringDI,		2,	eVType::V_REAL,		eVType::V_INT);
	RegisterBuiltIn(_T("STRING$"),	eVType::V_STRING,		fnStringDD,		2,	eVType::V_REAL,		eVType::V_REAL);
	RegisterBuiltIn(_T("STRING$"),	eVType::V_STRING,		fnStringIS,		2,	eVType::V_INT,		eVType::V_STRING);
	RegisterBuiltIn(_T("STRING$"),	eVType::V_STRING,		fnStringDS,		2,	eVType::V_REAL,		eVType::V_STRING);
	RegisterBuiltIn(_T("STRIP$"),		eVType::V_STRING,		fnStrip,			1,	eVType::V_STRING);
	RegisterBuiltIn(_T("SUM"),			eVType::V_INT,			fnSum,				1,	eVType::V_INT);
	RegisterBuiltIn(_T("SUM"),			eVType::V_REAL,			fnSum,				1,	eVType::V_REAL);
	RegisterBuiltIn(_T("TAN"),			eVType::V_REAL,			fnTan,				1,	eVType::V_REAL);
	RegisterBuiltIn(_T("TIME"),			eVType::V_INT,			fnTimei,			0);
	RegisterBuiltIn(_T("TIME$"),		eVType::V_STRING,		fnTimes,			0);
	RegisterBuiltIn(_T("TIMER"),		eVType::V_REAL,			fnTimer,			0);
	RegisterBuiltIn(_T("TL$"),			eVType::V_STRING,		fnTl,					1,	eVType::V_STRING);
	RegisterBuiltIn(_T("TRIM$"),		eVType::V_STRING,		fnTrim,				1,	eVType::V_STRING);
	RegisterBuiltIn(_T("TRUE"),			eVType::V_INT,			fnTrue,				0);
	RegisterBuiltIn(_T("UCASE$"),		eVType::V_STRING,		fnUcase,			1,	eVType::V_STRING);
	RegisterBuiltIn(_T("UPC$"),			eVType::V_STRING,		fnUcase,			1,	eVType::V_STRING);
	RegisterBuiltIn(_T("VAL"),			eVType::V_REAL,			fnVal,				1,	eVType::V_STRING);
	RegisterBuiltIn(_T("VAL$"),			eVType::V_STRING,		fnValn,				1,	eVType::V_REAL);
}


/////////////////////////////////////////////////////////////////////////////////////////////////

void RegisterBuiltIn(const char *pIdent, enum eVType ValueType, pValue_s (* func)(pValue_s pValue, Stack_s *pStack), int argLength, ...)
{
Symbol_s **pSym;
Symbol_s *pNew, **pSub;
va_list ap;
int i;

	for(pSym = &BuiltInLibrary.pTable[Hash(pIdent)]; ((*pSym != NULL) && (_stricmp((*pSym)->pName, pIdent))); pSym = &((*pSym)->pNext));
	if(*pSym == NULL){																									// check for overload
		*pSym = (Symbol_s*)malloc(sizeof(Symbol_s));
		i = (strlen(pIdent) + 1);
		(*pSym)->pName = (char*)malloc(i * sizeof(char));
		strcpy_s((*pSym)->pName, i, pIdent);
		(*pSym)->pNext = NULL;
		pNew = (*pSym);
	}
	else{
		for(pSub = &((*pSym)->uType.sSubrtn.uFunc.uBuiltIn.pNext); *pSub; pSub = &((*pSub)->uType.sSubrtn.uFunc.uBuiltIn.pNext));
		*pSub = (Symbol_s*)malloc(sizeof(Symbol_s));
		pNew = (*pSub);
	}
	pNew->uType.sSubrtn.uFunc.uBuiltIn.pNext = NULL;
	pNew->SymbolType = eSymType::BUILTINFUNCTION;
	pNew->uType.sSubrtn.RetType = ValueType;
	pNew->uType.sSubrtn.ArgLength = argLength;
	if(argLength >= 0){
		pNew->uType.sSubrtn.pArgTypes = (eVType*)malloc(sizeof(enum eVType) * argLength);
		va_start(ap, argLength);
		for(i = 0; i < argLength; ++i) pNew->uType.sSubrtn.pArgTypes[i] = va_arg(ap, enum eVType);
		va_end(ap);
	}
	else pNew->uType.sSubrtn.pArgTypes = NULL;
	pNew->uType.sSubrtn.uFunc.uBuiltIn.call = func;
}

/////////////////////////////////////////////////////////////////////////////////////////////////

void DestroyBuiltInFunctions(void)
{
int i;
Symbol_s **ppSymCurrent, *pSym, *pSymNext;

	for(i = 0; i < IDENT_HASHSIZE; ++i){
		ppSymCurrent = &BuiltInLibrary.pTable[i];
		while(*ppSymCurrent){
			pSym = *ppSymCurrent;
			pSymNext = pSym->pNext;
			if(pSym->SymbolType == eSymType::BUILTINFUNCTION){
				if(pSym->uType.sSubrtn.pArgTypes) free(pSym->uType.sSubrtn.pArgTypes);
				if(pSym->uType.sSubrtn.uFunc.uBuiltIn.pNext){
					Symbol_s *pOvl = pSym->uType.sSubrtn.uFunc.uBuiltIn.pNext;
					while(pOvl){
						if(pOvl->uType.sSubrtn.pArgTypes) free(pOvl->uType.sSubrtn.pArgTypes);
						Symbol_s *pNext = pOvl->uType.sSubrtn.uFunc.uBuiltIn.pNext;
						free(pOvl);
						pOvl = pNext;
					}
				}
				free(pSym->pName);
				free(pSym);
				*ppSymCurrent = pSymNext;
			}
			else ppSymCurrent = &pSym->pNext;
		}
	}
}

