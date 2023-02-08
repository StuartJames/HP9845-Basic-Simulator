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

#ifndef __AFXWIN_H__
	#BasicError "include 'stdafx.h' before including this file for PCH"
#endif

#include "resource.h"       // main symbols
#include "ThemeHelperST.h"
#include "SplashWnd.h"

extern const TCHAR szS45BasicClass[];

BOOL PASCAL NEAR ReadWindowPlacement(LPWINDOWPLACEMENT pwp);
void PASCAL NEAR WriteWindowPlacement(LPWINDOWPLACEMENT pwp);

////////////////////////////////////////////////////////////////////

class CS45BasicApp : public CWinApp
{
public:
													CS45BasicApp() noexcept;


	virtual BOOL						InitInstance();
 	virtual BOOL						PreTranslateMessage(MSG* pMsg);
	virtual int							ExitInstance();
	void										OnFilePrintSetup();

	UINT										m_nAppLook;
	afx_msg void						OnFileSetupT14();
	afx_msg void						OnFileSetupT15();
	DECLARE_MESSAGE_MAP()
};

extern CS45BasicApp theApp;
