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

Program_s 	*ProgramNew(Program_s *pProg);
void 				ProgramDestroy(Program_s *pProg);
void 				ProgramNoRun(Program_s *pProg);
int 				ProgramStore(Program_s *pProg, Token_s *pLine, long where);
int 				ProgramDelete(Program_s *pProg, int Line);
void 				ProgramAddScope(Program_s *pProg, Scope_s *pScope);
long				ProgramGetLabelLine(Program_s *pProg, Label_s *pLabel, PC_s *pPc);
PC_s 				*ProgramGoLine(Program_s *pProg, int LineNumber, PC_s *pPc);
PC_s 				*ProgramFromLine(Program_s *pProg, int LineNumber, PC_s *pPc);
PC_s 				*ProgramToLine(Program_s *pProg, int LineNumber, PC_s *pPc);
int 				ProgramScopeCheck(Program_s *pProg, PC_s *pPc, PC_s *fn);
PC_s 				*ProgramDataLine(Program_s *pProg, int LineNumber, PC_s *pPc);
PC_s 				*ProgramImageLine(Program_s *pProg, int LineNumber, PC_s *pPc);
long 				ProgramLineNumber(const Program_s *pProg, const PC_s *pPc);
PC_s 				*ProgramBeginning(Program_s *pProg, PC_s *pPc);
PC_s 				*ProgramEnd(Program_s *pProg, PC_s *pPc);
long				ProgramNextLine(Program_s *pProg, PC_s *pPc);
PC_s 				*ProgramPreviousLine(Program_s *pProg, PC_s *pPc);
PC_s 				*ProgramGetAtIndex(Program_s *pProg, PC_s *pPc, int Index);
PC_s 				*ProgramGetAtLine(Program_s *pProg, PC_s *pPc, int LineNumber);
long 				ProgramGetLineNumber(Program_s *pProg, int Index);
int 				ProgramSkipEOL(Program_s *pProg, PC_s *pPc);
void				ProgramTraceClear(void);
void				ProgramTracePause(Program_s *pProgram, PC_s *pPc);
void 				ProgramTraceFlow(Program_s *pProg, PC_s *pTo);
void				ProgramTraceVar(Program_s *pProgram, pValue_s pVal);
void				ProgramTraceMat(Program_s *pProgram, const char *pStr);
void 				ProgramPCtoError(Program_s *pProg, PC_s *pPc, Value_s *pValue);
int 				GetLineNumberWidth(Program_s *pProg);
Value_s 		*ProgramMerge(Program_s *pDest, Program_s *pSrc, Value_s *pValue);
Value_s 		*ProgramGet(Program_s *pProg, int FileIndex, Value_s *pValue);
Value_s 		*ProgramList(Program_s *pProg, int FileIndex, PC_s *pFrom, PC_s *pTo, Value_s *pValue);
Value_s 		*ProgramStore(Program_s *pProg, int FileIndex, PC_s *pFrom, PC_s *pTo, Value_s *pValue);
Value_s 		*ProgramLoad(Program_s *pProg, int FileIndex, Value_s *pValue);
BString    *GetProgLineNumber(Program_s *pProg, int Index, BString *pStr);
Value_s 		*ProgramAnalyse(Program_s *pProg, PC_s *pPc, Value_s *pValue);
void 				ProgramRenumber(Program_s *pProg, int first, int inc);
void 				ProgramUnnumber(Program_s *pProg);
int 				ProgramSetName(Program_s *pProg, const char *filename);

