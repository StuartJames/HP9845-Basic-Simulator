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

constexpr auto  SPLASH_TIME			= 5;

constexpr auto  CLIENTMINSIZEX	= 668;
constexpr auto  CLIENTMINSIZEY	= 500;

constexpr auto  GRAPHCLIPSIZEX	= 560;
constexpr auto  GRAPHCLIPSIZEY	= 455;

// Message screen constants
constexpr auto SCRNROWS					= 30;
constexpr auto SCRNCOLS					= 80;
constexpr auto ENTRYCOLS				= 160;
constexpr auto TABSTOP					= 8;
constexpr auto MAXMRU						= 8;
constexpr auto SCRN_FONT_X		  = 7;
constexpr auto SCRN_FONT_Y			= 12;
constexpr auto SCRN_MARGIN_X		= 50;
constexpr auto SCRN_MARGIN_Y		= 70;

// ASCII characters that receive special processing
constexpr auto ASCII_BEL				= 0x07;
constexpr auto ASCII_BS					= 0x08;
constexpr auto ASCII_TAB				= 0x09;
constexpr auto ASCII_LF					= 0x0A;
constexpr auto ASCII_VTAB				= 0x0B;
constexpr auto ASCII_CR					= 0x0D;
constexpr auto ASCII_XON				= 0x11;
constexpr auto ASCII_XOFF				= 0x13;

// Basic loop message types
constexpr auto CN_SEVENT				= 0x04;
constexpr auto CN_STATE_CHANGE	= 0x05;
constexpr auto CN_CLEAR_SCREEN	= 0x06;
constexpr auto CN_STOPEVENT     = 0x09;

// View update hints
constexpr auto VU_PARAMETERS_CHANGED	= 1L;
constexpr auto VU_DATAUPDATE          = 3L;
constexpr auto VU_LOAD_FILE           = 4L;
constexpr auto VU_CLEAR_DATA					= 8L;

constexpr auto WM_SYSTEMTEXTMESSAGE	  = (WM_USER + 1);
constexpr auto WM_BASICNOTIFY					= (WM_USER + 2);
constexpr auto UM_MOUSEBUTTONNOTIFY		=	(WM_USER + 3);

constexpr auto  SOFT_KEY_COUNT				= 10;
constexpr auto  CONTROL_KEY_COUNT			= 10;

constexpr auto   INVALID_DISPLAY_POSITION			= -1;

constexpr auto   NORMALMODE_SCROLL_TOP				= 0;
constexpr auto   NORMALMODE_SCROLL_SIZE				= 25;
constexpr auto   NORMALMODE_PROMPT_LINE				= 26;		// only visible in normal mode
constexpr auto   NORMALMODE_ENTRY_LINE				= 27;		// double line
constexpr auto   NORMALMODE_COMMENT_LINE			= 29; 

constexpr auto   EDITMODE_ENTRY_LINE					= 12;		// 14
constexpr auto   EDITMODE_COMMENT_LINE				= 14;		// 16
constexpr auto   EDITMODE_SECTION_SIZE				= 11;		
constexpr auto   EDITMODE_LOWER_START_LINE		= 16;		
constexpr auto   EDITMODE_SCROLL_SIZE					= 23;		// 11 + 11 + 5

constexpr auto   NORMALMODE_SYS_KEY_LINE1			= 30;
constexpr auto   NORMALMODE_SYS_KEY_LINE2			= 31;

constexpr auto   SP_PORTBIT										= 0x01;
constexpr auto   SP_BAUDBIT										= 0x02;
constexpr auto   SP_DATABIT										= 0x04;
constexpr auto   SP_PARITYBIT									= 0x08;
constexpr auto   SP_STOPBIT										= 0x10;
constexpr auto   SP_RTSBIT										= 0x20;
constexpr auto   SP_DTRBIT										= 0x40;
constexpr auto   SP_XONXOFFBIT								= 0x80;

//////////////////////////////////////////////////////////////////////////////////////////////

enum COMMS_TYPE {
	CT_INTERNAL = 0,
	CT_PARALLEL,
	CT_SERIAL,
	CT_NETWORK,
};

enum SYSTEM_MODE {
	SYS_IDLE = 0,
	SYS_RUN,
	SYS_PAUSED,
	SYS_EDITLINE,
	SYS_EDITKEY,
	SYS_MODE_COUNT
};

enum DISPLAY_MODES{
	DISPLAY_NORMAL = 0,
	DISPLAY_GRAPHIC,
	DISPLAY_EDIT,
	DISP_MODE_COUNT,
};

enum DISPLAY_RGNS{
	SYS_PROMPT = 0,
	USER_INPUT,
	SYS_COMMENT,
	DISP_AREA_COUNT,
};

typedef struct PrintRgn_t{
	int			Row[DISP_MODE_COUNT];		// regions 'Y' location
	bool		DoubleLine;							// true for edit line, 160 character
	CString Text;										// text buffer
} PrintRgn_t;

typedef struct CommParam_t{
	bool			bConnected;
	UINT			nType;
	UINT			nConnectedType;
 	CString		sPort;
	CString		sTCPAddress;
	UINT			nTCPPort;
	long			nBaud;
	int				nDataBits;
	bool			bDTRDSR;
	int				nParity;
	bool			bRTSCTS;
	int				nStopBits;
	bool			bXONXOFF;
	UCHAR			AckTimeout;
  UCHAR     ResendCount;
} CommParam_t;

typedef struct CommParam_t *PCommParam_t;

