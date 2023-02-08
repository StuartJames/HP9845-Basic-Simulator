// XButtonXP.cpp  Version 1.4
//
// Author:  Hans Dietrich
//          hdietrich@gmail.com
//
// Description:
//     XButtonXP implements CXButtonXP, an ownerdraw button that is XP
//     theme-aware and may be used as a pushbutton or a toggle button.
//     An icon and/or text may be displayed on the button.  Please see
//     article at www.codeproject.com.
//
//     This code is based partially on Ewan Ward's article
//     "Native Win32 Theme aware Owner-draw Controls without MFC"
//     http://www.codeproject.com/KB/buttons/nativewin32xpthemes.aspx
//
// History
//     Version 1.4 - 2008 January 25
//     - Added auto-repeat support.
//
//     Version 1.3 - 2008 January 19
//     - Fixed bug when button is released and mouse is moved, reported by 
//       rootdial.
//     - Fixed problem with drawing as default button, reported by fredwobus 
//       and programbyran.
//     - Fixed resource leak in DrawIcon(), reported by grin.
//     - Fixed transparency bug for toolbar style, reported by Pandele Florin.
//     - Fixed bug where WM_COMMAND was being sent twice when return pressed, 
//       reported by fredwobus.
//     - Implemented WM_MOUSELEAVE Handler, suggested by ksk, Roman Komary. 
//       fredwobus, 630596399 and EIEN.
//     - Added multi-line support, requested by jlatorref.
//     - Added grayscale support for disabled icon, requested by programbyran.
//     - Added support for custom text and background colors, requested by 
//       PatLeCat and Alex Evans.
//
//     Version 1.2 - 2005 April 20
//     - Fixed bug with hot state, reported by ksk
//     - Added SetDrawToolbar() and GetDrawToolbar(), suggested by Dominik Reichl
//
//     Version 1.1 - 2005 March 30
//     - Fixed bug in drawing toggle button, reported by Dominik Reichl
//     - Added SetToggleState()
//     - Removed #include "XButtonXPTest.h"
//
//     Version 1.0 - 2005 March 22
//     - Initial public release
//
// Public APIs:
//          NAME                              DESCRIPTION
//     ------------------    --------------------------------------------------
//     EnableTheming()       Enable visual themes for button
//     GetBackgroundColor()  Retrieve button custom background color 
//                           (XBUTTONXP_NO_COLOR if none) 
//     GetDrawToolbar()      Get toolbar style (TRUE = use toolbar style) 
//     GetTextColor()        Retrieve button custom text color 
//                           (XBUTTONXP_NO_COLOR if none) 
//     GetToggle()           Get toggle style (TRUE = button acts like toggle) 
//     GetToggleState()      Get toggle state (TRUE = button is pressed).
//     IsThemed()            Get theming state (TRUE = visual theme is being 
//                           used) 
//     SetBackgroundColor()  Set button custom background color. Default is 
//                           XBUTTONXP_NO_COLOR 
//     SetDrawToolbar()      Set toolbar style.  Default is FALSE 
//     SetIcon()             Set icon from resource id 
//     SetIcon()             Set icon from HICON 
//     SetIconAlignment()    Set icon alignment on button.  Default is LEFT 
//     SetTextColor()        Set button custom text color. Default is 
//                           XBUTTONXP_NO_COLOR 
//     SetToggle()           Set toggle style. Default is FALSE 
//     SetToggleState()      Set toggle state.  Default is FALSE 
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

#include "stdafx.h"
#include "XButtonXP.h"
#include "XThemeHelper.h"
#include "CreateGrayscaleIcon.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#ifndef __noop
#if _MSC_VER < 1300
#define __noop ((void)0)
#endif
#endif

#undef TRACE
#define TRACE __noop

//=============================================================================
// if you want to see the TRACE output, uncomment this line:
//#include "XTrace.h"
//=============================================================================


//=============================================================================	
// various defines that are needed
//=============================================================================	
#ifndef ODS_NOFOCUSRECT
#define ODS_NOFOCUSRECT     0x0200		// from WinUser.h
#endif

#ifndef GET_X_LPARAM
#define GET_X_LPARAM(lp)    ((int)(short)LOWORD(lp))
#define GET_Y_LPARAM(lp)    ((int)(short)HIWORD(lp))
#endif

