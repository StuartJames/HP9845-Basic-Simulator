/*
 *        Copyright (c) 2011-2020 HydraSystems.
 *
 *  This software is copyrighted by and is the sole property of HydraSystems.
 *  All rights, title, ownership, or other interests in the software
 *  remain the property of HydraSystems.
 *  This software may only be used in accordance with the corresponding
 *  license agreement.  Any unauthorised use, duplication, transmission,
 *  distribution, or disclosure of this software is expressly forbidden.
 *
 *  Based on a design by Michael Haardt
 *
 * Edit Date/Ver   Edit Description
 * ==============  ===================================================
 * SJ   19/08/2011  Original
 *
 *
 */

#include "stdafx.h"
#include "BasicDefs.h"
#include <stdlib.h>
#include <stdio.h>


//////////////////////////////////////////////////////////////////////////////////////

CToken::CToken(void)
{
	m_pNext = nullptr;
	m_pPrev = nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////

CToken::~CToken(void)
{
	switch(m_Type){
		case T_CASEELSE:
		case T_CASE:			  free(m_Obj.pCaseValue); break;
		case T_DATAINPUT:   free(m_Obj.pDataInput); break;
		case T_IDENTIFIER:  free(m_Obj.pIdentifier); break;
		case T_LABEL:			  free(m_Obj.pLabel->pName); free(m_Obj.pLabel); break;
		case T_NEXT:        free(m_Obj.pNext); break;
		case T_ON:          if(m_Obj.On.pPc) free(m_Obj.On.pPc);break;
		case T_REM:         free(m_Obj.pRem); break;
		case T_SELECT:		  free(m_Obj.pSelectCase); break;
		case T_STRING:      StringDestroy(m_Obj.pString); free(m_Obj.pString); break;
		case T_WEND:        free(m_Obj.pWhilePc); break;
		case T_WHILE:       free(m_Obj.pAfterWend); break;
		default:            break;
	}
}

