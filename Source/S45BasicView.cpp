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

#ifndef SHARED_HANDLERS
#include "S45Basic.h"
#endif

#include "S45BasicDoc.h"
#include "S45BasicView.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

constexpr auto CARET_INSERT_SIZE		= 4;
constexpr auto CARET_OVERWRITE_SIZE	= 1;

extern CRITICAL_SECTION ViewUpdateSemaphore;

IMPLEMENT_DYNCREATE(CS45BasicView, CView)

//////////////////////////////////////////////////////////////////////////

BEGIN_MESSAGE_MAP(CS45BasicView, CView)
	ON_WM_ERASEBKGND()
	ON_WM_CTLCOLOR()
	ON_WM_SIZE()
	ON_WM_CHAR()
	ON_WM_KILLFOCUS()
	ON_WM_SETFOCUS()
	ON_WM_KEYDOWN()
	ON_WM_KEYUP()
END_MESSAGE_MAP()

//////////////////////////////////////////////////////////////////////////

CS45BasicView::CS45BasicView() noexcept
{
	m_CaretPos = {SCRN_MARGIN_X, 0};
	m_CarretType = false;
	m_CursorPos = {0, 0};
	m_CursorSize = {0, 0, 0, 0};
  m_CursorState = 0;									 // OFF
	m_CursorColumn = 0;
	m_EraseClient = false;
	m_ShiftState = false;
	m_NewPage = true;
	m_NewPrintJob = true;
	m_pDevMode = nullptr;
	m_PrintActive = false;
}

//////////////////////////////////////////////////////////////////////////

CS45BasicView::~CS45BasicView()
{
}

//////////////////////////////////////////////////////////////////////////

#ifdef _DEBUG
void CS45BasicView::AssertValid() const
{
	CView::AssertValid();
}

void CS45BasicView::Dump(CDumpContext& dc) const
{
	CView::Dump(dc);
}

CS45BasicDoc* CS45BasicView::GetDocument() const // non-debug version is inline
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CS45BasicDoc)));
	return (CS45BasicDoc*)m_pDocument;
}
#endif 

//////////////////////////////////////////////////////////////////////////

BOOL CS45BasicView::PreCreateWindow(CREATESTRUCT& cs)
{
	return CView::PreCreateWindow(cs);
}

/////////////////////////////////////////////////////////////////////////////

void CS45BasicView::OnInitialUpdate() 
{
	m_pDoc = GetDocument();
  CView::OnInitialUpdate();
  GetFontSize();
}

/////////////////////////////////////////////////////////////////////////////

void CS45BasicView::OnPrepareDC(CDC *pDC, CPrintInfo *pInfo)
{
	CS45BasicDoc* pDoc = (CS45BasicDoc *)GetDocument();
	CView::OnPrepareDC(pDC, pInfo);
	if(pInfo != NULL){																												// Set the mapping mode for the device context so that it sets a 1:1 ratio with the screen coordinate system
		int MarginX, MarginY;
		CDC screenDC;																														// and the printer coordinate system
		screenDC.CreateDC(_T("DISPLAY"), NULL, NULL, NULL);
		pDC->SetMapMode(MM_ANISOTROPIC);
		pDC->SetWindowExt(screenDC.GetDeviceCaps(LOGPIXELSX), screenDC.GetDeviceCaps(LOGPIXELSY));
		pDC->SetViewportExt(pDC->GetDeviceCaps(LOGPIXELSX), pDC->GetDeviceCaps(LOGPIXELSY));
		m_pPrintInfo->m_rectDraw.SetRect(0, 0, m_DCPrint.GetDeviceCaps(PHYSICALWIDTH), m_DCPrint.GetDeviceCaps(PHYSICALHEIGHT));			// set up drawing rect to entire page (in logical coordinates)
		if((MarginX = m_DCPrint.GetDeviceCaps(PHYSICALOFFSETX)) == 0) MarginX = DEF_PRINT_MARGIN_X;
		if((MarginY = m_DCPrint.GetDeviceCaps(PHYSICALOFFSETY)) == 0) MarginY = DEF_PRINT_MARGIN_Y;
		m_pPrintInfo->m_rectDraw.DeflateRect(MarginX, MarginY);
		m_DCPrint.DPtoLP(&m_pPrintInfo->m_rectDraw);
		m_PCharHeight = m_CharSize.cy;
		m_PCharWidth = m_CharSize.cx;
		m_pageWidth = (m_pPrintInfo->m_rectDraw.right - m_pPrintInfo->m_rectDraw.left);
		m_pageHeight = (m_pPrintInfo->m_rectDraw.bottom - m_pPrintInfo->m_rectDraw.top);
		m_MaxLineLength = (m_pageWidth / m_PCharWidth);
		pInfo->m_bContinuePrinting = TRUE;
	}
}