#ifndef WM_THEMECHANGED
#define WM_THEMECHANGED     0x031A
#endif

constexpr auto REPEAT_INITIAL_DELAY_TIMER	 = 1;
constexpr auto REPEAT_DELAY_TIMER					 = 2;

//=============================================================================	
BEGIN_MESSAGE_MAP(CXButtonXP, COddButton)
//=============================================================================	
	ON_WM_MOUSEMOVE()
	ON_WM_ERASEBKGND()
	ON_WM_TIMER()
	ON_MESSAGE(WM_MOUSELEAVE, OnMouseLeave)
END_MESSAGE_MAP()

//=============================================================================	

CXButtonXP::CXButtonXP()
{
	m_bMouseOverButton    = FALSE;
	m_bFirstTime          = TRUE;
	m_ToggleEnable        = FALSE;
	m_IsToggled           = FALSE;
	m_bLButtonDown        = FALSE;
	m_bSent               = FALSE;
	m_bEnableTheming      = TRUE;
	m_bDrawToolbar        = FALSE;
	m_bRepeat             = FALSE;
	m_ButtonNotify				= FALSE;
	m_nInitialRepeatDelay = 500;
	m_nRepeatDelay        = 100;
	m_hTheme              = NULL;
	m_hIcon               = NULL;
	m_hGrayIcon           = NULL;
	m_eIconAlignment      = LEFT;
	m_pOldParentBitmap    = NULL;
	m_crBackground        = XBUTTONXP_NO_COLOR;
	m_crText              = XBUTTONXP_NO_COLOR;
}

//=============================================================================	

CXButtonXP::~CXButtonXP()
{
	if(m_hTheme) ThemeHelper.CloseThemeData(m_hTheme);
	m_hTheme = NULL;
	if(m_hIcon) DestroyIcon(m_hIcon);
	m_hIcon = NULL;
	if(m_hGrayIcon) DestroyIcon(m_hGrayIcon);
	m_hGrayIcon = NULL;
	if(m_pOldParentBitmap && m_dcParent.GetSafeHdc()) m_dcParent.SelectObject(m_pOldParentBitmap);
	m_pOldParentBitmap = NULL;
	if(m_dcParent.GetSafeHdc()) m_dcParent.DeleteDC();
	if(m_bmpParent.GetSafeHandle()) m_bmpParent.DeleteObject();
}


//=============================================================================	

BOOL CXButtonXP::IsThemed()
{
	BOOL bAppThemed = ThemeHelper.IsAppThemed();	// user can disable theming for app by right-clicking on exe, 
																								// then clicking on Properties | Compatibility | Disable visual themes
	return (m_hTheme && m_bEnableTheming && bAppThemed);
}

//=============================================================================	

CXButtonXP& CXButtonXP::SetIcon(UINT nIDResource, ICON_ALIGNMENT ia /*= LEFT*/)
{
	if(nIDResource != 0){
		HICON hIcon = (HICON) LoadImage(AfxGetInstanceHandle(),	MAKEINTRESOURCE(nIDResource), IMAGE_ICON,	0, 0,	0);
		return SetIcon(hIcon, ia);
	}
	if(m_hIcon)	DestroyIcon(m_hIcon);
	m_hIcon = NULL;
	if(m_hGrayIcon)	DestroyIcon(m_hGrayIcon);	
	m_hGrayIcon = NULL;
	RedrawWindow();
	return *this;
}

//=============================================================================	

CXButtonXP& CXButtonXP::SetIcon(HICON hIcon, ICON_ALIGNMENT ia /*= LEFT*/)
{
	if(m_hIcon)	DestroyIcon(m_hIcon);
	m_hIcon = hIcon;
	TRACE(_T("m_hIcon=%X\n"), m_hIcon);
	if(m_hGrayIcon)	DestroyIcon(m_hGrayIcon);	// create grayscale icon for disabled button
	m_hGrayIcon = NULL;
	if(hIcon) m_hGrayIcon = CreateGrayscaleIcon(hIcon);
	m_eIconAlignment = ia;
	RedrawWindow();
	return *this;
}


//=============================================================================	

CXButtonXP& CXButtonXP::SetIconAlignment(ICON_ALIGNMENT ia) 
{ 
	m_eIconAlignment = ia; 
	RedrawWindow();
	return *this;
}

