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


enum class eSymType {
	LOCALVAR = 0,
	LOCALARRAY,
	COMNVAR,
	COMNARRAY,
	STACKVAR,
	BUILTINFUNCTION,
	USERFUNCTION,
	SUBPROGRAMME,
	LOCALLABEL
};

/////////////////////////////////////////////////////////////////////////////

typedef struct Symbol_s{
	char *pName;
	eSymType SymbolType;																											// contains hash of program/function name or 0 for global symbols
	union{
		Var_s Var;																																			// eSymType::LOCALVAR, eSymType::LOCALARRAY, eSymType::COMNVAR, eSymType::COMNARRAY 
		struct{																																					// eSymType::STACKVAR 
			int Offset;											
			enum eVType ValType;
		} Stack;
		struct{
			union{
				struct{																																			// eSymType::BUILTINFUNCTION 
					pValue_s (* call)(Value_s *pValue, Stack_s *pStack);
					struct Symbol_s *pNext;
				} uBuiltIn;
				struct{																																			// eSymType::USERFUNCTION  & eSymType::SUBPROGRAMME
					Scope_s					Scope;
					int							LocalLength;
					enum eVType*		pLocalTypes;
					pSubCntx_s			pCallerCntx;
					pSubCntx_s			pSubCntx;
					Registry_s*			pCallerIdents;																							
					Registry_s*			pSubIdents;																								// user variable and function identifiers
				} sUser;
			} uFunc;
			int ArgLength;
			enum eVType *pArgTypes;
			enum eVType RetType;
		} sSubrtn;
		struct{																																					// LABEL 
			struct PC_s Line;
		} sLabel;
	} uType;
	struct Symbol_s *pNext;
} Symbol_s;

/////////////////////////////////////////////////////////////////////////////

typedef struct Label_s{
	Symbol_s *pSym;
	long int LineNumber;
	char *pName;
} Label_s;


typedef struct Identifier_s{
	Symbol_s *pSym;
	enum eVType DefType;
	char Name[2 /* ... */];
} Identifier_s;


#include "Program.h"
#include "Strings.h"


typedef struct Next_s{
	PC_s Frame;
	PC_s Var;
	PC_s Limit;
	PC_s Body;
} Next_s;


typedef struct On_s{
	int PcLength;
	PC_s *pPc;
} On_s;


typedef struct Selectcase_s{
	PC_s EndSelect;
	enum eVType ValueType;
	PC_s NextCaseValue;
} Selectcase_s;


typedef struct Casevalue_s{
	PC_s EndSelect;
	PC_s NextCaseValue;
} Casevalue_s;

/////////////////////////////////////////////////////////////////////////////

