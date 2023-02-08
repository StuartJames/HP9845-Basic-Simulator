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
#include "S45BasicDefs.h"
#include "globals.h"
#include "functionbar.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CFunctionBar dialog


BEGIN_MESSAGE_MAP(CFunctionBar, CDialogBar)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////

BOOL CFunctionBar::Create(CWnd* pParentWnd, UINT nIDTemplate, UINT nStyle, UINT nID, BOOL bChange)
{
	BOOL result = CDialogBar::Create(pParentWnd, nIDTemplate, nStyle, nID);
  m_bChangeDockedSize = bChange;
  m_sizeFloating = m_sizeDocked = m_sizeDefault;
	if(result) SetButtons();
	return result;
}

/////////////////////////////////////////////////////////////////////////////

BOOL CFunctionBar::Create( CWnd* pParentWnd, LPCTSTR lpszTemplateName, UINT nStyle, UINT nID, BOOL bChange)
{
  BOOL result = CDialogBar::Create( pParentWnd, lpszTemplateName, nStyle, nID);
  m_bChangeDockedSize = bChange;
  m_sizeFloating = m_sizeDocked = m_sizeDefault;
	if(result) SetButtons();
	return result;
}

/////////////////////////////////////////////////////////////////////////////

void CFunctionBar::SetButtons()
{
LOGFONT logfont;
ButtonStyle_s BtnStyle;

  ZeroMemory(&logfont, sizeof(logfont));
  logfont.lfHeight = -11;
  logfont.lfWeight = FW_THIN;
  logfont.lfCharSet = ANSI_CHARSET;
  logfont.lfPitchAndFamily = DEFAULT_PITCH | FF_DONTCARE;
  static char BASED_CODE szFaceName[] = "MS San Serif";
  lstrcpy(logfont.lfFaceName, szFaceName);
  if(!m_Font.CreateFontIndirect(&logfont)) return;
	m_SoftKeyStyle.GetStyle(&BtnStyle);
	BtnStyle.Radius = 2;
	BtnStyle.ColorFace.Enabled = RGB(100, 95, 53);
	BtnStyle.ColorFace.Clicked = RGB(63, 58, 33);
	BtnStyle.ColorFace.Pressed = RGB(63, 58, 33);
	m_SoftKeyStyle.SetStyle(&BtnStyle);
	m_ControlStyle.GetStyle(&BtnStyle);
	BtnStyle.Radius = 2;
	BtnStyle.ColorFace.Enabled = RGB(225, 176, 89);
	BtnStyle.ColorFace.Clicked = RGB(175, 126, 39);
	BtnStyle.ColorFace.Pressed = RGB(175, 126, 39);
	m_ControlStyle.SetStyle(&BtnStyle);
	m_Buttons[0].SubclassDlgItem(ID_SOFTKEY_F1, this);
	m_Buttons[1].SubclassDlgItem(ID_SOFTKEY_F2, this);
	m_Buttons[2].SubclassDlgItem(ID_SOFTKEY_F3, this);
	m_Buttons[3].SubclassDlgItem(ID_SOFTKEY_F4, this);
	m_Buttons[4].SubclassDlgItem(ID_SOFTKEY_F5, this);
	m_Buttons[5].SubclassDlgItem(ID_SOFTKEY_F6, this);
	m_Buttons[6].SubclassDlgItem(ID_SOFTKEY_F7, this);
	m_Buttons[7].SubclassDlgItem(ID_SOFTKEY_F8, this);
	m_Buttons[8].SubclassDlgItem(ID_SOFTKEY_F9, this);
	m_Buttons[9].SubclassDlgItem(ID_SOFTKEY_F10, this);
	m_Buttons[10].SubclassDlgItem(ID_CONTROL_K1, this);
	m_Buttons[11].SubclassDlgItem(ID_CONTROL_K2, this);
	m_Buttons[12].SubclassDlgItem(ID_CONTROL_K3, this);
	m_Buttons[13].SubclassDlgItem(ID_CONTROL_K4, this);
	m_Buttons[14].SubclassDlgItem(ID_CONTROL_K5, this);
	m_Buttons[15].SubclassDlgItem(ID_CONTROL_K6, this);
	m_Buttons[16].SubclassDlgItem(ID_CONTROL_K7, this);
	m_Buttons[17].SubclassDlgItem(ID_CONTROL_K8, this);
	m_Buttons[18].SubclassDlgItem(ID_CONTROL_K9, this);
	m_Buttons[19].SubclassDlgItem(ID_CONTROL_K10, this);
	for(int i = 0; i < SOFT_KEY_COUNT; ++i)	SetButtonColour(i, &m_SoftKeyStyle, RGB(255, 255, 255));
	for(int j = SOFT_KEY_COUNT, i = 0; i < CONTROL_KEY_COUNT; ++j, ++i)	SetButtonColour(j, &m_ControlStyle, RGB(0, 0, 0));
}

/////////////////////////////////////////////////////////////////////////////

void CFunctionBar::SetButtonColour(int Index, CRoundButtonStyle *pStyle, COLORREF TextColour)
{
ColorScheme_s TxtColor;

	if((Index >= 0) && (Index < DEF_BUTTON_COUNT)){
		m_Buttons[Index].SetBtnStyle(pStyle);
		m_Buttons[Index].GetTextColor(&TxtColor);	
		TxtColor.Enabled	= TextColour;
		TxtColor.Clicked	= TextColour;
		TxtColor.Pressed	= TextColour;
		m_Buttons[Index].SetTextColor(&TxtColor);	
	}
}

/////////////////////////////////////////////////////////////////////////////

void CFunctionBar::SetButtonText(int Index, LPCTSTR lpszText)
{
	if((Index >= 0) && (Index < DEF_BUTTON_COUNT)){
		m_Buttons[Index].SetWindowText(lpszText);	
	}
}

/////////////////////////////////////////////////////////////////////////////
