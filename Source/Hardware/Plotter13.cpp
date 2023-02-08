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
#include "./Basic/BasicDefs.h"
#include <stdlib.h>
#include <stdio.h>


const int	PenStyles[10] = {PS_NULL, PS_SOLID, PS_NULL, PS_DOT, PS_DASH, PS_DASHDOT, PS_DASHDOTDOT, PS_DASHDOTDOT, PS_DASHDOTDOT, PS_SOLID}; 


//////////////////////////////////////////////////////////////////////////////////////

CPlotter13::CPlotter13(void) : CPlotter()
{
	m_pView = nullptr;
	m_PhysicalLimits = CRect(DEF_PLOT_ORG_X, DEF_PLOT_ORG_Y, DEF_PLOT_SIZE_X, DEF_PLOT_SIZE_Y);
	ResetParameters();
	m_GCharset.IsSet = false;
}

//////////////////////////////////////////////////////////////////////////////////////

CPlotter13::~CPlotter13(void)
{
}

//////////////////////////////////////////////////////////////////////////////////////

void CPlotter13::Initialise(void)
{
CRect rect;

	CPlotter::Initialise();
  if(m_pDoc == nullptr)	return;
	m_pView = m_pDoc->GetSystemView();																													// setup the default graphics device (CRT)
	CDC	*pDC = m_pView->GetDC();
	pDC->GetClipBox(&rect);
	m_pView->m_GraphicsDC.CreateCompatibleDC(pDC);
	m_BitMap.CreateCompatibleBitmap(pDC, rect.Width(),rect.Height());
	m_pView->m_GraphicsDC.SelectObject(&m_BitMap);
	GetProperties();
	ResetParameters();
}

//////////////////////////////////////////////////////////////////////////////////////

void CPlotter13::GetProperties(void)
{
CRect rect;

  if(m_pDoc == nullptr)	return;
	m_pView->GetClientRect(&rect);
	rect.DeflateRect(DEF_PLOT_ORG_X, DEF_PLOT_ORG_Y);
	m_PhysicalLimits = rect;
}

//////////////////////////////////////////////////////////////////////////////////////

void CPlotter13::Destroy(void)
{
}

//////////////////////////////////////////////////////////////////////////////////////

void CPlotter13::ResetParameters(void)
{
CRectDbl rect(m_PhysicalLimits);

	m_InUserUnits		= true;
	rect.Offset(-DEF_PLOT_ORG_X, -DEF_PLOT_ORG_Y);
	rect.SwapRect(false, true);
	m_HardLimits		= rect;
	m_HardClipRect	= m_HardLimits;
	m_ClipRect			= m_HardClipRect;
	m_LocateRect		= m_HardClipRect;
	m_SoftClipRect	= m_HardClipRect;
	m_ScaleRect			= m_HardClipRect;
	m_LabelOrgn			= DEF_LABEL_ORGIGIN;
	m_LabelAngle		= DEF_LABEL_DIRECTION;
	m_PlotAngle			= DEF_PLOT_DIRECTION;
	m_PenNumber			= DEF_PEN_SELECT;
	m_LineType			= DEF_LINETYPE;
	m_LineTypeLen		= DEF_LINE_LENGTH;
	m_ScaleGDU			= (m_HardClipRect.top - m_HardClipRect.bottom) / 100;												// 1 GDU is 1% of shortest side	(Y)
	m_ScaleX				= m_ScaleGDU;																																// 1% of X is 1% of Y 
	m_ScaleY				= m_ScaleGDU;
	m_CharSize			= DEF_CHAR_SIZE * m_ScaleGDU;
	m_CharRatio			= DEF_CHAR_RATIO;
	m_IsSoftClip		= false;
	m_IsLocateClip	= false;
	m_CurrentPenPos = CPointDbl(0.0, 0.0);
	m_PenDown				= false;																																		// pen up
	m_CursorPos			= CPoint(0, 0);
	m_CursorType		= 0;
}

//////////////////////////////////////////////////////////////////////

void CPlotter13::ResetHardware(void)
{																																															// Note should call ResetPerameters prior to this function
	Unclip();																																										// set to default clipping area
	Clear();																																										// clear all plotted points
	Move(m_CurrentPenPos);																																			// set the pen to the home position
	if(m_pView != nullptr) m_pView->SetGraphicsCursor(CPoint(0,0), CRect(0,0,0,0), 0);				  // hide the cursor
}