enum LEXType_e{
	Y_BACK_UP = 0,
	Y_CHANNEL,
	Y_REALNUM,
	Y_INTNUM,
	Y_HEXNUM,
	Y_OCTNUM,
	Y_STRING,			/* rule 6 can match eol */
	Y_STRING2,		/* rule 7 can match eol */
	Y_OP,
	Y_CP,
	Y_OSB,			
	Y_CSB,
	Y_MULT,
	Y_PLUS,
	Y_MINUS,
	Y_COLON,
	Y_COMMA,
	Y_DIVIDE,
	Y_SEMICOLON,
	Y_AMPERSAND,
	Y_LT,				
	Y_LE,
	Y_EL,				
	Y_NE,
	Y_EQ,
	Y_GT,
	Y_GE,
	Y_EG,
	Y_POW,
	Y_AND,			
	Y_AS,
	Y_ASSIGN,
	Y_AXES,
	Y_BEEP,
	Y_BUFFER,
	Y_CALL,
	Y_CASEELSE,
	Y_CASE,
	Y_CAT,
	Y_CATHASH,
	Y_CAT_TO,
	Y_CHDIR,
	Y_CHECK_READ,
	Y_CLEAR,
	Y_CLIP,
	Y_CLOSE,
	Y_CLOSEHASH,		
	Y_CLS,
	Y_COLOUR,
	Y_COM,
	Y_CON,
	Y_COPY,
	Y_CREATE,
	Y_CREATEDIR,	
	Y_CSIZE,
	Y_CURSOR,
	Y_DATA,
	Y_DATSTRING,	 
	Y_DATSTRING2,
	Y_DATCOMMA,
	Y_DATAINPUT,
	Y_EOL_1,
	Y_EOL_2,					/* rule can match eol */
	Y_DATCOLON,
	Y_DEFAULT_OFF,
	Y_DEFAULT_ON,
	Y_DEFDBL,			
	Y_DEFINT,
	Y_DEFSTR,			
	Y_DEFFN,
	Y_DEFPROC,
	Y_DEG,
	Y_DELETE,
	Y_DELAY,
	Y_DIM,
	Y_DIGITIZE,
	Y_DISABLE,
	Y_DISP,
	Y_DIV,
	Y_DO,
	Y_DOUNTIL,		 
	Y_DOWHILE,
	Y_DRAW,
	Y_DUMP,
	Y_ELSE,
	Y_ELSEIFELSE,
	Y_ELSEIFIF,
	Y_ENABLE,
	Y_ENDIF,
	Y_ENDPROC,
	Y_ENDSELECT,
	Y_END,
	Y_EQV,
	Y_EXECUTE,
	Y_EXITDO,
	Y_EXITFOR,
	Y_EXITGRAPH,
	Y_FIXED,
	Y_FLOAT,
	Y_FNEND,			 
	Y_FOR,
	Y_FRAME,
	Y_GCHARSET,
	Y_GCLEAR,
	Y_GET,
	Y_GLOAD,
	Y_GOSUB,
	Y_GOTO,
	Y_GPLOT,
	Y_GPRINT,
	Y_GRAD,
	Y_GRAPHICS,
	Y_GRID,
	Y_GSTORE,
	Y_IDN,
	Y_IF,
	Y_IMAGE_1,
	Y_IMAGESTRING,
	Y_IMAGE_2,
	Y_IMP,
	Y_INPUT,				
	Y_INPUTHASH,
	Y_INTEGER,
	Y_INV,
	Y_IPLOT,
	Y_IS,
	Y_GLABEL,
	Y_LDIR,
	Y_LET,
	Y_LETTER,
	Y_LIMIT,
	Y_LIN,
	Y_LINETYPE,
	Y_LINK,
	Y_LIST,
	Y_LOAD,
	Y_LOCAL,			
	Y_LOCATE,			
	Y_LOCK,
	Y_LOOP,
	Y_LOOPUNTIL,
	Y_LPRINT,
	Y_LORG,
	Y_LSET,
	Y_MASSTORAGEIS,
	Y_MATINPUT,
	Y_MATPRINT,		
	Y_MATREAD,
	Y_MAT,
	Y_MOD,
	Y_MOVE,
	Y_MSCALE,
	Y_NAME,
	Y_NEXT,
	Y_NORMAL,
	Y_NOT,
	Y_OFFKEY,
	Y_ONERROROFF,
	Y_ONERROR,
	Y_ONKEY,
	Y_ON,
	Y_OPTIONBASE,
	Y_OR,
	Y_OUT,
	Y_OVERLAP,
	Y_PAGE,
	Y_PAUSE,
	Y_PDIR,
	Y_PEN,
	Y_PENUP,
	Y_PLOT,
	Y_PLOTTERIS,
	Y_POINTER,
	Y_PRINT,
	Y_PRINTALLIS,
	Y_PRINTERIS,
	Y_PURGE,
	Y_RAD,
	Y_RANDOMIZE,
	Y_RATIO,
	Y_READ,
	Y_REAL,
	Y_REDIM,
	Y_RENUM,
	Y_REPEAT,
	Y_RESTORE,
	Y_RESULT,
	Y_RESUME,
	Y_RETURN,
	Y_RPLOT,
	Y_RSET,
	Y_RUN,
	Y_SAVE,
	Y_SCALE,
	Y_SCRATCH,
	Y_SCRATCH_A,
	Y_SCRATCH_C,
	Y_SCRATCH_K,
	Y_SCRATCH_P,
	Y_SCRATCH_V,
	Y_SELECT,
	Y_SERIAL,
	Y_SHORT,
	Y_SHOW,
	Y_SPA,
	Y_STANDARD,
	Y_STEP,
	Y_STOP,
	Y_STORE,
	Y_SUBEND,			
	Y_SUBEXIT,
	Y_SUB,
	Y_SYSTEM,
	Y_THEN,
	Y_TAB,
	Y_TO,
	Y_TRN,
	Y_TRACE,
	Y_TRALL,				 
	Y_TRPAUSE,				 
	Y_TRVARS,				 
	Y_TRALLVARS,				 
	Y_TRWAIT,				 
	Y_TRUNCATE,
	Y_UNCLIP,
	Y_UNLOCK,
	Y_UNNUM,
	Y_UNTIL,
	Y_USING,
	Y_WAIT,
	Y_WEND,
	Y_WHERE,
	Y_WHILE,
	Y_WIDTH,
	Y_WIDTHHASH,	 
	Y_WRITE,
	Y_WRITEHASH,
	Y_XOR,
	Y_ZER,
	Y_REM,
	Y_RENAME,
	Y_QUOTE,
	Y_LINEINPUT,		
	Y_IDENTIFIER,
	Y_LABEL,
	Y_RSPACE,
	Y_JUNK,
	Y_EOL_4
};

