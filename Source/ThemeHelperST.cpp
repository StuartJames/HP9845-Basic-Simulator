#include "stdafx.h"
#include "ThemeHelperST.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

CThemeHelperST::CThemeHelperST()
{	// Load DLL. This call will fails (and return NULL) if we are not running under Windows XP (or greater)
	m_hDll = ::LoadLibrary(_T("UxTheme.dll"));
}

CThemeHelperST::~CThemeHelperST()
{	// Unload DLL (if any)
	if (m_hDll)	::FreeLibrary(m_hDll);
	m_hDll = NULL;
}

LPVOID CThemeHelperST::GetProc(LPCSTR szProc, LPVOID pfnFail)
{
	LPVOID lpRet = pfnFail;
 	if (m_hDll)	lpRet = GetProcAddress(m_hDll, szProc);
	return lpRet;
}

HTHEME CThemeHelperST::OpenThemeData(HWND hwnd, LPCWSTR pszClassList)
{
	PFNOPENTHEMEDATA pfnOpenThemeData = (PFNOPENTHEMEDATA)GetProc("OpenThemeData", (LPVOID)OpenThemeDataFail);
	return (*pfnOpenThemeData)(hwnd, pszClassList);
}

HRESULT CThemeHelperST::CloseThemeData(HTHEME hTheme)
{
	PFNCLOSETHEMEDATA pfnCloseThemeData = (PFNCLOSETHEMEDATA)GetProc("CloseThemeData", (LPVOID)CloseThemeDataFail);
	return (*pfnCloseThemeData)(hTheme);
}

HRESULT CThemeHelperST::DrawThemeBackground(HTHEME hTheme, HWND hWnd, HDC hdc, int iPartId, int iStateId, const RECT* pRect, const RECT* pClipRect)
{
  PFNDRAWTHEMEPARENTBACKGROUND pfnDrawThemeParentBackground = (PFNDRAWTHEMEPARENTBACKGROUND)GetProc("DrawThemeParentBackground", NULL);
  if (pfnDrawThemeParentBackground && hWnd){    
	  (*pfnDrawThemeParentBackground)(hWnd, hdc, (PRECT)pRect);  
  }
  PFNDRAWTHEMEBACKGROUND pfnDrawThemeBackground = (PFNDRAWTHEMEBACKGROUND)GetProc("DrawThemeBackground", (LPVOID)DrawThemeBackgroundFail);
	return (*pfnDrawThemeBackground)(hTheme, hdc, iPartId, iStateId, pRect, pClipRect);
}

HRESULT CThemeHelperST::DrawThemeText(HTHEME hTheme, HDC hdc, int iPartId, int iStateId, LPCWSTR pszText, int iCharCount, DWORD dwTextFlags, DWORD dwTextFlags2, const RECT *pRect)
{
	PFNDRAWTHEMETEXT pfn = (PFNDRAWTHEMETEXT)GetProc("DrawThemeText", (LPVOID)DrawThemeTextFail);
	return (*pfn)(hTheme, hdc, iPartId, iStateId, pszText, iCharCount, dwTextFlags, dwTextFlags2, pRect);
}

BOOL CThemeHelperST::IsAppThemed()
{
	PFNISAPPTHEMED pfnIsAppThemed = (PFNISAPPTHEMED)GetProc("IsAppThemed", (LPVOID)IsAppThemedFail);
	return (*pfnIsAppThemed)();
}

BOOL CThemeHelperST::IsThemeActive()
{
	PFNISTHEMEACTIVE pfn = (PFNISTHEMEACTIVE)GetProc("IsThemeActive", (LPVOID)IsThemeActiveFail);
	return (*pfn)();
}