//////////////////////////////////////////////////////////////////////////

void CS45BasicView::OnDraw(CDC* pDC)
{
CMemoryDC *pMemDC = nullptr;
int nVertPos;
CBrush *poldBrush;
CRect rect;
CString cstr;
int DisplayMode;

	ASSERT_VALID(m_pDoc);
	if(!m_pDoc) return;
	CBrush backBrush(m_pDoc->GetBackColour());
	DisplayMode = m_pDoc->GetDisplayMode();
	if(DisplayMode != DISPLAY_GRAPHIC){
		pMemDC = new CMemoryDC(pDC);																						// flicker free
		poldBrush = pMemDC->SelectObject(&backBrush);														// erase the background
		pMemDC->GetClipBox(&rect);														
		pMemDC->PatBlt(rect.left, rect.top, rect.Width(), rect.Height(), PATCOPY);
		pMemDC->SelectObject(poldBrush);
		pMemDC->SetBkColor(m_pDoc->GetBackColour());
		pMemDC->SetTextColor(m_pDoc->GetForeColour());
		pMemDC->SetBkMode(OPAQUE);
		if(DisplayMode == DISPLAY_NORMAL){
		  EnterCriticalSection(&ViewUpdateSemaphore);														// exclude modification of scroll data
			int TopRow, Count = m_pDoc->m_ScrollLines.GetCount();
 			if(Count > NORMALMODE_SCROLL_SIZE){
				TopRow = (Count - NORMALMODE_SCROLL_SIZE - m_pDoc->m_ScrollRowOffset);
				Count  = 	NORMALMODE_SCROLL_SIZE;
			}
			else TopRow = 0;
			ExTextRow *pRow;
			for(int i = 0; i < NORMALMODE_SCROLL_SIZE; ++i){
				nVertPos = i * m_CharSize.cy;
				if(i < Count){
					pRow = (ExTextRow*)m_pDoc->m_ScrollLines.GetAt(TopRow + i);
					((CPrinter16*)g_pSysPrinter)->RenderText(pMemDC, CPoint(0, nVertPos), pRow, false);
				}
				else break;
			}
		  LeaveCriticalSection(&ViewUpdateSemaphore);
		}
		else{
			int j = 0, Count = m_pDoc->m_EditLines.GetCount();									 // the line being edited will be displayed by the DisplayRgns() function
			for(int i = 0; i < EDITMODE_SECTION_SIZE; ++i, ++j){
				nVertPos = i * m_CharSize.cy;
				if(j < Count){
					cstr = m_pDoc->m_EditLines.GetAt(j);
					g_pSysPrinter->RenderText(pMemDC, CPoint(0, nVertPos), (LPTSTR)cstr.GetBuffer(), false, true); // show control codes
				}
			}
			++j;
			for(int i = EDITMODE_LOWER_START_LINE; i < EDITMODE_LOWER_START_LINE + EDITMODE_SECTION_SIZE; ++i, ++j){
				nVertPos = i * m_CharSize.cy;
				if(j < Count){
					cstr = m_pDoc->m_EditLines.GetAt(j);
					g_pSysPrinter->RenderText(pMemDC, CPoint(0, nVertPos), (LPTSTR)cstr.GetBuffer(), false, true);
				}
			}
		}
		DisplayRgns(pMemDC);
		if(m_pDoc->GetSystemMode() == SYS_RUN) pMemDC->FillSolidRect(630, 460, 8, 12, m_pDoc->GetForeColour());			 // draw run indicator
		delete pMemDC;
		return;
	}
	pDC->GetClipBox(&rect);																										// graphic mode
	if(m_EraseClient){																		
		m_EraseClient = false;																									// erase the background, usualy when changing modes
		poldBrush = pDC->SelectObject(&backBrush);				
		pDC->PatBlt(rect.left, rect.top, rect.Width(), rect.Height(), PATCOPY);
		pDC->SelectObject(poldBrush);
	}
	pDC->LPtoDP(&rect);
	pDC->BitBlt(rect.left, rect.top, rect.Width(), rect.Height(), &m_GraphicsDC, rect.left, rect.top, SRCCOPY);
	if(m_CursorState > 0){
		CPen gPen;
		gPen.CreatePen(PS_SOLID, 1, m_pDoc->GetForeColour());
		CPen *pOldPen = pDC->SelectObject(&gPen);
		pDC->MoveTo(m_CursorPos.x, m_CursorSize.bottom);															
		pDC->LineTo(m_CursorPos.x, m_CursorSize.top);
		pDC->MoveTo(m_CursorSize.left, m_CursorPos.y);															
		pDC->LineTo(m_CursorSize.right, m_CursorPos.y);
		pDC->SelectObject(pOldPen);
	}
}

