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

#include "BasicTypes.h"
#include "../Hardware/Doubles.h"
#include "LexScanner.h"
#include "Strings.h"
#include "Stringlist.h"
#include "Stack.h"
#include "Basic.h"
#include "BasicErrors.h"
#include "BiultInFunctions.h"
#include "GlobalRegistry.h"
#include "ComRegistry.h"
#include "LocalRegistry.h"
#include "LabelRegistry.h"
#include "../Hardware/Device.h"
#include "../Hardware/ExTextRow.h"
#include "../Hardware/MassStorage.h"
#include "../Hardware/Plotter.h"
#include "../Hardware/Printer.h"
#include "../Hardware/Printer0.h"
#include "../Hardware/Printer16.h"
#include "../Hardware/Plotter13.h"
#include "Devices.h"
#include "Program.h"
#include "Values.h"
#include "Variables.h"
#include "Statement.h"
#include "Graphics.h"
#include "MainFrm.h"
#include "S45BasicDoc.h"
#include "assert.h"


class CMainFrame;

//#define VERSION "System 45 Basic Version 1.1.5"

constexpr auto  DEF_MAX_DIMENSIONS  = 6;
//constexpr auto  CHAR_SIZE					  = sizeof(char);
constexpr auto  DEF_LINE_WIDTH		  = 80;
constexpr auto  DEF_TAB_WIDTH			  = 20;
constexpr auto  WAIT_CALIBRATION	  =	3.23;							// wait statement calibration factor
constexpr auto  DEF_BEEP_FREQU		  = 480;
constexpr auto  DEF_BEEP_LENGTH		  = 300;
constexpr auto  DEF_MAX_PRINT_WIDTH = 265;
constexpr auto  DEF_PRINT_MARGIN_X  = 150;
constexpr auto  DEF_PRINT_MARGIN_Y  = 150;

constexpr auto  FILE_BUFFER_SIZE	  = 1024;
constexpr auto  MSG_BUFFER_SIZE		  = 128;

 																										/* These macros hide type casts */
#define CAST(x, y) ((y)(x))
#define TOINT(x) ((int)(x))
#define TOFLOAT(x) ((double)(x))
#define TOSTRING(x) ((char *)(x))
#define TOINTADDR(x) ((int *)(x))

extern const char			FileTypes[5][6];  

extern const eVType ValueCommonType[eVType::V_END][eVType::V_END];

extern CMainFrame*		g_pWnd;
extern Context_s			SystemContexts[eCntxType::END];
extern pContext_s			pCntx;
extern CPlotter				*g_pPlotter;;
extern CPrinter				*g_pPrinter;
extern CPrinter				*g_pSysPrinter;
extern CMassStorage		*g_pMassStorage;
extern void           *g_pBasicDoc;
extern Common_s				g_CommonVars;
extern Registry_s*    g_pGlobalRegistry;								
extern int						EditLineNumber;
extern BString				g_SufixErrorMsg;
extern Value_s				g_ResultValue;
extern Trace_s				g_Trace;
extern bool           g_EventPending;
extern bool           g_ControlCodesDisabled;
extern bool           g_EscapeDisabled;

bool          BiuldStringUsing(pValue_s pVal, BString *pDestStr, BString *pUsingStr, UINT *pUsingPos);
pValue_s      BuildArrayStringUsing(pValue_s pValue, BString *pDestStr, BString *pUsingStr, UINT *pUsingPos);
BOOL          GetInput(BString *pStr);
unsigned int  Hash(const char *pStr);