//=============================================================================	

CXButtonXP& CXButtonXP::SetToggleState(BOOL bToggled) 
{ 
	m_IsToggled = bToggled; 
	RedrawWindow(); 
	return *this;
}
	
//=============================================================================	

BOOL CXButtonXP::ToggleState() 
{ 
	m_IsToggled = !m_IsToggled; 
	RedrawWindow(); 
	return m_IsToggled;
}
	
//=============================================================================	

CXButtonXP& CXButtonXP::EnableTheming(BOOL bEnable) 
{ 
	m_bEnableTheming = bEnable; 
	if(m_bEnableTheming){
		TRACE(_T("XButtonXP theming will be enabled only on XP and Vista ")
			_T("and only if the app is themed with a manifest file.\n"));
	}
	RedrawWindow();
	return *this;
}

//=============================================================================	

CXButtonXP& CXButtonXP::SetDrawToolbar(BOOL bDrawToolbar) 
{ 
	m_bDrawToolbar = bDrawToolbar; 
	m_bFirstTime = TRUE;	// force OpenThemeData() call
	RedrawWindow();
	return *this;
}

//=============================================================================	

CXButtonXP& CXButtonXP::SetToggle(BOOL bIsToggle) 
{
	if(m_ToggleEnable != bIsToggle){
		m_ToggleEnable = bIsToggle;
		m_IsToggled  = FALSE;
		if(m_ToggleEnable){
			ASSERT(!m_bRepeat);
		}
		RedrawWindow();
	}
	return *this;
}

//=============================================================================	

// times in milliseconds
CXButtonXP& CXButtonXP::SetRepeat(BOOL bRepeat, int nInitialDelay/*=500*/, int nRepeatDelay/*=100*/)
{
	m_bRepeat = bRepeat;
	m_nInitialRepeatDelay = nInitialDelay;
	m_nRepeatDelay = nRepeatDelay;
	if(m_bRepeat){
		ASSERT(m_nInitialRepeatDelay > 0);
		ASSERT(m_nRepeatDelay > 0);
		ASSERT(!m_ToggleEnable);
	}
	return *this; 
}

//=============================================================================	

void CXButtonXP::PrepareImageRect(BOOL bHasText, CRect& rectItem, CRect& rectText, BOOL bIsPressed, BOOL bIsThemed, DWORD dwWidth, DWORD dwHeight, CRect& rectImage)
{
	rectImage.CopyRect(&rectItem);
	if(bHasText){
		int nTextWidth = rectText.right - rectText.left;		// image must be placed just inside the focus rect
		if(m_eIconAlignment == RIGHT){
			rectText.right = nTextWidth - dwWidth - dwWidth / 2;
			rectText.left  = dwWidth / 2;
			rectImage.left = rectItem.right - dwWidth - dwWidth / 2;
		}
		else{	// icon is on the left
			rectText.right = nTextWidth - dwWidth / 2;
			rectText.left  = 2 * dwWidth;
			rectImage.left = dwWidth / 2;
		}
	}
	else{	// no text, center image horizontally
		int nImageWidth = rectImage.right - rectImage.left;
		rectImage.left += ((nImageWidth - dwWidth)/2);
	}
	int nImageHeight = rectImage.bottom - rectImage.top;	// center image vertically
	rectImage.top += ((nImageHeight - dwHeight) / 2);
	if(bIsPressed && !bIsThemed) rectImage.OffsetRect(1, 1); // if button is pressed then press image also
}

//=============================================================================	

