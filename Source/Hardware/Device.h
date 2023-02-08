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

constexpr auto MAXIMUM_DEVICES				= 17;				// 16 base 1

// reserved select codes
constexpr auto SC_EXTERN_PRINTER			= 0;
constexpr auto SC_GRAPHICS						= 13;
constexpr auto SC_OPTION_TAPE					= 14;
constexpr auto SC_STANDARD_TAPE				= 15;
constexpr auto SC_TEXT_DISPLAY				= 16;

constexpr auto BEL_CHAR								= 0x07;
constexpr auto BS_CHAR								= 0x08;
constexpr auto HT_CHAR								= 0x09;
constexpr auto LF_CHAR								= 0x0A;
constexpr auto FF_CHAR								= 0x0C;
constexpr auto CR_CHAR								= 0x0D;
constexpr auto ESC_CHAR								= 0x1B;

constexpr auto DEVICE_BUFFER_SIZE			= 1024;

class CS45BasicDoc;

class CDevice
{
public:

										CDevice();	
										CDevice(CString Name);
  virtual						~CDevice();
	eDType						GetType(void){ return m_Type; };
	void							SetType(eDType type){ m_Type = type; };
	int								GetSelectCode(void){ return m_SelectCode; };
	void							SetSelectCode(int code){ m_SelectCode = code; };
	CString						GetDevName(void){ return m_DevName; };
	void							SetDevName(CString name){ m_DevName = name; };
	int								CompareName(LPCSTR str){ return m_DevName.CompareNoCase(str); };
	virtual void			Initialise(void) = 0;
	virtual void			Destroy(void) = 0;
	virtual void			GetProperties(void);
	virtual void			Clear(void);

	CS45BasicDoc			*m_pDoc;
	bool							m_IsMultiUse;

private:
	eDType						m_Type;
	int								m_SelectCode;
	CString						m_DevName;

};
