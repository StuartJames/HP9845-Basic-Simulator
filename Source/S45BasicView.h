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
#include "S45BasicDoc.h"
#include "CDib08.h"

class CS45BasicDoc;

class CS45BasicView : public CView
{
protected:
	DECLARE_DYNCREATE(CS45BasicView)
													CS45BasicView() noexcept;
public:
	virtual									~CS45BasicView();
	CS45BasicDoc*				    GetDocument() const;
	void										NewCaretPosition(int Column);
	void										SetCaretType(bool Mode);
	void										SetGraphicsCursor(CPoint Pos, CRect Size, int State);
	bool										GetShiftState(void){ return m_ShiftState; };
	virtual BOOL						PreCreateWindow(CREATESTRUCT& cs);
	virtual void						OnInitialUpdate();
	virtual void						OnDraw(CDC* pDC);  
	bool										DoSpooledPrint(CString NewData);
	void										EndPrinting(bool bError);

protected:
	void										GetFontSize(void);
	bool										StartPrinting(void);

	CS45BasicDoc*				    m_pDoc;
	CSize										m_CharSize;
	POINT										m_CaretPos;
	bool										m_CarretType;
	CPoint									m_CursorPos;
	CRect										m_CursorSize;
	int											m_CursorColumn;
	int											m_CursorState;
	RECT										m_ClientSize;
	bool										m_EraseClient;
	bool										m_ShiftState;

  CPrintInfo*							m_pPrintInfo; 
  CDC											m_DCPrint;	
 	int											m_PCharHeight, m_PCharWidth;
	int											m_pageWidth, m_pageHeight;
  int											m_MaxLineLength;
  POINT										m_LastPos;
  CString									m_PrintBuff;
  bool										m_NewPrintJob;
  bool										m_NewPage;
  bool										m_PrintActive;
	int											m_LineWidth;
  PDEVMODE								m_pDevMode;

private:
	void										SetCaretPosition();
	void										DisplayRgns(CDC* pDC);

public:
	virtual void 						OnPrepareDC( CDC *pDC, CPrintInfo *pInfo = NULL ) ;
#ifdef _DEBUG
	virtual void						AssertValid() const;
	virtual void						Dump(CDumpContext& dc) const;
#endif
	CDC											m_GraphicsDC;

protected:
	afx_msg BOOL						OnEraseBkgnd(CDC* pDC);
  afx_msg HBRUSH					OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
	afx_msg void						OnSize(UINT nType, int cx, int cy);
	afx_msg void						OnChar(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void						OnKillFocus(CWnd* pNewWnd);
	afx_msg void						OnSetFocus(CWnd* pOldWnd);
	afx_msg void						OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void						OnKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags);
	DECLARE_MESSAGE_MAP()
};

#ifndef _DEBUG  // debug version in S45BasicView.cpp
inline CS45BasicDoc* CS45BasicView::GetDocument() const
   { return reinterpret_cast<CS45BasicDoc*>(m_pDocument); }
#endif