//////////////////////////////////////////////////////////////////////////

void CS45BasicView::DisplayRgns(CDC* pDC)
{
int nVertPos;

  if(g_pSysPrinter == nullptr) return;
  EnterCriticalSection(&ViewUpdateSemaphore);																// exclude modification of scroll data
	for(int i = 0; i < DISP_AREA_COUNT; ++i){																	// display the system text lines
		PrintRgn_t *pRgn =	m_pDoc->GetRegion(i);
		nVertPos = pRgn->Row[m_pDoc->GetDisplayMode()] * m_CharSize.cy;
		if(nVertPos >= 0) g_pSysPrinter->RenderText(pDC, CPoint(0, nVertPos), (LPTSTR)pRgn->Text.GetBuffer(), pRgn->DoubleLine);
	}
  LeaveCriticalSection(&ViewUpdateSemaphore);
}

//////////////////////////////////////////////////////////////////////////
 
BOOL CS45BasicView::OnEraseBkgnd(CDC* pDC)
{
	m_EraseClient = true;
  return FALSE;
}

//////////////////////////////////////////////////////////////////////////

HBRUSH CS45BasicView::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
	HBRUSH hbr = CView::OnCtlColor(pDC, pWnd, nCtlColor);
  pDC->SetBkColor(m_pDoc->GetBackColour());
  pDC->SetTextColor(m_pDoc->GetForeColour());
	return hbr;
}

//////////////////////////////////////////////////////////////////////////

void CS45BasicView::OnChar(UINT nChar, UINT nRepCnt, UINT nFlags)
{
char cChar;

  cChar = (char)nChar;
	if((nChar > 31) && (nChar < 128 )){	
    m_pDoc->KeyEntry(cChar);
		return;
  }
	CView::OnChar(nChar, nRepCnt, nFlags);
}

//////////////////////////////////////////////////////////////////////////

void CS45BasicView::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	bool CtrlState = (GetKeyState(VK_CONTROL) & ~1) ? true : false;		 
	bool NewShift = (GetKeyState(VK_SHIFT) & ~1) ? true : false;		 
	if(m_ShiftState != NewShift){
		((CMainFrame*)AfxGetMainWnd())->SetSoftKeys(NewShift);
		m_ShiftState = NewShift;
	}
	switch(nChar){
		case VK_F1:
		case VK_F2:
		case VK_F3:
		case VK_F4:
		case VK_F5:
		case VK_F6:
		case VK_F7:
		case VK_F8:
		case VK_F9:
		case VK_F10:
		case VK_ESCAPE:
		case VK_RETURN:
		case VK_CLEAR: 
		case VK_TAB:
		case VK_HOME: 
		case VK_END:  
		case VK_PRIOR:		// Page Up
		case VK_NEXT:			// Page Down
		case VK_LEFT: 
		case VK_RIGHT: 
		case VK_UP:  
		case VK_DOWN:  
		case VK_INSERT: 
		case VK_BACK:
		case VK_DELETE:{
			m_pDoc->SpecialKeyEntry(nChar, m_ShiftState, CtrlState);
		}
		return;
	}
	CView::OnKeyDown(nChar, nRepCnt, nFlags);
}

//////////////////////////////////////////////////////////////////////////

