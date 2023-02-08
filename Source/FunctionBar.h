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

#include "RoundButtonEx.h"

constexpr auto  COLOR_ITEM_HEIGHT		= 18;
constexpr auto  DEF_BUTTON_COUNT		= 20;

/////////////////////////////////////////////////////////////////////////////
// CFunctionBar dialog

class CFunctionBar : public CDialogBar
{
public:
  BOOL								Create(CWnd* pParentWnd, UINT nIDTemplate, UINT nStyle, UINT nID, BOOL = TRUE);
  BOOL								Create(CWnd* pParentWnd, LPCTSTR lpszTemplateName, UINT nStyle, UINT nID, BOOL = TRUE);		
	void								SetButtonText(int nIndex, LPCTSTR lpszText);
	void								SetButtonColour(int Index, CRoundButtonStyle *pStyle, COLORREF TextColour);

	CRoundButtonStyle		m_SoftKeyStyle;
	CRoundButtonStyle		m_ControlStyle;
	CRoundButtonEx			m_Buttons[DEF_BUTTON_COUNT];
  CSize								m_sizeDocked;
  CSize								m_sizeFloating;
  BOOL								m_bChangeDockedSize;   // Indicates whether to keep a default size for docking
	CFont								m_Font;

protected:
	void								SetButtons();

	DECLARE_MESSAGE_MAP()
};
/////////////////////////////////////////////////////////////////////////////