//////////////////////////////////////////////////////////////////////

inline void CPlotter13::ViewUpdate(void)
{
	if(m_pView != nullptr){
		m_pView->Invalidate(FALSE);
		m_pView->UpdateWindow();
	}
}

//////////////////////////////////////////////////////////////////////

inline LONG CPlotter13::xToDC(int PosX)
{
	if(m_InUserUnits) return (LONG)(m_LocateRect.left + ((double)PosX - m_ScaleRect.left) * m_ScaleX + DEF_PLOT_ORG_X);
	else return (LONG)(m_HardLimits.left + PosX);
}

//////////////////////////////////////////////////////////////////////

inline LONG CPlotter13::xToDC(double PosX)
{
	if(m_InUserUnits) return (LONG)(m_LocateRect.left + (PosX - m_ScaleRect.left) * m_ScaleX + DEF_PLOT_ORG_X);
	else return (LONG)(m_HardLimits.left + PosX);
}

//////////////////////////////////////////////////////////////////////

inline LONG CPlotter13::yToDC(int PosY)
{
	if(m_InUserUnits) return (LONG)(m_PhysicalLimits.bottom - (m_LocateRect.bottom + ((double)PosY - m_ScaleRect.bottom) * m_ScaleY));
	else return (LONG)(m_HardLimits.bottom + m_HardLimits.bottom - PosY);
}

//////////////////////////////////////////////////////////////////////

inline LONG CPlotter13::yToDC(double PosY)
{
	if(m_InUserUnits) return (LONG)(m_PhysicalLimits.bottom - (m_LocateRect.bottom + (PosY - m_ScaleRect.bottom) * m_ScaleY));
	else return (LONG)(m_HardLimits.bottom - PosY);
}

//////////////////////////////////////////////////////////////////////

inline CPoint CPlotter13::ToDC(CPointDbl Point)
{
CPoint point;

	if(m_InUserUnits) {
		point.x = (LONG)(m_LocateRect.left + (Point.x - m_ScaleRect.left) * m_ScaleX + DEF_PLOT_ORG_X);
		point.y = (LONG)(m_PhysicalLimits.bottom - (m_LocateRect.bottom + (Point.y - m_ScaleRect.bottom) * m_ScaleY));
	}
	else{
		point.x = (LONG)(m_HardLimits.left + Point.x);
		point.y = (LONG)(m_HardLimits.bottom - Point.y);
	}
	return point;
}

//////////////////////////////////////////////////////////////////////

inline CRectDbl CPlotter13::ToDC(CRectDbl Rect)
{
CRectDbl rect;

	if(m_InUserUnits) {
		rect.left = m_LocateRect.left + (Rect.left - m_ScaleRect.left) * m_ScaleX + DEF_PLOT_ORG_X;
		rect.right = m_LocateRect.left + (Rect.right - m_ScaleRect.left) * m_ScaleX + DEF_PLOT_ORG_X;
		rect.bottom = m_PhysicalLimits.bottom - (m_LocateRect.bottom + (Rect.bottom - m_ScaleRect.bottom) * m_ScaleY);
		rect.top = m_PhysicalLimits.bottom - (m_LocateRect.bottom + (Rect.top - m_ScaleRect.bottom) * m_ScaleY);
	}
	else{
		rect.left = m_HardLimits.left + Rect.left;
		rect.right = m_HardLimits.left + Rect.right;
		rect.bottom = m_HardLimits.bottom - Rect.bottom;
		rect.top = m_HardLimits.bottom - Rect.top;
	}
	return rect;
}

//////////////////////////////////////////////////////////////////////

inline CPointDbl CPlotter13::ToUser(CPoint Pos)
{
CPointDbl Point;

	Point.x = ((double)Pos.x - DEF_PLOT_ORG_X - m_LocateRect.left) / m_ScaleX + m_ScaleRect.left;
	Point.y = ((m_PhysicalLimits.bottom - (double)Pos.y) - m_LocateRect.bottom) / m_ScaleY + m_ScaleRect.bottom;
	return Point;
}

//////////////////////////////////////////////////////////////////////

