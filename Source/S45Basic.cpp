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
#include "afxwinappex.h"
#include "afxdialogex.h"
#include "S45Basic.h"
#include "MainFrm.h"

#include "S45BasicDoc.h"
#include "S45BasicView.h"

#pragma comment(linker,"/manifestdependency:\"type='win32' name='microsoft.windows.Common-Controls' "\
	"version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

TXTREGN Regions[4] = {
	{60, 30, 60, 20, _T("Arial Black"), {-1, -1}, RGB(0, 0, 0), DT_VCENTER | DT_SINGLELINE, true},
{90, 65, 60, 20, _T("Lucida"), {-1, -1}, RGB(0, 0, 0), DT_VCENTER | DT_SINGLELINE, true},
{160, 15, 20, 20, _T("Tahoma"), {-1, -1}, RGB(0, 0, 0), DT_VCENTER | DT_SINGLELINE, true},
{25, 30, 60, 20, _T("Tahoma"),{-1, -1}, RGB(0, 0, 0), DT_VCENTER | DT_SINGLELINE, true }
};

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

const TCHAR szS45Registry[]		= _T("S45Basic");
const TCHAR szS45BasicClass[] = _T("S45BasicClass");

TCHAR szSettings[]			= _T("Settings");
static TCHAR szWindowPos[]		= _T("WindowPos");
static TCHAR szWFormat[]			= _T("%u,%u,%d,%d,%d,%d,%d,%d,%d,%d");


/////////////////////////////////////////////////////////////////////////////

CS45BasicApp theApp;

BEGIN_MESSAGE_MAP(CS45BasicApp, CWinApp)
	ON_COMMAND(ID_FILE_T14_SETUP, OnFileSetupT14)
	ON_COMMAND(ID_FILE_T15_SETUP, OnFileSetupT15)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////

CS45BasicApp::CS45BasicApp() noexcept
{
	m_dwRestartManagerSupportFlags = AFX_RESTART_MANAGER_SUPPORT_ALL_ASPECTS;								// support Restart Manager
#ifdef _MANAGED
	System::Windows::Forms::Application::SetUnhandledExceptionMode(System::Windows::Forms::UnhandledExceptionMode::ThrowException);
#endif
	SetAppID(_T("S45Basic.AppID.NoVersion"));
}

/////////////////////////////////////////////////////////////////////////////

BOOL CS45BasicApp::InitInstance()
{
INITCOMMONCONTROLSEX InitCtrls;
InitCtrls.dwSize = sizeof(InitCtrls);

	HWND hwnd = ::FindWindow (szS45BasicClass, NULL);
	if(hwnd){
		if (IsIconic(hwnd)) ShowWindow(hwnd, SW_RESTORE);
		SetForegroundWindow (hwnd);
		return FALSE;
	}
	InitCtrls.dwICC = ICC_WIN95_CLASSES;
	InitCommonControlsEx(&InitCtrls);
	CWinApp::InitInstance();
	EnableTaskbarInteraction(FALSE);
	AfxEnableControlContainer();
	EnableTaskbarInteraction(FALSE);
	SetRegistryKey(szS45Registry);
	LoadStdProfileSettings(4);																																// Load standard INI file options (including MRU)
	CSingleDocTemplate* pDocTemplate;
	pDocTemplate = new CSingleDocTemplate(IDR_MAINFRAME, RUNTIME_CLASS(CS45BasicDoc), RUNTIME_CLASS(CMainFrame), RUNTIME_CLASS(CS45BasicView));
	if(!pDocTemplate) return FALSE;
	AddDocTemplate(pDocTemplate);
	CCommandLineInfo cmdInfo;
	ParseCommandLine(cmdInfo);																																// Parse command line for standard shell commands, DDE, file open
	if(!ProcessShellCommand(cmdInfo))	return FALSE;
	m_nCmdShow = SW_SHOWNORMAL;
	((CMainFrame*)m_pMainWnd)->InitialShowWindow(m_nCmdShow);
	m_pMainWnd->UpdateWindow();
	((CMainFrame*)m_pMainWnd)->IntializeViews();																							// Show any previouse views
	m_pMainWnd->DragAcceptFiles();
	EnableShellOpen();
	RegisterShellFileTypes(TRUE);
	m_hDevNames = ((CMainFrame*)m_pMainWnd)->GetDocument()->GetPrinterDes(m_hDevNames);
	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////

BOOL CS45BasicApp::PreTranslateMessage(MSG* pMsg)
{
	return CWinApp::PreTranslateMessage(pMsg);
}

/////////////////////////////////////////////////////////////////////////////

int CS45BasicApp::ExitInstance()
{
	AfxOleTerm(FALSE);
	return CWinApp::ExitInstance();
}

/////////////////////////////////////////////////////////////////////////////

void CS45BasicApp::OnFilePrintSetup()
{
CPrintDialog pd(TRUE);

	CMainFrame* pWnd = (CMainFrame*)m_pMainWnd;
	m_hDevNames = pWnd->GetDocument()->GetPrinterDes(m_hDevNames);
	::GlobalUnlock(m_hDevNames);
	if(DoPrintDialog(&pd) == IDCANCEL){
	}
	pWnd->GetDocument()->SetPrinterDes((LPDEVNAMES)::GlobalLock(m_hDevNames));
	::GlobalUnlock(m_hDevNames);
}

/////////////////////////////////////////////////////////////////////////////

void CS45BasicApp::OnFileSetupT14()
{
	CMainFrame* pWnd = (CMainFrame*)m_pMainWnd;
	pWnd->SetTapeDevice(false);
}

/////////////////////////////////////////////////////////////////////////////

void CS45BasicApp::OnFileSetupT15()
{
	CMainFrame* pWnd = (CMainFrame*)m_pMainWnd;
	pWnd->SetTapeDevice(true);
}

/////////////////////////////////////////////////////////////////////////////

BOOL PASCAL NEAR ReadWindowPlacement(LPWINDOWPLACEMENT pwp)
{

  CString strBuffer = AfxGetApp()->GetProfileString(szSettings, szWindowPos);
  if (strBuffer.IsEmpty()) return FALSE;
  WINDOWPLACEMENT wp;
	int nRead = sscanf_s(strBuffer, szWFormat,
    &wp.flags, &wp.showCmd,
    &wp.ptMinPosition.x, &wp.ptMinPosition.y,
    &wp.ptMaxPosition.x, &wp.ptMaxPosition.y,
    &wp.rcNormalPosition.left, &wp.rcNormalPosition.top,
    &wp.rcNormalPosition.right, &wp.rcNormalPosition.bottom);
  if (nRead != 10) return FALSE;
  wp.length = sizeof wp;
  *pwp = wp;
  return TRUE;
}

/////////////////////////////////////////////////////////////////////////////

void PASCAL NEAR WriteWindowPlacement(LPWINDOWPLACEMENT pwp)
{
char szBuffer[sizeof("-32767") * 8 + sizeof("65535") * 2];

  sprintf_s(szBuffer, sizeof(szBuffer), szWFormat,
    pwp->flags, pwp->showCmd,
    pwp->ptMinPosition.x, pwp->ptMinPosition.y,
    pwp->ptMaxPosition.x, pwp->ptMaxPosition.y,
    pwp->rcNormalPosition.left, pwp->rcNormalPosition.top,
    pwp->rcNormalPosition.right, pwp->rcNormalPosition.bottom);
  AfxGetApp()->WriteProfileString(szSettings, szWindowPos, szBuffer);
}

/////////////////////////////////////////////////////////////////////////////


