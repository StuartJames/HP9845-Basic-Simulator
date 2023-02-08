/*
 * Copyright (c) 2001-2002 Paolo Messina and Jerzy Kaczorowski
 * 
 * The contents of this file are subject to the Artistic License (the "License").
 * You may not use this file except in compliance with the License. 
 * You may obtain a copy of the License at:
 * http://www.opensource.org/licenses/artistic-license.html
 * 
 * THIS PACKAGE IS PROVIDED "AS IS" AND WITHOUT ANY EXPRESS OR IMPLIED 
 * WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF 
 * MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 * 
 */

#pragma once

/// Control's type mask
#define ODDBTN_BS_TYPEMASK 0x0000000FL

/////////////////////////////////////////////////////////////////////////////
// COddButton window

/*!
	\brief pOwner-Draw Default Button is a pBase class for owner-draw buttons that provides basic 
	support for default state handling.

	Derived class can indicate the default state when appropriate simply by calling 
	COddButton::IsDefault method to determine the default state and using 
	any visual effect (e.g. a thin black frame around the button) when it becomes default.

	\note A special static method COddButton::SetDefID can be used to set the default button 
	for the dialog to work around the problems with using a CDialog::SetDefID described in MS kb Q67655 
	(<A HREF="http://support.microsoft.com/default.aspx?scid=KB;EN-US;Q67655&">
	HOWTO: Change or Set the Default Push Button in a Dialog Box
	</A>)
*/
class COddButton : public CButton
{
public:
	COddButton();

protected:
	virtual void PreSubclassWindow();

public:
	virtual ~COddButton();

protected:
	afx_msg UINT OnGetDlgCode();
	afx_msg LRESULT OnSetStyle(WPARAM wParam, LPARAM lParam);
	
	DECLARE_MESSAGE_MAP()

private:
	BOOL m_bCanBeDefault;	/*!< TRUE to enable default state handling */
	BOOL m_bDefault;		/*!< Set to TRUE when control has default state */
	UINT m_nTypeStyle;		/*!< Type of control */

public:
	void EnableDefault(BOOL bEnable);
	BOOL IsDefault() const;
	UINT GetControlType() const;

	static void SetDefID(CDialog* pDialog, const UINT nID);
};