inline CRectDbl CPlotter13::ToUser(CRectDbl Rect)
{
CRectDbl rect;

	rect.left = (Rect.left - m_LocateRect.left) / m_ScaleX + m_ScaleRect.left;
	rect.right = (Rect.right - m_LocateRect.left) / m_ScaleX + m_ScaleRect.left;
	rect.bottom = (Rect.bottom - m_LocateRect.bottom) / m_ScaleY + m_ScaleRect.bottom;
	rect.top = (Rect.top - m_LocateRect.bottom) / m_ScaleY + m_ScaleRect.bottom;
	return rect;
}

//////////////////////////////////////////////////////////////////////

inline CPointDbl	CPlotter13::RotatePoint(CPointDbl Pos)
{
CPointDbl DPoint;

	DPoint.x = Pos.x * cos(m_PlotAngle) + Pos.y * sin(-m_PlotAngle);
	DPoint.y = Pos.x * sin(m_PlotAngle) + Pos.y * cos(m_PlotAngle);
	return DPoint;
}

//////////////////////////////////////////////////////////////////////

inline CPoint	CPlotter13::RotatePlot(CPointDbl Pos)
{
CPointDbl DPoint;
CPoint Point;

	DPoint.x = Pos.x * cos(m_PlotAngle) + Pos.y * sin(-m_PlotAngle);
	DPoint.y = Pos.x * sin(m_PlotAngle) + Pos.y * cos(m_PlotAngle);
	if(m_InUserUnits){
		Point.x = (LONG)(m_LocateRect.left + (DPoint.x - m_ScaleRect.left) * m_ScaleX + DEF_PLOT_ORG_X);
		Point.y = (LONG)(m_PhysicalLimits.bottom - (m_LocateRect.bottom + (DPoint.y - m_ScaleRect.bottom) * m_ScaleY));
	}
	else{
		Point.x = (LONG)(m_HardLimits.left + DPoint.x);
		Point.y = (LONG)(m_HardLimits.bottom - DPoint.y);
	}
	return Point;
}

//////////////////////////////////////////////////////////////////////

inline CRectDbl CPlotter13::UDUtoPDU(CRectDbl Rect)
{
CRectDbl rect;

	rect.left = m_LocateRect.left + (Rect.left - m_ScaleRect.left) * m_ScaleX;
	rect.right = m_LocateRect.left + (Rect.right - m_ScaleRect.left) * m_ScaleX;
	rect.bottom = m_LocateRect.bottom + (Rect.bottom - m_ScaleRect.bottom) * m_ScaleY;
	rect.top = m_LocateRect.bottom + (Rect.top - m_ScaleRect.bottom) * m_ScaleY;
	return rect;
}

//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////

void	CPlotter13::Clear(void)																																	// clear the entire client area
{
CBrush *poldBrush;
CRect rect, rectClip;
CRgn clipRgn;

  if(m_pDoc == nullptr)	return;
	CBrush backBrush(m_pDoc->GetBackColour());
  CDC *pDC = &m_pView->m_GraphicsDC;
	pDC->GetClipBox(&rectClip);																																	// save existing clipping setting		
	pDC->SelectClipRgn(nullptr);																																// clear any clipping
  poldBrush = pDC->SelectObject(&backBrush);																									// erase the background
	pDC->GetClipBox(&rect);																																			// get the client area
	pDC->PatBlt(rect.left, rect.top, rect.Width(), rect.Height(), PATCOPY);
	pDC->SelectObject(poldBrush);
	ViewUpdate();
	clipRgn.CreateRectRgnIndirect(rectClip);																										// restore the saved clipping area
//	pDC->SelectClipRgn(&clipRgn, RGN_COPY);
}

//////////////////////////////////////////////////////////////////////