void CXButtonXP::SaveParentBackground()
{
	GetWindowRect(&m_rectButton);
	CRect rectClient(m_rectButton);
	GetParent()->ScreenToClient(&rectClient);
	// repaint rect for button on parent's window without button, to get the original background of the parent
	BOOL bVisible = GetStyle() & WS_VISIBLE;
	ModifyStyle(WS_VISIBLE, 0);
	GetParent()->InvalidateRect(&rectClient);
	GetParent()->UpdateWindow();
	CRect rectParent; 	// get button position in parent window coordinates
	GetParent()->GetWindowRect(&rectParent);
	int left = m_rectButton.left - rectParent.left;
	int top = m_rectButton.top - rectParent.top;
	int w = m_rectButton.Width();
	int h = m_rectButton.Height();
	m_rectButton.left = left;
	m_rectButton.top  = top;
	m_rectButton.right = left + w;
	m_rectButton.bottom  = top + h;
//	TRACERECT(m_rectButton);
	if(m_pOldParentBitmap) m_dcParent.SelectObject(m_pOldParentBitmap);
	m_pOldParentBitmap = NULL;
	if(m_dcParent.GetSafeHdc())	m_dcParent.DeleteDC();
	if(m_bmpParent.GetSafeHandle())	m_bmpParent.DeleteObject();
	CDC *pdcParent = GetParent()->GetWindowDC();
	ASSERT(pdcParent);
	if(pdcParent){	// make a copy of the bits in the parent's window that will be covered by this button
		m_dcParent.CreateCompatibleDC(pdcParent);
		m_bmpParent.CreateCompatibleBitmap(pdcParent, m_rectButton.Width(), m_rectButton.Height());
		m_pOldParentBitmap = m_dcParent.SelectObject(&m_bmpParent);
		m_dcParent.BitBlt(0, 0, m_rectButton.Width(), m_rectButton.Height(), pdcParent, m_rectButton.left, m_rectButton.top, SRCCOPY);
		ReleaseDC(pdcParent);
	}
	if(bVisible){	// restore visibility
		ModifyStyle(0, WS_VISIBLE);
		GetParent()->UpdateWindow();
	}
}

//=============================================================================	

void CXButtonXP::DrawIcon(CDC *pDC, BOOL bHasText, CRect& rectItem, CRect& rectText, BOOL bIsPressed, BOOL bIsThemed, BOOL bIsDisabled)
{
	if(m_hIcon){	// first get size of icon
		DWORD dwWidth = 32;		// assume 32x32
		DWORD dwHeight = 32;
		ICONINFO iconinfo;
		if(GetIconInfo(m_hIcon, &iconinfo)){
			CBitmap* pBitmap = CBitmap::FromHandle(iconinfo.hbmColor);
			if(pBitmap){
				BITMAP bm;
				pBitmap->GetBitmap(&bm);
				dwWidth = bm.bmWidth;
				dwHeight = bm.bmHeight;
			}
			if(iconinfo.hbmColor) ::DeleteObject(iconinfo.hbmColor);
			if(iconinfo.hbmMask) ::DeleteObject(iconinfo.hbmMask);
		}
		CRect rectImage(rectItem);
		PrepareImageRect(bHasText, rectItem, rectText, bIsPressed, bIsThemed,	dwWidth, dwHeight, rectImage);
		HICON hIcon = m_hIcon;
		UINT nFlags = bIsDisabled ? DSS_DISABLED : DSS_NORMAL;
		if(bIsDisabled && m_hGrayIcon){
			hIcon = m_hGrayIcon;
			nFlags = DSS_NORMAL;
		}
		nFlags |= DST_ICON;
		pDC->DrawState(CPoint(rectImage.left, rectImage.top), CSize(rectImage.right - rectImage.left, rectImage.bottom - rectImage.top), hIcon, nFlags, (CBrush *) NULL);
	}
}

//=============================================================================	

void CXButtonXP::DrawText(CDC *pDC, LPCTSTR lpszText, CRect& rect, BOOL bIsPressed, BOOL bIsThemed, BOOL bIsDisabled)
{
	ASSERT(pDC);
	ASSERT(lpszText);
	CRect rectDraw(rect);
	if(bIsPressed && !bIsThemed) rectDraw.OffsetRect(1, 1); // if button is pressed then "press" title also
	UINT uTextAlignment = DT_LEFT;
	if(m_hIcon == NULL) uTextAlignment = DT_CENTER;
	uTextAlignment |= DT_VCENTER;
	uTextAlignment |= (GetStyle() & BS_MULTILINE) ? DT_WORDBREAK : DT_SINGLELINE;
	CRect rectText = rectDraw;	// center text vertically (DT_VCENTER does not work if BS_MULTILINE is set)
	pDC->DrawText(lpszText, -1, &rectDraw, uTextAlignment | DT_CALCRECT);
	rectDraw.OffsetRect((rectText.Width() - rectDraw.Width()) / 2, (rectText.Height() - rectDraw.Height()) / 2);
	if(m_bDrawToolbar || !bIsThemed){
		if(bIsDisabled){
			rectDraw.OffsetRect(1, 1);
			pDC->SetTextColor(::GetSysColor(COLOR_3DHILIGHT));
			pDC->DrawText(lpszText, -1, &rectDraw, uTextAlignment);
			rectDraw.OffsetRect(-1, -1);
			pDC->SetTextColor(::GetSysColor(COLOR_3DSHADOW));
			pDC->DrawText(lpszText, -1, &rectDraw, uTextAlignment);
		}
		else{
			if(m_crText == XBUTTONXP_NO_COLOR) pDC->SetTextColor(::GetSysColor(COLOR_BTNTEXT));
			else pDC->SetTextColor(m_crText);
			pDC->DrawText(lpszText, -1, &rectDraw, uTextAlignment);
		}
	}
	else{ //if (bIsThemed)
		ThemeHelper.DrawThemeText(m_hTheme, pDC->m_hDC, BP_PUSHBUTTON, bIsDisabled ? PBS_DISABLED : PBS_NORMAL,	lpszText, uTextAlignment, 0, &rectDraw);
	}
}

