10   ! THIS PROGRAM DRAWS THE WORD "TEST" USING EACH OF THE LORG PARAMETERS
20   ! AN "X" IS DRAWN TO INDICATE THE POSITION OF THE PEN FOR EACH LORG.
30   !
40   PLOTTER IS 13, "GRAPHICS"           ! SPECIFY CRT AS THE PLOTTER.
50   GRAPHICS                            ! SET CRT GRAPHICS MODE.
60   SCALE 0,10,-10,0                    ! SPECIFY USER UNITS.
70   FRAME                               ! FRAME PLOTTING AREA.
80   !
90   ! ***** DRAW THE WORD "TEST" USING EACH LORG, "X" INDICATES PEN POSITION.
100  FOR I=1 TO 9                        ! SPECIFY ONE OF NINE LABEL ORIGINS.
110  LORG 2                              ! SPECIFY LORG FOR LEFT LABELS.
120  CSIZE 4                             ! SPECIFY CHARACTER SIZE.
130  MOVE 1,-I                           ! MOVE TO LEFT LABEL POSITION.
140  LABEL USING "5A,D,2A";"LORG ",I," ="    ! LABEL LINE.
150  !
160  ! ***** MOVE TO POINT, DRAW AN "X" TO INDICATE INITIAL PEN POSITION.
170  LORG 5                              ! SPECIFY LORG TO DRAW AN "X".
180  CSIZE 3                             ! SPECIFY CHARACTER SIZE FOR THE "X".
190  MOVE 8,-I                           ! MOVE TO POSITION FOR "X".
200  LABEL USING "K";"X"                 ! DRAW THE "X" TO INDICATE PEN POSITION.
210  !
220  ! ***** MOVE TO POINT, LABEL WITH THE WORD "TEST".
230  LORG I                              ! SPECIFY LOEG FOR THE WORD "TEST".
240  CSIZE 8                             ! SPECIFY THE CHAR SIZE FOR THE WORD "TEST".
250  MOVE 8,-I                           ! MOVE TO THE POSITION OF THE "X".
260  LABEL USING "K";"TEST"              ! DRAW THE LABEL "TEST".
270  NEXT I                              ! DO IT AGAIN FOR THE NEXT LORG.
280  END