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
#include "./Basic/BasicDefs.h"
#include "HersheyFunctions.h"

/*
*  PDU = Plotter device units, usualy pixels or step resolution.
*  GDU = Graphics device units, 1% of the shortest side usualy the Y coordinate.
*  UDU = User difined units, as defined by SCALE or SHOW. 
*/

const double PI = 3.1415926535897931;

class CHersheyFunctions;
class CPointDbl;
class CRectDbl;

typedef struct Axes_Grid_s{
	CPointDbl		TicSpacing;
	CPointDbl		MajorCount;
	CPointDbl		Intersect;
	UINT				TicSize;
} Axes_Grid_s;

//////////////////////////////////////////////////////////////

class CPlotter : public CDevice
{
public:
										CPlotter();	
  virtual						~CPlotter();

	virtual void			Initialise(void);
	virtual	void			Destroy(void);
	virtual void			ResetParameters(void);
	virtual void			ResetHardware(void);
	virtual void			Axes(Axes_Grid_s Axes);
	virtual	void			Clip(CRectDbl Clip);
	virtual void			CSize(float CharSize, float m_CharRatio);
	virtual void			Frame(void);
	virtual void			GClear(void);
	virtual void			GPrint(const char *pBitStream, CPoint To, CSze Size, int Flag, int Offset);
	virtual void			Grid(Axes_Grid_s	Grid);
	virtual int				Label(BString *pStr);
	virtual void			LDir(double Angle);
	virtual bool			Limit(CRectDbl Limits);
	virtual void			LineType(int Type, int Length);
	virtual void			Locate(CRectDbl ToDC);
	virtual void			LOrg(int Origin);
	virtual void			Move(CPointDbl To);
	virtual void			PDir(double Angle);
	virtual void			Pen(int Number);
	virtual void			PenUp(void);
	virtual void			Plot(CPointDbl To, int PlotType, int PenCntrl);
	virtual double		Ratio(void);
	virtual void			Scale(CRectDbl ToDC);
	virtual void			SetUnits(int Unit);
	virtual void			Show(CRectDbl Show);
	virtual void			Unclip(void);
	virtual void			ClipToSoft(void);
	virtual void			ClipToHard(void);
	virtual CPointDbl	GetPenPos(int *pPenStatus){ *pPenStatus = (m_PenDown) ? 1 : 0; return m_CurrentPenPos; };
	virtual CPointDbl	GetCursorPos(int *pPenStatus){ *pPenStatus = (m_PenDown) ? 1 : 0; return m_CursorPos; };
	virtual void			SetPointer(CPointDbl To, int CursorType);
	virtual CPointDbl	GetHome(void){ return CPointDbl(m_ScaleRect.left + 1, m_ScaleRect.bottom + 1); };

	virtual inline LONG				xToDC(int PosX);
	virtual inline LONG				yToDC(int PosY);
	virtual inline LONG				xToDC(double PosX);
	virtual inline LONG				yToDC(double PosY);
	virtual inline CPoint			ToDC(CPointDbl Point);
	virtual inline CRectDbl		ToDC(CRectDbl Rect);
	virtual inline CPointDbl	ToUser(CPoint Pos);
	virtual inline CRectDbl		ToUser(CRectDbl Rect);
	virtual inline CRectDbl		UDUtoPDU(CRectDbl Rect);
	virtual inline CPointDbl	RotatePoint(CPointDbl Pos);
	virtual	inline CPoint			RotatePlot(CPointDbl Pos);
	virtual inline CRectDbl		GetClipRect(void);

protected:
	bool									m_InUserUnits;
	CRectDbl							m_PhysicalLimits;							// plotting area as seen by the hardware
	CRectDbl							m_HardLimits;									// plotting area as seen by the software
	CRectDbl							m_HardClipRect;										// clipping area set by the LIMIT	statement
	CRectDbl							m_LocateRect;									// clipping area set by the LOCATE statement
	CRectDbl							m_ClipRect;										// clipping area set by the CLIP	statement
	CRectDbl							m_SoftClipRect;								// intersection of LOCATE and CLIP clipping areas
	CRectDbl							m_ScaleRect;
	bool									m_IsSoftClip;									// set when any soft clip statement is active
	bool									m_IsLocateClip;								// set when the LOCATE statement is active
	double								m_ScaleX;
	double								m_ScaleY;
	double								m_ScaleGDU;
	int										m_LineType,	m_LineTypeLen;
	double								m_CharSize, m_CharRatio;
	int										m_LabelOrgn;
	double								m_LabelAngle;
	double								m_PlotAngle;
	bool									m_PenDown;
	int										m_PenNumber;
	CPointDbl							m_CurrentPenPos;
	CPointDbl							m_LastAbsolutePos;						// used for relative plot movement
	CPointDbl							m_CursorPos;
	int										m_CursorType;
	CHersheyFunctions			*m_pPlotterFont;
};
