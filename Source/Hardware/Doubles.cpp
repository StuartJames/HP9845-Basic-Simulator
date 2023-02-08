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
#include "Doubles.h"


//////////////////////////////////////////////////////////////////////

CSze::CSze(void) 
{
	sx = 0;
	sy = 0;
}

//////////////////////////////////////////////////////////////////////

CSze::CSze(int inx, int iny)
{
	sx = inx;
	sy = iny;
}

//////////////////////////////////////////////////////////////////////

CSze::~CSze(void)
{
}

//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////

CPointDbl::CPointDbl(void) 
{
	x = 0.0;
	y = 0.0;
}

//////////////////////////////////////////////////////////////////////

CPointDbl::CPointDbl(double inx, double iny)
{
	x = inx;
	y = iny;
}

//////////////////////////////////////////////////////////////////////

CPointDbl::~CPointDbl(void)
{
}

//////////////////////////////////////////////////////////////////////

inline CPointDbl::CPointDbl(PointDbl_s iPt)
{
	*(PointDbl_s*)this = iPt;
}

//////////////////////////////////////////////////////////////////////

void CPointDbl::operator+=(CPointDbl iPt)
{
	x += iPt.x;
	y += iPt.y;
}

//////////////////////////////////////////////////////////////////////

void CPointDbl::operator-=(CPointDbl iPt) 
{
	x -= iPt.x;
	y -= iPt.y;
}

//////////////////////////////////////////////////////////////////////

CPointDbl CPointDbl::operator+(CPointDbl iPt) const
{
	return CPointDbl(x + iPt.x, y + iPt.y);
}

//////////////////////////////////////////////////////////////////////

CPointDbl CPointDbl::operator-(CPointDbl iPt) const
{
	return CPointDbl(x - iPt.x, y - iPt.y);
}

//////////////////////////////////////////////////////////////////////

void CPointDbl::operator=(const CPointDbl &srcPoint) throw()
{
	CopyPointDbl(&srcPoint);
}

//////////////////////////////////////////////////////////////////////

void CPointDbl::operator=(const CPoint &SrcPoint) throw()
{
	x = SrcPoint.x;
	y = SrcPoint.y;
}

//////////////////////////////////////////////////////////////////////

inline void CPointDbl::CopyPointDbl(const CPointDbl *pSrcPoint)
{
	x = pSrcPoint->x;
	y = pSrcPoint->y;
}

//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////

CRectDbl::CRectDbl() throw()
{
	left = 0.0;
	top = 0.0;
	right = 0.0;
	bottom = 0.0;
}

//////////////////////////////////////////////////////////////////////

CRectDbl::CRectDbl(double l, double t, double r, double b) throw()
{
	left = l;
	top = t;
	right = r;
	bottom = b;
}

//////////////////////////////////////////////////////////////////////

CRectDbl::CRectDbl(const CRectDbl &srcRect) throw()
{
	CopyRectDbl(&srcRect);
}

//////////////////////////////////////////////////////////////////////

CRectDbl::CRectDbl(CRectDbl *pSrcRect) throw()
{
	CopyRectDbl(pSrcRect);
}

//////////////////////////////////////////////////////////////////////

CRectDbl::CRectDbl(CPointDbl topLeft, CPointDbl bottomRight) throw()
{
	left = topLeft.x;
	top = topLeft.y;
	right = bottomRight.x;
	bottom = bottomRight.y;
}

//////////////////////////////////////////////////////////////////////

CRect	CRectDbl::Rect(void) throw()
{
CRect rect((int)left, (int)top, (int)right, (int)bottom);

  return rect;
}

//////////////////////////////////////////////////////////////////////

CPointDbl CRectDbl::TopLeft() const throw()
{
  return CPointDbl(left, top);
}

//////////////////////////////////////////////////////////////////////

CPointDbl CRectDbl::BottomRight() const throw()
{
	return CPointDbl(right, bottom);
}

//////////////////////////////////////////////////////////////////////

CPointDbl CRectDbl::CenterPoint() const throw()
{
	return CPointDbl((left + right) / 2, (top + bottom) / 2);
}