void CPlotter13::Axes(Axes_Grid_s Axes)
{
CPen gPen;
int ticsize;
 
  if(m_pDoc == nullptr)	return;
  CDC *pDC = &m_pView->m_GraphicsDC;
	CRectDbl ClipRect = m_SoftClipRect; 
	CRectDbl rect = ToUser(ClipRect);
	int MinorSize = (Axes.TicSize > 1) ? Axes.TicSize / 2 : 1;
	int xInt = xToDC(Axes.Intersect.x);
	int yInt = yToDC(Axes.Intersect.y);
	gPen.CreatePen(PS_SOLID, 1, m_pDoc->GetForeColour());
	CPen *pOldPen = pDC->SelectObject(&gPen);
	pDC->MoveTo((int)ClipRect.left + DEF_PLOT_ORG_X, yInt);																			// draw the axes at the intersection
	pDC->LineTo((int)ClipRect.right + DEF_PLOT_ORG_X, yInt);
	pDC->MoveTo(xInt, (int)(m_HardLimits.top - ClipRect.bottom) + DEF_PLOT_ORG_Y);
	pDC->LineTo(xInt, (int)(m_HardLimits.top - ClipRect.top) + DEF_PLOT_ORG_Y);
	if(Axes.TicSpacing.x > 0){
		int mtic = (int)((rect.left - Axes.Intersect.x) / Axes.TicSpacing.x); 											// calcuate the first horizontal tic point...
		double tic = Axes.Intersect.x + Axes.TicSpacing.x * mtic; 																	// so that one major tic falls on the intersection
		while(tic < rect.right){ 																																		// draw the horizontal tic marks
			if(!(mtic % (int)Axes.MajorCount.x)) ticsize = Axes.TicSize;
			else ticsize = MinorSize;
			int y = yToDC(Axes.Intersect.y);
			pDC->MoveTo(xToDC(tic), yInt - ticsize);
			pDC->LineTo(xToDC(tic), yInt + ticsize);
			tic += Axes.TicSpacing.x;
			++mtic;
		}
	}
	if(Axes.TicSpacing.y > 0){
		int mtic = (int)((rect.bottom - Axes.Intersect.y) / Axes.TicSpacing.y); 												// calcuate the first vertical grid point...
		double tic =  Axes.Intersect.y + Axes.TicSpacing.y * mtic; 													
		while(tic < rect.top){																																			// draw the vertical tic marks
			if(!(mtic % (int)Axes.MajorCount.y)) ticsize = Axes.TicSize;
			else ticsize = MinorSize;
			pDC->MoveTo(xInt - ticsize, yToDC(tic));
			pDC->LineTo(xInt + ticsize, yToDC(tic));
			tic += Axes.TicSpacing.y;
			++mtic;
		}
	}
	pDC->SelectObject(pOldPen);
 	ViewUpdate();
}

//////////////////////////////////////////////////////////////////////

void CPlotter13::Clip(CRectDbl Clip)
{
	if(m_InUserUnits)	Clip = UDUtoPDU(Clip);				// UDU to PDU
	else Clip.ScaleRect(m_ScaleGDU, m_ScaleGDU);		// GDU to PDU
	if((m_HardClipRect.PtInRect(Clip.TopLeft())) && (m_HardClipRect.PtInRect(Clip.BottomRight()))){
		m_ClipRect = Clip;
		m_IsSoftClip = true;
		ClipToSoft();
	}
}

//////////////////////////////////////////////////////////////////////

void CPlotter13::CSize(float CharSize, float CharRatio)
{
	m_CharSize = CharSize * m_ScaleGDU;
	m_CharRatio = CharRatio;
}

//////////////////////////////////////////////////////////////////////

void CPlotter13::Frame(void)
{
  if(m_pDoc == nullptr)	return;
  CDC *pDC = &m_pView->m_GraphicsDC;
	CBrush Brush(m_pDoc->GetForeColour());
	CRectDbl drect = m_SoftClipRect; 
	CRect rect((int)drect.left, (int)(m_HardLimits.top - drect.top), (int)drect.right, (int)(m_HardLimits.top - drect.bottom));
	rect.OffsetRect(DEF_PLOT_ORG_X, DEF_PLOT_ORG_Y);
	pDC->FrameRect(rect, &Brush); 
	ViewUpdate();
}

//////////////////////////////////////////////////////////////////////

void CPlotter13::GClear(void)
{
	Clear();
}

//////////////////////////////////////////////////////////////////////

