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
#include "BasicDefs.h"
#include <sys/locking.h>
#include <fcntl.h>      /* Needed only for _O_RDWR definition */
#include <io.h>
#include <stdlib.h>
#include <stdio.h>
#include <share.h>

static int						m_DevicesAvailable = MAXIMUM_DEVICES;
static CDevice				*pDevices[MAXIMUM_DEVICES];

CMassStorage					Tape14(_T(":T14"), _T("./T14/"));
CMassStorage					Tape15(_T(":T15"), _T("./T15/"));
CPrinter0							Printer0;
CPrinter16						Printer16;
CPlotter13						Plotter13;

static int	CloseDevice(int SelectCode);
static int	DeviceRegisterDisplay();
static int	DeviceRegisterPrinter();
static int	DeviceRegisterGraphics();
static int	DeviceRegisterT14();
static int	DeviceRegisterT15();

//////////////////////////////////////////////////////////////////////

int InitialiseHardware(void)
{
	InitDevices();
	SetPrinterIs(SC_TEXT_DISPLAY, DEF_LINE_WIDTH);								// the standard printer
	g_pSysPrinter = g_pPrinter;																		// all system messages go here
	SetPlotterIs(SC_GRAPHICS, 0);																	// make the screen the default plotter
	MassStorageIs(SC_STANDARD_TAPE);															// make the mass storage device the default tape drive
	return 0;
}

//////////////////////////////////////////////////////////////////////

int CloseHardware(void)
{
	for(int i = 0; i < MAXIMUM_DEVICES; i++) CloseDevice(i);
	return 0;
}

//////////////////////////////////////////////////////////////////////

inline int GetErrorMessage(BOOL clear)
{
	if(clear)	g_SufixErrorMsg.Clear();
	else{
		char Buff[MSG_BUFFER_SIZE];
		strerror_s(Buff, MSG_BUFFER_SIZE, errno);
		g_SufixErrorMsg.AppendChars(Buff);
	}
	return DEV_ERROR;
}

//////////////////////////////////////////////////////////////////////

inline int SetErrorMessage(const char *Msg)
{
	g_SufixErrorMsg.AppendChars(Msg);
	return DEV_ERROR;
}

//////////////////////////////////////////////////////////////////////

static int RegisterDevice(CDevice *pDev, int SelectCode)
{
	if(SelectCode < MAXIMUM_DEVICES){
		if(pDevices[SelectCode] == nullptr){
			pDevices[SelectCode] = pDev;
			pDev->SetSelectCode(SelectCode);
			g_SufixErrorMsg.Clear();
			return 0;
		}
	}
	return DEV_ERROR;
}

//////////////////////////////////////////////////////////////////////

int InitDevices(void)
{
	DeviceRegisterDisplay();
	DeviceRegisterPrinter();
	DeviceRegisterGraphics();
	DeviceRegisterT14();
	DeviceRegisterT15();
	return 0;
}

//////////////////////////////////////////////////////////////////////

static int CloseDevice(int SelectCode)
{
	if(SelectCode <= MAXIMUM_DEVICES){
		if(pDevices[SelectCode] != nullptr){
			pDevices[SelectCode]->Destroy();
			pDevices[SelectCode] = nullptr;
			return 0;
		}
	}
	return DEV_ERROR;
}

//////////////////////////////////////////////////////////////////////

int FindDevice(BString *DeviceName, eDType Type)
{
int i = 0;

	do{
		if(pDevices[i] != nullptr){
			if(!pDevices[i]->CompareName(DeviceName->GetBuffer()) && (pDevices[i]->GetType() == Type)) return i;
		}
	}
	while(++i <= MAXIMUM_DEVICES);
	return DEV_ERROR;
}

//////////////////////////////////////////////////////////////////////

static int DeviceRegisterDisplay()
{
	if(RegisterDevice(&Printer16, SC_TEXT_DISPLAY) == DEV_ERROR){
		SetErrorMessage(_T("Printer device 16 registration failed"));
		return DEV_ERROR;
	}
	Printer16.SetType(eDType::PRINTER);
	Printer16.SetDevName(_T(":CRT"));
	Printer16.Initialise();
	return DEV_OK;
}

