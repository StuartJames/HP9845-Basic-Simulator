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

class CS45BasicView;

#include "Strings.h"
#include "StringList.h"

constexpr auto  MAX_SOFT_KEYS			=	63;							
constexpr auto  IDENT_HASHSIZE		= 31;

// Basic types
typedef unsigned char   UCHAR;
typedef unsigned char   BYTE;   // 8-bit unsigned entity
typedef unsigned short  WORD;   // 16-bit unsigned number
typedef unsigned short  USHORT;
typedef unsigned int    UINT;   // machine sized unsigned number (preferred)
typedef unsigned long   DWORD ;  // 32-bit unsigned number
typedef unsigned long   ULONG;
typedef void*    				LPVOID;
typedef DWORD*   				LPDWORD;
typedef WORD*    				LPWORD;
typedef BYTE*    				LPBYTE;
typedef BOOL*    				LPBOOL;
//typedef char*    				LPSTR;
//typedef const char*    	LPCSTR;

enum class ePass{
	DECLARE = 0,
	COMPILE,
	INTERPRET,
	END
};

enum class eVType{
	V_NONE =  0,
	V_ERROR = 1,
	V_INT,
	V_NIL,
	V_REAL,
	V_STRING,
	V_ARRAY,
	V_VOID,
	V_END
};

enum class eOnKeyState{
	INACTIVE = 0,
	ACTIVE,
	FIRED,
	BLOCKED
};

enum class eCntxType{
	RUN = 0,
	EXECUTE,
	END
};

enum class eRunMode {
	STOP = 0,
	GO,
	HALT,
	SUSPEND,
	END
};

enum class eRunState {
	STOPPED = 0,
	RUNNING,
	PAUSED,
	SUSPENDED,
	WAITING,
	END
};

enum class eCaridgeSuppresion{
	NONE = 0,
	NOLF,
	NOCR,
	BOTH,
};

enum class eTrigMode{
	DEG,
	GRAD,
	RAD
};

enum class eRounding{
	STANDARD,
	FIXED,
	FLOAT
};

enum class eDispArea{
	SCROLL = 0,
	PROMPT,
	COMMENT,
};

enum class eMType{
	IF = 1,
	ELSE,
	DO,
	DO_CND,
	FOR,
	FOR_VAR,
	FOR_LIMIT,
	FOR_BODY,
	REPEAT,
	SELECT,
	WHILE,
	FUNC
};

enum class eDType{
	PRINTER = 0,
	GRAPHICS,
	MASS_STORAGE,
	PLOTTER,
};

enum class eOpenType{
	READ = 0,
	WRITE,
	RANDOM,
	BINARY,
	RAND_BIN
};

enum class eEscSequ{
	NONE = 0,											// normal mode
	ESCCHAR,											// ESC character received
	AMPCHAR,											// '&' character received
	LCDCHAR,											// lower case 'd' character received
	LCACHAR,											// lower case 'a' character received
	END
};

enum class eTraceMode{
	NONE = 0,											// normal mode
	FLOW,													// trace program branching
	ALL,													// trace everything
	VARS,													// trace specified variables
	ALLVARS,											// trace all variables
	END
};

//////////////////////////////////////////////////////////////////////////////

typedef union                   // union used for extracting text flags
{
	struct{
    UCHAR Char;
    UCHAR Flags;
  } b;
  USHORT Code;
}ExtChar;

//////////////////////////////////////////////////////////////////////////////

typedef struct Message_s{
	UINT 	Length;
	char 	*pBuffer;
} Message_s;


typedef struct PC_s{
	int Index;
	struct Token_s *pToken;
} PC_s;


typedef struct Scope_s{
	struct PC_s Start;
	struct PC_s Begin;
	struct PC_s End;
	struct Scope_s *pNext;
} Scope_s;

typedef struct Trace_s{
	bool								IsActive;
	int 								From;
	int									To;
	enum eTraceMode			Mode;
	int									WaitPeriod;			// period in milliseconds
	bool								Wait;					 	// wait between trace outputs.
	bool								Pause;					// pause between trace outputs. Note pause and wait are mutualy exclusive.
	struct PC_s		  		*pLastPc;				// jump from PC
	struct Identifier_s *pIdentifier; 	// changed value
	StringList					VarList;
	int									PauseLine;
	int									PauseCount;
} Trace_s;


//////////////////////////////////////////////////////////////////////////////

typedef struct OnKeys_s{
	eOnKeyState State;
	int Priority;
	enum TokenType_e JumpType;
	PC_s Pc;
} OnKeys_s;

//////////////////////////////////////////////////////////////////////////////

typedef struct Value_s{
	enum eVType Type;
	union{
		struct { char *pStr; int Code; int Number; } Error;		/* eVType::V_ERROR   */
		long int Integer;																			/* eVType::V_INT		 */
																													/* eVType::V_NIL     */
		double Real;																					/* eVType::V_REAL    */
		BString *pString;																			/* eVType::V_STRING  */
		struct Var_s *pArray; 																/* eVType::V_ARRAY	 */
																													/* eVType::V_VOID    */
	} uData;
	UINT Index;																							// array index of this value
} Value_s;

typedef Value_s *pValue_s;

