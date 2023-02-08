/*
 * Copyright (c) 2001-2002 Paolo Messina and Jerzy Kaczorowski
 * 
 * The contents of this file are subject to the Artistic License (the "License").
 * You may not use this file except in compliance with the License. 
 * You may obtain a copy of the License at:
 * http://www.opensource.org/licenses/artistic-license.html
 * 
 * THIS PACKAGE IS PROVIDED "AS IS" AND WITHOUT ANY EXPRESS OR IMPLIED 
 * WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF 
 * MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 * 
 */

// OddButton.cpp : implementation file
//

#include "stdafx.h"
#include "OddButton.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// COddButton

COddButton::COddButton()
{
	m_bDefault = FALSE;
	m_bCanBeDefault = FALSE;
	
	// invalid value, since type still unknown
	m_nTypeStyle = ODDBTN_BS_TYPEMASK;
}

COddButton::~COddButton()
{
}


BEGIN_MESSAGE_MAP(COddButton, CButton)
	ON_WM_GETDLGCODE()
	ON_MESSAGE(BM_SETSTYLE, OnSetStyle)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// COddButton message handlers
void COddButton::PreSubclassWindow() 
{
	m_nTypeStyle = GetStyle() & ODDBTN_BS_TYPEMASK;	// set initial control type
	if(m_nTypeStyle == BS_DEFPUSHBUTTON){	// set initial default state flag
		m_bCanBeDefault = TRUE;				// enable default state handling for push buttons
		m_bDefault = TRUE;						// set default state for a default button
		m_nTypeStyle = BS_PUSHBUTTON;	// adjust style for default button
	}
	else if(m_nTypeStyle == BS_PUSHBUTTON){
		m_bCanBeDefault = TRUE;				// enable default state handling for push buttons
	}
	// you should not set the pOwner Draw before this call (don't use the resource editor "pOwner Draw" or
	// ModifyStyle(0, BS_OWNERDRAW) before calling PreSubclassWindow() )
	ASSERT(m_nTypeStyle != BS_OWNERDRAW);
	ModifyStyle(ODDBTN_BS_TYPEMASK, BS_OWNERDRAW, SWP_FRAMECHANGED);	// switch to owner-draw
	CButton::PreSubclassWindow();
}


/// WM_GETDLGCODE message ErrorHandler, indicate to the system whether we want to handle default state
UINT COddButton::OnGetDlgCode() 
{
	UINT nCode = CButton::OnGetDlgCode();
	switch(GetControlType()){// handle standard control types
		case BS_RADIOBUTTON:
		case BS_AUTORADIOBUTTON:
			nCode |= DLGC_RADIOBUTTON;
			break;
		case BS_GROUPBOX:
			nCode = DLGC_STATIC;
			break;
	}
	// tell the system if we want default state handling (losing default state always allowed)
	if(m_bCanBeDefault || m_bDefault) nCode |= (m_bDefault ? DLGC_DEFPUSHBUTTON : DLGC_UNDEFPUSHBUTTON);
	return nCode;
}


/// BM_SETSTYLE message ErrorHandler, update internal default state data member
LRESULT COddButton::OnSetStyle(WPARAM wParam, LPARAM lParam)
{
	UINT nNewType = (UINT)(wParam & ODDBTN_BS_TYPEMASK);
	if(nNewType == BS_DEFPUSHBUTTON){	// update default state flag
		ASSERT(m_bCanBeDefault);		// we must like default state at this point
		m_bDefault = TRUE;
	}
	else if(nNewType == BS_PUSHBUTTON){
		m_bDefault = FALSE;		// losing default state always allowed
	}
	// can't change control type after owner-draw is set. let the system process changes to other style bits
	// and redrawing, while keeping owner-draw style
	return DefWindowProc(BM_SETSTYLE,	(wParam & ~ODDBTN_BS_TYPEMASK) | BS_OWNERDRAW, lParam);
}


/////////////////////////////////////////////////////////////////////////////

UINT COddButton::GetControlType() const
{
	return m_nTypeStyle;
}


BOOL COddButton::IsDefault() const
{
	ASSERT((m_bCanBeDefault && m_bDefault) == m_bDefault);	// if we have default state, we must like it!
	return m_bDefault;
}


void COddButton::EnableDefault(BOOL bEnable)
{
	m_bCanBeDefault = bEnable;
	if(!bEnable && m_bDefault){	// disabling default when control has default state needs removing the default state
		SendMessage(BM_SETSTYLE, (GetStyle() & ~ODDBTN_BS_TYPEMASK) | BS_PUSHBUTTON, MAKELPARAM(TRUE, 0));	// remove default state
		ASSERT(m_bDefault == FALSE);
		CWnd* pParent = GetParent();		// update default button
		if(pParent){
			LRESULT lRes = pParent->SendMessage(DM_GETDEFID);
			if(HIWORD(lRes) == DC_HASDEFID){
				pParent->SendMessage(DM_SETDEFID, LOWORD(lRes));
			}
		}
	}
}


void COddButton::SetDefID(CDialog* pDialog, const UINT nID)
{
	if(!pDialog || !::IsWindow(pDialog->m_hWnd)){
		ASSERT(FALSE); // Bad pointer or dialog is not a window
		return;
	}
	const DWORD dwPrevDefID = pDialog->GetDefID();	// get the current default button
	const UINT nPrevID = (HIWORD(dwPrevDefID) == DC_HASDEFID) ? LOWORD(dwPrevDefID) : 0;
	pDialog->SetDefID(nID);
	LRESULT lRes = (nPrevID == 0) ? 0 : pDialog->SendDlgItemMessage(nPrevID, WM_GETDLGCODE);	// check previous ID is a default-compatible button and it has the default state
	if((lRes & DLGC_BUTTON) && (lRes & DLGC_DEFPUSHBUTTON)){
		pDialog->SendDlgItemMessage(nPrevID, BM_SETSTYLE,	BS_PUSHBUTTON, MAKELPARAM(TRUE, 0));
	}
	lRes = (nID == 0) ? 0 : pDialog->SendDlgItemMessage(nID, WM_GETDLGCODE);	// check new ID is a button
	if(lRes & DLGC_BUTTON){
		CWnd* pFocusWnd = GetFocus();
		LRESULT lResFocus = (pFocusWnd == NULL) ? 0 : pFocusWnd->SendMessage(WM_GETDLGCODE);
		if((lResFocus & DLGC_BUTTON) && (lResFocus & (DLGC_DEFPUSHBUTTON | DLGC_UNDEFPUSHBUTTON))){
			if((lRes & DLGC_DEFPUSHBUTTON) && (nID != (UINT)pFocusWnd->GetDlgCtrlID())){
				pDialog->SendDlgItemMessage(nID, BM_SETSTYLE,	BS_PUSHBUTTON, MAKELPARAM(TRUE, 0));
			}
			if(lResFocus & DLGC_UNDEFPUSHBUTTON){
				pFocusWnd->SendMessage(BM_SETSTYLE,	BS_DEFPUSHBUTTON, MAKELPARAM(TRUE, 0));
			}
		}
		else if(lRes & DLGC_UNDEFPUSHBUTTON){
			pDialog->SendDlgItemMessage(nID, BM_SETSTYLE,	BS_DEFPUSHBUTTON, MAKELPARAM(TRUE, 0));	// not default-compatible button has the focus set default state
		}
	}
}
