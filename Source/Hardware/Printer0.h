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

class CPrinter0 : public CPrinter
{
public:
										CPrinter0();	
  virtual						~CPrinter0();
	virtual void			EndPrint(void);
  virtual int				PrintChars(const char *pChars);
	virtual int				GetPrintCol(void){ return m_RowBuf.GetIndex(); };
	virtual int				Flush(void);
	virtual void			CarridgeReturn(void);
	virtual void			LineFeed(void);
	virtual int				PageFeed();
	virtual int				SetCursorColumn(UINT Column);
	virtual int				NextTab(void);
	virtual int 			PrintLIN(int Count);

protected:
	virtual int				PushChar(char ch, UCHAR Flags);
	int								IncDecRow(bool Down);
	int								IncDecCol(bool Left);

private:
	eEscSequ  				m_EscSequence;
	ExTextRow					m_RowBuf;
};