/////////////////////////////////////////////////////////////////////////////

enum TokenType_e{
	T_NOTOKEN = 0,
	T_AMPERSAND,
	T_AND,
	T_AS,
	T_ASSIGN,
	T_AXES,
	T_BEEP,
	T_BUFFER,
	T_CALL,
	T_CASEELSE,
	T_CASE,
	T_CAT,
	T_CATHASH,
	T_CAT_TO,
	T_CHANNEL,
	T_CHDIR,
	T_CHECK_READ,
	T_CLEAR,
	T_CLIP,
	T_CLOSE,
	T_CLS,
	T_COLON,
	T_COLOUR,
	T_COMMA,
	T_COM,
	T_CON,
	T_COPY,
	T_CP,
	T_CREATE,
	T_CREATEDIR,
  T_CSB,
	T_CSIZE,
	T_CURSOR,
	T_DATA,
	T_DATAINPUT,
	T_DEFAULT_OFF,
	T_DEFAULT_ON,
	T_DEFDBL,
	T_DEFFN,
	T_DEFINT,
	T_DEFSTR,
	T_DEG,
	T_DELAY,
	T_DELETE,
	T_DIM,
	T_DIGITIZE,
	T_DISABLE,
	T_DISP,
	T_DIV,
	T_DIVIDE,
	T_DO,
	T_DOUNTIL,
	T_DOWHILE,
	T_DRAW,
	T_DUMP,
	T_ELSE,
	T_ELSEIFELSE,
	T_ELSEIFIF,
	T_ENABLE,
	T_END,
	T_ENDIF,
	T_ENDPROC,
	T_ENDSELECT,
	T_EOL,
	T_EQ,
	T_EQV,
	T_EXECUTE,
	T_EXITDO,
	T_EXITFOR,
	T_EXITGRAPH,
	T_FIXED,
	T_FLOAT,
	T_FNEND,
	T_FOR,
	T_FRAME,
	T_GCHARSET,
	T_GCLEAR,
	T_GE,
	T_GET,
	T_GLABEL,
	T_GLOAD,
	T_GOSUB,
	T_GOTO,
	T_GPLOT,
	T_GPRINT,
	T_GRAD,
	T_GRAPHICS,
	T_GRID,
	T_GSTORE,
	T_GT,
	T_HEXNUM,
	T_OCTNUM,
	T_IDENTIFIER,
	T_IDN,
	T_IF,
	T_IMAGE,
	T_IMP,
	T_INPUT,
	T_INTEGER,
	T_INTNUM,
	T_INV,
	T_IPLOT,
	T_IS,
	T_JUNK,
	T_KILL,
	T_LABEL,
	T_LDIR,
	T_LE,
	T_LET,
	T_LETTER,
	T_LIMIT,
	T_LIN,
	T_LINEINPUT,
	T_LINETYPE,
	T_LINK,
	T_LIST,
	T_LOAD,
	T_LOCAL,
	T_LOCATE,
	T_LOCK,
	T_LOOP,
	T_LOOPUNTIL,
	T_LORG,
	T_LPRINT,
	T_LSET,
	T_LT,
	T_MASSTORAGEIS,
	T_MAT,
	T_MATINPUT,
	T_MATPRINT,
	T_MATREAD,
	T_MINUS,
	T_MKDIR,
	T_MOD,
	T_MOVE,
	T_MSCALE,
	T_MULT,
	T_NAME,
	T_NE,
	T_NEXT,
	T_NORMAL,
	T_NOT,
	T_OFFKEY,
	T_ON,
	T_ONERROR,
	T_ONERROROFF,
	T_ONKEY,
	T_OP,
	T_OPEN,
	T_OPTIONBASE,
	T_OR,
  T_OSB,
	T_OUT,
	T_OVERLAP,
	T_PAGE,
	T_PAUSE,
	T_PDIR,
	T_PEN,
	T_PENUP,
	T_PLOT,
	T_PLOTTERIS,
	T_PLUS,
	T_POINTER,
	T_POW,
	T_PRINT,
	T_PRINTALLIS,
	T_PRINTERIS,
	T_RAD,
	T_RANDOMIZE,
	T_RATIO,
	T_READ,
	T_REAL,
	T_REALNUM,
	T_REDIM,
	T_REM,
	T_RENAME,
	T_RENUM,
	T_REPEAT,
	T_RESTORE,
	T_RESULT,
	T_RESUME,
	T_RETURN,
	T_RPLOT,
	T_RSET,
	T_RUN,
	T_SAVE,
	T_SCALE,
	T_SCRATCH,
	T_SCRATCH_A,
	T_SCRATCH_C,
	T_SCRATCH_K,
	T_SCRATCH_P,
	T_SCRATCH_V,
	T_SELECT,
	T_SERIAL,
	T_SEMICOLON,
	T_SHORT,
	T_SHOW,
	T_SPA,
	T_STANDARD,
	T_STEP,
	T_STOP,
	T_STORE,
	T_STRING,
	T_SUB,
	T_SUBEND,
	T_SUBEXIT,
	T_SYSTEM,
	T_TAB,
	T_THEN,
	T_TO,
	T_TRN,
	T_TRACE,
	T_TRALL,				 
	T_TRPAUSE,				 
	T_TRVARS,				 
	T_TRALLVARS,				 
	T_TRWAIT,				 
	T_TRUNCATE,
	T_UNCLIP,
	T_UNLOCK,
	T_UNNUM,
	T_UNNUMBERED,
	T_UNTIL,
	T_USING,
	T_WAIT,
	T_WEND,
	T_WHERE,
	T_WHILE,
	T_WIDTH,
	T_WRITE,
	T_XOR,
	T_ZER,
	T_LASTTOKEN = T_ZER
};

