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

#include "stdafx.h"


/////////////////////////////////////////////////////////////////////////////

typedef struct Sze_s
{
	int  sx;
	int  sy;
} Sze_s, *pSze;

/////////////////////////////////////////////////////////////////////////////

class CSze
{
public:
										CSze();	
										CSze(int inx, int iny);	
										CSze(Sze_s iSze);
  virtual						~CSze();

	int								sx;
	int								sy;
};

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

typedef struct PointDbl_s
{
	double  x;
	double  y;
} PointDbl_s, *pPointDbl;

/////////////////////////////////////////////////////////////////////////////

class CPointDbl
{
public:
										CPointDbl();	
										CPointDbl(double inx, double iny);	
										CPointDbl(PointDbl_s iPt);
  virtual						~CPointDbl();
	void							CopyPointDbl(const CPointDbl *pSrcPt) throw();														
	void							operator=(const CPointDbl &srcPt);
	void							operator=(const CPoint &srcPoint) throw();
	void							operator+=(CPointDbl iPt);
	void							operator-=(CPointDbl iPt);
 	CPointDbl					operator+(CPointDbl iPt) const;
 	CPointDbl					operator-(CPointDbl iPt) const;


	double						x;
	double						y;

};

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

typedef struct RectDbl_s
{
	double	left;
  double	right;
  double	top;
  double	bottom;
} RectDbl_s, *pRectDbl_s;

typedef const RectDbl_s FAR* lpRectDbl_s;

/////////////////////////////////////////////////////////////////////////////

class CRectDbl : public RectDbl_s
{
public:
												CRectDbl() throw();																								// uninitialized rectangle
												CRectDbl(double l, double t, double r, double b) throw();					// from left, top, right, and bottom
												CRectDbl(const CRectDbl &srcRect) throw();												// copy constructor
												CRectDbl(CRectDbl *pSrcRect) throw();															// from a pointer to another rect
												CRectDbl(CPointDbl topLeft, CPointDbl bottomRight) throw();				// from two points
	CRect									Rect(void) throw();																								// get as ints
	double								Width(void) const throw() {	return abs(right - left); };					// retrieves the width
	double								Height(void) const throw(){ return abs(bottom - top); };					// returns the height
	CPointDbl							TopLeft(void) const throw();																					// const reference to the top-left point
	CPointDbl							BottomRight(void) const throw();																			// const reference to the bottom-right point
	CPointDbl							CenterPoint(void) const throw();																			// the geometric center point of the rectangle
	bool									PtInRect(CPoint Point) const throw();															// determines whether the specified point lies within this rect
	bool									PtInRect(CPointDbl Point) const throw();												  // determines whether the specified point lies within this rect
	bool									IsNull() const throw();																						// test for null rectangle
	void									SetRect(double x1, double y1, double x2, double y2) throw();			// set rectangle from left, top, right, and bottom
	void									SetRect(CPointDbl topLeft, CPointDbl bottomRight) throw();
	void									SetEmpty(void) throw();																								// empty the rectangle
	void									CopyRect(CRectDbl *pSrcRect) throw();															// copy from another rectangle
	void									Normalize(void) throw();
	void									InflateRect(double x, double y) throw();
	void									InflateRect(CRectDbl *pRect) throw();
	void									InflateRect(double l, double t, double r, double b) throw();
	void									DeflateRect(double x, double y) throw();
	void									DeflateRect(CRectDbl *pRect) throw();
	void									DeflateRect(double l, double t, double r, double b) throw();
	void									SwapRect(bool x, bool y) throw();
	void									CopyRectDbl(const CRectDbl *pSrcRect) throw();														
	void									Offset(double x, double y) throw();
	void									Offset(PointDbl_s point) throw();
	bool									ConstrainRect(CRectDbl *pRect) throw();
	void									ScaleRect(double x, double y) throw();
	CRectDbl							IntersectRect(CRectDbl Rect1, CRectDbl Rect2) throw();
	void 									operator=(const CRectDbl &srcRect) throw();
	void									operator=(const CRect &srcRect) throw();
	bool									operator==(const CRectDbl &rect) const throw();
	bool									operator!=(const CRectDbl &rect) const throw();
	void									operator+=(CPointDbl point) throw();
	void									operator+=(CRectDbl *pRect) throw();
	void									operator-=(CPointDbl point) throw();
	void									operator-=(CRectDbl *pRect) throw();

// Operators returning CRectDbl values
	CRectDbl							operator+(CPointDbl point) const throw();
	CRectDbl							operator-(CPointDbl point) const throw();
	CRectDbl							operator+(CRectDbl *pRect) const throw();
	CRectDbl							operator-(CRectDbl *pRect) const throw();
};

