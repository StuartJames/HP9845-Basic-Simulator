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

class CPrinter : public CDevice
{
public:
										CPrinter();	
  virtual						~CPrinter();
	virtual bool 			IsCRT(void){ return false; };
	virtual void			Initialise(void);
	virtual	void			Destroy(void);
	virtual void			EndPrint(void);
	virtual void			SetLineWidth(int width){ m_LineWidth = width; };
	virtual int 			GetLineWidth(void){ return m_LineWidth; };
	virtual void			SetForeColour(COLORREF Colour){ m_ColorFore = Colour; };
	virtual void			RenderText(CDC *pDC, CPoint To, LPTSTR pStr, bool DblLine, bool ShowCodes = false);
	virtual int				GetPrintCol(void){ return 0; };
	virtual int				Flush(void);
	virtual int				NextTab(void);
	virtual void			CarridgeReturn(void);
	virtual void			LineFeed(void);
	virtual int				PageFeed(void);
	virtual int				SetCursorColumn(UINT Column);
	virtual int				SetCursorRow(UINT Line);
	virtual int				PrintChars(const char *pChars);
	virtual int				PrintString(BString *pStr){ return PrintChars(pStr->GetBuffer()); };
	virtual int				PrintFormat(LPCSTR lpStrFmt, ...);
	virtual int				PrintText(eDispArea DisplayArea, LPCSTR lpStr);
	virtual int				PrintArray(pValue_s pValue, bool Tabbed);
	virtual int 			PrintLIN(int Count);

protected:
	inline void				RenderChar(CDC *pDC, int x, int y, BYTE c, bool Inverse, bool Undline);
	virtual int				PushChar(char ch);

	int 							m_LineWidth;
	int 							m_TabWidth;
	COLORREF					m_ColorFore;
};