void CPlotter13::GPrint(const char *pBitStream, CPoint To, CSze Size, int Flag, int Offset)
{
long X, Y;
int k = Offset;


	if(m_pDoc == nullptr)	return;
  CDC *pDC = &m_pView->m_GraphicsDC;
	COLORREF ColorFore = m_pDoc->GetForeColour();
	COLORREF ColorBack = m_pDoc->GetBackColour();
	Y = (long)(m_SoftClipRect.top - (To.y - DEF_PLOT_ORG_Y));
	for(int j = 0; j < Size.sy; ++j){
		X = (long)To.x + DEF_PLOT_ORG_X;
		for(int i = 0; i < Size.sx; ++i){
			int bit = (pBitStream[k / 8] >> (7 - (k++ % 8)) & 0x01);
			if(Flag < 2){
				if(bit > 0) pDC->SetPixel(X, Y, (Flag > 0) ? ColorFore : ColorBack);
			}
			else{
				if(bit > 0) pDC->SetPixel(X, Y, ColorFore);
				else pDC->SetPixel(X, Y, ColorBack);
			}
			X++;
		}
		--Y;
	}
	ViewUpdate();
}

//////////////////////////////////////////////////////////////////////

void CPlotter13::Grid(Axes_Grid_s	Grid)
{
CPen gPen;
double x, y;
int xm, ym;
bool DoMajorY = true;

  if((m_pDoc == nullptr) || (Grid.TicSize == 0))	return;
	gPen.CreatePen(PS_SOLID, 1, m_pDoc->GetForeColour());
  CDC *pDC = &m_pView->m_GraphicsDC;
	CPen *pOldPen = pDC->SelectObject(&gPen);
	CRectDbl ClipBox = ToUser(m_SoftClipRect);
	int MinorSize = (Grid.TicSize > 1) ? Grid.TicSize / 2 : 1;
	xm = (int)((ClipBox.left - Grid.Intersect.x) / Grid.TicSpacing.x); 						// calculate the minor counts before the first major X tick...
	x = Grid.Intersect.x + Grid.TicSpacing.x * xm; 																// so that one point falls on the intersection
	while(x <= ClipBox.right){ 																				
		if(!(xm % (int)Grid.MajorCount.x)){
			pDC->MoveTo(xToDC(x), yToDC(ClipBox.bottom));															// draw full vertical	major tic
			pDC->LineTo(xToDC(x), yToDC(ClipBox.top));
		}
		ym = (int)((m_ScaleRect.bottom - Grid.Intersect.y) / Grid.TicSpacing.y);		// calculate the minor counts before the first major Y tick
		y = Grid.Intersect.y + Grid.TicSpacing.y * ym;															// calculate the first horizontal grid point
		bool YDone = false;
		while(y <= ClipBox.top){
			if(!(ym % (int)Grid.MajorCount.y) && DoMajorY){														// only draw majors once
				pDC->MoveTo(xToDC(ClipBox.left), yToDC(y));															// draw full  horizontal major tic
				pDC->LineTo(xToDC(ClipBox.right), yToDC(y));
				YDone = true;
			}
			if(ym % (int)Grid.MajorCount.y){
				pDC->MoveTo(xToDC(x) - MinorSize, yToDC(y));														// draw cross	for minor tic
				pDC->LineTo(xToDC(x) + MinorSize, yToDC(y));
				pDC->MoveTo(xToDC(x), yToDC(y) - MinorSize);
				pDC->LineTo(xToDC(x), yToDC(y) + MinorSize);
			}
			y += Grid.TicSpacing.y;																										// move to next minor tick Y position
			++ym;																																			// increment Y tick count
		}
		if(YDone) DoMajorY = false;																									// stop future drawing of major ticks
		x += Grid.TicSpacing.x;																											// move to next minor tick X position
		++xm;																																				// increment X tick count
	}
	pDC->SelectObject(pOldPen);
	ViewUpdate();
}

//////////////////////////////////////////////////////////////////////

int CPlotter13::Label(BString *pStr)
{
CString cstr = pStr->GetBuffer();
CPen gPen;

	if(m_pDoc == nullptr)	return -1;
	CDC *pDC = &m_pView->m_GraphicsDC;
	ClipToHard();
	gPen.CreatePen(PS_SOLID, 1, m_pDoc->GetForeColour());
	CPen *pOldPen = pDC->SelectObject(&gPen);
	m_CurrentPenPos = ToUser(m_pPlotterFont->DrawString(pDC, RotatePlot(m_CurrentPenPos), m_LabelAngle, m_CharSize * CHARACTER_SCALE, m_CharRatio, m_LabelOrgn, SMALL_SIMPLEX, cstr));
	pDC->SelectObject(pOldPen);
	ViewUpdate();
	ClipToSoft();
	return DEV_OK;
}

//////////////////////////////////////////////////////////////////////

