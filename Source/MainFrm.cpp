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
#include "S45Basic.h"

#include "MainFrm.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

static TCHAR  szMainName[]				= _T("S45");
static TCHAR  szFullVersion[]			= _T("%d.%d.%d");
static TCHAR  szVersionThree[]		= _T("%d.%d.%d");
static TCHAR  szVersionTwo[]			= _T("%d.%d");
static TCHAR  szVersionOne[]			= _T("%d");

static TCHAR	AppPath[]						= _T("\\S45Basic\\");										// points to apps subdir in user document area

extern TCHAR  szSettings[];
static TCHAR  szSystemVariables[] = _T("SystemVariables");
static TCHAR  szSystemFormat[]		= _T("%s\n,%s\n");

/////////////////////////////////////////////////////////////////////////////

enum RunCommands {
	RC_RUN,
	RC_CONT,
	RC_STEP,
	RC_EXECUTE,
	RC_LOAD,
	RC_SAVE,
	RC_SCRATCH
};

/////////////////////////////////////////////////////////////////////////////

char CMainFrame::m_SoftKeysTitles[2][SOFT_KEY_COUNT][20] = {
	{_T("k0"), _T("k1"), _T("k2"), _T("k3"), _T("k4"), _T("k5"), _T("k6"), _T("k7"), _T("k8"), _T("k9")},
	{_T("k10"), _T("k11"), _T("k12"), _T("k13"), _T("k14"), _T("k15"), _T("k16"), _T("k17"), _T("k18"), _T("k19")}
};

char CMainFrame::m_ControlKeysTitles[SYS_MODE_COUNT][CONTROL_KEY_COUNT][20] = {
		{_T("  RECALL  "), _T("STORE"), _T("RUN"), _T("STEP"), _T("RESULT"), _T("EDIT"), _T("LIST"), _T(""), _T("CLEAR"), _T("SCRATCH")},	// IDLE
		{_T("  RECALL  "), _T("STORE"), _T("PAUSE"), _T(""), _T(""), _T(""), _T(""), _T(""), _T(""), _T("STOP")},													// RUN
		{_T("  RECALL  "), _T("STORE"), _T("CONT"),_T("STEP"), _T("RESULT"), _T(""), _T(""), _T(""), _T("CLEAR"), _T("STOP")},						// PAUSE
		{_T("  RECALL  "), _T("INS LINE"), _T("DEL LINE"), _T("INS CHAR"), _T(""), _T(""), _T(""), _T(""), _T(""), _T("EXIT")},						// EDIT LINE
		{_T(""), _T(""), _T(""), _T(""), _T(""), _T(""), _T(""), _T(""), _T(""), _T("STOP")},																							// EDIT KEY
};

/////////////////////////////////////////////////////////////////////////////

IMPLEMENT_DYNCREATE(CMainFrame, CMinMaxFrame)


BEGIN_MESSAGE_MAP(CMainFrame, CMinMaxFrame)
	ON_WM_CREATE()
	ON_WM_CLOSE()
	ON_MESSAGE(WM_BASICNOTIFY, OnBasicNotify)
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////

CMainFrame::CMainFrame() : CMinMaxFrame(CLIENTMINSIZEX, CLIENTMINSIZEY) 
{
	m_pDoc = NULL;
	m_pView = NULL;
	m_T14Path = _T(".\\T14\\");
	m_T15Path = _T(".\\T15\\");
}

/////////////////////////////////////////////////////////////////////////////

CMainFrame::~CMainFrame()
{
	if(m_pProductName != nullptr) delete m_pProductName;
	if(m_pCompanyName != nullptr) delete m_pCompanyName;
	if(m_pVersion != nullptr) delete m_pVersion;
	if(m_pCopyright != nullptr) delete m_pCopyright;
}

/////////////////////////////////////////////////////////////////////////////

BOOL CMainFrame::PreCreateWindow(CREATESTRUCT& cs)
{
	WNDCLASS wndcls;

	cs.style &= ~(LONG)(FWS_ADDTOTITLE);														// we provide the title
	BOOL bRes = CMinMaxFrame::PreCreateWindow(cs);
	HINSTANCE hInst = AfxGetInstanceHandle();
	if(!::GetClassInfo(hInst, szS45BasicClass, &wndcls)){						// see if the class already exists
		::GetClassInfo(hInst, cs.lpszClass, &wndcls);									// get default stuff
		wndcls.style &= ~(CS_HREDRAW | CS_VREDRAW);
		wndcls.lpszClassName = szS45BasicClass;												// register a new class
		wndcls.hIcon = ::LoadIcon(hInst, MAKEINTRESOURCE(IDR_MAINFRAME));
		ASSERT(wndcls.hIcon != NULL);
		if(!AfxRegisterClass(&wndcls)) AfxThrowResourceException();
	}
	cs.lpszClass = szS45BasicClass;
	return TRUE;
}