//////////////////////////////////////////////////////////////////////

static int DeviceRegisterPrinter()
{
	if(RegisterDevice(&Printer0, SC_EXTERN_PRINTER) == DEV_ERROR) return SetErrorMessage(_T("Printer device 0 registration failed"));
	Printer0.SetType(eDType::PRINTER);
	Printer0.SetDevName(_T(":LPT"));
	Printer0.Initialise();
	Printer0.SetForeColour(RGB(0, 0, 0));
	return DEV_OK;
}

//////////////////////////////////////////////////////////////////////

static int DeviceRegisterGraphics()
{
	if(RegisterDevice(&Plotter13, SC_GRAPHICS) == DEV_ERROR) return SetErrorMessage(_T("Plotter device 13 registration failed"));
	Plotter13.SetType(eDType::GRAPHICS);
	Plotter13.SetDevName(_T(":GPH"));
	Plotter13.Initialise();
	return DEV_OK;
}

//////////////////////////////////////////////////////////////////////

static int DeviceRegisterT14()
{
	if(RegisterDevice(&Tape14, SC_OPTION_TAPE) == DEV_ERROR) return	SetErrorMessage(_T("Tape device 14 registration failed"));
	Tape14.SetType(eDType::MASS_STORAGE);
	Tape14.SetDevName(_T(":T14"));
	Tape14.Initialise();
	return DEV_OK;
}

//////////////////////////////////////////////////////////////////////

static int DeviceRegisterT15()
{
	if(RegisterDevice(&Tape15, SC_STANDARD_TAPE) == DEV_ERROR) return SetErrorMessage(_T("Tape device 15 registration failed"));
	Tape15.SetType(eDType::MASS_STORAGE);
	Tape15.SetDevName(_T(":T15"));
	Tape15.Initialise();
	return DEV_OK;
}

//////////////////////////////////////////////////////////////////////

int SelectDevice(int Device, eDType Type , CDevice **ppDevice)
{
	if((Device < MAXIMUM_DEVICES) && (pDevices[Device]->GetType() == Type)){
		*ppDevice = pDevices[Device];
		return DEV_OK;
	}
	return DEV_ERROR;
}

//////////////////////////////////////////////////////////////////////

int SetPrinterIs(int Device, int Width)
{
CDevice *pDevice = nullptr;

	if(SelectDevice(Device, eDType::PRINTER , &pDevice) == DEV_ERROR) return DEV_ERROR;
	g_pPrinter = (CPrinter*)pDevice;
	g_pPrinter->SetLineWidth(Width);
	return DEV_OK;
}

//////////////////////////////////////////////////////////////////////

int SetPlotterIs(int Device, int Address)
{
CDevice *pDevice = nullptr;

	if(SelectDevice(Device, (Device == SC_GRAPHICS) ? eDType::GRAPHICS : eDType::PLOTTER, &pDevice) == -1) return DEV_ERROR;
	g_pPlotter = (CPlotter*)pDevice;
	g_pPlotter->ResetParameters();
	g_pPlotter->ResetHardware();
	return DEV_OK;
}

//////////////////////////////////////////////////////////////////////

int	MassStorageIs(int Device)
{
CDevice *pDevice = nullptr;

  if(g_pMassStorage != nullptr) g_pMassStorage->CloseAll();													// close all active open files before switching device 
	if(SelectDevice(Device, eDType::MASS_STORAGE, &pDevice) == -1) return DEV_ERROR;
	g_pMassStorage = (CMassStorage*)pDevice;
	return DEV_OK;
}

//////////////////////////////////////////////////////////////////////

int DevicePortInput(int address)
{
	return SetErrorMessage(_T("Direct port access not available"));
}

//////////////////////////////////////////////////////////////////////

int DevicePortOutput(int address, int value)
{
	return SetErrorMessage(_T("Direct port access not available"));
}

//////////////////////////////////////////////////////////////////////
