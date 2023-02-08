// XButtonXP.h  Version 1.4
//
// Author:  Hans Dietrich
//          hdietrich@gmail.com
//
// License:
//     This software is released under the Code Project Open License (CPOL),
//     which may be found here:  http://www.codeproject.com/info/eula.aspx
//     You are free to use this software in any way you like, except that you 
//     may not sell this source code.
//
//     This software is provided "as is" with no expressed or implied warranty.
//     I accept no liability for any damage or loss of business that this 
//     software may cause.
//
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include "OddButton.h"
#include "S45BasicDefs.h"

//=============================================================================
// Following file is available in Microsoft Platform SDK.
//=============================================================================
#include "uxtheme.h"

#define XBUTTONXP_NO_COLOR				((COLORREF) -1)

//=============================================================================

class CXButtonXP : public COddButton
{
public:
	CXButtonXP();
	virtual ~CXButtonXP();

public:
	enum ICON_ALIGNMENT
	{
		CENTER = 0,
		LEFT,
		RIGHT
	};

	CXButtonXP&				SetIconAlignment(ICON_ALIGNMENT ia);
	BOOL							GetToggle(){ return m_ToggleEnable; }
	CXButtonXP&				SetToggle(BOOL bIsToggle); 
	BOOL							GetToggleState(){ return m_IsToggled; }
	CXButtonXP&				SetToggleState(BOOL bToggled); 
	void							SendMouseNotification(BOOL bNotify){ m_ButtonNotify = bNotify; }; 
	BOOL							ToggleState(); 
	CXButtonXP&				EnableTheming(BOOL bEnable); 
	BOOL							SetFlashRate(int Rate = 500); 
	BOOL							IsThemed();
	BOOL							GetDrawToolbar(){ return m_bDrawToolbar; }
	CXButtonXP&				SetDrawToolbar(BOOL bDrawToolbar); 
	COLORREF					GetTextColor(){ return m_crText; }
	CXButtonXP&				SetTextColor(COLORREF rgb = XBUTTONXP_NO_COLOR){ m_crText = rgb; return *this; }
	COLORREF					GetBackgroundColor(){ return m_crBackground; }
	CXButtonXP&				SetBackgroundColor(COLORREF rgb = XBUTTONXP_NO_COLOR){ m_crBackground = rgb; return *this; }
	CXButtonXP&				SetRepeat(BOOL bRepeat, int nInitialDelay = 500, int nRepeatDelay = 100);
	CXButtonXP&				SetWindowText(LPCTSTR lpszText){ CButton::SetWindowText(lpszText); return *this; }
	CXButtonXP&				SetIcon(UINT nIDResource, ICON_ALIGNMENT ia = LEFT);
	CXButtonXP&				SetIcon(HICON hIcon, ICON_ALIGNMENT ia = LEFT);
	virtual void			DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct);

protected:
	virtual LRESULT		DefWindowProc(UINT message, WPARAM wParam, LPARAM lParam);
	void							DrawIcon(CDC *pDC, BOOL bHasText, CRect& rectItem, CRect& rectText, BOOL bIsPressed, BOOL bIsThemed, BOOL bIsDisabled);
	void							DrawText(CDC *pDC, LPCTSTR lpszText, CRect& rect, BOOL bIsPressed, BOOL bIsThemed, BOOL bIsDisabled);
	void							PrepareImageRect(BOOL bHasText, CRect& rectItem,  CRect& rectText, BOOL bIsPressed, BOOL bIsThemed, DWORD dwWidth, DWORD dwHeight,  CRect& rectImage);
	void							SaveParentBackground();

	BOOL							m_bFirstTime;
	BOOL							m_bMouseOverButton;
	BOOL							m_IsToggled;
	BOOL							m_ToggleEnable;
	BOOL							m_bLButtonDown;
	BOOL							m_bSent;
	BOOL							m_bEnableTheming;
	BOOL							m_bDrawToolbar;
	BOOL							m_bRepeat;
	BOOL							m_ButtonNotify;
	int								m_nInitialRepeatDelay;
	int								m_nRepeatDelay;
	HTHEME						m_hTheme;
	HICON							m_hIcon;
	HICON							m_hGrayIcon;
	ICON_ALIGNMENT		m_eIconAlignment;
	CRect							m_rectButton;		// button rect in parent window coordinates
	CDC								m_dcParent;
	CBitmap						m_bmpParent;
	CBitmap*					m_pOldParentBitmap;
	COLORREF					m_crBackground;		// optional button background color - when a background color is specified, theming is switched off
	COLORREF					m_crText;			// optional button text color - when a background color is specified, theming is switched off

	afx_msg void			OnMouseMove(UINT nFlags, CPoint point);
	afx_msg BOOL			OnEraseBkgnd(CDC* pDC);
	afx_msg void			OnTimer(UINT nIDEvent);
	afx_msg LRESULT		OnMouseLeave(WPARAM, LPARAM);

	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