void CPlotter13::LDir(double Angle)
{
	m_LabelAngle = ToRadians(Angle);
}

//////////////////////////////////////////////////////////////////////

bool CPlotter13::Limit(CRectDbl Limits)															// physical clip region in mm set in GDU
{
bool Error = false;

	Limits.ScaleRect(SCREEN_MM_SCALE, SCREEN_MM_SCALE);								// scale to mm
	Error = m_HardLimits.ConstrainRect(&Limits);											// limit to the physical hadware
	m_HardClipRect = Limits;
	m_ClipRect = Limits;																							// overide soft clip as well
	m_ScaleGDU = m_HardClipRect.top - m_HardClipRect.bottom / 100;		// 1 GDU is 1% of shortest side
	m_InUserUnits = false;																						// set to GDUs
	ClipToHard();
	return Error;
}

//////////////////////////////////////////////////////////////////////

void CPlotter13::LineType(int Type, int Length)
{
	m_LineType = (Type > 0) ? ((Type <= PS_MAX) ? Type : 1) : 1;
	m_LineTypeLen = Length;
}

//////////////////////////////////////////////////////////////////////

void CPlotter13::Locate(CRectDbl Locate) 														// soft clip region set in GDU
{
	Locate.ScaleRect(m_ScaleGDU, m_ScaleGDU);													// scale to GDU
	if((m_HardClipRect.PtInRect(Locate.TopLeft())) && (m_HardClipRect.PtInRect(Locate.BottomRight()))){
		m_LocateRect = Locate;
		m_IsLocateClip = true;
		m_InUserUnits = true;
	}
	ClipToSoft();
}

//////////////////////////////////////////////////////////////////////

void CPlotter13::LOrg(int Origin)
{
	m_LabelOrgn = Origin;
}

//////////////////////////////////////////////////////////////////////

void CPlotter13::Move(CPointDbl To)
{
	m_PenDown = false;
  CDC *pDC = &m_pView->m_GraphicsDC;
	pDC->MoveTo(ToDC(To)); 
	m_CurrentPenPos = To;
	m_LastAbsolutePos = To;
}

//////////////////////////////////////////////////////////////////////

void CPlotter13::Pen(int Number)
{
	if(m_PenNumber != Number) m_PenDown = false;															// selecting a new pen lifts the pen
	m_PenNumber = (Number >= -1) ? ((Number <= 1) ? Number : 1) : -1;
	if(m_PenNumber < 0) m_LineType = 1;																				// a negative pen number resets the line type
}
//////////////////////////////////////////////////////////////////////

void CPlotter13::PenUp(void)
{
	m_PenDown = false;
}

//////////////////////////////////////////////////////////////////////

void CPlotter13::PDir(double Angle)
{
	m_PlotAngle = ToRadians(Angle);
}

//////////////////////////////////////////////////////////////////////

void CPlotter13::Plot(CPointDbl To, int PlotType, int PenCntrl)
{
CPen Pen;

  CDC *pDC = &m_pView->m_GraphicsDC;
	if(m_PenNumber > 0) Pen.CreatePen(PenStyles[m_LineType], 1, m_pDoc->GetForeColour());				// draw pen
	else if(m_PenNumber < 0) Pen.CreatePen(PenStyles[m_LineType], 1, m_pDoc->GetBackColour());	// erase pen
	else Pen.CreatePen(PenStyles[0], 1, m_pDoc->GetForeColour());																// null pen
	CPen *pOldPen = pDC->SelectObject(&Pen);
	if(PenCntrl < 0) m_PenDown = ((abs(PenCntrl) % 2) > 0);																			// negative pen control pen change before action
	switch(PlotType){
		case T_DRAW:{
			m_PenDown = true;
			pDC->LineTo(ToDC(To)); 
			m_CurrentPenPos = To;
			m_LastAbsolutePos = To;
			break;
		}
		case T_IPLOT:{
			if(m_PenDown)	pDC->LineTo(RotatePlot(m_CurrentPenPos + To)); 
			else	pDC->MoveTo(RotatePlot(m_CurrentPenPos + To)); 
			m_CurrentPenPos += To;
			break;
		}
		case T_PLOT:{
			if(m_PenDown)	pDC->LineTo(ToDC(To)); 
			else	pDC->MoveTo(ToDC(To)); 
			m_CurrentPenPos = To;
			m_LastAbsolutePos = To;
			break;
		}
		case T_RPLOT:{
			if(m_PenDown)	pDC->LineTo(RotatePlot(m_LastAbsolutePos + To));													// plot relative to last absolute position 
			else	pDC->MoveTo(RotatePlot(m_LastAbsolutePos + To)); 
			m_CurrentPenPos = m_LastAbsolutePos + To;
			break;
		}
	}
	m_PenDown = ((abs(PenCntrl) % 2) > 0);																											// even for pen up, odd for pen down
	pDC->SelectObject(pOldPen);
	ViewUpdate();
}

