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

/////////////////////////////////////////////////////////////////////////////

constexpr auto BASERR = 0;

//		Definition name,  Error number,  S45 error code,  Error message

#define ALREADYDECLARED    BASERR + 0, 8,		 _T("Formal parameter already declared")
#define ALREADYLOCAL       BASERR + 1, 12,	 _T("Variable already declared as LOCAL")
#define BADIDENTIFIER      BASERR + 2, 0,    _T("Identifier can not be declared as %s")
#define BADRANGE           BASERR + 3, 0,    _T("Ranges must be constructed from single letter identifiers")
#define INVALIDLINE        BASERR + 4, 0,    _T("Missing line number at the beginning of text line %d")
#define INVALIDUOPERAND    BASERR + 5, 0,    _T("Invalid unary operand")
#define INVALIDOPERAND     BASERR + 6, 0,    _T("Invalid binary operand")
#define MISSINGAS          BASERR + 7, 0,    _T("Missing AS")
#define MISSINGCOLON       BASERR + 8, 0,    _T("Missing colon ':'")
#define MISSINGCOMMA       BASERR + 9, 0,    _T("Missing comma ','")
#define MISSINGCP          BASERR + 10, 0,   _T("Missing right parenthesis ')'")
#define MISSINGDATAINPUT   BASERR + 11, 0,   _T("Missing DATA input")
#define IMPROPERCHRARG		 BASERR + 12, 33,  _T("Improper %s argument")
#define MISSINGEQ          BASERR + 13, 0,   _T("Missing equal sign '='")
#define MISSINGEXPR        BASERR + 14, 0,   _T("Expected %s expression")
#define MISSINGFILE        BASERR + 15, 0,   _T("Missing FILE")
#define MISSINGGOTOSUB     BASERR + 16, 0,   _T("Missing GOTO or GOSUB")
#define MISSINGVARIDENT    BASERR + 17, 0,   _T("Missing variable identifier")
#define MISSINGPROCIDENT   BASERR + 18, 0,   _T("Missing procedure identifier")
#define MISSINGFUNCIDENT   BASERR + 19, 0,   _T("Missing function identifier")
#define MISSINGARRIDENT    BASERR + 20, 13,  _T("Missing array variable identifier")
#define MISSINGSTRIDENT    BASERR + 21, 0,   _T("Missing string variable identifier")
#define MISSINGLOOPIDENT   BASERR + 22, 0,   _T("Missing loop variable identifier")
#define MISSINGFORMIDENT   BASERR + 23, 0,   _T("Missing formal parameter identifier")
#define MISSINGREADIDENT   BASERR + 24, 0,   _T("Missing READ variable identifier")
#define MISSINGSWAPIDENT   BASERR + 25, 0,   _T("Missing SWAP variable identifier")
#define MISSINGMATIDENT    BASERR + 26, 13,  _T("Missing matrix variable identifier")
#define MISSINGINCREMENT   BASERR + 27, 0,   _T("Missing line increment")
#define MISSINGLEN         BASERR + 28, 0,   _T("Missing LEN")
#define MISSINGLINENUMBER  BASERR + 29, 0,   _T("Missing line number")
#define MISSINGOP          BASERR + 30, 0,   _T("Missing left parenthesis `('")
#define MISSINGSEMICOLON   BASERR + 31, 0,   _T("Missing semicolon ';'")
#define MISSINGSEMICOMMA   BASERR + 32, 0,   _T("Missing semicolon ';' or comma ','")
#define MISSINGMULT        BASERR + 33, 0,   _T("Missing star '*'")
#define MISSINGSTATEMENT   BASERR + 34, 0,   _T("Missing statement")
#define MISSINGTHEN        BASERR + 35, 0,   _T("Missing THEN")
#define MISSINGTO          BASERR + 36, 0,   _T("Missing TO")
#define NESTEDDEFINITION   BASERR + 37, 48,  _T("Nested definition")
#define NOPROGRAM          BASERR + 38, 0,   _T("No programme")
#define NOSUCHDATALINE     BASERR + 39, 0,   _T("No such DATA line")
#define NOSUCHLINE         BASERR + 40, 3,   _T("No such line")
#define REDECLARATION      BASERR + 41, 12,  _T("Redeclaration as different kind of symbol")
#define STRAYCASE          BASERR + 42, 0,   _T("CASE without SELECT CASE")
#define STRAYDO            BASERR + 43, 0,   _T("DO without LOOP")
#define STRAYDOcondition   BASERR + 44, 0,   _T("DO WHILE or DO UNTIL without LOOP")
#define STRAYELSE1         BASERR + 45, 0,   _T("ELSE without IF")
#define STRAYELSE2         BASERR + 46, 0,   _T("ELSE without END IF")
#define STRAYENDIF         BASERR + 47, 0,   _T("END IF without multiline IF or ELSE")
#define STRAYSUBEND        BASERR + 49, 0,   _T("SUBEND or ENDPROC without SUB or DEF PROC inside %s")
#define STRAYSUBEXIT       BASERR + 50, 0,   _T("SUBEXIT without SUB inside %s")
#define STRAYENDSELECT     BASERR + 51, 0,   _T("END SELECT without SELECT CASE")
#define STRAYENDEQ         BASERR + 53, 4,   _T("'=' returning from function without DEF FN")
#define STRAYEXITDO        BASERR + 54, 0,   _T("EXIT DO without DO")
#define STRAYEXITFOR       BASERR + 55, 0,   _T("EXIT FOR without FOR")
#define STRAYFNEND         BASERR + 56, 0,   _T("FNEND without DEF FN")
#define STRAYFNEXIT        BASERR + 57, 0,   _T("FNEXIT outside function declaration")
#define STRAYFNRETURN      BASERR + 58, 0,   _T("RETURN without DEF FN")
#define STRAYFOR           BASERR + 59, 6,   _T("FOR without NEXT")
#define STRAYFUNC          BASERR + 60, 5,   _T("Function/procedure declaration without END")
#define STRAYIF            BASERR + 61, 0,   _T("IF without END IF")
#define STRAYLOCAL         BASERR + 62, 0,   _T("LOCAL without DEF FN or DEF PROC")
#define STRAYLOOP          BASERR + 63, 0,   _T("LOOP without DO")
#define STRAYLOOPUNTIL     BASERR + 64, 0,   _T("LOOP UNIT without DO")
#define STRAYNEXT          BASERR + 65, 6,   _T("NEXT without FOR inside %s")
#define STRAYREPEAT        BASERR + 66, 0,   _T("REPEAT without UNTIL")
#define STRAYSELECTCASE    BASERR + 67, 0,   _T("SELECT CASE without END SELECT")
#define STRAYUNTIL         BASERR + 68, 0,   _T("UNTIL without REPEAT")
#define STRAYWEND          BASERR + 69, 0,   _T("WEND without WHILE inside %s")
#define STRAYWHILE         BASERR + 70, 0,   _T("WHILE without WEND")
#define SYNTAX             BASERR + 71, 0,   _T("Syntax")
#define TOOFEW             BASERR + 72, 9,   _T("Too few parameters")
#define TOOMANY            BASERR + 73, 9,   _T("Too many parameters")
#define TYPEMISMATCH1      BASERR + 74, 8,   _T("Type mismatch (has %s, need %s)")
#define TYPEMISMATCH2      BASERR + 75, 8,   _T("Type mismatch of argument %d")
#define TYPEMISMATCH3      BASERR + 76, 0,   _T("%s of argument %d")
#define TYPEMISMATCH4      BASERR + 77, 10,  _T("Type mismatch (need string variable)")
#define TYPEMISMATCH5      BASERR + 78, 11,  _T("Type mismatch (need numeric variable)")
#define TYPEMISMATCH6      BASERR + 79, 11,  _T("Type mismatch (need numeric value)")
#define UNDECLARED         BASERR + 80, 7,   _T("Undeclared function or variable")
#define UNNUMBERED         BASERR + 81, 0,   _T("Use RENUM to number program first")
#define OUTOFSCOPE         BASERR + 82, 3,   _T("Line out of scope")
#define VOIDVALUE          BASERR + 83, 0,   _T("Sub-programme returning a value")
#define UNREACHABLE        BASERR + 84, 0,   _T("Unreachable statement")
#define WRONGMODE          BASERR + 85, 0,   _T("Wrong access mode")
#define FORMISMATCH        BASERR + 86, 6,   _T("NEXT variable does not match FOR variable")
#define NOSUCHIMAGELINE    BASERR + 87, 34,  _T("No such IMAGE line")
#define MISSINGFMT         BASERR + 88, 35,  _T("Missing IMAGE format")
#define MISSINGRELOP       BASERR + 89, 0,   _T("Missing relational operator")
#define DUPLICATION		     BASERR + 90, 0,   _T("Duplication of symbol")
#define MISSINGIDENTIFIER  BASERR + 91, 0,   _T("Missing identifier")
#define IMPROPERVALTYPE		 BASERR + 92, 19,  _T("Improper value type")
#define MISSINGOSB         BASERR + 93, 0,   _T("Missing left bracket '['")
#define MISSINGCSB         BASERR + 94, 0,   _T("Missing right bracket ']'")
#define SUBSCRIPTVALUE     BASERR + 95, 17,  _T("Improper subscript value")
#define INVALIDDEVICE      BASERR + 96, 0,   _T("Invalid device specifier")
#define INVALIDVALUE       BASERR + 97, 19,  _T("Invalid variable")
#define STRAYSPROG         BASERR + 98, 5,   _T("Sub-programme declaration without end")
#define INVALIDMATDIM      BASERR + 99, 16,  _T("Invalid matrix dimension %d")
#define PASSINGPARAMETERS  BASERR + 100, 0,  _T("Passing parameters to an event sub-programme")
#define MISSINGINPUTDATA   BASERR + 101, 36, _T("Missing READ data")
#define MISSINGCHARACTER   BASERR + 102, 0,  _T("Missing character after underscore `_' in format string")
#define NOTINDIRECTMODE    BASERR + 103, 0,  _T("Not allowed in interactive mode")
#define NOTINPROGRAMMODE   BASERR + 104, 0,  _T("Not allowed in program mode")
#define UNDEFINED          BASERR + 105, 0,  _T("%s is undefined")
#define OUTOFRANGE         BASERR + 106, 0,  _T("%s is out of range")
#define STRAYRESUME        BASERR + 107, 0,  _T("RESUME without exception")
#define STRAYRETURN        BASERR + 108, 0,  _T("RETURN without GOSUB")
#define BADCONVERSION      BASERR + 109, 0,  _T("Improper %s conversion")
#define INVALIDIOFUNC      BASERR + 110, 38, _T("Mass-Storage function not allowed")
#define DUPLICATEFILE      BASERR + 111, 54, _T("Duplicate file name (%s)")
#define FILENOTFOUND       BASERR + 112, 56, _T("Filename not found (%s)")
#define MASSSTROREERR      BASERR + 113, 81,  _T("Mass-Storage error %s")
#define PLOTTERERROR       BASERR + 114, 110, _T("Plotter specification error")
#define PRINTERERROR       BASERR + 115, 0,		_T("Printer spool/format error")
#define REDIM              BASERR + 116, 0,  _T("Trying to redimension existing array")
#define ENDOFDATA          BASERR + 118, 0,  _T("End of DATA")
#define DIMENSION          BASERR + 119, 16, _T("Dimension mismatch")
#define NOMATRIX           BASERR + 120, 0,  _T("Variable dimension must be <= 6 (is %d)")
#define SINGULAR           BASERR + 121, 0,  _T("Singular matrix")
#define BADFORMAT          BASERR + 122, 0,  _T("Syntax error in print format")
#define OUTOFMEMORY        BASERR + 123, 2,  _T("Out of memory")
#define INTEGEROVERFLOW    BASERR + 124, 20, _T("Integer overflow of %s")
#define NEGATIVELOGVALUE   BASERR + 125, 28, _T("Logarithm of negative value")
#define ZERONEGPOWER			 BASERR + 126, 26, _T("Zero to negative power")
#define IMPROPERVALARG		 BASERR + 127, 32, _T("No Number in string")
#define MATRIXNOTSQR		   BASERR + 128, 43, _T("Matrix not square")
#define COMREDIM           BASERR + 130, 47, _T("Trying to redimension existing COM array")
#define ONNOSUCHLINE       BASERR + 131, 49, _T("No such line in ON statement")
#define INVALIDFILENUM     BASERR + 132, 50, _T("File number is out of range")
#define INVALIDFILENAME    BASERR + 133, 53, _T("Invalid file name")
#define FILEUNASSIGNED     BASERR + 134, 51, _T("File number is not assigned")
#define INVALIDLIMITS      BASERR + 135, 113, _T("Invalid LIMIT specification")
#define UNRECOGNISEDTOKS   BASERR + 136, 113, _T("Unrecognised tokens in line")
#define IOERROR		         BASERR + 137, 0,  _T("IO errror %s")
#define PAUSE			         BASERR + 138, 0,  _T(" ")
#define BREAK              BASERR + 139, 0,  _T("Break")


/////////////////////////////////////////////////////////////////////////////