/////////////////////////////////////////////////////////////////////////////

int CMainFrame::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
WINDOWPLACEMENT wp;
CRect temp;
CString cstr;

	if(CMinMaxFrame::OnCreate(lpCreateStruct) == -1) return -1;
  if(ReadWindowPlacement(&wp)) SetWindowPlacement(&wp);
  if(!m_FunctionBar.Create(this, IDR_FUNCTIONBAR, CBRS_BOTTOM | CBRS_TOOLTIPS | CBRS_FLYBY, IDR_FUNCTIONBAR)){
    TRACE0("Failed to create functionbar\n");
    return -1;
  }
	m_FunctionBar.EnableDocking(CBRS_ALIGN_ANY);
	EnableDocking(CBRS_ALIGN_ANY);
	DockControlBar(&m_FunctionBar);
	SetSoftKeys();
	SetControlKeys();
	SetToolBar(&m_FunctionBar);
	cstr.LoadString(IDS_SYSTEMTITLE);
	SetWindowText(cstr);																											// set the name of the main window
	SetTapeDirectories();
	if((m_pBasicThread = AfxBeginThread(BasicLoop, this, THREAD_PRIORITY_NORMAL, 0, CREATE_SUSPENDED, NULL)) == NULL) return -1; // create suspended
	return 0;
}


/////////////////////////////////////////////////////////////////////////////

BOOL CMainFrame::PreTranslateMessage(MSG* pMsg) 
{
	return CMinMaxFrame::PreTranslateMessage(pMsg);
}

/////////////////////////////////////////////////////////////////////////////

void CMainFrame::InitialShowWindow(UINT nCmdShow)
{
}

/////////////////////////////////////////////////////////////////////////////

void CMainFrame::IntializeViews(void) 
{
	if((m_pDoc = GetDocument()) == NULL) return;
	m_pDoc->InitialiseDocument();
	m_pView = m_pDoc->GetSystemView();
	GetSystemVars();
	GetAppStrings();
	m_pDoc->PrintFormatAt(SYS_COMMENT, _T("BASIC READY %s"), m_pVersion);
  m_pBasicThread->ResumeThread();																					// it's now safe to start the thread
}

/////////////////////////////////////////////////////////////////////////////

void CMainFrame::OnClose() 
{
WINDOWPLACEMENT wp;

	SaveSystemVars();
	wp.length = sizeof wp;
	if(GetWindowPlacement(&wp)){
		wp.flags = 0;
		if(IsZoomed()) wp.flags |= WPF_RESTORETOMAXIMIZED;
		WriteWindowPlacement(&wp);
	}
	CFrameWnd::OnClose();
}

/////////////////////////////////////////////////////////////////////////////

void CMainFrame::SetControlKeys(int SystemMode /*= 0*/) 
{
	if(!IsWindow(m_FunctionBar.m_hWnd)) return;
	for(int j = SOFT_KEY_COUNT, i = 0; i < CONTROL_KEY_COUNT; ++j, ++i) m_FunctionBar.SetButtonText(j, m_ControlKeysTitles[SystemMode][i]);
	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if(SystemMode != SYS_IDLE) pSysMenu->EnableMenuItem(SC_CLOSE, MF_BYCOMMAND | MF_GRAYED);
	else pSysMenu->EnableMenuItem(SC_CLOSE, MF_BYCOMMAND | MF_ENABLED);
}

/////////////////////////////////////////////////////////////////////////////

void CMainFrame::SetSoftKeys(bool Shift) 
{
	for(int i = 0; i < SOFT_KEY_COUNT; ++i) m_FunctionBar.SetButtonText(i, m_SoftKeysTitles[Shift == true][i]);
}

/////////////////////////////////////////////////////////////////////////////

void CMainFrame::SetWindowTitle(LPCSTR pStr) 
{
CString cstr;

	cstr.LoadString(IDS_SYSTEMTITLE);
	if(pStr != nullptr){
		cstr += _T(" - ");
		cstr += pStr;
	}
	SetWindowText(cstr);																										// set the name of the main window
}

/////////////////////////////////////////////////////////////////////////////

void CMainFrame::OnSetFocus(CWnd* pOldWnd)
{
	m_pView->SetFocus();																										// forward focus to the view window
}

/////////////////////////////////////////////////////////////////////////////

BOOL CMainFrame::OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo)
{
	if(m_pView != NULL) if(m_pView->OnCmdMsg(nID, nCode, pExtra, pHandlerInfo))	return TRUE;	// let the view have first crack at the command
	return CMinMaxFrame::OnCmdMsg(nID, nCode, pExtra, pHandlerInfo);
}

