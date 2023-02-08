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

/*constexpr auto MAXIMUM_DEVICES				= 17;				// 16 base 1

// reserved select codes
constexpr auto SC_EXTERN_PRINTER			= 0;
constexpr auto SC_GRAPHICS						= 13;
constexpr auto SC_OPTION_TAPE					= 14;
constexpr auto SC_STANDARD_TAPE				= 15;
constexpr auto SC_TEXT_DISPLAY				= 16;

constexpr auto DEVICE_BUFFER_SIZE			= 1024;		*/

enum Dev_State_e{
	DEV_ERROR = -1,
	DEV_OK = 0,
	DEV_EOF,
	DEV_EOR,
};

extern char	g_DeviceErrorMsg[];

extern inline int	GetErrorMessage(BOOL clear);
extern inline int	SetErrorMessage(const char *Msg);

int		InitialiseHardware(void);
int		CloseHardware(void);
int 	InitDevices(void);
int		SelectDevice(int Device, eDType Type , CDevice **ppDevice);
int		FindDevice(BString *DeviceName, eDType Type);
int 	DevicePortInput(int Address);
int 	DevicePortOutput(int Address, int Value);
int		SetPrinterIs(int Device, int Width);
int		SetPlotterIs(int Device, int Address);
int		MassStorageIs(int Device);