//////////////////////////////////////////////////////////////////////

bool	CRectDbl::PtInRect(CPoint Point) const throw()
{
	return ((Point.x >= (long)left) && (Point.x <= (long)right) && (Point.y <= (long)top) && (Point.y >= (long)bottom));
}

//////////////////////////////////////////////////////////////////////

bool	CRectDbl::PtInRect(CPointDbl Point) const throw()
{
	return ((Point.x >= left) && (Point.x <= right) && (Point.y <= top) && (Point.y >= bottom));
}

//////////////////////////////////////////////////////////////////////

bool CRectDbl::IsNull() const throw()
{
	return ((left == 0) && (right == 0) && (top == 0) && (bottom == 0));
}

//////////////////////////////////////////////////////////////////////

void CRectDbl::SetRect(double x1, double y1, double x2, double y2) throw()
{
	left = x1;
	right = x2;
	top = y2;
	bottom = y1;
}

//////////////////////////////////////////////////////////////////////

void CRectDbl::SetRect(CPointDbl topLeft, CPointDbl bottomRight) throw()
{
	left = topLeft.x;
	right = bottomRight.x;
	top = topLeft.y;
	bottom = bottomRight.y;
}

//////////////////////////////////////////////////////////////////////

void CRectDbl::SetEmpty() throw()
{
	left = 0.0;
	right = 0.0;
	top = 0.0;
	bottom = 0.0;
}

//////////////////////////////////////////////////////////////////////

void CRectDbl::CopyRect(CRectDbl *pSrcRect) throw()
{
	CopyRectDbl(pSrcRect);
}

//////////////////////////////////////////////////////////////////////

void CRectDbl::operator=(const CRectDbl &srcRect) throw()
{
	CopyRectDbl(&srcRect);
}

//////////////////////////////////////////////////////////////////////

void CRectDbl::operator=(const CRect &srcRect) throw()
{
	left = (double)srcRect.left;
	right = (double)srcRect.right;
	top = (double)srcRect.top;
	bottom = (double)srcRect.bottom;
}

//////////////////////////////////////////////////////////////////////

inline bool CRectDbl::operator==(const CRectDbl &rect) const throw()
{
	return (left == rect.left && right == rect.right && top == rect.top && bottom == rect.bottom);
}

//////////////////////////////////////////////////////////////////////

inline bool CRectDbl::operator!=(const CRectDbl &rect) const throw()
{
	return !(left == rect.left && right == rect.right && top == rect.top && bottom == rect.bottom);
}

//////////////////////////////////////////////////////////////////////

inline void CRectDbl::operator+=(CPointDbl point) throw()
{
	Offset(point.x, point.y);
}

//////////////////////////////////////////////////////////////////////

inline void CRectDbl::operator+=(CRectDbl *pRect) throw()
{
	InflateRect(pRect);
}

//////////////////////////////////////////////////////////////////////

inline void CRectDbl::operator-=(CPointDbl point) throw()
{
	Offset(-point.x, -point.y);
}

//////////////////////////////////////////////////////////////////////

inline void CRectDbl::operator-=(CRectDbl *pRect) throw()
{
	DeflateRect(pRect);
}

//////////////////////////////////////////////////////////////////////

inline CRectDbl CRectDbl::operator+(CPointDbl point) const throw()
{
	CRectDbl rect(*this);
	rect.Offset(point.x, point.y);
	return rect;
}

//////////////////////////////////////////////////////////////////////

inline CRectDbl CRectDbl::operator-(CPointDbl point) const throw()
{
	CRectDbl rect(*this);
	rect.Offset(-point.x, -point.y);
	return rect;
}

//////////////////////////////////////////////////////////////////////

inline CRectDbl CRectDbl::operator+(CRectDbl *pRect) const throw()
{
	CRectDbl rect(*this);
	rect.InflateRect(pRect);
	return rect;
}

//////////////////////////////////////////////////////////////////////

