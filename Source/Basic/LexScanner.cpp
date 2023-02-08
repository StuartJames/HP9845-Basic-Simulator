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
#include <stdio.h>
#include <io.h>

constexpr auto LEX_FLEX_MAJOR_VERSION				= 2;
constexpr auto LEX_FLEX_MINOR_VERSION				= 5;
constexpr auto LEX_FLEX_SUBMINOR_VERSION		= 35;
constexpr auto LEX_END_OF_BUFFER_CHAR				= 0;
constexpr auto EOB_ACT_CONTINUE_SCAN				= 0;
constexpr auto EOB_ACT_END_OF_FILE					= 1;
constexpr auto EOB_ACT_LAST_MATCH						= 2;

constexpr auto LEX_BUFFER_NEW								= 0;
constexpr auto LEX_BUFFER_NORMAL						= 1;

constexpr auto LEX_BUF_SIZE									= 16384;							/* Size of default input buffer. */

constexpr auto INITIAL		= 0;
constexpr auto DATAINPUT	= 1;
constexpr auto ELSEIF			= 2;
constexpr auto IMAGEFMT	 	= 3;

constexpr auto LEX_NUM_RULES			= 247;
constexpr auto LEX_END_OF_BUFFER	= 248;
constexpr auto LEX_MAX_STATES			= 902;

constexpr auto LEX_READ_BUF_SIZE	= 8192;						/* Amount of stuff to slurp up with each read. */

	/* When an EOF's been seen but there's still some text to process
	* then we mark the buffer as LEX_EOF_PENDING, to indicate that we
	* shouldn't try reading from the input source any more.  We might
	* still have a bunch of tokens to match, though, because of
	* possible backing-up.
	*
	* When we actually see the EOF, we change the status to "new"
	* (via LexRestart()), so that the user can continue scanning by
	* just pointing pStream at a new input file.
	*/
constexpr auto LEX_BUFFER_EOF_PENDING				= 2;


/* Promotes a possibly negative, possibly signed char to an unsigned
 * integer for use as an array index.  If the signed char is negative,
 * we want to instead treat it as an 8-bit unsigned char, hence the
 * double cast.	 */
static inline unsigned int LexChar2Uint(char c){ return((unsigned int)(char)c);};


/* Enter a start condition.  This macro really ought to take a parameter,
 * but we do it the disgusting crufty way forced on us by the ()-less
 * definition of BEGIN.	 */
#define BEGIN (LexStart) = 1 + 2 *


/* Translate the current start state into a value that can be later handed
 * to BEGIN to return to the state.  The LEXSTATE alias is for lex compatibility. */
#define LEX_START								(((LexStart) - 1) / 2)
#define LEXSTATE								LEX_START

#define LEX_STATE_EOF(state)		(LEX_END_OF_BUFFER + state + 1)						/* Action number for EOF rule of a given start state. */

#define LEX_NEW_FILE						LexRestart(pStream)								/* Action number for EOF rule of a given start state. */

//////////////////////////////////////////////////////////////////////////////////////

/* No semi-colon after return; correct usage is to write "LEX_TERMINATE();" -
 * we don't want an extra ';' after the "return" because that will cause
 * some compilers to complain about unreachable statements.
 */
#ifndef LEX_TERMINATE
#define LEX_TERMINATE() return 0
#endif

#ifndef LEX_EXTRA_TYPE
#define LEX_EXTRA_TYPE void*
#endif

//////////////////////////////////////////////////////////////////////////////////////

#define unput(c) Lexunput( c, (pLexText)  )

//////////////////////////////////////////////////////////////////////////////////////

typedef struct ScanObject_s
{
	FILE *pStream;
	char *pBuffer;    /* input buffer */
	char *pCurrentPos;   /* current position in input buffer */

	/* Size of input buffer in bytes, not including room for EOB characters. */
	UINT BufferSize;

	/* Number of characters read into pBuffer, not including EOB characters. */
	int CharCount;

	/* Whether we "own" the buffer - i.e., we know we created it, and
	 * can realloc() it to grow it, and should free() it to delete it. */
	int IsOurBuffer;

	/* Whether this is an "interactive" input source; if so, and if we're
	 * using stdio for input, then we want to use getc() instead of fread(),
	 * to make sure we stop fetching input after each newline. */
	int IsInteractive;

	/* Whether we're considered to be at the beginning of a line. If so,
	  * '^' rules will be active on the next match, otherwise not. */
	int IsAtLineStart;
	int BsLineNumber;   /**< The line count. */
	int BsColumnCount;   /**< The column count. */

	/* Whether to try to fill the input buffer when we reach the end of it.	 */
	int DoFillBuffer;
	int BufferStatus;
} ScanObject_s;

typedef struct ScanObject_s *pScanObject_s;

//////////////////////////////////////////////////////////////////////////////////////

																																	/* Stack of input buffers. */
static UINT									ScanObjStackTop = 0;								/**< index of top of stack. */
static UINT									ScanObjStackMax = 0;								/**< capacity of stack. */
static pScanObject_s				*ppScanObjStack = nullptr;							/**< Stack as an array. */


/* LexHoldChar holds the character lost when pLexText is formed. */
static char			LexHoldChar;
static int			CharCount;    /* number of characters read into pBuffer */
int							LexLength;

/* Points to current character in buffer. */
static char			*pLexBufferChar = nullptr;
static int			LexInitialised = 0;   /* whether we need to initialize */
static int			LexStart = 0;  /* start state number */

/* Flag which is used to allow LexWrap()'s to do buffer switches instead of setting up a fresh pStream.  A bit of a hack. */
static int			LexDidBufferSwitchOnEOF;

int							Lexlex(void);
void 						LexRestart(FILE *pFile);
void 						SwitchScanObj(pScanObject_s pScanObj);
void 						DeleteScanObj(pScanObject_s pScanObj);
void 						FlushScanObj(pScanObject_s pScanObj);
void 						PushScanObj(pScanObject_s pScanObj);
void 						PopScanObj(void);
static void 		CreateScanObjStack(void);
static void 		LoadObjectState(void);
static void 		InitFileScanObj(pScanObject_s pScanObj, FILE *pFile);

pScanObject_s		CreateFileScanObj(FILE *pFile, int Size);
pScanObject_s		CreateScanObject(char *pBase, UINT Size);
pScanObject_s		LexScanString(const char *pLexStr);
pScanObject_s		LexScanBytes(const char *pBytes, int Len);

static inline void LexFatalError(const char* msg);

//////////////////////////////////////////////////////////////////////////////////////

/* Begin user sect3 */

#define LexWrap() 1

typedef unsigned char LEX_CHAR;
typedef int LexStateType;

FILE *pStream = nullptr;
FILE *pLexOutputStream = nullptr;

int LexLineNumber = 1;

extern char					*pLexText;

static LexStateType LexGetPreviousState(void);

static LexStateType LexTryTransition0(LexStateType CurrentState);

static int					LexGetNextBuffer(void);

//////////////////////////////////////////////////////////////////////////////////////


/* This struct is not used in this scanner,	but its presence is necessary. */
typedef struct LexTransInfo_s
{
	int LexVerify;
	int LexNext;
} LexTransInfo_s;

//////////////////////////////////////////////////////////////////////////////////////

#include "ScannerTables.cpp"

//////////////////////////////////////////////////////////////////////////////////////

static LexStateType LexLastAcceptingState;
static char					*pLexLastAcceptingPos;
static LexStateType LexStateBuffer[LEX_BUF_SIZE + 2], *pLexStatePtr;
static char					*pLexFullMatch;
static int					LexL;
static int					LexLookingForTrailBegin = 0;
static int					LexFullL;
static int					*pLexFullState;

extern int					LexFlexDebug;
int									LexFlexDebug = 0;
char								*pLexText;
char								*pLexC, *pLexB;

constexpr auto			LEX_TRAILING_MASK				= 0x2000;
constexpr auto			LEX_TRAILING_HEAD_MASK	= 0x4000;
constexpr auto			LEX_MORE_ADJ = 0;

static int					MatchData;

static Token_s			*pCurrentTok;

 //////////////////////////////////////////////////////////////////////////////////////

 /* We provide macros for accessing buffer states in case in the future we want to put the buffer states in a more general "scanner state".
 *
 * Returns the top of the stack, or NULL.
 */
static inline pScanObject_s GetLexCurrentBuffer(){ return (ppScanObjStack != nullptr) ? ppScanObjStack[ScanObjStackTop] : nullptr; };

/* Same as previous macro, but useful when we know that the buffer stack is not NULL or when we need an GetValue. For internal use only. */
#define LEX_FLUSH_BUFFER FlushScanObj(GetLexCurrentBuffer())

 //////////////////////////////////////////////////////////////////////////////////////

static inline void LexSetInteractive(int IsInteractive)
{ 
	if(GetLexCurrentBuffer() == nullptr){
		CreateScanObjStack();
		ppScanObjStack[ScanObjStackTop] = CreateFileScanObj(pStream, LEX_BUF_SIZE);
	}
	ppScanObjStack[ScanObjStackTop]->IsInteractive = IsInteractive; 
};

//////////////////////////////////////////////////////////////////////////////////////

/* Performed after the current pattern has been matched and before the corresponding action - sets up pLexText. */
static inline void LexDoBeforeAction()
{
  pLexText = pLexB;
  LexLength = (UINT)(pLexC - pLexB);
  LexHoldChar = *pLexC;
  *pLexC = '\0';
  pLexBufferChar = pLexC;
};

//////////////////////////////////////////////////////////////////////////////////////

/* Return all but the first "n" matched characters back to the input stream. */
static inline void Lexless(int Skip)
{
	*pLexC = LexHoldChar;
	pLexBufferChar = pLexC = pLexB + Skip - LEX_MORE_ADJ;
	LexDoBeforeAction(); /* set up pLexText again */ \
}

//////////////////////////////////////////////////////////////////////////////////////

static void String(const char *pText)
{
const char *pFrom;
UINT Len = 0;

	if(pCurrentTok){
		for(pFrom = pText + 1; *(pFrom + 1); ++pFrom, ++Len) if(*pFrom == '"') ++pFrom;
		pCurrentTok->Obj.pString = new BString;
		pCurrentTok->Obj.pString->FilterCopy(pText + 1, Len, '"');
	}
}

//////////////////////////////////////////////////////////////////////////////////////

static void String2(void)
{
char *pFrom;
UINT Len = 0;

	if(pCurrentTok){
		for(pFrom = pLexText + 1; *pFrom; ++pFrom, ++Len) if(*pFrom == '"') ++pFrom;
		pCurrentTok->Obj.pString = new BString;
		pCurrentTok->Obj.pString->FilterCopy(pLexText + 1, Len, '"');
	}
}

//////////////////////////////////////////////////////////////////////////////////////

static inline void setBit(int *field, int bit)
{
	field[bit / (sizeof(int) * 8)] |= 1 << (bit & ((sizeof(int) * 8) - 1));
}

//////////////////////////////////////////////////////////////////////////////////////

static int LexInitGlobals(void);

/* Accessor methods to globals.
   These are made visible to non-reentrant scanners for convenience. */

int							LexDestroy(void);
int							LexGetDebug(void);
void						LexSetDebug(int debug_flag);
FILE						*LexGetInput(void);
void						LexSetInput(FILE * in_str);
FILE						*LexGetOutput(void);
void						LexSetOutput(FILE * out_str);
int							LexGetLength(void);
char						*LexGetText(void);
void						LexSetLineNumber(int line_number);
int							Lexlex(void);

/* Macros after this point can all be overridden by user definitions in section 1. */

#ifndef LEX_NO_INPUT
static int Lexinput (void);
#endif

/* Copy whatever the last rule matched to the standard output. */
#ifndef ECHO
/* This used to be an fputs(), but since the string might contain NUL's, we now use fwrite(). */
#define ECHO fwrite(pLexText, LexLength, 1, pLexOutputStream)
#endif

//////////////////////////////////////////////////////////////////////////////////////

/* Gets input and stuffs it into "buf".  number of characters read, or NULL, is returned in "result". */
static inline void LexInput(char *buf, int result, int max_size)
{
  if(ppScanObjStack[ScanObjStackTop]->IsInteractive){
		int c = '*'; 
		int n; 
		for(n = 0; n < max_size && (c = getc(pStream ))  !=  EOF && c  !=  '\n'; ++n) buf[n] = (char)c; 
		if(c  ==  '\n') buf[n++] = (char) c;  
		if(c  ==  EOF && ferror(pStream) ) LexFatalError("input in flex scanner failed"); 
		result = n;
	}
	else{ 
		errno = 0;
		while((result = fread(buf, 1, max_size, pStream)) == 0 && ferror(pStream)){
		  if(errno  !=  EINTR){ 
		    LexFatalError( "input in flex scanner failed" ); 
		    break; 
			}
		  errno = 0;
		  clearerr(pStream);
		} 
	}
};

constexpr auto LEX_START_STACK_INCR		= 25;			/* Number of entries by which start-condition stack grows. */
constexpr auto LEX_DECL_IS_OURS				= 1;			/* Default declaration of generated scanner - a define so the user can easily add parameters. */
constexpr auto LEX_USER_ACTION				= 1;			/* Code executed at the beginning of each rule, after pLexText and LexLength have been set up. */

static inline void LexRuleSetup(void){};

//////////////////////////////////////////////////////////////////////////////////////

