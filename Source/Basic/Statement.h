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

pValue_s stmt_ASSIGN(pValue_s pValue);
pValue_s stmt_BEEP(pValue_s pValue);
pValue_s stmt_CALL(pValue_s pValue);
pValue_s stmt_CASE(pValue_s pValue);
pValue_s stmt_CAT(pValue_s pValue);
pValue_s stmt_CHECK_READ(pValue_s pValue);
pValue_s stmt_CLEAR(pValue_s pValue);
pValue_s stmt_CLOSE(pValue_s pValue);
pValue_s stmt_CLS(pValue_s pValue);
pValue_s stmt_COM(pValue_s pValue);
pValue_s stmt_CREATE(pValue_s pValue);
pValue_s stmt_DATA(pValue_s pValue);
pValue_s stmt_DEFAULT(pValue_s pValue);
pValue_s stmt_DEFFN(pValue_s pValue);
pValue_s stmt_DEFINT_DEFDBL_DEFSTR(pValue_s pValue);
pValue_s stmt_DELETE(pValue_s pValue);
pValue_s stmt_DELAY(pValue_s pValue);
pValue_s stmt_DIM(pValue_s pValue);
pValue_s stmt_DO(pValue_s pValue);
pValue_s stmt_DO_CONDITION(pValue_s pValue);
pValue_s stmt_DUMP(pValue_s pValue);
pValue_s stmt_ELSE_ELSEIFELSE(pValue_s pValue);
pValue_s stmt_ENABLE_DISABLE(pValue_s pValue);
pValue_s stmt_END(pValue_s pValue);
pValue_s stmt_ENDIF(pValue_s pValue);
pValue_s stmt_ENDSELECT(pValue_s pValue);
pValue_s stmt_COLON_EOL(pValue_s pValue);
pValue_s stmt_QUOTE_REM(pValue_s pValue);
pValue_s stmt_EQ_FNEND(pValue_s pValue);
pValue_s stmt_EXECUTE(pValue_s pValue);
pValue_s stmt_EXITDO(pValue_s pValue);
pValue_s stmt_EXITFOR(pValue_s pValue);
pValue_s stmt_FIXED(pValue_s pValue);
pValue_s stmt_FLOAT(pValue_s pValue);
pValue_s stmt_FOLDER(pValue_s pValue);
pValue_s stmt_FOR(pValue_s pValue);
pValue_s stmt_GET(pValue_s pValue);
pValue_s stmt_GOSUB(pValue_s pValue);
pValue_s stmt_GOTO_RESUME(pValue_s pValue);
pValue_s stmt_LABEL(pValue_s pValue);
pValue_s stmt_LET(pValue_s pValue);
pValue_s stmt_LINK(pValue_s pValue);
pValue_s stmt_LIST(pValue_s pValue);
pValue_s stmt_LOAD(pValue_s pValue);
pValue_s stmt_LOCAL(pValue_s pValue);
pValue_s stmt_LOCATE(pValue_s pValue);
pValue_s stmt_LOCK_UNLOCK(pValue_s pValue);
pValue_s stmt_LOOP(pValue_s pValue);
pValue_s stmt_LOOPUNTIL(pValue_s pValue);
pValue_s stmt_LSET_RSET(pValue_s pValue);
pValue_s stmt_IDENTIFIER(pValue_s pValue);
pValue_s stmt_IF_ELSEIFIF(pValue_s pValue);
pValue_s stmt_IMAGE(pValue_s pValue);
pValue_s stmt_INPUT(pValue_s pValue);
pValue_s stmt_MASSTORAGEIS(pValue_s pValue);
pValue_s stmt_MAT(pValue_s pValue);
pValue_s stmt_MATINPUT(pValue_s pValue);
pValue_s stmt_MATPRINT(pValue_s pValue);
pValue_s stmt_MATREAD(pValue_s pValue);
pValue_s stmt_NAME(pValue_s pValue);
pValue_s stmt_NEXT(pValue_s pValue);
pValue_s stmt_NUMERIC(pValue_s pValue);
pValue_s stmt_OFFKEY(pValue_s pValue);
pValue_s stmt_ON(pValue_s pValue);
pValue_s stmt_ONERROR(pValue_s pValue);
pValue_s stmt_ONERROROFF(pValue_s pValue);
pValue_s stmt_ONKEY(pValue_s pValue);
pValue_s stmt_OPTIONBASE(pValue_s pValue);
pValue_s stmt_OUTPUT(pValue_s pValue);
pValue_s stmt_OVERLAP_SERIAL(pValue_s pValue);
pValue_s stmt_PAUSE(pValue_s pValue);
pValue_s stmt_PRINT_LPRINT(pValue_s pValue);
pValue_s stmt_PRINTERIS(pValue_s pValue);
pValue_s stmt_RANDOMIZE(pValue_s pValue);
pValue_s stmt_READ(pValue_s pValue);
pValue_s stmt_REDIM(pValue_s pValue);
pValue_s stmt_COPY_RENAME(pValue_s pValue);
pValue_s stmt_RENUM(pValue_s pValue);
pValue_s stmt_REPEAT(pValue_s pValue);
pValue_s stmt_RESULT(pValue_s pValue);
pValue_s stmt_RESTORE(pValue_s pValue);
pValue_s stmt_RETURN(pValue_s pValue);
pValue_s stmt_RUN(pValue_s pValue);
pValue_s stmt_SAVE(pValue_s pValue);
pValue_s stmt_SCRATCH(pValue_s pValue);
pValue_s stmt_SELECT(pValue_s pValue);
pValue_s stmt_STANDARD(pValue_s pValue);
pValue_s stmt_STOP(pValue_s pValue);
pValue_s stmt_STORE(pValue_s pValue);
pValue_s stmt_SUBDEF(pValue_s pValue);
pValue_s stmt_SUBEND(pValue_s pValue);
pValue_s stmt_SUBEXIT(pValue_s pValue);
pValue_s stmt_SYSTEM(pValue_s pValue);
pValue_s stmt_TRIGMODE(pValue_s pValue);
pValue_s stmt_TRACE(pValue_s pValue);
pValue_s stmt_TRUNCATE(pValue_s pValue);
pValue_s stmt_UNNUM(pValue_s pValue);
pValue_s stmt_UNTIL(pValue_s pValue);
pValue_s stmt_WAIT(pValue_s pValue);
pValue_s stmt_WHILE(pValue_s pValue);
pValue_s stmt_WEND(pValue_s pValue);
pValue_s stmt_WRITE(pValue_s pValue);
pValue_s stmt_XREF(pValue_s pValue);