//=============================================================================	
void CXButtonXP::DrawItem(LPDRAWITEMSTRUCT lpDIS)
{
	CDC *pDC = CDC::FromHandle(lpDIS->hDC);
	CRect rectItem = lpDIS->rcItem;
	CDC memDC;	// set up for double buffering
	memDC.CreateCompatibleDC(pDC);
	CBitmap bmp;
	bmp.CreateCompatibleBitmap(pDC, rectItem.Width(), rectItem.Height());
	CBitmap *pOldBitmap = memDC.SelectObject(&bmp);
	memDC.BitBlt(0, 0, rectItem.Width(), rectItem.Height(),	pDC, 0, 0, SRCCOPY);	// initialize dc from existing dc
	memDC.SetBkMode(TRANSPARENT);
	if(m_bFirstTime){
		m_bFirstTime = FALSE;	// open theme for Button
		if(ThemeHelper.IsThemeLibAvailable()){
			if(m_hTheme)	ThemeHelper.CloseThemeData(m_hTheme);
			m_hTheme = NULL;
			if(m_bDrawToolbar){
				m_hTheme = ThemeHelper.OpenThemeData(m_hWnd, _T("Toolbar"));
			}
			else{
				m_hTheme = ThemeHelper.OpenThemeData(m_hWnd, _T("Button"));
			}
		}
		SaveParentBackground();
	}
	CFont *pFont = GetParent()->GetFont();
	if(pFont)	memDC.SelectObject(pFont);
	CString strTitle = _T("");
	GetWindowText(strTitle);							// get the button's title
	BOOL bHasText       = !strTitle.IsEmpty();
	BOOL bIsFocused     = lpDIS->itemState & ODS_FOCUS;
	BOOL bIsDisabled    = lpDIS->itemState & ODS_DISABLED;
	BOOL bDrawFocusRect = !(lpDIS->itemState & ODS_NOFOCUSRECT);
	BOOL bIsPressed     = lpDIS->itemState & ODS_SELECTED;
	BOOL bIsDefault     = IsDefault();
	BOOL bIsThemed      = IsThemed();
	BOOL bCustomColors  = (m_crBackground != XBUTTONXP_NO_COLOR) || (m_crText != XBUTTONXP_NO_COLOR);
	if(bCustomColors){
		bIsThemed = FALSE;
	}
	if(m_ToggleEnable){
		if(m_bLButtonDown && !m_bMouseOverButton){			// left mouse button is down outside button
			bIsPressed = !m_IsToggled;
		}
		else{
			bIsPressed = m_IsToggled;
		}
	}
	if(m_bDrawToolbar){		// restore parent window bits
		memDC.BitBlt(0, 0, m_rectButton.Width(), m_rectButton.Height(),	&m_dcParent, 0, 0, SRCCOPY);
		bIsFocused = FALSE;
		bIsDefault = FALSE;
	}
	if(bIsThemed){
		DWORD state = bIsPressed ? PBS_PRESSED : PBS_NORMAL;
		if(bIsDisabled){
			state = PBS_DISABLED;
		}
		else if(state == PBS_NORMAL){
			if(bIsFocused || bIsDefault){
				state = PBS_DEFAULTED;
			}
			if(m_bMouseOverButton) state = PBS_HOT;
		}
		ThemeHelper.DrawThemeParentBackground(::GetParent(m_hWnd), memDC.m_hDC, &rectItem);
		ThemeHelper.DrawThemeBackground(m_hTheme, memDC.m_hDC, BP_PUSHBUTTON, state, &rectItem, NULL);
	}
	else{	// not themed
		CBrush brBackground;
		if(m_crBackground == XBUTTONXP_NO_COLOR) brBackground.CreateSolidBrush(GetSysColor(COLOR_BTNFACE));
		else brBackground.CreateSolidBrush(m_crBackground);
		if(!m_bDrawToolbar || bCustomColors){	// fill in background, if custom color or not toolbar
			CRect rectClient(rectItem);
			InflateRect(&rectClient, -1, -1);
			memDC.FillRect(&rectClient, &brBackground);
		}
		if(bIsFocused || bIsDefault){
			if(!m_ToggleEnable || !bIsPressed){
				CBrush brush(RGB(0,0,0));
				memDC.FrameRect(&rectItem, &brush);
			}
			InflateRect(&rectItem, -1, -1);
		}
		if(bIsPressed){		// draw the traditional pushbutton edge
			memDC.DrawEdge(&rectItem, EDGE_SUNKEN, BF_RECT);
		}
		else if(!m_bDrawToolbar){
			memDC.DrawEdge(&rectItem, EDGE_RAISED, BF_RECT);
		}
	}
	CRect rectDraw = lpDIS->rcItem;
	DrawIcon(&memDC, bHasText, rectItem, rectDraw, bIsPressed, bIsThemed, bIsDisabled);	// draw the icon
	if(bHasText){	// draw the button title (if any)
		DrawText(&memDC, strTitle, rectDraw, bIsPressed, bIsThemed, bIsDisabled);
	}
	if(bIsFocused && bDrawFocusRect){	// draw the focus rect
		CRect rectFocus = rectItem;
		rectFocus.InflateRect(-3, -3);
		memDC.DrawFocusRect(&rectFocus);
	}
	pDC->BitBlt(0, 0, rectItem.Width(), rectItem.Height(), &memDC, 0, 0, SRCCOPY);		// end double buffering
	if(pOldBitmap) memDC.SelectObject(pOldBitmap);
	if(bmp.GetSafeHandle())	bmp.DeleteObject();
	memDC.DeleteDC();
}