//////////////////////////////////////////////////////////////////////////////

typedef struct Geometry_s{
	int  Size;
  int  Base;
} Geometry_s;

typedef struct Var_s{
	int Dimensions;
	Geometry_s *pGeometry;
	Value_s *pValue;
	int Size;
	eVType Type;
	bool IsReference;													// used when an argument of sub-programme is passed by referencce not value
	struct Var_s *pRefVar;										// the variable passed passed as a reference in calling programme
	bool	IsDefined;													// used to signify if a common variable is already defined
} Var_s;

//////////////////////////////////////////////////////////////////////////////

typedef struct Program_s{
	BOOL Numbered;
	int Size;
	int Capacity;
	BOOL Runable;
	BOOL Unsaved;
	BString Name;
	Token_s **ppCode;
	Scope_s *pScope;
} Program_s;

//////////////////////////////////////////////////////////////////////////////

typedef struct Stack_s{
	long 						Position;
	long 						Size;
	long 						FramePosition;
	long 						FrameSize;
	PC_s 						OnErrorPc;
	TokenType_e			OnErrorType;
	union StackSlot *pSlot;
	long 						ErrorLine;
	PC_s 						ErrorPc;
	Value_s 				Error;
	Value_s 				LastDet;
	int 						Resumeable;
	struct Symbol_s	*pLocalSymbols;
	struct Symbol_s	*pAllSymbols;					/* should be hung off the funcs/procs */
} Stack_s;

typedef struct StackFrameSlot_s{
	long						FramePosition;
	long						FrameSize;
	PC_s						Pc;
	int							KeyIndex;
} StackFrameSlot_s;

typedef struct AutoExceptionSlot_s{
	PC_s						OnError;
	int							Resumeable;
} AutoExceptionSlot_s;

union StackSlot{
	StackFrameSlot_s ReturnFrame;
	AutoExceptionSlot_s ReturnException;
	Var_s Var;
};

//////////////////////////////////////////////////////////////////////////////

typedef struct Labels_s{
	Symbol_s *pTable[IDENT_HASHSIZE];
} Labels_s;

//////////////////////////////////////////////////////////////////////////////

typedef	struct MarkerItem_s{
	enum eMType Type;
	PC_s							Pc;
} MarkerItem_s;


typedef	struct MarkerStack_s{
	struct MarkerItem_s		*pStack;
	UINT									Size;
	UINT									Index;
} MarkerStack_s;

//////////////////////////////////////////////////////////////////////////////

typedef	struct LabelStack_s{
	PC_s Line;
	BString Name;
} LabelStack_s;

//////////////////////////////////////////////////////////////////////////////

typedef	struct LineNumber_s{
		PC_s Line;
		struct LineNumber_s *pNextLine;
} LineNumber_s;

//////////////////////////////////////////////////////////////////////////////

typedef struct Registry_s{
	Symbol_s *pTable[IDENT_HASHSIZE];
} Registry_s;

typedef Registry_s *pIdentifiers_s;

//////////////////////////////////////////////////////////////////////////////

typedef struct Xref_s{
	const void	*pKey;
	LineNumber_s	*pLines;
	Xref_s *l, *r;
}Xref_s;

//////////////////////////////////////////////////////////////////////////////

typedef struct IOBuffer_s{				// printers and plotters etc.
	char 						*pBuffer;
	UINT 						FillPos;
	UINT 						DrainPos;
	UINT						Count;
	UINT						Size;
} IOBuffer_s;

//////////////////////////////////////////////////////////////////////////////

typedef struct FileTable_s{
	bool						InUse;
	bool						Verify;
	int							IsBuffered;
	int 						Handle;
	eOpenType				OpenType;
	UINT 						RecordLength;
	char 						*pBuffer;
	UINT 						FillPos;
	UINT 						DrainPos;
	UINT						Count;
	int 						RecordPosition;
} FileTable_s;

typedef FileTable_s *pFileTable_s;

//////////////////////////////////////////////////////////////////////////////

typedef struct SubCntx_s{
	int								OptionBase;
	UINT							RoundingPrecision;
	eRounding					RoundingMode;
	eTrigMode					TrigMode;
	Labels_s					*pLabels;									 // line label identifiers
	PC_s 							BeginData;								 // data pointers for read statement
	PC_s							CurrentData;
	PC_s							NextData;
	FileTable_s				**ppFileTable;						 // FCB for programme and each sub-programme
} SubCntx_s;

typedef SubCntx_s *pSubCntx_s;

//////////////////////////////////////////////////////////////////////////////

typedef struct Context_s{
	MarkerStack_s			Marker;
	PC_s							*pDataListEnd;
	PC_s							Pc;
	OnKeys_s					EventKeys[MAX_SOFT_KEYS];
	eRunState				  RunState;
	eRunMode					RunMode;
	bool							Step;
	bool							WaitingInput;
	bool							WaitingVKey;
	Stack_s						Stack;
	Program_s					Program;
	ePass							Pass;
	bool							OnEventsEnabled;
	bool							IsPassVar;
	bool							IsAssign;
	pSubCntx_s				pSubCntx;									 // main & sub-programme contexts 
} Context_s;

typedef Context_s *pContext_s;

