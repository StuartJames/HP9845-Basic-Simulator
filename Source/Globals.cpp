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
#include "globals.h"

/////////////////////////////////////////////////////////////////////////////
// Cached system metrics, etc

GLOBAL_DATA globalData;

/////////////////////////////////////////////////////////////////////////////
// Initialization code

GLOBAL_DATA::GLOBAL_DATA()
{
	UpdateSysMetrics();																		// Cached system metrics (updated in CWnd::OnWinIniChange)
	hbrLtGray = ::CreateSolidBrush(RGB(192, 192, 192));		// Border attributes
	hbrDkGray = ::CreateSolidBrush(RGB(128, 128, 128));
	ASSERT(hbrLtGray != NULL);
	ASSERT(hbrDkGray != NULL);
	hbrBtnFace = NULL;																		// Cached system values (updated in CWnd::OnSysColorChange)
	hbrBtnShadow = NULL;
	hbrBtnHilite = NULL;
	hbrWindowFrame = NULL;
	hpenBtnShadow = NULL;
	hpenBtnHilite = NULL;
	hpenBtnText = NULL;
	UpdateSysColors();
	hcurWait = ::LoadCursor(NULL, IDC_WAIT);
	hcurArrow = ::LoadCursor(NULL, IDC_ARROW);
	ASSERT(hcurWait != NULL);
	ASSERT(hcurArrow != NULL);
	cxBorder2 = CX_BORDER * 2;														// cxBorder2 and cyBorder are 2x borders for Win4
	cyBorder2 = CY_BORDER * 2;
	hStatusFont = NULL;																		// allocated on demand
	hToolTipsFont = NULL;
}

/////////////////////////////////////////////////////////////////////////////
// Termination code

GLOBAL_DATA::~GLOBAL_DATA()
{
	AfxDeleteObject((HGDIOBJ*)&hbrLtGray);								// cleanup standard brushes
	AfxDeleteObject((HGDIOBJ*)&hbrDkGray);
	AfxDeleteObject((HGDIOBJ*)&hbrBtnFace);
	AfxDeleteObject((HGDIOBJ*)&hbrBtnShadow);
	AfxDeleteObject((HGDIOBJ*)&hbrBtnHilite);
	AfxDeleteObject((HGDIOBJ*)&hbrWindowFrame);
	AfxDeleteObject((HGDIOBJ*)&hpenBtnShadow);						// cleanup standard pens
	AfxDeleteObject((HGDIOBJ*)&hpenBtnHilite);
	AfxDeleteObject((HGDIOBJ*)&hpenBtnText);
	AfxDeleteObject((HGDIOBJ*)&hStatusFont);							// clean up objects we don't actually create
	AfxDeleteObject((HGDIOBJ*)&hToolTipsFont);
}

/////////////////////////////////////////////////////////////////////////////

void GLOBAL_DATA::UpdateSysColors()
{
	clrBtnFace = ::GetSysColor(COLOR_BTNFACE);
	clrBtnShadow = ::GetSysColor(COLOR_BTNSHADOW);
	clrBtnHilite = ::GetSysColor(COLOR_BTNHIGHLIGHT);
	clrBtnText = ::GetSysColor(COLOR_BTNTEXT);
	clrWindowFrame = ::GetSysColor(COLOR_WINDOWFRAME);
	AfxDeleteObject((HGDIOBJ*)&hbrBtnFace);
	AfxDeleteObject((HGDIOBJ*)&hbrBtnShadow);
	AfxDeleteObject((HGDIOBJ*)&hbrBtnHilite);
	AfxDeleteObject((HGDIOBJ*)&hbrWindowFrame);
	hbrBtnFace = ::CreateSolidBrush(clrBtnFace);
	ASSERT(hbrBtnFace != NULL);
	hbrBtnShadow = ::CreateSolidBrush(clrBtnShadow);
	ASSERT(hbrBtnShadow != NULL);
	hbrBtnHilite = ::CreateSolidBrush(clrBtnHilite);
	ASSERT(hbrBtnHilite != NULL);
	hbrWindowFrame = ::CreateSolidBrush(clrWindowFrame);
	ASSERT(hbrWindowFrame != NULL);
	AfxDeleteObject((HGDIOBJ*)&hpenBtnShadow);
	AfxDeleteObject((HGDIOBJ*)&hpenBtnHilite);
	AfxDeleteObject((HGDIOBJ*)&hpenBtnText);
	hpenBtnShadow = ::CreatePen(PS_SOLID, 0, clrBtnShadow);
	ASSERT(hpenBtnShadow != NULL);
	hpenBtnHilite = ::CreatePen(PS_SOLID, 0, clrBtnHilite);
	ASSERT(hpenBtnHilite != NULL);
	hpenBtnText = ::CreatePen(PS_SOLID, 0, clrBtnText);
	ASSERT(hpenBtnText != NULL);
}

/////////////////////////////////////////////////////////////////////////////

void GLOBAL_DATA::UpdateSysMetrics()
{
	cxIcon = GetSystemMetrics(SM_CXICON);												// System metrics
	cyIcon = GetSystemMetrics(SM_CYICON);
	cxVScroll = GetSystemMetrics(SM_CXVSCROLL) + CX_BORDER;
	cyHScroll = GetSystemMetrics(SM_CYHSCROLL) + CY_BORDER;
	HDC hDCScreen = GetDC(NULL);																// Device metrics for screen
	ASSERT(hDCScreen != NULL);
	cxPixelsPerInch = GetDeviceCaps(hDCScreen, LOGPIXELSX);
	cyPixelsPerInch = GetDeviceCaps(hDCScreen, LOGPIXELSY);
	ReleaseDC(NULL, hDCScreen);
}

/////////////////////////////////////////////////////////////////////////////

void MiniGlobalFree(HGLOBAL hGlobal)
{
	if(hGlobal == NULL) return;
	ASSERT(GlobalFlags(hGlobal) != GMEM_INVALID_HANDLE);
  UINT nCount = GlobalFlags(hGlobal) & GMEM_LOCKCOUNT;
  while (nCount--) GlobalUnlock(hGlobal);
  GlobalFree(hGlobal);
}

/////////////////////////////////////////////////////////////////////////////

void MyTrace(const char *szFormat, ...)
{
char szBuff[1024];
va_list arg;
	
	va_start(arg, szFormat);
  _vsnprintf_s(szBuff, sizeof(szBuff), szFormat, arg);
  va_end(arg);
	OutputDebugString(szBuff);
}