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
#include "BasicDefs.h"

constexpr auto DEF_PIXEL_SIZE				= 3;				// Pixels per mm		
constexpr auto DEF_PLOT_ORG_X				= 50;						
constexpr auto DEF_PLOT_ORG_Y				= 18;						
constexpr auto DEF_PLOT_SIZE_X			= 560;			// Physical size in pixels		
constexpr auto DEF_PLOT_SIZE_Y			= 455;			// 3 pixels per mm	
constexpr auto DEF_PLOT_RATIO				= 1.23;			// X/Y	
constexpr auto DEF_GDU_SCALE				= 4.55;			// Y=100, X=123 	
constexpr auto DEF_LINETYPE					= 1;				// LINETYPE
constexpr auto DEF_LINE_LENGTH			= 4;					
constexpr auto DEF_PLOT_DIRECTION		= 0.0;					
constexpr auto DEF_PEN_SELECT				= 1;				// PEN
constexpr auto DEF_LABEL_ORGIGIN		= 1;				// LORG
constexpr auto DEF_LABEL_DIRECTION	= 0;				// LDIR
constexpr auto DEF_CHAR_SIZE				= 3.3F;			// CSIZE
constexpr auto DEF_CHAR_RATIO				= 0.6F;			
constexpr auto CHARACTER_SCALE			= 1.00;//53;		
constexpr auto SCREEN_MM_SCALE			= 3.04;			// pixels to mm

constexpr auto PS_MAX								= 9;			

typedef	struct GCharset_s{
		Var_s						*pVar;
		int							Width;
		int 						Height;
		int 						xKerning;
		char 						yKerning;
		int							Rows;
		int							BitOffset;
		bool						IsSet;
	} GCharset_s;

//////////////////////////////////////////////////////////////////////

class CS45BasicView;

class CPlotter13 : public CPlotter
{
public:

										CPlotter13();	
  virtual						~CPlotter13();
	virtual void			Initialise(void);
	virtual void			Destroy(void);
	virtual void			ResetParameters(void);
	virtual void			ResetHardware(void);
	virtual void			GetProperties(void);
	virtual void			Clear(void);
	virtual void			Axes(Axes_Grid_s Axes);
	virtual	void			Clip(CRectDbl Clip);
	virtual void			CSize(float CharSize, float m_CharRatio);
	virtual void			Frame(void);
	virtual void			GClear(void);
	virtual void			GPrint(const char *pBitStream, CPoint To, CSze Size, int Flag, int Offset);
	virtual void			Grid(Axes_Grid_s	Grid);
	virtual int				Label(BString *pStr);
	virtual void			LDir(double Dir);
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

	inline void								ViewUpdate(void);

	GCharset_s								GetCharset(void) { return m_GCharset;}; 
	void											SetCharset(Var_s *pVar, int Width, int	Height, int Skew, char Flag);

private:
	CS45BasicView			*m_pView;
	CBitmap						m_BitMap;												// CRT graphics bitmap
	GCharset_s				m_GCharset;
};