//=============================================================================	

LRESULT CXButtonXP::DefWindowProc(UINT message, WPARAM wParam, LPARAM lParam)
{
	switch(message){
		case WM_GETDLGCODE:{
			if(lParam){
				MSG *pMsg = (MSG *) lParam;
				if(pMsg){
					if((pMsg->message == WM_KEYDOWN) && (pMsg->wParam == VK_TAB)){// don't interfere with tab processing
						return 0;
					}
				}
			}
			return DLGC_WANTALLKEYS;
			break;
		}
		case WM_KEYDOWN:	// ignore repeat messages if key is being held down
			if(lParam & 0x40000000)	return 0;
			if((wParam == VK_SPACE) || (wParam == VK_RETURN)){
				SendMessage(WM_LBUTTONDOWN, 0, MAKELPARAM(1, 1));				// convert to left button click
			}
			break;
		case WM_KEYUP:
			if((wParam == VK_SPACE) || (wParam == VK_RETURN)){
				SendMessage(WM_LBUTTONUP, 0, MAKELPARAM(1, 1));				// convert to left button click
			}
			break;
		case WM_LBUTTONUP:{
			m_bLButtonDown = FALSE;
			KillTimer(REPEAT_INITIAL_DELAY_TIMER);
			KillTimer(REPEAT_DELAY_TIMER);
			if(m_bSent){
				SendMessage(BM_SETSTATE, 0);
				m_bSent = FALSE;
				CWnd* pParent = GetParent();
				if(pParent != NULL){
					::SendMessage(pParent->m_hWnd, WM_COMMAND, (BN_CLICKED << 16) | this->GetDlgCtrlID(),	(LPARAM)this->m_hWnd);
				}
			}
			if(m_ToggleEnable){	// if button is toggled and mouse button is released outside of button, ignore the mouse click and restore button state
				CPoint point(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
				ClientToScreen(&point);
				CRect rectButton;
				GetWindowRect(&rectButton);
				if(!rectButton.PtInRect(point)){
					m_IsToggled = !m_IsToggled;
					InvalidateRect(NULL);
				}
			}
			if(m_ButtonNotify){
				CWnd *pWndParent = GetParent();
				if(pWndParent != NULL) pWndParent->PostMessage(UM_MOUSEBUTTONNOTIFY, MAKEWPARAM(this->GetDlgCtrlID(), 0), (LPARAM)WM_LBUTTONUP);
			}
			break;
		}
		case WM_LBUTTONDOWN:{
 			m_bLButtonDown = TRUE;
			if(m_ToggleEnable){
				m_IsToggled = !m_IsToggled;
				InvalidateRect(NULL);
			}
			if(m_bRepeat)	SetTimer(REPEAT_INITIAL_DELAY_TIMER, m_nInitialRepeatDelay, NULL);
			if(m_ButtonNotify){
				CWnd *pWndParent = GetParent();
				if(pWndParent != NULL) pWndParent->PostMessage(UM_MOUSEBUTTONNOTIFY, MAKEWPARAM(this->GetDlgCtrlID(), 0), (LPARAM)WM_LBUTTONDOWN);
			}
			break;
		}
		case WM_LBUTTONDBLCLK:{
			if(m_ToggleEnable){		// for toggle buttons, treat doubleclick as singleclick
				SendMessage(BM_SETSTATE, m_IsToggled);
			}
			else{
				SendMessage(BM_SETSTATE, 1);
				m_bSent = TRUE;
			}
			return 0;
		}
		case WM_THEMECHANGED:{
			if(IsThemed()){
				if(m_hTheme){				// when user changes themes, close pCurrent theme and re-open
					ThemeHelper.CloseThemeData(m_hTheme);
					m_hTheme = NULL;
					m_bFirstTime = TRUE;
				}
			}
			break;
		}
	}
	return COddButton::DefWindowProc(message, wParam, lParam);
}

//=============================================================================	

void CXButtonXP::OnMouseMove(UINT nFlags, CPoint point)
{
	CPoint pt(point);
	ClientToScreen(&pt);	// check to see if mouse is over button
	CRect rect;
	GetWindowRect(&rect);
	BOOL bMouseOverButton = m_bMouseOverButton;
	if(rect.PtInRect(pt))	m_bMouseOverButton = TRUE;
	else m_bMouseOverButton = FALSE;
	if(bMouseOverButton != m_bMouseOverButton){
		InvalidateRect(NULL);
		if(m_bMouseOverButton){			// mouse is now over button
			TRACKMOUSEEVENT tme;
			tme.cbSize    = sizeof(tme);
			tme.dwFlags   = TME_LEAVE;
			tme.hwndTrack = m_hWnd;
			::_TrackMouseEvent(&tme);
		}
	}
	//TRACE(_T("in CXButtonXP::OnMouseMove:  m_bMouseOverButton=%d\n"), m_bMouseOverButton);
	COddButton::OnMouseMove(nFlags, point);
}

//=============================================================================	

LRESULT CXButtonXP::OnMouseLeave(WPARAM /*wParam*/, LPARAM /*lParam*/)
{
	TRACE(_T("in CXButtonXP::OnMouseLeave\n"));
	m_bMouseOverButton = FALSE;
	RedrawWindow();
	return 0;
}

//=============================================================================	

BOOL CXButtonXP::OnEraseBkgnd(CDC* /*pDC*/)
{
	return TRUE;
}

//=============================================================================	

void CXButtonXP::OnTimer(UINT nIDEvent) 
{
	if(nIDEvent == REPEAT_INITIAL_DELAY_TIMER){
		KillTimer(nIDEvent);
		SetTimer(REPEAT_DELAY_TIMER, m_nRepeatDelay, NULL);
	}
	else if(nIDEvent == REPEAT_DELAY_TIMER){
		if(m_bRepeat && m_bMouseOverButton){
			CWnd* pParent = GetParent();
			if(pParent != NULL){
				::SendMessage(pParent->m_hWnd, WM_COMMAND, (BN_CLICKED << 16) | this->GetDlgCtrlID(),	(LPARAM)this->m_hWnd);
			}
		}
		else if(!m_bRepeat){
			KillTimer(nIDEvent);
		}
	}
	COddButton::OnTimer(nIDEvent);
}