void CS45BasicView::OnKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags)
{
bool NewShift;
	NewShift = (GetKeyState(VK_SHIFT) & ~1) ? true : false;		 
	if(m_ShiftState != NewShift){
		((CMainFrame*)AfxGetMainWnd())->SetSoftKeys(NewShift);
		m_ShiftState = NewShift;
	}
}

//////////////////////////////////////////////////////////////////////////

void CS45BasicView::OnKillFocus(CWnd* pNewWnd)
{
	CView::OnKillFocus(pNewWnd);
  DestroyCaret();
}

//////////////////////////////////////////////////////////////////////////

void CS45BasicView::OnSetFocus(CWnd* pOldWnd)
{
	CView::OnSetFocus(pOldWnd);
	if(!m_CarretType) CreateSolidCaret(m_CharSize.cx, CARET_INSERT_SIZE);			// Create insert caret
	else  CreateSolidCaret(m_CharSize.cx, CARET_OVERWRITE_SIZE);							// Create overwrite caret 
	if(m_pDoc->GetDisplayMode() != DISPLAY_GRAPHIC)	ShowCaret();						  // caret stays off when in graphics mode
}

//////////////////////////////////////////////////////////////////////////

void CS45BasicView::SetCaretPosition()
{
	int EditLineIndex = m_pDoc->GetEntryLinePosition() + 1;
  m_CaretPos.x = SCRN_MARGIN_X + (m_CursorColumn % SCRNCOLS) * m_CharSize.cx;
	m_CaretPos.y = SCRN_MARGIN_Y + (EditLineIndex + 1 * (m_CursorColumn / SCRNCOLS)) * m_CharSize.cy;
//	MyTrace(_T("Pos %d, %d\n"), m_CaretPos.x, m_CaretPos.y); 
	SetCaretPos(m_CaretPos);																									// caret must be hiden 
	if(m_pDoc->GetDisplayMode() != DISPLAY_GRAPHIC)	ShowCaret();						  // caret stays off when in graphics mode
}

//////////////////////////////////////////////////////////////////////////

void CS45BasicView::NewCaretPosition(int Column)
{
  m_CursorColumn =  Column;
	HideCaret();
	SetCaretPosition();
}

//////////////////////////////////////////////////////////////////////////

void CS45BasicView::SetCaretType(bool Mode)
{
	m_CarretType = Mode;
  DestroyCaret();
	if(!m_CarretType) CreateSolidCaret(m_CharSize.cx, CARET_INSERT_SIZE);			// Create insert caret
	else  CreateSolidCaret(m_CharSize.cx, CARET_OVERWRITE_SIZE);							// Create overwrite caret
	SetCaretPosition();
}

//////////////////////////////////////////////////////////////////////////

void CS45BasicView::SetGraphicsCursor(CPoint Pos, CRect Size, int State)
{
	m_CursorPos = Pos;
	m_CursorSize = Size;
  m_CursorState = State;
}

//////////////////////////////////////////////////////////////////////////

void CS45BasicView::OnSize(UINT nType, int cx, int cy)
{
	CView::OnSize(nType, cx, cy);
}

//////////////////////////////////////////////////////////////////////////


void CS45BasicView::GetFontSize()
{
  m_CharSize.cx = SCRN_FONT_X;
  m_CharSize.cy = SCRN_FONT_Y;
}

//////////////////////////////////////////////////////////////////////////