/** The main scanner function which does all the work. */
int Lexlex(void)
{
register LexStateType LexCurrentState;
register int Lex_act;

	/* flex rules */
	if(MatchData) BEGIN(DATAINPUT);
	if(!LexInitialised){
		LexInitialised = 1;
#ifdef LEX_USER_INIT
		LEX_USER_INIT;
#endif
		if(!LexStart) LexStart = 1;																									// first start state 
		if(!pStream) pStream = stdin;
		if(!pLexOutputStream) pLexOutputStream = stdout;
		if(GetLexCurrentBuffer() == nullptr){
			CreateScanObjStack();
			ppScanObjStack[ScanObjStackTop] = CreateFileScanObj(pStream, LEX_BUF_SIZE);
		}
		LoadObjectState();
	}
	while(1){																																			// loops until end-of-file is reached 
		pLexC = pLexBufferChar;
		*pLexC = LexHoldChar;																												// Support of pLexText. 
		pLexB = pLexC;																															// pLexB points to the position in pBuffer of the start of the current run.	 
		LexCurrentState = LexStart;
LexMatch:
		do{
			register LEX_CHAR Lex_c = Lex_ec[LexChar2Uint(*pLexC)];
			if(Lex_accept[LexCurrentState]){
				LexLastAcceptingState = LexCurrentState;
				pLexLastAcceptingPos = pLexC;
			}
			while(Lex_chk[Lex_base[LexCurrentState] + Lex_c]  !=  LexCurrentState){
				LexCurrentState = (int)Lex_def[LexCurrentState];
				if(LexCurrentState >= LEX_MAX_STATES) Lex_c = Lex_meta[(unsigned int)Lex_c];
			}
			LexCurrentState = LexNext[Lex_base[LexCurrentState] + (unsigned int)Lex_c];
			++pLexC;
		}
		while(Lex_base[LexCurrentState]  !=  2322);
LexFindAction:
		Lex_act = Lex_accept[LexCurrentState];
		if(Lex_act == 0){																														// have to back up 
			pLexC = pLexLastAcceptingPos;
			LexCurrentState = LexLastAcceptingState;
			Lex_act = Lex_accept[LexCurrentState];
		}
		LexDoBeforeAction();
do_action:																																			// This label is used only to access EOF actions. 
		switch(Lex_act){																														// beginning of action switch 
			case Y_BACK_UP:																														// must back up, undo the effects of YY_DO_BEFORE_ACTION 
				*pLexC = LexHoldChar;
				pLexC = pLexLastAcceptingPos;
				LexCurrentState = LexLastAcceptingState;
				goto LexFindAction;
			case Y_CHANNEL:
				LexRuleSetup();
				return T_CHANNEL;
			case Y_REALNUM:{
				LexRuleSetup();
				int overflow;
				double d = ValueStringToReal(pLexText, nullptr, &overflow);
				if(overflow){
					if(pCurrentTok) pCurrentTok->Obj.Junk = pLexText[0];
					Lexless(1);
					return T_JUNK;
				}
				if(pCurrentTok) pCurrentTok->Obj.Real = d;
				return T_REALNUM;
			}
			case Y_INTNUM:{
				LexRuleSetup();
				int overflow;
				long n = ValueStringToInteger(pLexText, nullptr, &overflow);
				if(overflow){
					double d = ValueStringToReal(pLexText, nullptr, &overflow);
					if(overflow){
						if(pCurrentTok) pCurrentTok->Obj.Junk = pLexText[0];
						Lexless(1);
						return T_JUNK;
					}
					if(pCurrentTok) pCurrentTok->Obj.Real = d;
					return T_REALNUM;
				}
				if(pCurrentTok) pCurrentTok->Obj.Integer = n;
				return T_INTNUM;
			}
			case Y_HEXNUM:{
				LexRuleSetup();
				int overflow;
				long n = ValueStringToInteger(pLexText, nullptr, &overflow);
				if(overflow){
					if(pCurrentTok) pCurrentTok->Obj.Junk = pLexText[0];
					Lexless(1);
					return T_JUNK;
				}
				if(pCurrentTok) pCurrentTok->Obj.HexInteger = n;
				return T_HEXNUM;
			}
			case Y_OCTNUM:{
				LexRuleSetup();
				int overflow;
				long n = ValueStringToInteger(pLexText, nullptr, &overflow);
				if(overflow){
					if(pCurrentTok) pCurrentTok->Obj.Junk = pLexText[0];
					Lexless(1);
					return T_JUNK;
				}
				if(pCurrentTok) pCurrentTok->Obj.OctInteger = n;
				return T_OCTNUM;
			}
			case Y_STRING:{		/* rule 6 can match eol */
				LexRuleSetup();
				String(pLexText);
				return T_STRING;
			}
			case Y_STRING2:{	/* rule 7 can match eol */
				*pLexC = LexHoldChar; /* undo effects of setting up pLexText */
				(pLexBufferChar) = pLexC -= 1;
				LexDoBeforeAction(); /* set up pLexText again */
				LexRuleSetup();
				String2();
				return T_STRING;
			}
			case Y_OP:
				LexRuleSetup();
				return T_OP;
			case Y_CP:
				LexRuleSetup();
				return T_CP;
			case Y_OSB:
				LexRuleSetup();
				return T_OSB;
			case Y_CSB:
				LexRuleSetup();
				return T_CSB;
			case Y_MULT:
				LexRuleSetup();
				return T_MULT;
			case Y_PLUS:
				LexRuleSetup();
				return T_PLUS;
			case Y_MINUS:
				LexRuleSetup();
				return T_MINUS;
			case Y_COLON:
				LexRuleSetup();
				return T_COLON;
			case Y_COMMA:
				LexRuleSetup();
				return T_COMMA;
			case Y_DIVIDE:
				LexRuleSetup();
				return T_DIVIDE;
			case Y_SEMICOLON:
				LexRuleSetup();
				return T_SEMICOLON;
			case Y_AMPERSAND:
				LexRuleSetup();
				return T_AMPERSAND;
			case Y_LT:
				LexRuleSetup();
				return T_LT;
			case Y_LE:
				LexRuleSetup();
				return T_LE;
			case Y_EL:
				LexRuleSetup();
				return T_LE;
			case Y_NE:
				LexRuleSetup();
				return T_NE;
			case Y_EQ:{
				LexRuleSetup();
				if(pCurrentTok)	pCurrentTok->pStatement = stmt_EQ_FNEND;
				return T_EQ;
			}
			case Y_GT:
				LexRuleSetup();
				return T_GT;
			case Y_GE:
				LexRuleSetup();
				return T_GE;
			case Y_EG:
				LexRuleSetup();
				return T_GE;
			case 	Y_POW:
				LexRuleSetup();
				return T_POW;
			case Y_AND:
				LexRuleSetup();
				return T_AND;
			case Y_AS:
				LexRuleSetup();
				return T_AS;
			case Y_ASSIGN:
				LexRuleSetup();
				if(pCurrentTok)	pCurrentTok->pStatement = stmt_ASSIGN;
				return T_ASSIGN;
			case Y_AXES:
				LexRuleSetup();
				if(pCurrentTok)	pCurrentTok->pStatement = stmt_AXES;
				return T_AXES;
			case Y_BEEP:{
				LexRuleSetup();
				if(pCurrentTok)	pCurrentTok->pStatement = stmt_BEEP;
				return T_BEEP;
			}
			case Y_BUFFER:{
				LexRuleSetup();
				return T_BUFFER;
			}
			case Y_CALL:{
				LexRuleSetup();
				if(pCurrentTok)	pCurrentTok->pStatement = stmt_CALL;
				return T_CALL;
			}
			case Y_CASEELSE:{
				LexRuleSetup();
				if(pCurrentTok){
					pCurrentTok->pStatement = stmt_CASE;
					pCurrentTok->Obj.pCaseValue = (Casevalue_s*)malloc(sizeof(Casevalue_s));
				}
				return T_CASEELSE;
			}
			case Y_CASE:{
				LexRuleSetup();
				if(pCurrentTok){
					pCurrentTok->pStatement = stmt_CASE;
					pCurrentTok->Obj.pCaseValue = (Casevalue_s*)malloc(sizeof(Casevalue_s));
				}
				return T_CASE;
			}
			case Y_CAT:{
				LexRuleSetup();
				if(pCurrentTok)	pCurrentTok->pStatement = stmt_CAT;
				return T_CAT;
			}
			case Y_CATHASH:{
				*pLexC = LexHoldChar; /* undo effects of setting up pLexText */
				pLexBufferChar = pLexC = pLexB + 5;
				LexDoBeforeAction(); /* set up pLexText again */
				LexRuleSetup();
				if(pCurrentTok)	pCurrentTok->pStatement = stmt_CAT;
				return T_CATHASH;
			}
			case Y_CAT_TO:{
				LexRuleSetup();
				if(pCurrentTok)	pCurrentTok->pStatement = stmt_CAT;
				return T_CAT_TO;
			}
			case Y_CHDIR:{
				LexRuleSetup();
				if(pCurrentTok)	pCurrentTok->pStatement = stmt_FOLDER;
				return T_CHDIR;
			}
			case Y_CHECK_READ:{
				LexRuleSetup();
				if(pCurrentTok)	pCurrentTok->pStatement = stmt_CHECK_READ;
				return T_CHECK_READ;
			}
			case Y_CLEAR:{
				LexRuleSetup();
				if(pCurrentTok)	pCurrentTok->pStatement = stmt_CLEAR;
				return T_CLEAR;
			}
			case Y_CLIP:{
				LexRuleSetup();
				if(pCurrentTok)	pCurrentTok->pStatement = stmt_CLIP;
				return T_CLIP;
			}
			case Y_CLOSE:{
				LexRuleSetup();
				if(pCurrentTok)	pCurrentTok->pStatement = stmt_CLOSE;
				return T_CLOSE;
			}
			case Y_CLOSEHASH:{
				*pLexC = LexHoldChar; /* undo effects of setting up pLexText */
				pLexBufferChar = pLexC = pLexB + 5;
				LexDoBeforeAction(); /* set up pLexText again */
				LexRuleSetup();
				if(pCurrentTok)	pCurrentTok->pStatement = stmt_CLOSE;
				return T_CLOSE;
			}
			case Y_CLS:{
				LexRuleSetup();
				if(pCurrentTok)	pCurrentTok->pStatement = stmt_CLS;
				return T_CLS;
			}
			case Y_COLOUR:{
				LexRuleSetup();
				if(pCurrentTok)	pCurrentTok->pStatement = stmt_COLOUR;
				return T_COLOUR;
			}
			case Y_COM:{
				LexRuleSetup();
				if(pCurrentTok)	pCurrentTok->pStatement = stmt_COM;
				return T_COM;
			}
			case Y_CON:
				LexRuleSetup();
				return T_CON;
				break;
			case Y_COPY:{
				LexRuleSetup();
				if(pCurrentTok)	pCurrentTok->pStatement = stmt_COPY_RENAME;
				return T_COPY;
			}
			case Y_CREATE:{
				LexRuleSetup();
				if(pCurrentTok)	pCurrentTok->pStatement = stmt_CREATE;
				return T_CREATE;
			}
			case Y_CREATEDIR:{
				LexRuleSetup();
				if(pCurrentTok)	pCurrentTok->pStatement = stmt_FOLDER;
				return T_CREATEDIR;
			}
			case Y_CSIZE:{
				LexRuleSetup();
				if(pCurrentTok)	pCurrentTok->pStatement = stmt_CSIZE;
				return T_CSIZE;
			}
			case Y_CURSOR:{
				LexRuleSetup();
				if(pCurrentTok)	pCurrentTok->pStatement = stmt_CURSOR;
				return T_CURSOR;
			}
			case Y_DATA:{
				LexRuleSetup();
				BEGIN(DATAINPUT);
				if(pCurrentTok)	pCurrentTok->pStatement = stmt_DATA;
				return T_DATA;
			}
			case Y_DATSTRING:							/* rule 44 can match eol */
				LexRuleSetup();
				String(pLexText);
				return T_STRING;
			case Y_DATSTRING2:						/* rule 45 can match eol */
				*pLexC = (LexHoldChar);			/* undo effects of setting up pLexText */
				(pLexBufferChar) = pLexC -= 1;
				LexDoBeforeAction();				/* set up pLexText again */
				LexRuleSetup();
				String2();
				return T_STRING;
			case Y_DATCOMMA:
				LexRuleSetup();
				return T_COMMA;
			case Y_DATAINPUT:{
				LexRuleSetup();
				if(pCurrentTok){
					int len = strlen(pLexText) + 1;
					pCurrentTok->Obj.pDataInput = (char*)malloc(len);
					strcpy_s(pCurrentTok->Obj.pDataInput, len, pLexText);
				}
				return T_DATAINPUT;
			}
			case Y_EOL_1:
				LexRuleSetup();
				break;
			case Y_EOL_2:					/* rule 49 can match eol */
				LexRuleSetup();
				BEGIN(INITIAL);
				break;
			case Y_DEFAULT_OFF:{
				LexRuleSetup();
				if(pCurrentTok)	pCurrentTok->pStatement = stmt_DEFAULT;
				return T_DEFAULT_OFF;
			}
			case Y_DEFAULT_ON:{
				LexRuleSetup();
				if(pCurrentTok)	pCurrentTok->pStatement = stmt_DEFAULT;
				return T_DEFAULT_ON;
			}
			case Y_DEFDBL:{
				LexRuleSetup();
				if(pCurrentTok)	pCurrentTok->pStatement = stmt_DEFINT_DEFDBL_DEFSTR;
				return T_DEFDBL;
			}
			case Y_DEFINT:{
				LexRuleSetup();
				if(pCurrentTok)	pCurrentTok->pStatement = stmt_DEFINT_DEFDBL_DEFSTR;
				return T_DEFINT;
			}
			case Y_DEFSTR:{
				LexRuleSetup();
				if(pCurrentTok)	pCurrentTok->pStatement = stmt_DEFINT_DEFDBL_DEFSTR;
				return T_DEFSTR;
			}
			case Y_DEFFN:{
				*pLexC = (LexHoldChar); /* undo effects of setting up pLexText */
				pLexBufferChar = pLexC = pLexB + 3;
				LexDoBeforeAction(); /* set up pLexText again */
				LexRuleSetup();
				if(pCurrentTok){
					pCurrentTok->pStatement = stmt_DEFFN;
					pCurrentTok->Obj.pLocalSyms = nullptr;
				}
				return T_DEFFN;
			}
			case Y_DEFPROC:{
				*pLexC = LexHoldChar; /* undo effects of setting up pLexText */
				pLexBufferChar = pLexC = pLexB + 3;
				LexDoBeforeAction(); /* set up pLexText again */
				LexRuleSetup();
				if(pCurrentTok){
					pCurrentTok->pStatement = stmt_SUBDEF;
					pCurrentTok->Obj.pLocalSyms = nullptr;
				}
				return T_SUB;
			}
			case Y_DEG:{
				LexRuleSetup();
				if(pCurrentTok)	pCurrentTok->pStatement = stmt_TRIGMODE;
				return T_DEG;
			}
			case Y_DELETE:{
				LexRuleSetup();
				if(pCurrentTok)	pCurrentTok->pStatement = stmt_DELETE;
				return T_DELETE;
			}
			case Y_DELAY:{
				LexRuleSetup();
				if(pCurrentTok)	pCurrentTok->pStatement = stmt_DELAY;
				return T_DELAY;
			}
			case Y_DIM:{
				LexRuleSetup();
				if(pCurrentTok)	pCurrentTok->pStatement = stmt_DIM;
				return T_DIM;
			}
			case Y_DIGITIZE:{
				LexRuleSetup();
				if(pCurrentTok)	pCurrentTok->pStatement = stmt_DIGITIZE;
				return T_DIGITIZE;
			}
			case Y_DISABLE:{
				LexRuleSetup();
				if(pCurrentTok)	pCurrentTok->pStatement = stmt_ENABLE_DISABLE;
				return T_DISABLE;
			}
			case Y_DISP:{
				LexRuleSetup();
				if(pCurrentTok)	pCurrentTok->pStatement = stmt_PRINT_LPRINT;
				return T_DISP;
			}
			case Y_DIV:{
				LexRuleSetup();
				return T_DIV;
			}
			case Y_DO:{
				LexRuleSetup();
				if(pCurrentTok)	pCurrentTok->pStatement = stmt_DO;
				return T_DO;
			}
			case Y_DOUNTIL:{
				LexRuleSetup();
				if(pCurrentTok)	pCurrentTok->pStatement = stmt_DO_CONDITION;
				return T_DOUNTIL;
			}
			case Y_DOWHILE:{
				LexRuleSetup();
				if(pCurrentTok)	pCurrentTok->pStatement = stmt_DO_CONDITION;
				return T_DOWHILE;
			}
			case Y_DRAW:{
				LexRuleSetup();
				if(pCurrentTok)	pCurrentTok->pStatement = stmt_PLOT;
				return T_DRAW;
			}
			case Y_DUMP:{
				LexRuleSetup();
				if(pCurrentTok)	pCurrentTok->pStatement = stmt_DUMP;
				return T_DUMP;
			}
			case Y_ELSE:{
				LexRuleSetup();
				if(pCurrentTok)	pCurrentTok->pStatement = stmt_ELSE_ELSEIFELSE;
				return T_ELSE;
			}
			case Y_ELSEIFELSE:{
				*pLexC = (LexHoldChar); /* undo effects of setting up pLexText */
				pLexBufferChar = pLexC = pLexB + 4;
				LexDoBeforeAction(); /* set up pLexText again */
				LexRuleSetup();
				BEGIN(ELSEIF);
				if(pCurrentTok)	pCurrentTok->pStatement = stmt_ELSE_ELSEIFELSE;
				return T_ELSEIFELSE;
			}
			case Y_ELSEIFIF:{
				LexRuleSetup();
				BEGIN(INITIAL);
				if(pCurrentTok)	pCurrentTok->pStatement = stmt_IF_ELSEIFIF;
				return T_ELSEIFIF;
			}
			case Y_ENABLE:{
				LexRuleSetup();
				if(pCurrentTok)	pCurrentTok->pStatement = stmt_ENABLE_DISABLE;
				return T_ENABLE;
			}
			case Y_ENDIF:{
				LexRuleSetup();
				if(pCurrentTok)	pCurrentTok->pStatement = stmt_ENDIF;
				return T_ENDIF;
			}
			case Y_ENDPROC:{
				LexRuleSetup();
				if(pCurrentTok)	pCurrentTok->pStatement = stmt_SUBEND;
				return T_ENDPROC;
			}
			case Y_ENDSELECT:{
				LexRuleSetup();
				if(pCurrentTok)	pCurrentTok->pStatement = stmt_ENDSELECT;
				return T_ENDSELECT;
			}
			case Y_END:{
				LexRuleSetup();
				if(pCurrentTok)	pCurrentTok->pStatement = stmt_END;
				return T_END;
			}
			case Y_EQV:{
				LexRuleSetup();
				return T_EQV;
			}
			case Y_EXECUTE:{
				LexRuleSetup();
				if(pCurrentTok)	pCurrentTok->pStatement = stmt_EXECUTE;
				return T_EXECUTE;
			}
			case Y_EXITDO:{
				LexRuleSetup();
				if(pCurrentTok)	pCurrentTok->pStatement = stmt_EXITDO;
				return T_EXITDO;
			}
			case Y_EXITFOR:{
				LexRuleSetup();
				if(pCurrentTok)	pCurrentTok->pStatement = stmt_EXITFOR;
				return T_EXITFOR;
			}
			case Y_EXITGRAPH:{
				LexRuleSetup();
				if(pCurrentTok)	pCurrentTok->pStatement = stmt_GRAPHICS;
				return T_EXITGRAPH;
			}
			case Y_FIXED:{
				LexRuleSetup();
				if(pCurrentTok)	pCurrentTok->pStatement = stmt_FIXED;
				return T_FIXED;
			}
			case Y_FLOAT:{
				LexRuleSetup();
				if(pCurrentTok)	pCurrentTok->pStatement = stmt_FLOAT;
				return T_FLOAT;
			}
			case Y_FNEND:{
				LexRuleSetup();
				if(pCurrentTok)	pCurrentTok->pStatement = stmt_EQ_FNEND;
				return T_FNEND;
			}
			case Y_FOR:{
				LexRuleSetup();
				if(pCurrentTok)	pCurrentTok->pStatement = stmt_FOR;
				return T_FOR;
			}
			case Y_FRAME:{
				LexRuleSetup();
				if(pCurrentTok)	pCurrentTok->pStatement = stmt_FRAME;
				return T_FRAME;
			}
			case Y_GCHARSET:{
				LexRuleSetup();
				if(pCurrentTok)	pCurrentTok->pStatement = stmt_GCHARSET;
				return T_GCHARSET;
			}
			case Y_GCLEAR:{
				LexRuleSetup();
				if(pCurrentTok)	pCurrentTok->pStatement = stmt_GCLEAR;
				return T_GCLEAR;
			}
			case Y_GET:{
				LexRuleSetup();
				if(pCurrentTok)	pCurrentTok->pStatement = stmt_GET;
				return T_GET;
			}
			case Y_GLABEL:{
				LexRuleSetup();
				if(pCurrentTok)	pCurrentTok->pStatement = stmt_PRINT_LPRINT;
				return T_GLABEL;
			}
			case Y_GLOAD:{
				LexRuleSetup();
				if(pCurrentTok)	pCurrentTok->pStatement = stmt_GLOAD;
				return T_GLOAD;
			}
			case Y_GOSUB:{
				LexRuleSetup();
				if(pCurrentTok)	pCurrentTok->pStatement = stmt_GOSUB;
				return T_GOSUB;
			}
			case Y_GOTO:{
				LexRuleSetup();
				if(pCurrentTok)	pCurrentTok->pStatement = stmt_GOTO_RESUME;
				return T_GOTO;
			}
			case Y_GPLOT:{
				LexRuleSetup();
				if(pCurrentTok)	pCurrentTok->pStatement = stmt_GPLOT;
				return T_GPLOT;
			}
			case Y_GPRINT:{
				LexRuleSetup();
				if(pCurrentTok)	pCurrentTok->pStatement = stmt_GPRINT;
				return T_GPRINT;
			}
			case Y_GRAD:{
				LexRuleSetup();
				if(pCurrentTok)	pCurrentTok->pStatement = stmt_TRIGMODE;
				return T_GRAD;
			}
			case Y_GRAPHICS:{
				LexRuleSetup();
				if(pCurrentTok)	pCurrentTok->pStatement = stmt_GRAPHICS;
				return T_GRAPHICS;
			}
			case Y_GRID:{
				LexRuleSetup();
				if(pCurrentTok)	pCurrentTok->pStatement = stmt_GRID;
				return T_GRID;
			}
			case Y_GSTORE:{
				LexRuleSetup();
				if(pCurrentTok)	pCurrentTok->pStatement = stmt_GSTORE;
				return T_GSTORE;
			}
			case Y_IDN:
				LexRuleSetup();
				return T_IDN;
			case Y_IF:{
				LexRuleSetup();
				if(pCurrentTok)	pCurrentTok->pStatement = stmt_IF_ELSEIFIF;
				return T_IF;
			}
			case Y_IMAGE_1:{
				*pLexC = LexHoldChar; /* undo effects of setting up pLexText */
				pLexBufferChar = pLexC -= 1;
				LexDoBeforeAction(); /* set up pLexText again */
				LexRuleSetup();
				BEGIN(IMAGEFMT);
				if(pCurrentTok)	pCurrentTok->pStatement = stmt_IMAGE;
				return T_IMAGE;
			}
			case Y_IMAGESTRING:{
				*pLexC = LexHoldChar; /* undo effects of setting up pLexText */
				pLexBufferChar = pLexC -= 1;
				LexDoBeforeAction(); /* set up pLexText again */
				LexRuleSetup();
				BEGIN(INITIAL);
				if(pCurrentTok){
					pCurrentTok->Obj.pString = new BString;
					pCurrentTok->Obj.pString->Copy(pLexText, strlen(pLexText));
				}
				return T_STRING;
			}
			case Y_IMAGE_2:{
				LexRuleSetup();
				BEGIN(IMAGEFMT);
				if(pCurrentTok)	pCurrentTok->pStatement = stmt_IMAGE;
				return T_IMAGE;
			}
			case Y_IMP:
				LexRuleSetup();
				return T_IMP;
			case Y_INPUT:{
				LexRuleSetup();
				if(pCurrentTok)	pCurrentTok->pStatement = stmt_INPUT;
				return T_INPUT;
			}
			case Y_INPUTHASH:{
				*pLexC = LexHoldChar; /* undo effects of setting up pLexText */
				pLexBufferChar = pLexC = pLexB + 5;
				LexDoBeforeAction(); /* set up pLexText again */
				LexRuleSetup();
				if(pCurrentTok)	pCurrentTok->pStatement = stmt_INPUT;
				return T_INPUT;
			}
			case Y_INTEGER:{
				if(pCurrentTok)	pCurrentTok->pStatement = stmt_NUMERIC;
        return T_INTEGER;
      }
			case Y_INV:
				LexRuleSetup();
				return T_INV;
			case Y_IPLOT:{
				LexRuleSetup();
				if(pCurrentTok)	pCurrentTok->pStatement = stmt_PLOT;
				return T_IPLOT;
			}
			case Y_IS:
				LexRuleSetup();
				return T_IS;
			case Y_LDIR:{
				LexRuleSetup();
				if(pCurrentTok)	pCurrentTok->pStatement = stmt_LDIR;
				return T_LDIR;
			}
			case Y_LET:{
				LexRuleSetup();
				if(pCurrentTok)	pCurrentTok->pStatement = stmt_LET;
				return T_LET;
			}
			case Y_LETTER:{
				LexRuleSetup();
				if(pCurrentTok)	pCurrentTok->pStatement = stmt_LETTER;
				return T_LETTER;
			}
			case Y_LIMIT:{
				LexRuleSetup();
				if(pCurrentTok)	pCurrentTok->pStatement = stmt_LIMIT;
				return T_LIMIT;
			}
			case Y_LIN:{
				LexRuleSetup();
				return T_LIN;
			}
			case Y_LINETYPE:{
				LexRuleSetup();
				if(pCurrentTok)	pCurrentTok->pStatement = stmt_LINETYPE;
				return T_LINETYPE;
			}
			case Y_LINK:{
				LexRuleSetup();
				if(pCurrentTok)	pCurrentTok->pStatement = stmt_LINK;
				return T_LINK;
			}
			case Y_LIST:{
				LexRuleSetup();
				if(pCurrentTok)	pCurrentTok->pStatement = stmt_LIST;
				return T_LIST;
			}
			case Y_LOAD:{
				LexRuleSetup();
				if(pCurrentTok)	pCurrentTok->pStatement = stmt_LOAD;
				return T_LOAD;
			}
			case Y_LOCAL:{
				LexRuleSetup();
				if(pCurrentTok)	pCurrentTok->pStatement = stmt_LOCAL;
				return T_LOCAL;
			}
			case Y_LOCATE:{
				LexRuleSetup();
				if(pCurrentTok)	pCurrentTok->pStatement = stmt_LOCATE;
				return T_LOCATE;
			}
			case Y_LOCK:{
				LexRuleSetup();
				if(pCurrentTok)	pCurrentTok->pStatement = stmt_LOCK_UNLOCK;
				return T_LOCK;
			}
			case Y_LOOP:{
				LexRuleSetup();
				if(pCurrentTok)	pCurrentTok->pStatement = stmt_LOOP;
				return T_LOOP;
			}
			case Y_LOOPUNTIL:{
				LexRuleSetup();
				if(pCurrentTok)	pCurrentTok->pStatement = stmt_LOOPUNTIL;
				return T_LOOPUNTIL;
			}
			case Y_LORG:{
				LexRuleSetup();
				if(pCurrentTok)	pCurrentTok->pStatement = stmt_LORG;
				return T_LORG;
			}
			case Y_LPRINT:{
				LexRuleSetup();
				if(pCurrentTok)	pCurrentTok->pStatement = stmt_PRINT_LPRINT;
				return T_LPRINT;
			}
			case Y_LSET:{
				LexRuleSetup();
				if(pCurrentTok)	pCurrentTok->pStatement = stmt_LSET_RSET;
				return T_LSET;
			}
			case Y_MASSTORAGEIS:{
				LexRuleSetup();
				if(pCurrentTok)	pCurrentTok->pStatement = stmt_MASSTORAGEIS;
				return T_MASSTORAGEIS;
			}
			case Y_MATINPUT:{
				LexRuleSetup();
				if(pCurrentTok)	pCurrentTok->pStatement = stmt_MATINPUT;
				return T_MATINPUT;
			}
			case Y_MATPRINT:{
				LexRuleSetup();
				if(pCurrentTok) pCurrentTok->pStatement = stmt_MATPRINT;
				return T_MATPRINT;
			}
			case Y_MATREAD:{
				LexRuleSetup();
				if(pCurrentTok)	pCurrentTok->pStatement = stmt_MATREAD;
				return T_MATREAD;
			}
			case Y_MAT:{
				LexRuleSetup();
				if(pCurrentTok)	pCurrentTok->pStatement = stmt_MAT;
				return T_MAT;
			}
			case Y_MOD:
				LexRuleSetup();
				return T_MOD;
			case Y_MOVE:{
				LexRuleSetup();
				if(pCurrentTok)	pCurrentTok->pStatement = stmt_MOVE;
				return T_MOVE;
			}
			case Y_MSCALE:{
				LexRuleSetup();
				if(pCurrentTok)	pCurrentTok->pStatement = stmt_SCALE;
				return T_MSCALE;
			}
			case Y_NAME:{
				LexRuleSetup();
				if(pCurrentTok)	pCurrentTok->pStatement = stmt_NAME;
				return T_NAME;
			}
			case Y_NEXT:{
				LexRuleSetup();
				if(pCurrentTok){
					pCurrentTok->pStatement = stmt_NEXT;
					pCurrentTok->Obj.pNext = (Next_s*)malloc(sizeof(Next_s));
				}
				return T_NEXT;
			}
			case Y_NORMAL:{
				LexRuleSetup();
				if(pCurrentTok)	pCurrentTok->pStatement = stmt_TRACE;
				return T_NORMAL;
			}
			case Y_NOT:
				LexRuleSetup();
				return T_NOT;
			case Y_OFFKEY:{
				LexRuleSetup();
				if(pCurrentTok)	pCurrentTok->pStatement = stmt_OFFKEY;
				return T_OFFKEY;
			}
			case Y_ONERROROFF:{
				LexRuleSetup();
				if(pCurrentTok)	pCurrentTok->pStatement = stmt_ONERROROFF;
				return T_ONERROROFF;
			}
			case Y_ONERROR:{
				LexRuleSetup();
				if(pCurrentTok)	pCurrentTok->pStatement = stmt_ONERROR;
				return T_ONERROR;
			}
			case Y_ONKEY:{
				LexRuleSetup();
				if(pCurrentTok)	pCurrentTok->pStatement = stmt_ONKEY;
				return T_ONKEY;
			}
			case Y_ON:{
				LexRuleSetup();
				if(pCurrentTok){
					pCurrentTok->pStatement = stmt_ON;
					pCurrentTok->Obj.On.PcLength = 1;
					pCurrentTok->Obj.On.pPc = nullptr;
				}
				return T_ON;
			}
			case Y_OPTIONBASE:{
				LexRuleSetup();
				if(pCurrentTok)	pCurrentTok->pStatement = stmt_OPTIONBASE;
				return T_OPTIONBASE;
			}
			case Y_OR:
				LexRuleSetup();
				return T_OR;
			case Y_OUT:{
				LexRuleSetup();
				if(pCurrentTok)	pCurrentTok->pStatement = stmt_OUTPUT;
				return T_OUT;
			}
			case Y_OVERLAP:{
				LexRuleSetup();
				if(pCurrentTok)	pCurrentTok->pStatement = stmt_OVERLAP_SERIAL;
				return T_OVERLAP;
			}
			case Y_PAGE:{
				LexRuleSetup();
				return T_PAGE;
			}
			case Y_PAUSE:{
				LexRuleSetup();
				if(pCurrentTok)	pCurrentTok->pStatement = stmt_PAUSE;
				return T_PAUSE;
			}
			case Y_PDIR:{
				LexRuleSetup();
				if(pCurrentTok)	pCurrentTok->pStatement = stmt_PDIR;
				return T_PDIR;
			}
			case Y_PEN:{
				LexRuleSetup();
				if(pCurrentTok)	pCurrentTok->pStatement = stmt_PEN;
				return T_PEN;
			}
			case Y_PENUP:{
				LexRuleSetup();
				if(pCurrentTok)	pCurrentTok->pStatement = stmt_PENUP;
				return T_PENUP;
			}
			case Y_PLOT:{
				LexRuleSetup();
				if(pCurrentTok)	pCurrentTok->pStatement = stmt_PLOT;
				return T_PLOT;
			}
			case Y_PLOTTERIS:{
				LexRuleSetup();
				if(pCurrentTok)	pCurrentTok->pStatement = stmt_PLOTTERIS;
				return T_PLOTTERIS;
			}
			case Y_POINTER:{
				LexRuleSetup();
				if(pCurrentTok)	pCurrentTok->pStatement = stmt_POINTER;
				return T_POINTER;
			}
			case Y_PRINT:{
				LexRuleSetup();
				if(pCurrentTok)	pCurrentTok->pStatement = stmt_PRINT_LPRINT;
				return T_PRINT;
			}
			case Y_PRINTALLIS:{
				LexRuleSetup();
				if(pCurrentTok)	pCurrentTok->pStatement = stmt_PRINTERIS;
				return T_PRINTALLIS;
			}
			case Y_PRINTERIS:{
				LexRuleSetup();
				if(pCurrentTok)	pCurrentTok->pStatement = stmt_PRINTERIS;
				return T_PRINTERIS;
			}
			case Y_RAD:{
				LexRuleSetup();
				if(pCurrentTok)	pCurrentTok->pStatement = stmt_TRIGMODE;
				return T_RAD;
			}
			case Y_RANDOMIZE:{
				LexRuleSetup();
				if(pCurrentTok)	pCurrentTok->pStatement = stmt_RANDOMIZE;
				return T_RANDOMIZE;
			}
			case Y_RATIO:{
				LexRuleSetup();
				if(pCurrentTok)	pCurrentTok->pStatement = stmt_RATIO;
				return T_RATIO;
			}
			case Y_READ:{
				LexRuleSetup();
				if(pCurrentTok)	pCurrentTok->pStatement = stmt_READ;
				return T_READ;
			}
			case Y_REAL:{
				LexRuleSetup();
				if(pCurrentTok)	pCurrentTok->pStatement = stmt_NUMERIC;
				return T_REAL;
			}
			case Y_REDIM:{
				LexRuleSetup();
				if(pCurrentTok)	pCurrentTok->pStatement = stmt_REDIM;
				return T_REDIM;
			}
			case Y_RENUM:{
				LexRuleSetup();
				if(pCurrentTok)	pCurrentTok->pStatement = stmt_RENUM;
				return T_RENUM;
			}
			case Y_REPEAT:{
				LexRuleSetup();
				if(pCurrentTok)	pCurrentTok->pStatement = stmt_REPEAT;
				return T_REPEAT;
			}
			case Y_RESTORE:{
				LexRuleSetup();
				if(pCurrentTok)	pCurrentTok->pStatement = stmt_RESTORE;
				return T_RESTORE;
			}
			case Y_RESULT:{
				LexRuleSetup();
				if(pCurrentTok)	pCurrentTok->pStatement = stmt_RESULT;
				return T_RESULT;
			}
			case Y_RESUME:{
				LexRuleSetup();
				if(pCurrentTok)	pCurrentTok->pStatement = stmt_GOTO_RESUME;
				return T_RESUME;
			}
			case Y_RETURN:{
				LexRuleSetup();
				if(pCurrentTok)	pCurrentTok->pStatement = stmt_RETURN;
				return T_RETURN;
			}
			case Y_RPLOT:{
				LexRuleSetup();
				if(pCurrentTok)	pCurrentTok->pStatement = stmt_PLOT;
				return T_RPLOT;
			}
			case Y_RSET:{
				LexRuleSetup();
				if(pCurrentTok)	pCurrentTok->pStatement = stmt_LSET_RSET;
				return T_RSET;
			}
			case Y_RUN:{
				LexRuleSetup();
				if(pCurrentTok)	pCurrentTok->pStatement = stmt_RUN;
				return T_RUN;
			}
			case Y_SAVE:{
				LexRuleSetup();
				if(pCurrentTok)	pCurrentTok->pStatement = stmt_SAVE;
				return T_SAVE;
			}
			case Y_SCALE:{
				LexRuleSetup();
				if(pCurrentTok)	pCurrentTok->pStatement = stmt_SCALE;
				return T_SCALE;
			}
			case Y_SCRATCH:{
				LexRuleSetup();
				if(pCurrentTok)	pCurrentTok->pStatement = stmt_SCRATCH;
				return T_SCRATCH;
			}
			case Y_SCRATCH_A:{
				LexRuleSetup();
				if(pCurrentTok)	pCurrentTok->pStatement = stmt_SCRATCH;
				return T_SCRATCH_A;
			}
			case Y_SCRATCH_C:{
				LexRuleSetup();
				if(pCurrentTok)	pCurrentTok->pStatement = stmt_SCRATCH;
				return T_SCRATCH_C;
			}
			case Y_SCRATCH_K:{
				LexRuleSetup();
				if(pCurrentTok)	pCurrentTok->pStatement = stmt_SCRATCH;
				return T_SCRATCH_K;
			}
			case Y_SCRATCH_P:{
				LexRuleSetup();
				if(pCurrentTok)	pCurrentTok->pStatement = stmt_SCRATCH;
				return T_SCRATCH_P;
			}
			case Y_SCRATCH_V:{
				LexRuleSetup();
				if(pCurrentTok)	pCurrentTok->pStatement = stmt_SCRATCH;
				return T_SCRATCH_V;
			}
			case Y_SELECT:{
				LexRuleSetup();
				if(pCurrentTok){
					pCurrentTok->pStatement = stmt_SELECT;
					pCurrentTok->Obj.pSelectCase = (Selectcase_s*)malloc(sizeof(Selectcase_s));
				}
				return T_SELECT;
			}
			case Y_SERIAL:{
				LexRuleSetup();
				if(pCurrentTok)	pCurrentTok->pStatement = stmt_OUTPUT;
				return T_SERIAL;
			}
			case Y_SHORT:{
				LexRuleSetup();
				if(pCurrentTok)	pCurrentTok->pStatement = stmt_NUMERIC;
				return T_SHORT;
			}
			case Y_SHOW:{
				LexRuleSetup();
				if(pCurrentTok)	pCurrentTok->pStatement = stmt_SHOW;
				return T_SHOW;
			}
			case Y_SPA:{
				LexRuleSetup();
				return T_SPA;
			}
			case Y_STANDARD:{
				LexRuleSetup();
				if(pCurrentTok)	pCurrentTok->pStatement = stmt_STANDARD;
				return T_STANDARD;
			}
			case Y_STEP:
				LexRuleSetup();
				return T_STEP;
			case Y_STOP:{
				LexRuleSetup();
				if(pCurrentTok)	pCurrentTok->pStatement = stmt_STOP;
				return T_STOP;
			}
			case Y_STORE:{
				LexRuleSetup();
				if(pCurrentTok)	pCurrentTok->pStatement = stmt_STORE;
				return T_STORE;
			}
			case Y_SUBEND:{
				LexRuleSetup();
				if(pCurrentTok)	pCurrentTok->pStatement = stmt_SUBEND;
				return T_SUBEND;
			}
			case Y_SUBEXIT:{
				LexRuleSetup();
				if(pCurrentTok)	pCurrentTok->pStatement = stmt_SUBEXIT;
				return T_SUBEXIT;
			}
			case Y_SUB:{
				LexRuleSetup();
				if(pCurrentTok){
					pCurrentTok->pStatement = stmt_SUBDEF;
					pCurrentTok->Obj.pLocalSyms = nullptr;
				}
				return T_SUB;
			}
			case Y_SYSTEM:{
				LexRuleSetup();
				if(pCurrentTok)	pCurrentTok->pStatement = stmt_SYSTEM;
				return T_SYSTEM;
			}
			case Y_THEN:
				LexRuleSetup();
				return T_THEN;
			case Y_TAB:
				LexRuleSetup();
				return T_TAB;
			case Y_TO:
				LexRuleSetup();
				return T_TO;
			case Y_TRN:
				LexRuleSetup();
				return T_TRN;
			case Y_TRACE:{
				LexRuleSetup();
				if(pCurrentTok) pCurrentTok->pStatement = stmt_TRACE;
				return T_TRACE;
			}
			case Y_TRALL:{
				LexRuleSetup();
				if(pCurrentTok)	pCurrentTok->pStatement = stmt_TRACE;
				return T_TRALL;
			}
			case Y_TRPAUSE:{
				LexRuleSetup();
				if(pCurrentTok)	pCurrentTok->pStatement = stmt_TRACE;
				return T_TRPAUSE;
			}
			case Y_TRVARS:{
				LexRuleSetup();
				if(pCurrentTok)	pCurrentTok->pStatement = stmt_TRACE;
				return T_TRVARS;
			}
			case Y_TRALLVARS:{
				LexRuleSetup();
				if(pCurrentTok)	pCurrentTok->pStatement = stmt_TRACE;
				return T_TRALLVARS;
			}
			case Y_TRWAIT:{
				LexRuleSetup();
				if(pCurrentTok)	pCurrentTok->pStatement = stmt_TRACE;
				return T_TRWAIT;
			}
			case Y_TRUNCATE:{
				LexRuleSetup();
				if(pCurrentTok)	pCurrentTok->pStatement = stmt_TRUNCATE;
				return T_TRUNCATE;
			}
			case Y_UNCLIP:{
				LexRuleSetup();
				if(pCurrentTok)	pCurrentTok->pStatement = stmt_UNCLIP;
				return T_UNCLIP;
			}
			case Y_UNLOCK:{
				LexRuleSetup();
				if(pCurrentTok)	pCurrentTok->pStatement = stmt_LOCK_UNLOCK;
				return T_UNLOCK;
			}
			case Y_UNNUM:{
				LexRuleSetup();
				if(pCurrentTok)	pCurrentTok->pStatement = stmt_UNNUM;
				return T_UNNUM;
			}
			case Y_UNTIL:{
				LexRuleSetup();
				if(pCurrentTok)	pCurrentTok->pStatement = stmt_UNTIL;
				return T_UNTIL;
			}
			case Y_USING:
				LexRuleSetup();
				return T_USING;
			case Y_WAIT:{
				LexRuleSetup();
				if(pCurrentTok)	pCurrentTok->pStatement = stmt_WAIT;
				return T_WAIT;
			}
			case Y_WEND:{
				LexRuleSetup();
				if(pCurrentTok){
					pCurrentTok->pStatement = stmt_WEND;
					pCurrentTok->Obj.pWhilePc = (PC_s*)malloc(sizeof(PC_s));
				}
				return T_WEND;
			}
			case Y_WHERE:{
				LexRuleSetup();
				if(pCurrentTok)	pCurrentTok->pStatement = stmt_WHERE;
				return T_WHERE;
			}
			case Y_WHILE:{
				LexRuleSetup();
				if(pCurrentTok){
					pCurrentTok->pStatement = stmt_WHILE;
					pCurrentTok->Obj.pAfterWendPc = (PC_s*)malloc(sizeof(PC_s));
				}
				return T_WHILE;
			}
			case Y_WIDTH:{
				LexRuleSetup();
				return T_WIDTH;
			}
			case Y_WIDTHHASH:{
				*pLexC = LexHoldChar; /* undo effects of setting up pLexText */
				pLexBufferChar = pLexC = pLexB + 5;
				LexDoBeforeAction(); /* set up pLexText again */
				LexRuleSetup();
				return T_WIDTH;
			}
			case Y_WRITE:{
				LexRuleSetup();
				if(pCurrentTok)	pCurrentTok->pStatement = stmt_WRITE;
				return T_WRITE;
			}
			case Y_WRITEHASH:{
				*pLexC = LexHoldChar; /* undo effects of setting up pLexText */
				pLexBufferChar = pLexC = pLexB + 5;
				LexDoBeforeAction(); /* set up pLexText again */
				LexRuleSetup();
				if(pCurrentTok)	pCurrentTok->pStatement = stmt_WRITE;
				return T_WRITE;
			}
			case Y_XOR:
				LexRuleSetup();
				return T_XOR;
			case Y_ZER:
				LexRuleSetup();
				return T_ZER;
			case Y_QUOTE:
			case Y_REM:{
				LexRuleSetup();
				if(pCurrentTok){
					int skip = (Lex_act == Y_REM) ? 3 : 1;				// skip 3 characters for 'rem' and 1 for '!'
					pCurrentTok->pStatement = stmt_QUOTE_REM;
					int len = strlen(pLexText + skip) + 1;
					pCurrentTok->Obj.pRem = (char*)malloc(len);
					strcpy_s(pCurrentTok->Obj.pRem, len, pLexText + skip);
				}
				return T_REM;
			}
			case Y_RENAME:{
				LexRuleSetup();
				if(pCurrentTok)	pCurrentTok->pStatement = stmt_COPY_RENAME;
				return T_RENAME;
			}
			case Y_LINEINPUT:{
				LexRuleSetup();
				if(pCurrentTok)	pCurrentTok->pStatement = stmt_INPUT;
				return T_LINEINPUT;
			}
			case Y_IDENTIFIER:{
				LexRuleSetup();
				if(pCurrentTok){
					UINT Len, StrLen;
					char *pStr;
					bool FuncId = false;
					pCurrentTok->pStatement = stmt_IDENTIFIER;
					if(tolower((int)pLexText[0]) == 'f' && tolower((int)pLexText[1]) == 'n'){
						for(Len = 2, pStr = &pLexText[2]; *pStr == ' ' || *pStr == '\t'; ++pStr) ;
						FuncId = true;
					}
					else{
						Len = 0;
						pStr = pLexText;
					}
					StrLen = strlen(pStr) + 1;
					Len += StrLen;
					Len += offsetof(Identifier_s, Name);
					pCurrentTok->Obj.pIdentifier = (Identifier_s*)malloc(Len);
					if(FuncId){
						memcpy(pCurrentTok->Obj.pIdentifier->Name, pLexText, 2);
						strcpy_s(pCurrentTok->Obj.pIdentifier->Name + 2, StrLen, pStr);
					}
					else strcpy_s(pCurrentTok->Obj.pIdentifier->Name, StrLen, pStr);
					switch(pLexText[LexLength - 1]){
						case '$': pCurrentTok->Obj.pIdentifier->DefType = eVType::V_STRING; break;
						case '%': pCurrentTok->Obj.pIdentifier->DefType = eVType::V_INT; break;
						default: pCurrentTok->Obj.pIdentifier->DefType = eVType::V_REAL; break;
					}
				}
				return T_IDENTIFIER;
			}
			case Y_LABEL:{
				LexRuleSetup();
				if(pCurrentTok){
					pCurrentTok->pStatement = stmt_LABEL;
					char *pPos, *pStr = pLexText;
					while(*pStr == ' ' || *pStr == '\t') ++pStr;
					if((pPos = strchr(pStr, ' ')) != nullptr) *pPos = 0;
					if((pPos = strchr(pStr, ':')) != nullptr) *pPos = 0;
					UINT StrLen = strlen(pStr) + 1;
					pCurrentTok->Obj.pLabel = (Label_s*)malloc(sizeof(Label_s));
					pCurrentTok->Obj.pLabel->pName = (char*)malloc(StrLen);
					strcpy_s(pCurrentTok->Obj.pLabel->pName, StrLen, pStr);
				}
				return T_LABEL;
			}
			case Y_RSPACE:					/* rule space can match eol */
				LexRuleSetup();
				break;
			case Y_JUNK:{
				LexRuleSetup();
				if(pCurrentTok) pCurrentTok->Obj.Junk = pLexText[0];
				return T_JUNK;
			}
			case Y_EOL_4:
				LexRuleSetup();
//				ECHO;
				break;
			case LEX_STATE_EOF(INITIAL):
			case LEX_STATE_EOF(DATAINPUT):
			case LEX_STATE_EOF(ELSEIF):
			case LEX_STATE_EOF(IMAGEFMT):
				BEGIN(INITIAL);
				LEX_TERMINATE();
			case LEX_END_OF_BUFFER:{
				int LexMatchedTextCount = (int)(pLexC - pLexText) - 1;	/* Amount of text matched not including the EOB char. */
				*pLexC = LexHoldChar;																							/* Undo the effects of LeDoBeforeAction(pLexB, pLexC). */
				if(ppScanObjStack[ScanObjStackTop]->BufferStatus  ==  LEX_BUFFER_NEW){
					/* We're scanning a new file or input source.  It's	possible that this happened because the user just pointed pStream at a new source and called
					 * Lexlex().  If so, then we have to assure	consistency between LEX_CURRENT_BUFFER and our globals.  Here is the right place to do so, because
					 * this is the first action (other than possibly a back-up) that will match for the new input source.	 */
					CharCount = ppScanObjStack[ScanObjStackTop]->CharCount;
					ppScanObjStack[ScanObjStackTop]->pStream = pStream;
					ppScanObjStack[ScanObjStackTop]->BufferStatus = LEX_BUFFER_NORMAL;
				}
				/* Note that here we test for pLexBufferChar "<=" to the position	of the first EOB in the buffer, since pLexBufferChar will	already have been incremented
				 * past the NUL character	(since all states make transitions on EOB to the end-of-buffer state).  Contrast this with the test in input(). */
				if(pLexBufferChar <= &ppScanObjStack[ScanObjStackTop]->pBuffer[CharCount]){ /* This was really a NUL. */
					LexStateType LexNextState;
					pLexBufferChar = pLexText + LexMatchedTextCount;
					LexCurrentState = LexGetPreviousState();
					/* Okay, we're now positioned to make the NULL transition.  We couldn't have LexGetPreviousState() go ahead and do it for us because it doesn't
					 * know how to deal	with the possibility of jamming (and we don't want to build jamming into it because then it will run more slowly).	 */
					LexNextState = LexTryTransition0(LexCurrentState);
					pLexB = pLexText + LEX_MORE_ADJ;
					if(LexNextState){																										/* Consume the NUL. */
						pLexC = ++pLexBufferChar;
						LexCurrentState = LexNextState;
						goto LexMatch;
					}
					else{
						pLexC = pLexBufferChar;
						goto LexFindAction;
					}
				}
				else{
					switch(LexGetNextBuffer()){
						case EOB_ACT_END_OF_FILE:{
							LexDidBufferSwitchOnEOF = 0;
							if(LexWrap()){
								/* Note: because we've taken care in LexGetNextBuffer() to have set up pLexText, we can now set up pLexBufferChar so that if some total
								 * hoser (like flex itself) wants to call the scanner after we return the NULL, it'll still work - another nullptr will get returned. */
								pLexBufferChar = pLexText + LEX_MORE_ADJ;
								Lex_act = LEX_STATE_EOF(LEX_START);
								goto do_action;
							}
							else if(!LexDidBufferSwitchOnEOF) LEX_NEW_FILE;
							break;
						}
 						case EOB_ACT_CONTINUE_SCAN:
							pLexBufferChar =	pLexText + LexMatchedTextCount;
							LexCurrentState = LexGetPreviousState();
							pLexC = pLexBufferChar;
							pLexB = pLexText + LEX_MORE_ADJ;
							goto LexMatch;
						case EOB_ACT_LAST_MATCH:
							pLexBufferChar =	&ppScanObjStack[ScanObjStackTop]->pBuffer[(CharCount)];
							LexCurrentState = LexGetPreviousState(  );
							pLexC = pLexBufferChar;
							pLexB = pLexText + LEX_MORE_ADJ;
							goto LexFindAction;
							break;
					}
				}
				break;
			}
			default:
				LexFatalError( "fatal flex scanner internal error--no action found" );
				break;
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////

/* LexGetNextBuffer - try to read in a new buffer
 *
 * Returns a code representing an action:
 *	EOB_ACT_LAST_MATCH -
 *	EOB_ACT_CONTINUE_SCAN - continue scanning from current position
 *	EOB_ACT_END_OF_FILE - end of file
 */
static int LexGetNextBuffer (void)
{
register char *dest = ppScanObjStack[ScanObjStackTop]->pBuffer;
register char *source = (pLexText);
register int number_to_move, i;
int ret_val;

	if(pLexBufferChar > &ppScanObjStack[ScanObjStackTop]->pBuffer[CharCount + 1])	LexFatalError("fatal flex scanner internal error--end of buffer missed" );
	if(ppScanObjStack[ScanObjStackTop]->DoFillBuffer  ==  0){ /* Don't try to fill the buffer, so this is an EOF. */
		if(pLexBufferChar - pLexText - LEX_MORE_ADJ  ==  1) return EOB_ACT_END_OF_FILE;	/* We matched a single character, the EOB, so treat this as a final EOF. */
 		else return EOB_ACT_LAST_MATCH;		/* We matched some text prior to the EOB, first	process it. */
	}	/* Try to read more data. */
	number_to_move = (int)(pLexBufferChar - pLexText) - 1;	/* First move last chars to start of buffer. */
	for(i = 0; i < number_to_move; ++i) *(dest++) = *(source++);
	if(ppScanObjStack[ScanObjStackTop]->BufferStatus  ==  LEX_BUFFER_EOF_PENDING){
		ppScanObjStack[ScanObjStackTop]->CharCount = CharCount = 0; /* don't do the read, it's not guaranteed to return an EOF, just force an EOF */
	}
 	else{
		int num_to_read =	ppScanObjStack[ScanObjStackTop]->BufferSize - number_to_move - 1;
		while(num_to_read <= 0){ /* Not enough room in the buffer - grow it. just a shorter name for the current buffer */
			pScanObject_s pScanObj = GetLexCurrentBuffer();
			int Lex_c_buf_p_offset = (int) (pLexBufferChar - pScanObj->pBuffer);
			if(pScanObj->IsOurBuffer){
				int new_size = pScanObj->BufferSize * 2;
				if(new_size <= 0) pScanObj->BufferSize += pScanObj->BufferSize / 8;
				else pScanObj->BufferSize *= 2;
				pScanObj->pBuffer = (char*)realloc((char*)pScanObj->pBuffer, pScanObj->BufferSize + 2);  /* Include room in for 2 EOB chars. */
			}
			else pScanObj->pBuffer = 0;	/* Can't grow it, we don't own it. */
			if(!pScanObj->pBuffer) LexFatalError("fatal error - scanner input buffer overflow" );
			pLexBufferChar = &pScanObj->pBuffer[Lex_c_buf_p_offset];
			num_to_read = ppScanObjStack[ScanObjStackTop]->BufferSize - number_to_move - 1;
		}
		if(num_to_read > LEX_READ_BUF_SIZE) num_to_read = LEX_READ_BUF_SIZE;
		LexInput((&ppScanObjStack[ScanObjStackTop]->pBuffer[number_to_move]), CharCount, (UINT)num_to_read);	/* Read in more data. */
		ppScanObjStack[ScanObjStackTop]->CharCount = CharCount;
	}
	if(CharCount  ==  0){
		if(number_to_move  ==  LEX_MORE_ADJ){
			ret_val = EOB_ACT_END_OF_FILE;
			LexRestart(pStream);
		}
		else{
			ret_val = EOB_ACT_LAST_MATCH;
			ppScanObjStack[ScanObjStackTop]->BufferStatus = LEX_BUFFER_EOF_PENDING;
		}
	}
	else ret_val = EOB_ACT_CONTINUE_SCAN;
	if((UINT)(CharCount + number_to_move) > ppScanObjStack[ScanObjStackTop]->BufferSize){
		UINT new_size = CharCount + number_to_move + (CharCount >> 1);	/* Extend the array by 50%, plus the number we really need. */
		ppScanObjStack[ScanObjStackTop]->pBuffer = (char*)realloc((char *)ppScanObjStack[ScanObjStackTop]->pBuffer, new_size);
		if(!ppScanObjStack[ScanObjStackTop]->pBuffer) LexFatalError( "out of dynamic memory in LexGetNextBuffer()" );
	}
	CharCount += number_to_move;
	ppScanObjStack[ScanObjStackTop]->pBuffer[CharCount] = LEX_END_OF_BUFFER_CHAR;
	ppScanObjStack[ScanObjStackTop]->pBuffer[CharCount + 1] = LEX_END_OF_BUFFER_CHAR;
	pLexText = &ppScanObjStack[ScanObjStackTop]->pBuffer[0];
	return ret_val;
}

//////////////////////////////////////////////////////////////////////////////////////

/* LexGetPreviousState - get the state just before the EOB char was reached */
static LexStateType LexGetPreviousState(void)
{
	register LexStateType LexCurrentState;
	register char *pLexC;

	LexCurrentState = LexStart;
	pLexStatePtr = LexStateBuffer;
	*pLexStatePtr++ = LexCurrentState;
	for(pLexC = (pLexText) + LEX_MORE_ADJ; pLexC < pLexBufferChar; ++pLexC){
		register LEX_CHAR Lex_c = (*pLexC ? Lex_ec[LexChar2Uint(*pLexC)] : 1);
		if(Lex_accept[LexCurrentState]){
			LexLastAcceptingState = LexCurrentState;
			pLexLastAcceptingPos = pLexC;
		}	 
		while(Lex_chk[Lex_base[LexCurrentState] + Lex_c]  !=  LexCurrentState){
			LexCurrentState = (int)Lex_def[LexCurrentState];
			if(LexCurrentState >= LEX_MAX_STATES) Lex_c = Lex_meta[(unsigned int)Lex_c];
		}
		LexCurrentState = LexNext[Lex_base[LexCurrentState] + (unsigned int)Lex_c];
		*pLexStatePtr++ = LexCurrentState;
	}
	return LexCurrentState;
}

//////////////////////////////////////////////////////////////////////////////////////

/* LexTryTransition0 - try to make a transition on the NUL character
 *
 * synopsis
 *	next_state = LexTryTransition0( current_state );
 */
static LexStateType LexTryTransition0(LexStateType LexCurrentState)
{
register int Lex_is_jam;
register char *pLexC = (pLexBufferChar);
register LEX_CHAR Lex_c = 1;

	if(Lex_accept[LexCurrentState]){
		LexLastAcceptingState = LexCurrentState;
		pLexLastAcceptingPos = pLexC;
	}
	while(Lex_chk[Lex_base[LexCurrentState] + Lex_c]  !=  LexCurrentState){
		LexCurrentState = (int)Lex_def[LexCurrentState];
		if(LexCurrentState >= LEX_MAX_STATES) Lex_c = Lex_meta[(unsigned int) Lex_c];
	}
	LexCurrentState = LexNext[Lex_base[LexCurrentState] + (unsigned int)Lex_c];
	Lex_is_jam = (LexCurrentState  ==  LEX_MAX_STATES - 1);
	return Lex_is_jam ? 0 : LexCurrentState;
}

//////////////////////////////////////////////////////////////////////////////////////

#ifndef LEX_NO_INPUT
static int Lexinput(void)
{
int c;

	*pLexBufferChar = LexHoldChar;
	if(*pLexBufferChar  ==  LEX_END_OF_BUFFER_CHAR){ // pLexBufferChar now points to the character we want to return. If this occurs *before* the EOB characters, then it's a valid NUL; if not, then we've hit the end of the buffer.
		if(pLexBufferChar < &ppScanObjStack[ScanObjStackTop]->pBuffer[CharCount]) *pLexBufferChar = '\0';			/* This was really a NUL. */
		else{ /* need more input */
			int offset = pLexBufferChar - pLexText;
			++pLexBufferChar;
			switch(LexGetNextBuffer()){
				case EOB_ACT_LAST_MATCH:
					/* This happens because Lex_g_n_b()	sees that we've accumulated a token and flags that we need to try matching the token before
					 * proceeding.  But for input(), there's no matching to consider. So convert the EOB_ACT_LAST_MATCH to EOB_ACT_END_OF_FILE.	*/
					LexRestart(pStream);		/* Reset buffer status. */
				/*FALLTHROUGH*/
				case EOB_ACT_END_OF_FILE:{
					if(LexWrap()) return EOF;
					if(!LexDidBufferSwitchOnEOF) LEX_NEW_FILE;
					return Lexinput();
				}
				case EOB_ACT_CONTINUE_SCAN:
					pLexBufferChar = pLexText + offset;
					break;
			}
		}
	}
	c = *(unsigned char *)pLexBufferChar;  /* cast for 8-bit char's */
	*pLexBufferChar = '\0'; /* preserve pLexText */
	LexHoldChar = *++pLexBufferChar;
	return c;
}

#endif  /* ifndef LEX_NO_INPUT */

//////////////////////////////////////////////////////////////////////////////////////

/** Immediately switch to a different input stream.
 * @param input_file A readable stream.
 *
 * @note This function does not reset the start condition to rule INITIAL .
 */
void LexRestart(FILE *pFile)
{
	if(GetLexCurrentBuffer() == nullptr){
		CreateScanObjStack();
		ppScanObjStack[ScanObjStackTop] = CreateFileScanObj(pStream, LEX_BUF_SIZE );
	}
	InitFileScanObj(GetLexCurrentBuffer(), pFile);
	LoadObjectState();
}

//////////////////////////////////////////////////////////////////////////////////////

/** Switch to a different input buffer.
 * @param new_buffer The new input buffer.
 *
 */
void SwitchScanObj(pScanObject_s pScanObj)
{
	/* TODO. We should be able to replace this entire function body
	 * with
	 *		PopScanObj();
	 *		PushScanObj(new_buffer);
	 */
	CreateScanObjStack();
	if(GetLexCurrentBuffer() == pScanObj) return;
	if(GetLexCurrentBuffer() != nullptr){	 		/* Flush out information for old buffer. */
		*pLexBufferChar = LexHoldChar;
		ppScanObjStack[ScanObjStackTop]->pCurrentPos = pLexBufferChar;
		ppScanObjStack[ScanObjStackTop]->CharCount = CharCount;
	}
	ppScanObjStack[ScanObjStackTop] = pScanObj;
	LoadObjectState();

	/* We don't actually know whether we did this switch during
	 * EOF (LexWrap()) processing, but the only time this flag
	 * is looked at is after LexWrap() is called, so it's safe
	 * to go ahead and always set it.
	 */
	LexDidBufferSwitchOnEOF = 1;
}

//////////////////////////////////////////////////////////////////////////////////////

static void LoadObjectState(void)
{
	CharCount = ppScanObjStack[ScanObjStackTop]->CharCount;
	pLexText = pLexBufferChar = ppScanObjStack[ScanObjStackTop]->pCurrentPos;
	pStream = ppScanObjStack[ScanObjStackTop]->pStream;
	LexHoldChar = *pLexBufferChar;
}

//////////////////////////////////////////////////////////////////////////////////////

/* Allocate and initialise an input buffer. */
pScanObject_s CreateFileScanObj(FILE *pFile, int size)
{
pScanObject_s pScanObj;

	pScanObj = (pScanObject_s)malloc(sizeof(ScanObject_s));
	if(!pScanObj) LexFatalError("out of dynamic memory in CreateFileScanObj()");
	pScanObj->BufferSize = size;
	/* pBuffer has to be 2 characters longer than the size given because we need to put in 2 end-of-buffer characters. */
	pScanObj->pBuffer = (char*)malloc(pScanObj->BufferSize + 2);
	if(!pScanObj->pBuffer) LexFatalError("out of dynamic memory in CreateFileScanObj()");
	pScanObj->IsOurBuffer = 1;
	InitFileScanObj(pScanObj, pFile);
	return pScanObj;
}

//////////////////////////////////////////////////////////////////////////////////////

/* Destroy the buffer. */
void DeleteScanObj(pScanObject_s pScanObj)
{
	if(!pScanObj) return;
	if(pScanObj  ==  GetLexCurrentBuffer()) ppScanObjStack[ScanObjStackTop] = nullptr;	  // Not sure if we should pop here. 
	if(ScanObjStackTop == 0){																															// no more scan objects...
		free(ppScanObjStack);																																// so free up memory
		ppScanObjStack = nullptr;																														// and invalidate pointer
	}
	if(pScanObj->IsOurBuffer) free((char*)pScanObj->pBuffer);
	free((ScanObject_s*)pScanObj);
}

//////////////////////////////////////////////////////////////////////////////////////

/* Initializes or reinitializes a buffer.	This function is sometimes called more than once on the same buffer, such as during a LexRestart() or at EOF.*/
static void InitFileScanObj(pScanObject_s pScanObj, FILE *pFile)
{
int oerrno = errno;

	FlushScanObj(pScanObj);
	pScanObj->pStream = pFile;
	pScanObj->DoFillBuffer = 1;
	// If pScanObj is current, then InitFileScanObj was probably called from LexRestart() or through LexGetNextBuffer. In that case, we don't want to reset the lineno or column.	 
	if(pScanObj  !=  GetLexCurrentBuffer()){
		pScanObj->BsLineNumber = 1;
		pScanObj->BsColumnCount = 0;
	}
	pScanObj->IsInteractive = pFile ? (_isatty(_fileno(pFile)) > 0) : 0;
	errno = oerrno;
}

//////////////////////////////////////////////////////////////////////////////////////

/** Discard all buffered characters. On the next scan, LEX_INPUT will be called.
 * @param pScanObj the buffer state to be flushed, usually @c LEX_CURRENT_BUFFER.
 *
 */
void FlushScanObj(pScanObject_s pScanObj)
{
	if(!pScanObj) return;
	pScanObj->CharCount = 0;
	/* We always need two end-of-buffer characters.  The first causes	a transition to the end-of-buffer state.  The second causes a jam in that state. */
	pScanObj->pBuffer[0] = LEX_END_OF_BUFFER_CHAR;
	pScanObj->pBuffer[1] = LEX_END_OF_BUFFER_CHAR;
	pScanObj->pCurrentPos = &pScanObj->pBuffer[0];
	pScanObj->IsAtLineStart = 1;
	pScanObj->BufferStatus = LEX_BUFFER_NEW;
	if(pScanObj  ==  GetLexCurrentBuffer()) LoadObjectState();
}

//////////////////////////////////////////////////////////////////////////////////////

/** Pushes the new state onto the stack. The new state becomes
 *  the current state. This function will allocate the stack
 *  if necessary.
 *  @param new_buffer The new state.
 *
 */
void PushScanObj(pScanObject_s new_buffer)
{
	if(new_buffer  ==  nullptr) return;
	CreateScanObjStack();
	if(GetLexCurrentBuffer() != nullptr){																				/* This block is copied from SwitchScanObj. */
		*(pLexBufferChar) = LexHoldChar;													/* Flush out information for old buffer. */
		ppScanObjStack[ScanObjStackTop]->pCurrentPos = pLexBufferChar;
		ppScanObjStack[ScanObjStackTop]->CharCount = CharCount;
	}
	if(GetLexCurrentBuffer() != nullptr) ScanObjStackTop++;									/* Only push if top exists. Otherwise, replace top. */
	ppScanObjStack[ScanObjStackTop] = new_buffer;
	LoadObjectState();																				/* copied from SwitchScanObj. */
	LexDidBufferSwitchOnEOF = 1;
}

//////////////////////////////////////////////////////////////////////////////////////

/** Removes and deletes the top of the stack, if present.
 *  The next element becomes the new top.
 *
 */
void PopScanObj(void)
{
	if(GetLexCurrentBuffer() == nullptr) return;
	DeleteScanObj(GetLexCurrentBuffer());
	ppScanObjStack[ScanObjStackTop] = nullptr;
	if(ScanObjStackTop > 0) --ScanObjStackTop;
	if(GetLexCurrentBuffer() != nullptr){
		LoadObjectState();
		LexDidBufferSwitchOnEOF = 1;
	}
}

//////////////////////////////////////////////////////////////////////////////////////

/* Allocates the stack if it does not exist. Guarantees space for at least one push. */
static void CreateScanObjStack(void)
{
int Slots;

	
	if(!ppScanObjStack){		// First allocation is just for 2 elements, since we don't know if this	scanner will even need a stack.
		Slots = 1;							// We use 2 instead of 1 to avoid an immediate realloc on the next call.
		ppScanObjStack = (ScanObject_s**)malloc(Slots * sizeof(ScanObject_s*));
		if(!ppScanObjStack) LexFatalError("out of dynamic memory in CreateScanObjStack()");
		memset(ppScanObjStack, 0, Slots * sizeof(ScanObject_s*));
		ScanObjStackMax = Slots;
		ScanObjStackTop = 0;
		return;
	}
	if(ScanObjStackTop >= ScanObjStackMax - 1){
		int grow_size = 8;																					/* Increase the buffer to prepare for a possible push. arbitrary grow size*/
		Slots = ScanObjStackMax + grow_size;
		ppScanObjStack = (ScanObject_s**)realloc((char*)(ppScanObjStack), (Slots * sizeof(ScanObject_s*)));
		if(!ppScanObjStack) LexFatalError("out of dynamic memory in CreateScanObjStack()");			// never comes back from this
		memset(ppScanObjStack + ScanObjStackMax, 0, grow_size * sizeof(ScanObject_s*));						// zero only the new slots.
		ScanObjStackMax = Slots;
	}
}

//////////////////////////////////////////////////////////////////////////////////////

/** Setup the input buffer state to scan directly from a user-specified character buffer.
 * @param base the character buffer
 * @param size the size in bytes of the character buffer
 *
 * @return the newly allocated buffer state object.
 */
pScanObject_s CreateScanObject(char * base, UINT size)
{
pScanObject_s pScanObj;

	if((size < 2) || (base[size - 2]  !=  LEX_END_OF_BUFFER_CHAR) || (base[size - 1]  !=  LEX_END_OF_BUFFER_CHAR)) return 0; /* They forgot to leave room for the EOB's. */
	pScanObj = (pScanObject_s)malloc(sizeof(struct ScanObject_s));
	if(!pScanObj) LexFatalError("out of dynamic memory in CreateScanObject()");
	pScanObj->BufferSize = size - 2;  /* "- 2" to take care of EOB's */
	pScanObj->pCurrentPos = pScanObj->pBuffer = base;
	pScanObj->IsOurBuffer = 0;
	pScanObj->pStream = 0;
	pScanObj->CharCount = pScanObj->BufferSize;
	pScanObj->IsInteractive = 0;
	pScanObj->IsAtLineStart = 1;
	pScanObj->DoFillBuffer = 0;
	pScanObj->BufferStatus = LEX_BUFFER_NEW;
	SwitchScanObj(pScanObj);
	return pScanObj;
}

//////////////////////////////////////////////////////////////////////////////////////

/** Setup the input buffer state to scan a string. The next call to Lexlex() will
 * scan from a @e copy of @a str.
 * @param Lexstr a NUL-terminated string to scan
 *
 * @return the newly allocated buffer state object.
 * @note If you want to scan bytes that may contain NUL values, then use
 *       LexScanBytes() instead.
 */
pScanObject_s LexScanString(const char *pLexStr)
{
	return LexScanBytes(pLexStr, strlen(pLexStr));
}

//////////////////////////////////////////////////////////////////////////////////////

/** Setup the input buffer state to scan the given bytes. The next call to Lexlex() will
 * scan from a copy of bytes.
 * @param bytes the byte buffer to scan
 * @param len the number of bytes in the buffer pointed to by @a bytes.
 *
 * @return the newly allocated buffer state object.
 */
pScanObject_s LexScanBytes(const char *pLexBytes, int Length)
{
pScanObject_s pScanObj;
char *pBuffer;
UINT Len;
int i;

	Len = Length + 2;													/* Get memory for full buffer, including space for trailing EOB's. */
	pBuffer = (char *)malloc(Len);
	if(!pBuffer) LexFatalError("out of dynamic memory in LexScanBytes()");
	for(i = 0; i < Length; ++i) pBuffer[i] = pLexBytes[i];
	pBuffer[Length] = pBuffer[Length + 1] = LEX_END_OF_BUFFER_CHAR;
	pScanObj = CreateScanObject(pBuffer, Len);
	if(!pScanObj) LexFatalError("bad buffer in LexScanBytes()");
	pScanObj->IsOurBuffer = 1;										/* It's okay to grow etc. this buffer, and we should throw it away when we're done. */
	return pScanObj;
}

//////////////////////////////////////////////////////////////////////////////////////

#ifndef LEX_EXIT_FAILURE
#define LEX_EXIT_FAILURE 2
#endif

static inline void LexFatalError(const char* msg)
{
	(void)fprintf( stderr, "%s\n", msg);
	exit(LEX_EXIT_FAILURE);
}

//////////////////////////////////////////////////////////////////////////////////////

/* Redefine Lexless() so it works in section 3 code. */
static inline void Lexless3(int Lexless_macro_arg)
{
	pLexText[LexLength] = LexHoldChar;
	pLexBufferChar = pLexText + Lexless_macro_arg;
	LexHoldChar = *(pLexBufferChar); 
	*(pLexBufferChar) = '\0';
	LexLength = Lexless_macro_arg;
}

//////////////////////////////////////////////////////////////////////////////////////

/* Accessor  methods (get/set functions) to struct members. */

/** Get the current line number. */
int LexGetLineNumber(void)
{
	return LexLineNumber;
}

//////////////////////////////////////////////////////////////////////////////////////

/** Get the input stream. */
FILE *LexGetInput(void)
{
	return pStream;
}

//////////////////////////////////////////////////////////////////////////////////////

/** Get the output stream. */
FILE *LexGetOutput(void)
{
	return pLexOutputStream;
}

//////////////////////////////////////////////////////////////////////////////////////

/** Get the length of the current token. */
int LexGetLength(void)
{
	return LexLength;
}

//////////////////////////////////////////////////////////////////////////////////////

/** Get the current token. */
char *LexGetText(void)
{
	return pLexText;
}

//////////////////////////////////////////////////////////////////////////////////////

/** Set the current line number. */
void LexSetLineNumber(int line_number)
{
	LexLineNumber = line_number;
}

//////////////////////////////////////////////////////////////////////////////////////

/** Set the input stream. This does not discard the current	 */
void LexSetInput(FILE *pStr)
{
	pStream = pStr ;
}

//////////////////////////////////////////////////////////////////////////////////////

void LexSetOutput(FILE *pStr)
{
	pLexOutputStream = pStr ;
}

//////////////////////////////////////////////////////////////////////////////////////

int LexGetDebug(void)
{
	return LexFlexDebug;
}

//////////////////////////////////////////////////////////////////////////////////////

void LexSetDebug(int Debug)
{
	LexFlexDebug = Debug ;
}

//////////////////////////////////////////////////////////////////////////////////////

static int LexInitGlobals(void)
{
	/* Initialization is the same as for the non-reentrant scanner.
	 * This function is called from LexDestroy(), so don't allocate here.
	 */

	ppScanObjStack = 0;
	ScanObjStackTop = 0;
	ScanObjStackMax = 0;
	pLexBufferChar = nullptr;
	LexInitialised = 0;
	LexStart = 0;

/* Defined in main.c */
#ifdef LEX_STDINIT
	pStream = stdin;
	pLexOutputStream = stdout;
#else
	pStream = nullptr;
	pLexOutputStream = nullptr;
#endif
	return 0;			/* For future reference: Set errno on error, since we are called by	Lexlex_init() */
}

//////////////////////////////////////////////////////////////////////////////////////

/* LexDestroy is for both reentrant and non-reentrant scanners. */
int LexDestroy(void)
{
	/* Pop the buffer stack, destroying each element. */
	while(GetLexCurrentBuffer() != nullptr){
		DeleteScanObj(GetLexCurrentBuffer());
		ppScanObjStack[ScanObjStackTop] = nullptr;
		PopScanObj();
	}
	/* Destroy the stack itself. */
	free((char*)ppScanObjStack);
	ppScanObjStack = nullptr;
	/* Reset the globals. This is important in a non-reentrant scanner so the next time
	 * Lexlex() is called, initialization will occur. */
	LexInitGlobals();

	return 0;
}

//////////////////////////////////////////////////////////////////////////////////////

#define LEXTABLES_NAME "Lextables"

int TokenProperty[T_LASTTOKEN];

Token_s *TokenNewCode(const char *pStr)
{
int TokCount, LastTok, ThisTok;
bool UnNumbered = false, SawIf = false, JumpTok = false;
Token_s *pBase = nullptr;
pScanObject_s pLexBuff;

	pCurrentTok = nullptr;
	pLexBuff = LexScanString(pStr);
	MatchData = 0;
	for(LastTok = T_EOL, TokCount = 1; (ThisTok = Lexlex()); ++TokCount){										// determine number of tokens 
		if(TokCount == 1 && ThisTok != T_INTNUM && ThisTok != T_REALNUM){ UnNumbered = true; ++TokCount; }								// no line number so add T_UNNUMBERED token
		if(((LastTok == T_THEN || LastTok == T_ELSE)) && ThisTok == T_INTNUM) ++TokCount;				// insert T_GOTO for line number
		if(ThisTok == T_IF) SawIf = true;																											// If statement should be IF EXP THEN [GOTO INT/LABEL] | [EXP] ...
		if(ThisTok == T_THEN) SawIf = false;																									// [ELSE [GOTO INT/LABEL] | [EXP]] EOF
		if(ThisTok == T_GOTO && SawIf) ++TokCount;																						// insert T_THEN
		if(ThisTok == T_THEN || ThisTok == T_ELSE) JumpTok = true;														// start looking for line Identifier
		if((ThisTok == T_ELSE || ThisTok == T_REM) && LastTok == T_IDENTIFIER && JumpTok){ JumpTok = false; ++TokCount;	}	// insert T_GOTO for line identifier
		if(ThisTok == T_EQ || ThisTok == T_GOTO || ThisTok == T_GOSUB) JumpTok = false;					// T_GOTO already present or part of an expresion
		if((LastTok == T_THEN || LastTok == T_ELSE)  && ThisTok != T_IDENTIFIER && ThisTok != T_INTNUM) JumpTok = false;
		LastTok = ThisTok;
	}	
	if(LastTok == T_IDENTIFIER && JumpTok) ++TokCount;																			// check for T_GOTO on EOL 
	if(TokCount == 1){
		UnNumbered = true;
		++TokCount;
	}	
	pCurrentTok = pBase = (Token_s*)malloc(sizeof(struct Token_s) * TokCount);
	DeleteScanObj(pLexBuff);
	if(UnNumbered){
		pCurrentTok->Type = T_UNNUMBERED;
		++pCurrentTok;
	}
	pLexBuff = LexScanString(pStr);
	LastTok = T_EOL;
	MatchData = 0;
	SawIf = false;
	JumpTok = false;
	while(pCurrentTok->pStatement = nullptr, (pCurrentTok->Type = (TokenType_e)Lexlex())){
		pCurrentTok->Column = pLexText - pLexBuff->pBuffer;
		if(pCurrentTok->Type == T_IF) SawIf = true;
		if(pCurrentTok->Type == T_THEN) SawIf = false;
		if(pCurrentTok->Type == T_THEN || pCurrentTok->Type == T_ELSE) JumpTok = true;					// start looking for line Identifier
		if(pCurrentTok->Type == T_EQ || pCurrentTok->Type == T_GOTO || pCurrentTok->Type == T_GOSUB) JumpTok = false;
		if((LastTok == T_THEN || LastTok == T_ELSE)  && pCurrentTok->Type != T_IDENTIFIER && pCurrentTok->Type != T_INTNUM) JumpTok = false;
		if(pCurrentTok->Type == T_GOTO && SawIf){																								// insert T_THEN
			SawIf = false;
			LastTok = pCurrentTok->Type;
			*(pCurrentTok + 1) = *pCurrentTok;
			pCurrentTok->Type = T_THEN;
			++pCurrentTok;
		}
		else if(((LastTok == T_THEN) || (LastTok == T_ELSE)) && (pCurrentTok->Type == T_INTNUM)){	// insert T_GOTO for line number	 
			LastTok = pCurrentTok->Type;
			*(pCurrentTok + 1) = *pCurrentTok;
			pCurrentTok->Type = T_GOTO;
			pCurrentTok->pStatement = stmt_GOTO_RESUME;
			++pCurrentTok;
		}
		else if((pCurrentTok->Type == T_ELSE  || pCurrentTok->Type == T_REM) && LastTok == T_IDENTIFIER && JumpTok){							// insert T_GOTO for line label
			LastTok = pCurrentTok->Type;
			*(pCurrentTok + 1) = *pCurrentTok;																										// save the current token to the next position
			*pCurrentTok = *(pCurrentTok - 1);																										// move the identifier to this position
			(pCurrentTok - 1)->Type = T_GOTO;																											// insert the T_GOTO in previous positions
			(pCurrentTok - 1)->pStatement = stmt_GOTO_RESUME;
			++pCurrentTok;
		}
		else LastTok = pCurrentTok->Type;
		++pCurrentTok;
	}
	if(LastTok == T_IDENTIFIER && JumpTok){																									// label was last token?
		*pCurrentTok = *(pCurrentTok - 1);																										// move the identifier to the current position
		(pCurrentTok - 1)->Type = T_GOTO;																											// insert the T_GOTO in previous position
		(pCurrentTok - 1)->pStatement = stmt_GOTO_RESUME;																			
		++pCurrentTok;
	}
	pCurrentTok->Type = T_EOL;
	pCurrentTok->pStatement = stmt_COLON_EOL;
	DeleteScanObj(pLexBuff);
	return pBase;
}

//////////////////////////////////////////////////////////////////////////////////////

Token_s *TokeniseData(const char *ln)
{
int l;
Token_s *result;
pScanObject_s buf;

	pCurrentTok = nullptr;
	buf = LexScanString(ln);
	MatchData = 1;
	for(l = 1; Lexlex(); ++l);
	DeleteScanObj(buf);
	pCurrentTok = result = (Token_s*)malloc(sizeof(Token_s) * l);
	buf = LexScanString(ln);
	MatchData = 1;
	while(pCurrentTok->pStatement = nullptr, (pCurrentTok->Type = (TokenType_e)Lexlex())) ++pCurrentTok;
	pCurrentTok->Type = T_EOL;
	pCurrentTok->pStatement = stmt_COLON_EOL;
	DeleteScanObj(buf);
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////

void TokenDestroy(Token_s *pToken)
{
	Token_s *pTok = pToken;
	do {
		switch(pTok->Type){
			case T_AMPERSAND:         break;
			case T_AND:               break;
			case T_AS:                break;
			case T_ASSIGN:						break;
			case T_AXES:							break;
			case T_BEEP:							break;
			case T_BUFFER:            break;
			case T_CALL:              break;
			case T_CASEELSE:
			case T_CASE:			        free(pTok->Obj.pCaseValue); break;
			case T_CAT:								break;
			case T_CATHASH:						break;
			case T_CAT_TO:						break;
			case T_CHANNEL:           break;
			case T_CHDIR:             break;
			case T_CHECK_READ:        break;
			case T_CLEAR:             break;
			case T_CLIP:							break;
			case T_CLOSE:							break;
			case T_CLS:               break;
			case T_COLOUR:            break;
			case T_COLON:             break;
			case T_COMMA:             break;
			case T_COM:               break;
			case T_CON:               break;
			case T_COPY:              break;
			case T_CP:                break;
			case T_CREATE:						break;
			case T_CREATEDIR:					break;
			case T_CSB:               break;
			case T_CSIZE:             break;
			case T_CURSOR:            break;
			case T_DATA:              break;
			case T_DATAINPUT:         free(pTok->Obj.pDataInput); break;
			case T_DEFAULT_OFF:       break;
			case T_DEFAULT_ON:        break;
			case T_DEFFN:             break;
			case T_DEFDBL:            break;
			case T_DEFINT:            break;
			case T_DEFSTR:            break;
			case T_DEG:								break;
			case T_DELAY:							break;
			case T_DELETE:            break;
			case T_DIM:               break;
			case T_DIGITIZE:          break;
			case T_DISABLE:           break;
			case T_DISP:		          break;
			case T_DIV:								break;
			case T_DIVIDE:						break;
			case T_DO:                break;
			case T_DOUNTIL:           break;
			case T_DOWHILE:           break;
			case T_DUMP:              break;
			case T_DRAW:              break;
			case T_ELSE:              break;
			case T_ELSEIFELSE:        break;
			case T_ELSEIFIF:          break;
			case T_ENABLE:            break;
			case T_END:               break;
			case T_ENDIF:             break;
			case T_ENDPROC:           break;
			case T_ENDSELECT:         break;
			case T_EOL:               break;
			case T_EQ:                break;
			case T_EQV:               break;
			case T_EXECUTE:           break;
			case T_EXITDO:            break;
			case T_EXITFOR:           break;
			case T_EXITGRAPH:         break;
			case T_FIXED:							break;
			case T_FLOAT:							break;
			case T_FNEND:             break;
			case T_FOR:               break;
			case T_FRAME:			        break;
			case T_GCHARSET:          break;
			case T_GCLEAR:            break;
			case T_GE:                break;
			case T_GET:               break;
			case T_GLABEL:            break;
			case T_GLOAD:             break;
			case T_GOSUB:             break;
			case T_GOTO:              break;
			case T_GPLOT:             break;
			case T_GPRINT:            break;
			case T_GRAD:              break;
			case T_GRAPHICS:					break;
			case T_GRID:							break;
			case T_GSTORE:            break;
			case T_GT:                break;
			case T_HEXNUM:						break;
			case T_OCTNUM:						break;
			case T_IDENTIFIER:        free(pTok->Obj.pIdentifier); break;
			case T_IDN:               break;
			case T_IF:                break;
			case T_IMAGE:             break;
			case T_IMP:               break;
			case T_INPUT:             break;
			case T_INTEGER:						break;
			case T_INTNUM:						break;
			case T_INV:               break;
			case T_IPLOT:							break;
			case T_IS:                break;
			case T_JUNK:              break;
			case T_KILL:              break;
			case T_LABEL:			        free(pTok->Obj.pLabel->pName); free(pTok->Obj.pLabel); break;
			case T_LDIR:							break;
			case T_LE:                break;
			case T_LET:               break;
			case T_LETTER:		        break;
			case T_LIMIT:							break;
			case T_LIN:	              break;
			case T_LINEINPUT:         break;
			case T_LINETYPE:					break;
			case T_LINK:							break;
			case T_LIST:              break;
			case T_LOAD:              break;
			case T_LOCAL:             break;
			case T_LOCATE:            break;
			case T_LOCK:              break;
			case T_LOOP:              break;
			case T_LOOPUNTIL:         break;
			case T_LORG:							break;
			case T_LPRINT:            break;
			case T_LSET:              break;
			case T_LT:                break;
			case T_MASSTORAGEIS:			break;
			case T_MAT:               break;
			case T_MATINPUT:          break;
			case T_MATPRINT:          break;
			case T_MATREAD:           break;
			case T_MINUS:             break;
			case T_MKDIR:             break;
			case T_MOD:               break;
			case T_MOVE:		          break;
			case T_MSCALE:            break;
			case T_MULT:              break;
			case T_NAME:              break;
			case T_NE:                break;
			case T_NEXT:              free(pTok->Obj.pNext); break;
			case T_NORMAL:            break;
			case T_NOT:               break;
			case T_OFFKEY:		        break;
			case T_ON:                if(pTok->Obj.On.pPc) free(pTok->Obj.On.pPc);break;
			case T_ONERROR:           break;
			case T_ONERROROFF:        break;
			case T_ONKEY:             break;
			case T_OP:                break;
			case T_OPEN:              break;
			case T_OPTIONBASE:        break;
			case T_OR:                break;
			case T_OSB:               break;
			case T_OUT:								break;
			case T_OVERLAP:						break;
			case T_PAGE:              break;
			case T_PAUSE:             break;
			case T_PDIR:			        break;
			case T_PEN:								break;
			case T_PENUP:							break;
			case T_PLOT:							break;
			case T_PLOTTERIS:         break;
			case T_PLUS:							break;
			case T_POINTER:           break;
			case T_POW:               break;
			case T_PRINT:             break;
			case T_PRINTALLIS:        break;
			case T_PRINTERIS:         break;
			case T_RAD:	              break;
			case T_RANDOMIZE:         break;
			case T_RATIO:             break;
			case T_READ:              break;
			case T_REDIM:							break;
			case T_REAL:							break;
			case T_REALNUM:           break;
			case T_REM:               free(pTok->Obj.pRem); break;
			case T_RENAME:            break;
			case T_RENUM:             break;
			case T_REPEAT:            break;
			case T_RESTORE:           break;
			case T_RESULT:            break;
			case T_RESUME:            break;
			case T_RETURN:            break;
			case T_RPLOT:							break;
			case T_RSET:              break;
			case T_RUN:               break;
			case T_SAVE:              break;
			case T_SCALE:             break;
			case T_SCRATCH:           break;
			case T_SCRATCH_A:         break;
			case T_SCRATCH_C:         break;
			case T_SCRATCH_K:         break;
			case T_SCRATCH_P:         break;
			case T_SCRATCH_V:         break;
			case T_SELECT:		        free(pTok->Obj.pSelectCase); break;
			case T_SERIAL:						break;
			case T_SEMICOLON:         break;
			case T_SHORT:             break;
			case T_SHOW:		          break;
			case T_SPA:               break;
			case T_STANDARD:          break;
			case T_STEP:              break;
			case T_STOP:              break;
			case T_STORE:             break;
			case T_STRING:            delete pTok->Obj.pString; break;
			case T_SUB:               break;
			case T_SUBEND:            break;
			case T_SUBEXIT:           break;
			case T_SYSTEM:            break;
			case T_TAB:               break;
			case T_THEN:              break;
			case T_TO:                break;
			case T_TRN:               break;
			case T_TRACE:             break;
			case T_TRALL:             break;
			case T_TRPAUSE:           break;
			case T_TRVARS:            break;
			case T_TRALLVARS:         break;
			case T_TRWAIT:            break;
			case T_TRUNCATE:          break;
			case T_UNCLIP :           break;
			case T_UNLOCK:            break;
			case T_UNNUM:             break;
			case T_UNNUMBERED:        break;
			case T_UNTIL:             break;
			case T_USING:             break;
			case T_WAIT:              break;
			case T_WEND:              free(pTok->Obj.pWhilePc); break;
			case T_WHERE:             break;
			case T_WHILE:             free(pTok->Obj.pAfterWendPc); break;
			case T_WIDTH:             break;
			case T_WRITE:             break;
			case T_XOR:               break;
			case T_ZER:               break;
			default:                  assert(0);
		}
	}
	while((pTok++)->Type != T_EOL);
	free(pToken);
}

//////////////////////////////////////////////////////////////////////////////////////

BString *TokenToString(Token_s *pToken, Token_s *pSpaceTo, BString *pDestStr, int *pIndent, int Width)
{
int ns = 0, infn = 0;
int ThisIndent = 0, ThisNotIndent = 0, NextIndent = 0;
UINT OldLength = pDestStr->GetLength();
Token_s *pTok;

static struct
{
	const char *pText;
	char Space;
}	table[] =	{
	/* 0                    */ {nullptr, -1},
	/* T_AMPERSAND          */ {"&", 0},
	/* T_AND                */ {"AND", 1},
	/* T_AS                 */ {"AS", 1},
	/* T_ASSIGN							*/ {"ASSIGN", 1},
	/* T_AXES								*/ {"AXES", 1},
	/* T_BEEP								*/ {"BEEP", 1},
	/* T_BUFFER							*/ {"BUFFER", 1},
	/* T_CALL               */ {"CALL", 1},
	/* T_CASEELSE           */ {"CASE ELSE", 1},
	/* T_CASE			          */ {"CASE", 1},
	/* T_CAT								*/ {"CAT", 1},
	/* T_CATHASH						*/ {"CAT#", 1},
	/* T_CAT_TO							*/ {"CAT TO", 1},
	/* T_CHANNEL            */ {"#", 0},
	/* T_CHDIR              */ {"CHDIR", 1},
	/* T_CHECK_READ					*/ {"CHECK READ", 1},
	/* T_CLEAR              */ {"CLEAR", 1},
	/* T_CLIP								*/ {"CLIP", 1},
	/* T_CLOSE              */ {"CLOSE", 1},
	/* T_CLS                */ {"CLS", 1},
	/* T_COLON              */ {":", 0},
	/* T_COLOUR             */ {"COLOUR", 1},
	/* T_COMMA              */ {",", 0},
	/* T_COM                */ {"COM", 0},
	/* T_CON                */ {"CON", 0},
	/* T_COPY               */ {"COPY", 1},
	/* T_CP                 */ {")", 0},
	/* T_CREATE							*/ {"CREATE", 1},
	/* T_CREATEDIR					*/ {"CREATDIR", 1},
	/* T_CSB                */ {"]", 0},
	/* T_CSIZE							*/ {"CSIZE", 1},
	/* T_CURSOR							*/ {"CURSOR", 1},
	/* T_DATA               */ {"DATA", 1},
	/* T_DATAINPUT          */ {(const char*)0, 0},
	/* T_DEFAULT_OFF				*/ {"DEFAULT OFF", 1},
	/* T_DEFAULT_ON					*/ {"DEFAULT ON", 1},
	/* T_DEFDBL             */ {"DEFDBL", 1},
	/* T_DEFFN              */ {"DEF", 1},
	/* T_DEFINT             */ {"DEFINT", 1},
	/* T_DEFSTR             */ {"DEFSTR", 1},
	/* T_DEG		            */ {"DEG", 1},
	/* T_DELAY							*/ {"DELAY", 1},
	/* T_DELETE             */ {"DELETE", 1},
	/* T_DIM                */ {"DIM", 1},
	/* T_DIGITIZE						*/ {"DIGITIZE", 1},
	/* T_DISABLE            */ {"DISABLE", 1},
	/* T_DISP		            */ {"DISP", 1},
	/* T_DIV                */ {"DIV", 0},
	/* T_DIVIDE             */ {"/", 0},
	/* T_DO                 */ {"DO", 1},
	/* T_DOUNTIL            */ {"DO UNTIL", 1},
	/* T_DOWHILE            */ {"DO WHILE", 1},
	/* T_DRAW               */ {"DRAW", 1},
	/* T_DUMP               */ {"DUMP", 1},
	/* T_ELSE               */ {"ELSE", 1},
	/* T_ELSEIFELSE         */ {"ELSEIF", 1},
	/* T_ELSEIFIF           */ {(const char*)0, 0},
	/* T_ENABLE             */ {"ENABLE", 1},
	/* T_END                */ {"END", 1},
	/* T_ENDIF              */ {"END IF", 1},
	/* T_ENDPROC            */ {"END PROC", 1},
	/* T_ENDSELECT          */ {"END SELECT", 1},
	/* T_EOL                */ {"", 0},
	/* T_EQ                 */ {"=", 0},
	/* T_EQV                */ {"EQV", 0},
	/* T_EXECUTE            */ {"EXECUTE", 1},
	/* T_EXITDO             */ {"EXIT DO", 1},
	/* T_EXITFOR            */ {"EXIT FOR", 1},
	/* T_EXITGRAPH					*/ {"EXIT GRAPHICS", 1},
	/* T_FIXED							*/ {"FIXED", 1},
	/* T_FLOAT							*/ {"FLOAT", 1},
	/* T_FNEND              */ {"FNEND", 1},
	/* T_FOR                */ {"FOR", 1},
	/* T_FRAME	            */ {"FRAME", 1},
	/* T_GCHARSET           */ {"GCHARSET", 1},
	/* T_GCLEAR             */ {"GCLEAR", 1},
	/* T_GE                 */ {">=", 0},
	/* T_GET                */ {"GET", 1},
	/* T_GLABEL             */ {"LABEL", 1},
	/* T_GLOAD							*/ {"GLOAD", 1},
	/* T_GOSUB              */ {"GOSUB", 1},
	/* T_GOTO               */ {"GOTO", 1},
	/* T_GPLOT							*/ {"GPLOT", 1},
	/* T_GPRINT							*/ {"GPRINT", 1},
	/* T_GRAD               */ {"GRAD", 1},
	/* T_GRAPHICS           */ {"GRAPHICS", 1},
	/* T_GRID								*/ {"GRID", 1},
	/* T_GSTORE							*/ {"GSTORE", 1},
	/* T_GT                 */ {">", 0},
	/* T_HEXNUM							*/ {nullptr, 0},
	/* T_OCTNUM							*/ {nullptr, 0},
	/* T_IDENTIFIER         */ {nullptr, 0},
	/* T_IDN                */ {"IDN", 0},
	/* T_IF                 */ {"IF", 1},
	/* T_IMAGE              */ {"IMAGE", 1},
	/* T_IMP                */ {"IMP", 0},
	/* T_INPUT              */ {"INPUT", 1},
	/* T_INTEGER            */ {"INTEGER", 1},
	/* T_INTNUM							*/ {nullptr, 0},
	/* T_INV                */ {"INV", 0},
	/* T_IPLOT              */ {"IPLOT", 1},
	/* T_IS                 */ {"IS", 1},
	/* T_JUNK               */ {nullptr, 0},
	/* T_KILL               */ {"KILL", 1},
  /* T_LABEL		          */ {(const char*)0,0},
	/* T_LDIR								*/ {"LDIR", 1},
	/* T_LE                 */ {"<=", 0},
	/* T_LET                */ {"LET", 1},
	/* T_LETTER             */ {"LETTER", 1},
	/* T_LIMIT              */ {"LIMIT", 1},
	/* T_LIN                */ {"LIN", 0},
	/* T_LINEINPUT          */ {"LINPUT", 1},
	/* T_LINETYPE           */ {"LINETYPE", 1},
	/* T_LINK								*/ {"LINK", 1},
	/* T_LIST               */ {"LIST", 1},
	/* T_LOAD               */ {"LOAD", 1},
	/* T_LOCAL              */ {"LOCAL", 1},
	/* T_LOCATE             */ {"LOCATE", 1},
	/* T_LOCK               */ {"LOCK", 1},
	/* T_LOOP               */ {"LOOP", 1},
	/* T_LOOPUNTIL          */ {"LOOP UNTIL", 1},
	/* T_LORG								*/ {"LORG", 1},
	/* T_LPRINT             */ {"LPRINT", 1},
	/* T_LSET               */ {"LSET", 1},
	/* T_LT                 */ {"<", 0},
	/* T_MASSTORAGEIS       */ {"MASS STORAGE IS", 1},
	/* T_MAT                */ {"MAT", 1},
	/* T_MATINPUT           */ {"MAT INPUT", 1},
	/* T_MATPRINT           */ {"MAT PRINT", 1},
	/* T_MATREAD            */ {"MAT READ", 1},
	/* T_MINUS              */ {"-", 0},
	/* T_MKDIR              */ {"MKDIR", 1},
	/* T_MOD                */ {"MOD", 0},
	/* T_MOVE								*/ {"MOVE", 1},
	/* T_MSCALE							*/ {"MSCALE", 1},
	/* T_MULT               */ {"*", 0},
	/* T_NAME               */ {"NAME", 1},
	/* T_NE                 */ {"<>", 0},
	/* T_NEXT               */ {"NEXT", 1},
	/* T_NORMAL             */ {"NORMAL", 0},
	/* T_NOT                */ {"NOT", 0},
	/* T_OFFKEY		          */ {"OFF KEY", 1},
	/* T_ON                 */ {"ON", 1},
	/* T_ONERROR            */ {"ON ERROR", 1},
	/* T_ONERROROFF         */ {"ON ERROR OFF", 1},
	/* T_ONKEY              */ {"ON KEY", 1},
	/* T_OP                 */ {"(", 0},
	/* T_OPEN               */ {"OPEN", 1},
	/* T_OPTIONBASE         */ {"OPTION BASE", 1},
	/* T_OR                 */ {"OR", 1},
	/* T_OSB                */ {"[", 0},
	/* T_OUT                */ {"OUT", 1},
	/* T_OVERLAP            */ {"OVERLAP", 1},
	/* T_PAGE               */ {"PAGE", 0},
	/* T_PAUSE              */ {"PAUSE", 1},
	/* T_PDIR		            */ {"PDIR", 1},
	/* T_PEN                */ {"PEN", 1},
	/* T_PENUP              */ {"PENUP", 1},
	/* T_PLOT               */ {"PLOT", 1},
	/* T_PLOTTERIS          */ {"PLOTTER IS", 1},
	/* T_PLUS               */ {"+", 0},
	/* T_POINTER						*/ {"POINTER", 1},
	/* T_POW                */ {"^", 0},
	/* T_PRINT              */ {"PRINT", 1},
	/* T_PRINTALLIS         */ {"PRINTALL IS", 1},
	/* T_PRINTERIS          */ {"PRINTER IS", 1},
	/* T_RAD	              */ {"RAD", 1},
	/* T_RANDOMIZE          */ {"RANDOMIZE", 1},
	/* T_RATIO							*/ {"RATIO", 1},
	/* T_READ               */ {"READ", 1},
	/* T_REAL								*/ {"REAL", 1},
	/* T_REALNUM            */ {nullptr, 0},
	/* T_REDIM							*/ {"REDIM", 1},
	/* T_REM                */ {nullptr, 1},
	/* T_RENAME             */ {"RENAME", 1},
	/* T_RENUM              */ {"RENUM", 1},
	/* T_REPEAT             */ {"REPEAT", 1},
	/* T_RESTORE            */ {"RESTORE", 1},
	/* T_RESULT             */ {"RESULT", 1},
	/* T_RESUME             */ {"RESUME", 1},
	/* T_RETURN             */ {"RETURN", 1},
	/* T_RPLOT              */ {"RPLOT", 1},
	/* T_RSET               */ {"RSET", 1},
	/* T_RUN                */ {"RUN", 1},
	/* T_SAVE               */ {"SAVE", 1},
	/* T_SCALE              */ {"SCALE", 1},
	/* T_SCRATCH            */ {"SCRATCH", 1},
	/* T_SCRATCH_A          */ {"SCRATCH A", 1},
	/* T_SCRATCH_C          */ {"SCRATCH C", 1},
	/* T_SCRATCH_K          */ {"SCRATCH KEY", 1},
	/* T_SCRATCH_P          */ {"SCRATCH P", 1},
	/* T_SCRATCH_V          */ {"SCRATCH V", 1},
	/* T_SELECT		          */ {"SELECT", 1},
	/* T_SERAIL             */ {"SERIAL", 1},
	/* T_SEMICOLON          */ {";", 0},
	/* T_SHORT              */ {"SHORT", 1},
	/* T_SHOW		            */ {"SHOW", 1},
	/* T_SPA                */ {"SPC", 0},
	/* T_STANDARD           */ {"STANDARD", 0},
	/* T_STEP               */ {"STEP", 1},
	/* T_STOP               */ {"STOP", 1},
	/* T_STORE              */ {"STORE", 1},
	/* T_STRING             */ {nullptr, 0},
	/* T_SUB                */ {"SUB", 1},
	/* T_SUBEND             */ {"SUBEND", 1},
	/* T_SUBEXIT            */ {"SUBEXIT", 1},
	/* T_SYSTEM             */ {"SYSTEM", 1},
	/* T_TAB                */ {"TAB", 0},
	/* T_THEN               */ {"THEN", 1},
	/* T_TO                 */ {"TO", 1},
	/* T_TRN                */ {"TRN", 0},
	/* T_TRACE              */ {"TRACE", 1},
	/* T_TRALL              */ {"TRACE ALL", 1},
	/* T_TRPAUSE            */ {"TRACE PAUSE", 1},
	/* T_TRVARS             */ {"TRACE VARIABLES", 1},
	/* T_TRALLVARS          */ {"TRACE ALL VARIABLES", 1},
	/* T_TRWAIT             */ {"TRACE WAIT", 1},
	/* T_TRUNCATE           */ {"TRUNCATE", 1},
	/* T_UNCLIP							*/ {"UNCLIP", 1},
	/* T_UNLOCK             */ {"UNLOCK", 1},
	/* T_UNNUM              */ {"UNNUM", 1},
	/* T_UNNUMBERED         */ {"", 0},
	/* T_UNTIL              */ {"UNTIL", 1},
	/* T_USING              */ {"USING", 0},
	/* T_WAIT               */ {"WAIT", 1},
	/* T_WEND               */ {"END WHILE", 1},
	/* T_WHERE              */ {"WHERE", 1},
	/* T_WHILE              */ {"WHILE", 1},
	/* T_WIDTH              */ {"WIDTH", 1},
	/* T_WRITE              */ {"WRITE", 1},
	/* T_XOR                */ {"XOR", 0},
	/* T_ZER                */ {"ZER", 0},
};

	/* precompute indentation */
	if(pIndent) ThisIndent = NextIndent = *pIndent;
	pTok = pToken;
	do {
		switch(pTok->Type){
			case T_CASEELSE:
			case T_CASE:{
				if(ThisNotIndent) --ThisNotIndent;
				else if(ThisIndent) --ThisIndent;
				break;
			}
			case T_DEFFN:{
				Token_s *cp;
				for(cp = pTok; cp->Type != T_EOL && cp->Type != T_CP; ++cp) ;
				if((cp + 1)->Type != T_EQ){
					++ThisNotIndent;
					++NextIndent;
				}
				infn = 1;
				break;
			}
			case T_DO:
			case T_DOUNTIL:
			case T_DOWHILE:
			case T_REPEAT:
			case T_SUB:
			case T_WHILE:{
				++ThisNotIndent;
				++NextIndent;
				break;
			}
			case T_FOR:{
				if((pTok>pToken && ((pTok - 1)->Type == T_INTNUM || (pTok - 1)->Type == T_UNNUMBERED))){
					++ThisNotIndent; ++NextIndent;
				}
				break;
			}
			case T_SELECT:{
				ThisNotIndent += 2;
				NextIndent += 2;
				break;
			}
			case T_EQ:{
				if(infn || (pTok>pToken && ((pTok - 1)->Type == T_INTNUM || (pTok - 1)->Type == T_UNNUMBERED))){
					if(ThisNotIndent) --ThisNotIndent;
					else if(ThisIndent) --ThisIndent;
					if(NextIndent) --NextIndent;
				}
				infn = 0;
				break;
			}
			case T_FNEND:
			case T_ENDIF:
			case T_ENDPROC:
			case T_SUBEND:
			case T_LOOP:
			case T_LOOPUNTIL:
			case T_UNTIL:
			case T_WEND:{
				if(ThisNotIndent) --ThisNotIndent;
				else if(ThisIndent) --ThisIndent;
				if(NextIndent) --NextIndent;
				break;
			}
			case T_ENDSELECT:{
				if(ThisNotIndent) --ThisNotIndent;
				else if(ThisIndent) --ThisIndent;
				if(ThisNotIndent) --ThisNotIndent;
				else if(ThisIndent) --ThisIndent;
				if(NextIndent) --NextIndent;
				if(NextIndent) --NextIndent;
				break;
			}
			case T_NEXT:{
				++pTok;
				while(1){
					if(ThisNotIndent) --ThisNotIndent;
					else if(ThisIndent) --ThisIndent;
					if(NextIndent) --NextIndent;
					if(pTok->Type == T_IDENTIFIER){
						int par = 0;
						++pTok;
						if(pTok->Type == T_OP){
							do{
								if(pTok->Type == T_OP) ++par;
								else if(pTok->Type == T_CP) --par;
								if(pTok->Type != T_EOL) ++pTok;
								else break;
							}
							while(par);
						}
						if(pTok->Type == T_COMMA) ++pTok;
						else break;
					}
					else break;
				}
				break;
			}
			case T_THEN:{
				if(((pTok + 1)->Type == T_EOL) || ((pTok + 1)->Type == T_REM)){
					++ThisNotIndent;
					++NextIndent;
				}
				break;
			}
			case T_ELSE:{
				if(pTok == pToken + 1){
					if(ThisNotIndent) --ThisNotIndent;
					else if(ThisIndent) --ThisIndent;
				}
				break;
			}
			case T_ELSEIFELSE:{
				if(pTok == pToken + 1){
					if(ThisNotIndent) --ThisNotIndent;
					else if(ThisIndent) --ThisIndent;
				}
				if(NextIndent) --NextIndent;
				break;
			}
			default:{
				break;
			}
		}
	}
	while(pTok++->Type != T_EOL);
	if(Width >= 0){ 																								/* whole line */
		if(Width){ 																										/* nicely formatted listing */
			assert((pToken->Type == T_UNNUMBERED) || (pToken->Type == T_INTNUM));
			if(pToken->Type == T_INTNUM) pDestStr->AppendPrintf("%*ld ", Width, pToken->Obj.Integer);
			else pDestStr->AppendPrintf("%*s ", Width, "");
		}
		else assert(pToken->Type == T_UNNUMBERED);
		++pToken;
	}
	while(ThisIndent--) pDestStr->AppendPrintf("  ");
	do{
		if((pDestStr->GetLength() > OldLength) && (pToken->Type != T_EOL)){
			const char *pKeyword;
			if((pKeyword = table[pToken->Type].pText) == nullptr) pKeyword = "X";
			if(ns && pDestStr->GetAt(pDestStr->GetLength() - 1) != ' ') pDestStr->AppendPrintf(" ");
			else{
				if(isalnum((int)(pDestStr->GetAt(pDestStr->GetLength() - 1))) && isalnum((int)*pKeyword)) pDestStr->AppendPrintf(" ");
				else{
					if(pDestStr->GetAt(pDestStr->GetLength() - 1) != ' ' && table[pToken->Type].Space) pDestStr->AppendChar(' ');
				}
			}
		}
		if(pSpaceTo && pToken == pSpaceTo) break;
		switch(pToken->Type){
			case T_DATAINPUT:{
				pDestStr->AppendChars(pToken->Obj.pDataInput);
				break;
			}
			case T_ELSEIFIF:{
				break;
			}
			case T_IDENTIFIER:{
				pDestStr->AppendChars(pToken->Obj.pIdentifier->Name);
				break;
			}
			case T_LABEL:{
				pDestStr->AppendChars(pToken->Obj.pLabel->pName);
				pDestStr->AppendChar(':');
				break;
			}
			case T_INTNUM:{
				pDestStr->AppendPrintf("%ld", pToken->Obj.Integer);
				break;
			}
			case T_HEXNUM:{
				pDestStr->AppendPrintf("&h%lx", pToken->Obj.HexInteger);
				break;
			}
			case T_OCTNUM:{
				pDestStr->AppendPrintf("&o%lo", pToken->Obj.OctInteger);
				break;
			}
			case T_JUNK:{
				pDestStr->AppendChar(pToken->Obj.Junk);
				break;
			}
			case T_REALNUM:{
				pDestStr->AppendPrintf("%.*g", DBL_DIG, pToken->Obj.Real);
				if((pToken->Obj.Real < ((double)LONG_MIN)) || (pToken->Obj.Real > ((double)LONG_MAX))) pDestStr->AppendChar('!');
				break;
			}
			case T_REM:{
				pDestStr->PadToColumn(pToken->Column);
				pDestStr->AppendPrintf("!%s", pToken->Obj.pRem);
				break;
			}
			case T_STRING:{
				UINT l = pToken->Obj.pString->GetLength();
				char *data = pToken->Obj.pString->GetBuffer();
				pDestStr->AppendPrintf("\"");
				while(l--){
					if(*data == '"') pDestStr->AppendPrintf("\"");
					pDestStr->AppendPrintf("%c", *data);
					++data;
				}
				pDestStr->AppendPrintf("\"");
				break;
			}
			default:{
				pDestStr->AppendChars(table[pToken->Type].pText);
			}
		}
		ns = table[pToken->Type].Space;
	}
	while((pToken++)->Type != T_EOL);
	if(pIndent) *pIndent = NextIndent;
//	if(pSpaceTo && pDestStr->Length > OldLength) memset(pDestStr->pBuffer + OldLength, ' ', pDestStr->Length - OldLength);
	return pDestStr;
}

//////////////////////////////////////////////////////////////////////////////////////

void TokenInit(void)
{
#define PROPERTY(Token, assoc, unary_priority, binary_priority, is_unary, is_binary) \
	TokenProperty[Token] = (assoc << 8) | (unary_priority << 5) | (binary_priority << 2) | (is_unary << 1) | is_binary

	PROPERTY(T_POW,				1, 0, 7, 0, 1);
	PROPERTY(T_MULT,			0, 0, 5, 0, 1);
	PROPERTY(T_DIV,				0, 0, 5, 0, 1);
	PROPERTY(T_DIVIDE,		0, 0, 5, 0, 1);
	PROPERTY(T_MOD,				0, 0, 5, 0, 1);
	PROPERTY(T_PLUS,			0, 6, 4, 1, 1);
	PROPERTY(T_MINUS,			0, 6, 4, 1, 1);
	PROPERTY(T_LT,				0, 0, 3, 0, 1);
	PROPERTY(T_LE,				0, 0, 3, 0, 1);
	PROPERTY(T_EQ,				0, 0, 3, 0, 1);
	PROPERTY(T_GE,				0, 0, 3, 0, 1);
	PROPERTY(T_GT,				0, 0, 3, 0, 1);
	PROPERTY(T_NE,				0, 0, 3, 0, 1);
	PROPERTY(T_NOT,				0, 2, 0, 1, 0);
	PROPERTY(T_AND,				0, 0, 1, 0, 1);
	PROPERTY(T_AMPERSAND,	0, 0, 1, 0, 1);
	PROPERTY(T_OR,				0, 0, 0, 0, 1);
	PROPERTY(T_XOR,				0, 0, 0, 0, 1);
	PROPERTY(T_EQV,				0, 0, 0, 0, 1);
	PROPERTY(T_IMP,				0, 0, 0, 0, 1);
}