/////////////////////////////////////////////////////////////////////////////

typedef struct Token_s{
	enum TokenType_e Type;
	UINT Column;
	pValue_s (*pStatement)(pValue_s pValue);
	union{
		/* T_AMPERSAND          */
		/* T_AND                */
		/* T_AS                 */
		/* T_ASSIGN             */
		/* T_AXES               */
		/* T_BEEP               */
		/* T_CALL               */
		/* T_CASEELSE           */ Casevalue_s *pCaseValue;
		/* T_CASE               */ /* Casevalue_s *pCaseValue; */
		/* T_CAT				        */
		/* T_CATHASH            */
		/* T_CAT_TO             */
		/* T_CHANNEL            */
		/* T_CHDIR              */
		/* T_CHECK_READ         */
		/* T_CLEAR              */
		/* T_CLIP               */
		/* T_CLOSE              */
		/* T_CLS                */
		/* T_COLON              */
		/* T_COLOUR             */
		/* T_COMMA              */
		/* T_COM                */
		/* T_CON                */
		/* T_COPY               */
		/* T_CP                 */
		/* T_CREATE             */
		/* T_CREATEDIR          */
		/* T_CSB                */
		/* T_CSIZE              */
		/* T_CURSOR             */
		/* T_DATA               */ PC_s NextDataPc;
		/* T_DATAINPUT          */ char *pDataInput;
		/* T_DEFAULT_OFF        */
		/* T_DEFAULT_ON         */
		/* T_DEFDBL             */
		/* T_DEFFN              */ Symbol_s *pLocalSyms;
		/* T_DEFINT             */
		/* T_DEFSTR             */
		/* T_DEG	              */
		/* T_DELAY              */
		/* T_DELETE             */
		/* T_DIM                */
		/* T_DIGITIZE           */
		/* T_DISABLE            */
		/* T_DISP               */
		/* T_DIV                */
		/* T_DIVIDE             */
		/* T_DO                 */ PC_s ExitDoPc;
		/* T_DOUNTIL            */ /* struct Pc exitdo; */
		/* T_DOWHILE            */ /* struct Pc exitdo; */
		/* T_DRAW               */ 
		/* T_DUMP               */
		/* T_ELSE               */ PC_s EndIfPc;
		/* T_ELSEIFELSE         */ /* struct Pc endifpc; */
		/* T_ELSEIFIF           */ PC_s ElsePc;
		/* T_ENABLE             */
		/* T_END                */ PC_s EndPc;
		/* T_ENDIF              */
		/* T_ENDPROC            */
		/* T_ENDSELECT          */
		/* T_EOL                */
		/* T_EQ                 */ enum eVType Type;
		/* T_EQV                */
		/* T_EXECUTE            */
		/* T_EXITDO             */ /* struct Pc exitdo; */
		/* T_EXITFOR            */ PC_s ExitForPc;
		/* T_EXITGRAPH          */
		/* T_FIXED              */
		/* T_FLOAT              */
		/* T_FNEND              */
		/* T_FOR                */ /* struct Pc exitfor */
		/* T_FRAME              */
		/* T_GCHARSET           */
		/* T_GCLEAR             */
		/* T_GE                 */
		/* T_GET                */
		/* T_GLABEL             */
		/* T_GLOAD              */
		/* T_GOSUB              */ PC_s GosubPc;
		/* T_GOTO               */ PC_s GotoPc;
		/* T_GPLOT              */
		/* T_GPRINT             */
		/* T_GRAD	              */
		/* T_GRAPHICS           */
		/* T_GRID               */
		/* T_GSTORE             */
		/* T_GT                 */
		/* T_HEXINT							*/ long int HexInteger;
		/* T_OCTINT							*/ long int OctInteger;
		/* T_IDENTIFIER         */ Identifier_s *pIdentifier;
		/* T_IDIV               */
		/* T_IDN                */
		/* T_IF                 */ /* Pc elsepc; */
		/* T_IMAGE              */ /* String *string; */
		/* T_IMP                */
		/* T_INPUT              */
		/* T_INTNUM								*/ 
		/* T_INTEGER            */ long int Integer;
		/* T_INV                */
		/* T_IPLOT  						*/ 
		/* T_IS                 */
		/* T_JUNK               */ char Junk;
		/* T_KILL               */
		/* T_LABEL							*/ Label_s *pLabel;
		/* T_LE                 */
		/* T_LEN                */
		/* T_LET                */
		/* T_LETTER							*/ 
		/* T_LIN                */
		/* T_LINEINPUT          */
		/* T_LINETYPE						*/ 
		/* T_LINK               */
		/* T_LIST               */
		/* T_LOAD               */
		/* T_LOCAL              */
		/* T_LOCATE             */
		/* T_LOCK               */
		/* T_LOOP               */ PC_s DoPc;
		/* T_LOOPUNTIL          */ /* struct Pc dopc; */
		/* T_LORG								*/ 
		/* T_LPRINT             */
		/* T_LSET               */
		/* T_LT                 */
		/* T_MASSTORAGEIS       */
		/* T_MAT                */
		/* T_MATINPUT           */
		/* T_MATPRINT           */
		/* T_MATREAD            */
		/* T_MINUS              */
		/* T_MKDIR              */
		/* T_MOD                */
		/* T_MOVE               */
		/* T_MSCALE             */
		/* T_MULT               */
		/* T_NAME               */
		/* T_NE                 */
		/* T_NEXT               */ Next_s *pNext;
		/* T_NORMAL             */
		/* T_NOT                */
		/* T_OFFKEY							*/
		/* T_ON                 */ On_s On;
		/* T_ONERROR            */
		/* T_ONERROROFF         */
		/* T_ONKEY              */
		/* T_OP                 */
		/* T_OPEN               */
		/* T_OPTIONBASE         */
		/* T_OR                 */
		/* T_OSB                */
		/* T_OUT                */
		/* T_OVERLAP            */
		/* T_PAGE								*/
		/* T_PAUSE              */
		/* T_PDIR               */
		/* T_PEN                */
		/* T_PENUP              */
		/* T_PLOT               */
		/* T_PLOTTERIS          */
		/* T_PLUS               */
		/* T_POINTER            */
		/* T_POW                */
		/* T_PRINT              */
		/* T_PRINTERALLIS       */
		/* T_PRINTERIS          */
		/* T_RAD	              */
		/* T_RANDOMIZE          */
		/* T_RATIO              */
		/* T_READ               */
		/* T_REAL			          */
		/* T_REALNUM               */ double Real;
		/* T_REDIM							*/
		/* T_REM                */ char *pRem;
		/* T_RENAME             */
		/* T_RENUM              */
		/* T_REPEAT             */
		/* T_RESTORE            */ PC_s RestorePc;
		/* T_RESULT             */
		/* T_RESUME             */ /* struct Pc gotopc; */
		/* T_RETURN             */
		/* T_RPLOT              */
		/* T_RSET               */
		/* T_RUN                */
		/* T_SAVE               */
		/* T_SCALE              */
		/* T_SCRATCH            */
		/* T_SCRATCH_A          */
		/* T_SCRATCH_C          */
		/* T_SCRATCH_K          */
		/* T_SCRATCH_P          */
		/* T_SCRATCH_V          */
		/* T_SELECT             */ Selectcase_s *pSelectCase;
		/* T_SERIAL             */
		/* T_SEMICOLON          */
		/* T_SHORT              */
		/* T_SHOW               */
		/* T_SPA                */
		/* T_STANDARD           */
		/* T_STEP               */
		/* T_STORE              */
		/* T_STRING             */ BString *pString;
		/* T_SUB                */ /* struct Symbol *localSyms; */
		/* T_SUBEND             */
		/* T_SUBEXIT            */
		/* T_SYSTEM             */
		/* T_TAB                */
		/* T_THEN               */
		/* T_TO                 */
		/* T_TRN                */
		/* T_TRACE              */
		/* T_TRALL              */
		/* T_TRPAUSE            */
		/* T_TRVARS             */
		/* T_TRALLVARS          */
		/* T_TRWAIT             */
		/* T_TRUNCATE           */
		/* T_UNCLIP             */
		/* T_UNLOCK             */
		/* T_UNNUM              */
		/* T_UNNUMBERED         */
		/* T_UNTIL              */ PC_s UntilPc;
		/* T_USING              */ PC_s ImagePc;
		/* T_WAIT               */
		/* T_WEND               */ PC_s *pWhilePc;
		/* T_WHERE              */
		/* T_WHILE              */ PC_s *pAfterWendPc;
		/* T_WIDTH              */
		/* T_WRITE              */
		/* T_XOR                */
		/* T_ZER                */
	} Obj;
} Token_s;

/////////////////////////////////////////////////////////////////////////////

Token_s 	*TokenNewCode(const char *ln);
 Token_s 	*TokeniseData(const char *ln);
void 			TokenDestroy(Token_s *pToken);
void 			TokenInit(void);

extern BString *TokenToString(Token_s *pToken, Token_s *spaceto, BString *s, int *indent, int full);
extern int TokenProperty[];

/////////////////////////////////////////////////////////////////////////////

#define TOKEN_ISBINARYOPERATOR(t)   (TokenProperty[t] & 1)
#define TOKEN_ISUNARYOPERATOR(t)    (TokenProperty[t] & (1 << 1))
#define TOKEN_BINARYPRIORITY(t)     ((TokenProperty[t] >> 2) & 7)
#define TOKEN_UNARYPRIORITY(t)      ((TokenProperty[t] >> 5) & 7)
#define TOKEN_ISRIGHTASSOCIATIVE(t) (TokenProperty[t] & (1 << 8))

/////////////////////////////////////////////////////////////////////////////