bool CS45BasicView::DoSpooledPrint(CString NewData)
{
int y = 0, x = 0;
int lfPos = 0, Len, ffPos = 0;		      
bool bError = false; 																																																// begin page printing loop
CString TxtOut;
CSize TxtSize;

	if(m_PrintActive) return bError;																																									// ok we just ignore the job and say it was succesfull
	m_PrintActive = true;																																															// stop re-entry
	if(m_NewPrintJob && !StartPrinting()){
		m_PrintActive = false;
		return true;
	}
	if(m_NewPage){
		m_NewPage = false;
		OnPrepareDC(&m_DCPrint, m_pPrintInfo);
		if(m_DCPrint.StartPage() < 0) bError = true;																																		// attempt to start the current page
		else{
			m_DCPrint.SetViewportOrg(0, 0);
			m_DCPrint.SetWindowOrg(0, 0);
			m_DCPrint.IntersectClipRect(&m_pPrintInfo->m_rectDraw);
			m_LineWidth = g_pPrinter->GetLineWidth();
			m_LineWidth = (m_LineWidth < m_MaxLineLength) ? m_LineWidth : m_MaxLineLength;
		  y = m_pPrintInfo->m_rectDraw.top;
		}
	}
	else y = m_LastPos.y;
	if(!bError){
		m_PrintBuff += NewData;																																													// add to existing data
		while(((lfPos = m_PrintBuff.Find('\n')) >= 0) && !m_NewPage){																										// break down data into lines
			x = m_pPrintInfo->m_rectDraw.left;
			TxtOut = m_PrintBuff.Left(lfPos++);																																						// only print text before line feed
			if((Len = TxtOut.GetLength()) > m_LineWidth) TxtOut = TxtOut.Left(m_LineWidth);																// test for page width
			TxtSize = m_DCPrint.GetTextExtent(TxtOut);
			if(Len > 0) m_DCPrint.TabbedTextOut(x, y, TxtOut, TxtOut.GetLength(), 0, NULL, 0);
			y += TxtSize.cy;
			if(lfPos < m_PrintBuff.GetLength()) m_PrintBuff = m_PrintBuff.Right(m_PrintBuff.GetLength() - lfPos);					// recover rest of string
			else m_PrintBuff.Empty();
			if((y > (m_pageHeight - m_PCharHeight * 2)) || (TxtOut.Find('\f') >= 0)) m_NewPage = true;										// check for full page
		}
		if(m_NewPage || ((ffPos = m_PrintBuff.Find('\f')) >= 0)){
			if(m_DCPrint.EndPage() < 0) bError = true;
			++m_pPrintInfo->m_nCurPage;
			if(ffPos >= 0){
				Len = m_PrintBuff.GetLength();
				if((ffPos < Len) && Len > 1) m_PrintBuff.Left(ffPos) = m_PrintBuff.Right(m_PrintBuff.GetLength() - ffPos);	// recover rest of string
				else m_PrintBuff.Empty();
			}
			else m_PrintBuff.Empty();
		}
		else m_LastPos.y = y;
	}
	m_PrintActive = FALSE;
	return bError;
}

//////////////////////////////////////////////////////////////////////////

bool CS45BasicView::StartPrinting(void)
{
	m_PrintBuff.Empty();
	m_pPrintInfo = new CPrintInfo;
	ASSERT(m_pPrintInfo->m_pPD != NULL);																						// must be set
	m_pPrintInfo->m_bDirect = TRUE;																									// set for no print dialog
	m_pPrintInfo->SetMaxPage(0xffff);																								// print as many pages as possible
	if(DoPreparePrinting(m_pPrintInfo)){																						
		ASSERT(m_pPrintInfo->m_pPD->m_pd.hDC != nullptr);															// should have no DC
		if(m_pPrintInfo->m_pPD->m_pd.Flags & PD_PRINTTOFILE) return false; 						// no print to file 
		DOCINFO docInfo;
		ZeroMemory(&docInfo, sizeof(DOCINFO));
		docInfo.cbSize = sizeof(DOCINFO);
		docInfo.lpszDocName = _T("S45Basic Print");
		docInfo.lpszOutput = nullptr;
		m_DCPrint.Attach(m_pPrintInfo->m_pPD->m_pd.hDC);															// attach printer dc
		m_DCPrint.m_bPrinting = TRUE;
		if(m_DCPrint.StartDoc(&docInfo) == SP_ERROR){																	// start document printing process
			m_DCPrint.Detach();																													// will be cleaned up by m_PrintInfo destructor
			delete m_pPrintInfo;
			AfxMessageBox(AFX_IDP_FAILED_TO_START_PRINT);
			return false;
		}
		m_NewPrintJob = false;
		m_NewPage = true;
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////

void CS45BasicView::EndPrinting(bool bError)
{
	if(m_NewPrintJob) return;
	if(!bError) m_DCPrint.EndDoc();																									// cleanup document printing process
	m_DCPrint.Detach();																															// will be cleaned up by CPrintInfo destructor
	delete m_pPrintInfo;
	m_NewPrintJob = true;
}

