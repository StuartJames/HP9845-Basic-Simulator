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


/////////////////////////////////////////////////////////////////////////////
// Auxiliary System/Screen metrics

struct GLOBAL_DATA
{
	// system metrics
	int cxVScroll, cyHScroll;
	int cxBorder2, cyBorder2;
	int cxIcon, cyIcon;

	// device metrics for screen
	int cxPixelsPerInch, cyPixelsPerInch;

	// solid brushes with convenient gray colors and system colors
	HBRUSH hbrLtGray, hbrDkGray;
	HBRUSH hbrBtnHilite, hbrBtnFace, hbrBtnShadow;
	HBRUSH hbrWindowFrame;
	HPEN hpenBtnHilite, hpenBtnShadow, hpenBtnText;

	// color values of system colors used for CToolBar
	COLORREF clrBtnFace, clrBtnShadow, clrBtnHilite;
	COLORREF clrBtnText, clrWindowFrame;

	// standard cursors
	HCURSOR hcurWait;
	HCURSOR hcurArrow;
	HCURSOR hcurHelp;       // cursor used in Shift+F1 help

	// special GDI objects allocated on demand
	HFONT   hStatusFont;
	HFONT   hToolTipsFont;

	// Implementation
	GLOBAL_DATA();
	~GLOBAL_DATA();
	void UpdateSysColors();
	void UpdateSysMetrics();
};

extern GLOBAL_DATA globalData;

// Note: afxData.cxBorder and afxData.cyBorder aren't used anymore
#define CX_BORDER   1
#define CY_BORDER   1

// determine number of elements in an array (not bytes)
#define _ecountof(array) (sizeof(array)/sizeof(array[0]))

BOOL AFXAPI AfxCustomLogFont(UINT nIDS, LOGFONT* pLogFont);
void AFXAPI AfxDeleteObject(HGDIOBJ* pObject);
void MiniGlobalFree(HGLOBAL hGlobal);

void MyTrace(const char *szFormat, ...);

/////////////////////////////////////////////////////////////////////////////
