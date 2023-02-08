/*
 * Module ID: mfcpp.cpp
 * Title    : MFC++: Extend MFC classes.
 *
 * Author   : Olivier Langlois <olanglois@sympatico.ca>
 * Date     : December 12, 2005
 *
 * For details on CStrechyStatusBar class, go to:
 * http://www3.sympatico.ca/olanglois/clover.html
 *
 * For details on CSubclassToolTipCtrl and CHyperLinkDlg classes, go to:
 * http://www3.sympatico.ca/olanglois/hyperlinkdemo.htm
 *
 * For details on CMinMaxFrame class, go to:
 * http://www3.sympatico.ca/olanglois/minmaxdemo.html
 *
 * Revision :
 *
 * 001        03-Feb-2006 - Olivier Langlois
 *            - Added CMinMaxFrame class
 */

#include "stdafx.h"
#include "MMFrame.h"

/////////////////////////////////////////////////////////////////////////////

BOOL CSubclassToolTipCtrl::AddWindowTool(HWND hWin, LPTSTR pszText)
{
	TOOLINFO ti;
	FillInToolInfo(ti, hWin, 0);
	ti.uFlags |= TTF_SUBCLASS;
	ti.hinst = AfxGetInstanceHandle();
	ti.lpszText = pszText;

	return (BOOL)SendMessage(TTM_ADDTOOL, 0, (LPARAM)&ti);
}

/////////////////////////////////////////////////////////////////////////////

BOOL CSubclassToolTipCtrl::AddRectTool(HWND hWin, LPTSTR pszText,
																			 LPCRECT lpRect, UINT nIDTool)
{
	TOOLINFO ti;
	FillInToolInfo(ti, hWin, nIDTool);
	ti.uFlags |= TTF_SUBCLASS;
	ti.hinst = AfxGetInstanceHandle();
	ti.lpszText = pszText;
	::CopyRect(&ti.rect, lpRect);

	return (BOOL)SendMessage(TTM_ADDTOOL, 0, (LPARAM)&ti);
}

/////////////////////////////////////////////////////////////////////////////

void CSubclassToolTipCtrl::FillInToolInfo(TOOLINFO &ti, HWND hWnd, UINT nIDTool) const
{
	::ZeroMemory(&ti, sizeof(TOOLINFO));
	ti.cbSize = sizeof(TOOLINFO);
	if(nIDTool == 0) {
		ti.hwnd = ::GetParent(hWnd);
		ti.uFlags = TTF_IDISHWND;
		ti.uId = (UINT)hWnd;
	}
	else {
		ti.hwnd = hWnd;
		ti.uFlags = 0;
		ti.uId = nIDTool;
	}
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

/* It might be more appropriate to install the OnBarCheck handlers in derived	classes as this class cannot know in advance which bars will be used. */
BEGIN_MESSAGE_MAP(CMinMaxFrame, CFrameWnd)
ON_WM_GETMINMAXINFO()
ON_COMMAND_EX(ID_VIEW_TOOLBAR, OnBarCheck)
ON_COMMAND_EX(ID_VIEW_STATUS_BAR, OnBarCheck)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////

CMinMaxFrame::CMinMaxFrame(LONG minX, LONG minY) : m_MinMaxLogic(minX, minY), m_pTB(NULL)
{
}

/////////////////////////////////////////////////////////////////////////////

/* This function is called by the MFC framework whenever a toolbar status is changing (is attached or detached to/from
 *  the frame). It is used as a hook to maintain this class internal state concerning the toolbar position and size.
 * It should not be called directly.	*/
void CMinMaxFrame::RecalcLayout(BOOL bNotify)
{
	CFrameWnd::RecalcLayout(bNotify);
	if(m_MinMaxLogic.m_tbPos != TBNOTCREATED) {
		if(!m_pTB->IsFloating()) {
			int newPos = FindDockSide();
			if(m_MinMaxLogic.m_tbPos != newPos) {
				m_MinMaxLogic.m_tbPos = newPos;
				m_MinMaxLogic.m_tbSize = GetTBSize(m_MinMaxLogic.m_tbPos);
				TriggerGetMinMaxInfoMsg();
			}
		}	 
		else {
			m_MinMaxLogic.m_tbPos = TBFLOAT;
			m_MinMaxLogic.m_tbSize = 0;
		}
	}
}

/////////////////////////////////////////////////////////////////////////////

void CMinMaxFrame::OnGetMinMaxInfo(MINMAXINFO FAR *lpMMI)
{
	m_MinMaxLogic.OnGetMinMaxInfo(lpMMI);
}

/////////////////////////////////////////////////////////////////////////////

BOOL CMinMaxFrame::OnBarCheck(UINT nID)
{
	BOOL res = CFrameWnd::OnBarCheck(nID);
	if(nID == ID_VIEW_STATUS_BAR) {
		m_MinMaxLogic.m_sbVisible = !m_MinMaxLogic.m_sbVisible;
		if(m_MinMaxLogic.m_sbVisible) {
			TriggerGetMinMaxInfoMsg();
		}
	}
	else if(nID == ID_VIEW_TOOLBAR) {
		m_MinMaxLogic.m_tbVisible = !m_MinMaxLogic.m_tbVisible;
		if(m_MinMaxLogic.m_tbVisible) {
			TriggerGetMinMaxInfoMsg();
		}
	}
	return res;
}

/////////////////////////////////////////////////////////////////////////////

void CMinMaxFrame::TriggerGetMinMaxInfoMsg()
{
	/* Trigger a WM_MINMAXINFO message by calling the function MoveWindow()	with the pCurrent frame size. The purpose of generating a call to the
	 * WM_GETMINMAXINFO Handler is to verify that the new client area size still respect the minimum size.     */
	RECT wRect;
	GetWindowRect(&wRect);
	MoveWindow(&wRect);
}

/////////////////////////////////////////////////////////////////////////////

#include "afxpriv.h"

/////////////////////////////////////////////////////////////////////////////

int CMinMaxFrame::FindDockSide()
{
static const DWORD dwDockBarMap[4] = {
		AFX_IDW_DOCKBAR_TOP,
		AFX_IDW_DOCKBAR_BOTTOM,
		AFX_IDW_DOCKBAR_LEFT,
		AFX_IDW_DOCKBAR_RIGHT
};
int res = TBFLOAT;

	for(int i = 0; i < 4; i++) {
		CDockBar *pDock = (CDockBar *)GetControlBar(dwDockBarMap[i]);
		if(pDock != NULL) {
			if(pDock->FindBar(m_pTB) != -1) {
				res = i;
				break;
			}
		}
	}
	return res;
}

/////////////////////////////////////////////////////////////////////////////

/* Purpose   : Returns the horizontal or the vertical toolbar size based on the	toolbar position.*/
int CMinMaxFrame::GetTBSize(int pos)
{
int res;
CSize cbSize = m_pTB->CalcFixedLayout(FALSE, (pos == TBTOP || pos == TBBOTTOM) ? TRUE : FALSE);

	if(pos == TBTOP || pos == TBBOTTOM) {
		res = cbSize.cy;
	}
	else {
		res = cbSize.cx;
	}
 	return res;
}