//////////////////////////////////////////////////////////////////////

double CPlotter13::Ratio(void)
{
	return m_ScaleRect.Width() / m_ScaleRect.Height();
}

//////////////////////////////////////////////////////////////////////

void CPlotter13::Scale(CRectDbl Scale)
{
	m_InUserUnits = true;
	m_ScaleRect = Scale;
	m_ScaleX = (double)(m_LocateRect.right - m_LocateRect.left) / (Scale.right -	Scale.left);
	m_ScaleY = (double)(m_LocateRect.top - m_LocateRect.bottom) / (Scale.top -	Scale.bottom);
}

//////////////////////////////////////////////////////////////////////

void	CPlotter13::SetCharset(Var_s *pVar, int Width, int	Height, int xKerning, char yKerning)
{
	div_t l = div((pVar->pValue[0].uData.pString->GetLength() * 8), Width);
	m_GCharset.pVar				= pVar;
	m_GCharset.Width			= Width;
	m_GCharset.Height			= Height;
	m_GCharset.xKerning		= xKerning;
	m_GCharset.yKerning		= yKerning;
	m_GCharset.Rows				= (int)l.quot;
	m_GCharset.BitOffset	= 8;//(int)l.rem;
	m_GCharset.IsSet			= true;
}		

//////////////////////////////////////////////////////////////////////

void CPlotter13::SetPointer(CPointDbl To, int CursorType)
{
CPoint Pos = ToDC(To);
CRect Size, Clip;
int State = 0;

  m_CursorType = (CursorType >= 0) ? CursorType : m_CursorType;
	Clip = CRect((int)m_HardClipRect.left, (int)(m_HardLimits.top - m_HardClipRect.top), (int)m_HardClipRect.right, (int)(m_HardLimits.top - m_HardClipRect.bottom));
	Clip.OffsetRect(DEF_PLOT_ORG_X, DEF_PLOT_ORG_Y);
  if(Clip.PtInRect(Pos)) State = ((m_CursorType % 2) == 0) ? 2 : 1;
	switch(State){
		case 1:{																																									// draw full cursor
			Size = Clip;
			break;
		}
		case 2:{																																									// draw small cross cursor
			Size.left = (Pos.x - 10 > Clip.left) ? Pos.x - 10 : Clip.left;															
			Size.right = (Pos.x + 10 < Clip.right) ? Pos.x + 10 : Clip.right;															
			Size.bottom = (Pos.y + 10 < Clip.bottom) ? Pos.y + 10 : Clip.bottom;															
			Size.top = (Pos.y - 10 > Clip.top) ? Pos.y - 10 : Clip.top;															
			break;
		}
	}
	m_pView->SetGraphicsCursor(Pos, Size, State);
	if(State) m_CursorPos = To;
	ViewUpdate();
}

//////////////////////////////////////////////////////////////////////

void CPlotter13::SetUnits(int Unit)
{
	m_InUserUnits = (Unit > 0);
}

//////////////////////////////////////////////////////////////////////

void CPlotter13::Show(CRectDbl Show)
{
double d;

	m_InUserUnits = true;
	double l = m_LocateRect.Width() / m_LocateRect.Height(); 
	if(abs(Show.Width()) < abs(Show.Height())){
		d = l * (Show.Height() / Show.Width());
		Show.top *= d;
		Show.bottom *= d;
		m_ScaleX = m_LocateRect.Width() / Show.Width();
		m_ScaleY = m_ScaleX;
	}
	else{
		d = l * (Show.Width() / Show.Height());
		Show.left *= d;
		Show.right *= d;
		m_ScaleY = m_LocateRect.Height() / Show.Height();
		m_ScaleX = m_ScaleY;
	}
	m_ScaleRect = Show;
}

