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

#pragma once

#include "S45BasicDefs.h"
#include "S45BasicView.h"
#include "S45BasicDoc.h"
#include "ThemeHelperST.h"
#include "MMFrame.h"
#include "functionbar.h"
#include "./Basic/BasicDefs.h"

class CS45BasicDoc;
class CS45BasicView;

extern CMassStorage		Tape14;
extern CMassStorage		Tape15;


UINT BasicLoop(LPVOID params);

/////////////////////////////////////////////////////////////////////////////
class CMainFrame : public CMinMaxFrame
{
	
protected: // create from serialization only
													CMainFrame();
	DECLARE_DYNCREATE(CMainFrame)

public:

	void                    InitialShowWindow(UINT nCmdShow);
	void                    IntializeViews(void);
	CS45BasicDoc*						GetDocument();
	void										SetWindowTitle(const char *pStr);
	void										SetControlKeys(int DisplayMode = 0); 
	void										SetSoftKeys(bool Shift = 0); 
	void										SetTapeDevice(bool IsT15 = false);
	void										SetTapeFolder(CString Folder, bool IsT15 = false);
	CString									GetTapeFolder(bool IsT15 = false);

	virtual									~CMainFrame();
	virtual BOOL						PreCreateWindow(CREATESTRUCT& cs);
	virtual BOOL            PreTranslateMessage(MSG* pMsg);
	virtual BOOL						OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo);

#ifdef _DEBUG
	virtual void						AssertValid() const;
	virtual void						Dump(CDumpContext& dc) const;
#endif

private:
	CFunctionBar						m_FunctionBar;
	CWinThread							*m_pBasicThread;
	CThemeHelperST					m_ThemeHelper;
	static char							m_SoftKeysTitles[2][SOFT_KEY_COUNT][20];
	static char							m_ControlKeysTitles[SYS_MODE_COUNT][CONTROL_KEY_COUNT][20];

protected:  // control bar embedded members
	void                    GetSystemVars();
	void                    SaveSystemVars();
	void										GetAppStrings();
	void										SetTapeDirectories();

	CS45BasicDoc						*m_pDoc;
	CS45BasicView						*m_pView;
	CString                 m_T14Path;
	CString                 m_T15Path;
	LPTSTR									m_pProductName;
	LPTSTR									m_pCompanyName;
	LPTSTR									m_pVersion;
	LPTSTR									m_pCopyright;

// Generated message map functions
	afx_msg int							OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void						OnSetFocus(CWnd *pOldWnd);
	afx_msg void						OnClose();
	afx_msg LRESULT					OnBasicNotify(WPARAM wParam, LPARAM lParam);

	DECLARE_MESSAGE_MAP()

};