inline CRectDbl CRectDbl::operator-(CRectDbl *pRect) const throw()
{
	CRectDbl rect(*this);
	rect.DeflateRect(pRect);
	return rect;
}

//////////////////////////////////////////////////////////////////////

void CRectDbl::Normalize(void) throw()
{
double d;

	if(left > right){
		d = left;
		left = right;
		right = d;
	}
	if(top > bottom){
		d = top;
		top = bottom;
		bottom = d;
	}
}

//////////////////////////////////////////////////////////////////////

void CRectDbl::Offset(double x, double y) throw()
{
	left += x;
	right += x;
	top += y;
	bottom += y;
}

//////////////////////////////////////////////////////////////////////

void CRectDbl::Offset(PointDbl_s point) throw()
{
	left += point.x;
	right += point.x;
	top += point.y;
	bottom += point.y;
}

//////////////////////////////////////////////////////////////////////

void	CRectDbl::SwapRect(bool x, bool y) throw()
{
double temp;

  if(x){
		temp = left;
		left = right;
		right = temp;
	}
	if(y){
		temp = top;
		top = bottom;
		bottom = temp;
	}
}

//////////////////////////////////////////////////////////////////////

void CRectDbl::InflateRect(double x, double y) throw()
{
	left -= x;
	top -= y;
	right += x;
	bottom += y;
}

//////////////////////////////////////////////////////////////////////

void CRectDbl::InflateRect(CRectDbl *pRect) throw()
{
	left -= pRect->left;
	top -= pRect->top;
	right += pRect->right;
	bottom += pRect->bottom;
}

//////////////////////////////////////////////////////////////////////

void CRectDbl::InflateRect(double l, double t, double r,	double b) throw()
{
	left -= l;
	top -= t;
	right += r;
	bottom += b;
}

//////////////////////////////////////////////////////////////////////

inline void CRectDbl::DeflateRect(double x,	double y) throw()
{
	left += x;
	top += y;
	right -= x;
	bottom -= y;
}

//////////////////////////////////////////////////////////////////////

void CRectDbl::DeflateRect(CRectDbl *pRect) throw()
{
	left += pRect->left;
	top += pRect->top;
	right -= pRect->right;
	bottom -= pRect->bottom;
}

//////////////////////////////////////////////////////////////////////

void CRectDbl::DeflateRect(double l,	double t,	double r,	double b) throw()
{
	left += l;
	top += t;
	right -= r;
	bottom -= b;
}

//////////////////////////////////////////////////////////////////////

void CRectDbl::CopyRectDbl(const CRectDbl *pSrcRect)
{
	left = pSrcRect->left;
	top = pSrcRect->top;
	right = pSrcRect->right;
	bottom = pSrcRect->bottom;
}

//////////////////////////////////////////////////////////////////////

bool CRectDbl::ConstrainRect(CRectDbl *pRect) throw()
{
bool IsModified = false;

	if(pRect->left < left){ pRect->left = left; IsModified = true; };
	if(pRect->top > top){ pRect->top = top; IsModified = true; };
	if(pRect->right > right){ pRect->right = right; IsModified = true; };
	if(pRect->bottom < bottom){ pRect->bottom = bottom; IsModified = true; };
	return IsModified;
}

//////////////////////////////////////////////////////////////////////

void CRectDbl::ScaleRect(double x, double y) throw()
{
	left *= x;
	top *= y;
	right *= x;
	bottom *= y;
}

//////////////////////////////////////////////////////////////////////

CRectDbl CRectDbl::IntersectRect(CRectDbl Rect1, CRectDbl Rect2) throw()
{
CRectDbl rect;

	rect.left = (Rect1.left < Rect2.left) ? Rect1.left : Rect2.left;
	rect.right = (Rect1.right > Rect2.right) ? Rect1.right : Rect2.right;
	rect.bottom = (Rect1.bottom < Rect2.bottom) ? Rect1.bottom : Rect2.bottom;
	rect.top = (Rect1.top > Rect2.top) ? Rect1.top : Rect2.top;
	return rect;
}

//////////////////////////////////////////////////////////////////////