/////////////////////////////////////////////////////////////////////////////

void CMainFrame::GetSystemVars()
{
int nRead = 0;
char T14Folder[_MAX_PATH] = {0}, T15Folder[_MAX_PATH] = {0};

	CString strBuffer = AfxGetApp()->GetProfileString(szSettings, szSystemVariables);
	if(!strBuffer.IsEmpty()){
		nRead = sscanf_s(strBuffer, szSystemFormat, T14Folder, _MAX_PATH, T15Folder, _MAX_PATH);
	}
	if(nRead == 2){
		T14Folder[_MAX_PATH - 1] = 0;
		T15Folder[_MAX_PATH - 1] = 0;
//		SetTapeFolder(T14Folder, false);
//		SetTapeFolder(T15Folder, true);
	}
}

/////////////////////////////////////////////////////////////////////////////

void CMainFrame::SaveSystemVars()
{
char szBuffer[_MAX_PATH * 2 + 10];

	if(sprintf_s(szBuffer, _MAX_PATH * 2 + 10, szSystemFormat, m_T14Path, m_T15Path) == -1){
		TRACE("Envoronment Save Error %d \n",errno);
	}
	AfxGetApp()->WriteProfileString(szSettings, szSystemVariables, szBuffer);		
}

/////////////////////////////////////////////////////////////////////////////

// CMainFrame diagnostics
#ifdef _DEBUG
void CMainFrame::AssertValid() const
{
	CMinMaxFrame::AssertValid();
}