//////////////////////////////////////////////////////////////////////

void CPlotter13::Unclip(void)
{
	m_ClipRect = m_HardClipRect;																																// make the soft clip area the full plotter area
	m_LocateRect = m_HardClipRect;																															// make the locate clip area the full plotter area
	m_IsSoftClip		= false;
	m_IsLocateClip	= false;
	ClipToHard();
}

//////////////////////////////////////////////////////////////////////

CRectDbl CPlotter13::GetClipRect(void)			
{
int Mode;																														// 1 = SoftClip active, 2 = LocateClip active, 3 = both active, 0 = niether active.

	Mode = (m_IsSoftClip) ? 1 : 0;
	Mode += (m_IsLocateClip) ? 2 : 0;
	switch(Mode){
		case 1:	return m_ClipRect;
		case 2:	return m_LocateRect;
		case 3:{
			CRectDbl rect;
			rect.IntersectRect(m_ClipRect, m_LocateRect);
			return rect;
		}
		default: return m_HardClipRect;
	}
}

//////////////////////////////////////////////////////////////////////

void CPlotter13::ClipToSoft(void)																															// soft & hard clip regions
{
CRgn clipRgn, clipRgnA, clipRgnB;
int Mode;																														// 1 = SoftClip active, 2 = LocateClip active, 3 = both active, 0 = niether active.

  if(m_pDoc == nullptr)	return;
  CDC *pDC = &m_pView->m_GraphicsDC;
	Mode = (m_IsSoftClip) ? 1 : 0;
	Mode += (m_IsLocateClip) ? 2 : 0;
	switch(Mode){
		case 1:{
			clipRgn.CreateRectRgn((int)round(m_ClipRect.left), (int)round(m_HardLimits.top - m_ClipRect.top), (int)round(m_ClipRect.right) + 1, (int)round(m_HardLimits.top - m_ClipRect.bottom) + 1);
			break;
		}
		case 2:{
			clipRgn.CreateRectRgn((int)round(m_LocateRect.left) - 1, (int)round(m_HardLimits.top - m_LocateRect.top) - 1, (int)round(m_LocateRect.right), (int)round(m_HardLimits.top - m_LocateRect.bottom));
			break;
		}
		case 3:{	
			clipRgn.CreateRectRgn((int)round(m_ClipRect.left), (int)round(m_HardLimits.top - m_ClipRect.top), (int)round(m_ClipRect.right) + 1, (int)round(m_HardLimits.top - m_ClipRect.bottom) + 1);
			clipRgn.CreateRectRgn((int)round(m_LocateRect.left) - 1, (int)round(m_HardLimits.top - m_LocateRect.top) - 1, (int)round(m_LocateRect.right), (int)round(m_HardLimits.top - m_LocateRect.bottom));
			clipRgn.CombineRgn(&clipRgnA, &clipRgnB, RGN_AND);
			break;
		}
		default:{
			clipRgn.CreateRectRgn((int)round(m_HardClipRect.left), (int)round(m_HardLimits.top - m_HardClipRect.top), (int)round(m_HardClipRect.right), (int)round(m_HardLimits.top - m_HardClipRect.bottom));
			break;
		}
	}
	m_SoftClipRect = GetClipRect();
	clipRgn.OffsetRgn(DEF_PLOT_ORG_X, DEF_PLOT_ORG_Y);
	CRect rect;
	clipRgn.GetRgnBox(&rect);
	pDC->SelectClipRgn(&clipRgn, RGN_COPY);
}

//////////////////////////////////////////////////////////////////////

void CPlotter13::ClipToHard(void)																															// physical clip region
{
CRgn clipRgn;

  if(m_pDoc == nullptr)	return;
  CDC *pDC = &m_pView->m_GraphicsDC;
	m_SoftClipRect = m_HardClipRect;
	clipRgn.CreateRectRgn((int)m_HardClipRect.left, (int)(m_HardLimits.top - m_HardClipRect.top), (int)m_HardClipRect.right, (int)(m_HardLimits.top - m_HardClipRect.bottom));
	clipRgn.OffsetRgn(DEF_PLOT_ORG_X, DEF_PLOT_ORG_Y);
	pDC->SelectClipRgn(nullptr);
	pDC->SelectClipRgn(&clipRgn, RGN_COPY);
}