void CMainFrame::Dump(CDumpContext& dc) const
{
	CMinMaxFrame::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////

CS45BasicDoc* CMainFrame::GetDocument() 
{ 
	if(m_pDoc == NULL){
		CView* pView = GetActiveView();  
		if(pView){
			CS45BasicDoc* pDoc = (CS45BasicDoc*) pView->GetDocument();
			if(pDoc != NULL){
				m_pDoc = pDoc;
				return pDoc;
			}
		}
	}
	return m_pDoc;
}

/////////////////////////////////////////////////////////////////////////////

void CMainFrame::GetAppStrings()
{	
DWORD dwHandle;							// ignored 
char *buf;									// pointer to buffer to receive file-version info.
LPVOID lplpBuffer;
UINT Length;
struct LANGANDCODEPAGE {
	WORD wLanguage;
	WORD wCodePage;
} *lpTranslate;
char SubBlock[51];
char moduleName[SMAX_PATH];

	::GetModuleFileName(NULL, moduleName, SMAX_PATH); 										// get name of executable
	DWORD verSize = GetFileVersionInfoSize(moduleName, &dwHandle);				// Get the size of the version information.
	if(verSize != 0){
		buf = new char[verSize + 1];
		BOOL res = GetFileVersionInfo(moduleName,	NULL, verSize, buf);
		ASSERT(res);
		VerQueryValue(buf, TEXT("\\VarFileInfo\\Translation"), (LPVOID*)&lpTranslate, &Length);  // get the language and code page
		sprintf_s(SubBlock, 50, TEXT("\\StringFileInfo\\%04x%04x\\ProductName"), lpTranslate[0].wLanguage, lpTranslate[0].wCodePage); 
		if(VerQueryValue(buf, SubBlock, &lplpBuffer, &Length)){
			m_pProductName = new TCHAR[Length + 1];
			strcpy_s(m_pProductName, Length + 1, (char*)lplpBuffer);
		}
		sprintf_s(SubBlock, 50, TEXT("\\StringFileInfo\\%04x%04x\\CompanyName"), lpTranslate[0].wLanguage, lpTranslate[0].wCodePage); 
		if(VerQueryValue(buf, SubBlock, &lplpBuffer, &Length)){
			m_pCompanyName = new TCHAR[Length + 1];
			strcpy_s(m_pCompanyName, Length + 1, (char*)lplpBuffer);
		}
		sprintf_s(SubBlock, 50, TEXT("\\StringFileInfo\\%04x%04x\\LegalCopyright"), lpTranslate[0].wLanguage, lpTranslate[0].wCodePage); 
		if(VerQueryValue(buf, SubBlock, &lplpBuffer, &Length)){
			m_pCopyright = new TCHAR[Length + 1];
			strcpy_s(m_pCopyright, Length + 1, (char*)lplpBuffer);
		}
		sprintf_s(SubBlock, 50, TEXT("\\StringFileInfo\\%04x%04x\\ProductVersion"), lpTranslate[0].wLanguage, lpTranslate[0].wCodePage); 
		if(VerQueryValue(buf, SubBlock, &lplpBuffer, &Length)){
			char numberString[VERSION_BUFF_SIZE];
      int versionNumbers[4] = {0, 0, 0, 0};
      char* ptr = strchr((char*)lplpBuffer, L'.');
      while(ptr != NULL) {
        *ptr = ','; // change dp to comma
        ptr = strchr((char*)lplpBuffer, L'.');
      }
      int numberOfValues = sscanf_s((char*)lplpBuffer, _T("%d,%d,%d,%d"), &versionNumbers[0], &versionNumbers[1], &versionNumbers[2], &versionNumbers[3]);
			if(versionNumbers[3] != 0) sprintf_s(numberString, VERSION_BUFF_SIZE, szFullVersion, versionNumbers[0], versionNumbers[1], versionNumbers[2]);
			else if(versionNumbers[2] != 0)	sprintf_s(numberString, VERSION_BUFF_SIZE, szVersionThree, versionNumbers[0], versionNumbers[1], versionNumbers[2]);
  		else if(versionNumbers[1] != 0)	sprintf_s(numberString, VERSION_BUFF_SIZE, szVersionTwo,  versionNumbers[0], versionNumbers[1]);
			else sprintf_s(numberString, VERSION_BUFF_SIZE, szVersionOne, versionNumbers[0]);
			m_pVersion = new char[strnlen(numberString, 50) + 1];
		  strcpy_s(m_pVersion, strnlen(numberString, 50) + 1, numberString);
		}
		delete [] buf;
	}
}

/////////////////////////////////////////////////////////////////////////////

void CMainFrame::SetTapeDirectories()
{
LPSECURITY_ATTRIBUTES lpSecurityAttributes = {0};
CString Path;

  TCHAR* pstr = Path.GetBufferSetLength(MAX_PATH);													// setup T14 & T15 default directories
  HRESULT Res = SHGetFolderPath(NULL, CSIDL_PERSONAL, NULL, SHGFP_TYPE_CURRENT, pstr);
  Path.ReleaseBuffer();
  if(Res == S_OK){																													// if this fails T14 & T15 default to local app directory
    Path.Append(AppPath);
 		if(!CreateDirectory(Path, lpSecurityAttributes) && (GetLastError() != ERROR_ALREADY_EXISTS)){
			TRACE(_T("Error: create path '%s' failed.\n"), (LPCTSTR)Path);
		}
		else{
			CString Tpath = Path + _T("T14\\");
			if(!CreateDirectory(Tpath, lpSecurityAttributes) && (GetLastError() != ERROR_ALREADY_EXISTS)) TRACE(_T("Error: create path '%s' failed.\n"), (LPCTSTR)Tpath);
		 	else m_T14Path = Tpath;
			Tpath = Path + _T("T15\\");
			if(!CreateDirectory(Tpath, lpSecurityAttributes) && (GetLastError() != ERROR_ALREADY_EXISTS)) TRACE(_T("Error: create path '%s' failed.\n"), (LPCTSTR)Tpath);
			else m_T15Path = Tpath;
		}
	}
	SetTapeFolder(m_T14Path, false);
	SetTapeFolder(m_T15Path, true);
}

/////////////////////////////////////////////////////////////////////////////

LRESULT CMainFrame::OnBasicNotify(WPARAM wParam, LPARAM lParam) 
{
	if(m_pDoc != NULL){
		m_pDoc->BasicNotify(wParam, lParam);
	}
	return NULL;
}

/////////////////////////////////////////////////////////////////////////////

void CMainFrame::SetTapeDevice(bool IsT15/*=false*/)
{
CString DirName;

	if(IsT15) DirName	= m_T15Path;
	else DirName = m_T14Path;
	CFolderPickerDialog FolderDlg(DirName, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT | OFN_EXPLORER, nullptr, sizeof(OPENFILENAME));
	if(FolderDlg.DoModal() == IDOK){
		if(IsT15) SetTapeFolder(FolderDlg.GetPathName(), true);
		else SetTapeFolder(FolderDlg.GetPathName(), false);
	}
}

/////////////////////////////////////////////////////////////////////////////

void CMainFrame::SetTapeFolder(CString Folder, bool IsT15/*=false*/)
{
	if(IsT15){
		m_T15Path = Folder;
		Tape15.SetRootDir(Folder);
	}
	else{
		m_T14Path = Folder;
		Tape14.SetRootDir(Folder);
	}
}

/////////////////////////////////////////////////////////////////////////////

CString CMainFrame::GetTapeFolder(bool IsT15/*=false*/)
{
	if(IsT15) return m_T15Path;
	else return m_T14Path;
}

/////////////////////////////////////////////////////////////////////////////
