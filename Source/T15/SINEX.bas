10   ! THIS PROGRAM PLOTS THE SINE(X)/X FROM -4PI TO +4PI TO DEMONSTRATE
20   ! SOME INTERESTING CAPABILITY AND PROGRAMMING TECHNIQUES.
30   ! THE PROGRAM WILL STOP AND WAIT FOR YOU TO MOVE THE CURSOR TO DIGITIZE
40   ! A POINT, AND THEN PRINT THE "X" AND "Y" VALUES.
50   !
60   PLOTTER IS 13, "GRAPHICS"           ! SPECIFY THE CRT AS PLOTTER.
70   GRAPHICS                            ! SET GRAPHICS MODE.
80   FRAME                               ! FRAME PLOTTING AREA.
90   PRINTER IS 0                        ! SPECIFY THE INTERNAL PRINTER.
100  RAD                                 ! SPECIFY RADIANS MODE.
110  LOCATE 10,120,20,90                 ! SPECIFY GRAPH BOUNDARIES.
120  !
130  ! ***** SPECIFY MAXIMUM AND MINIMUM VALUES FOR USER UNITS.
140  Xmin=-4*PI
150  Xmax=4*PI
160  Ymin=-0.5
170  Ymax=1.5
180  !
190  ! **** SPECIFY USER UNITS, DRAW AXES AND FRAME GRAPH.
200  SCALE Xmin,Xmax,Ymin,Ymax           ! SPECIFY USER UNITS.
210  AXES PI/6,0.5,0,0,3,10              ! DRAW AXES.
220  FRAME                               ! FRAME PLOTTING AREA.
230  !
240  ! ***** LABEL AXES, USING SUBROUTINES.
250  GOSUB Lxaxis                        ! LABEL X AXIS.
260  GOSUB Lyaxis                        ! LABEL Y AXIS.
270  !
280  ! ***** PLOT FUNCTION WITH FILL TO THE BASE LINE.
290  FOR X=Xmin TO Xmax STEP PI/20
300  IF X=Xmin THEN MOVE X,SIN(X)/X      ! MOVE TO START IF X=Xmin.
310  IF X=0 THEN 350                     ! AVOID DIVIDING BY ZERO.
320  DRAW X,SIN(X)/X                     ! DRAW TO NEXT POINT ON CURVE.
330  MOVE X,0                            ! MOVE TO BASE LINE.
340  DRAW X,SIN(X)/X                     ! DRAW BACK TO CURVE FOR FILL.
350  NEXT X                              ! NEXT POINT ON CURVE.
360  !
370  ! **** MOVE AND LABEL PLOT
380  MOVE -3.5*PI,1.1                    ! MOVE TO START OF LABEL.
390  LORG 1                              ! SPECIFY LABEL ORIGIN.
400  CSIZE 8                             ! SPECIFY LABEL SIZE.
410  LABEL USING "K";"SIN (X)/X"         ! LABEL PLOT.
420  CSIZE 3                             ! RESET CHAR SIZE FOR DIGITIZE.
430  POINTER -2*PI,0.5,0                 ! PUT POINTER AT 0,0 AS SMALL CROSS.
440  !
450  ! ***** THIS ALLOWS YOU TO DIGITIZE AND PRINT POINTS FROM THE CRT.
460 Looop:!
470  DIGITIZE X,Y                        ! WAIT FOR CONTINUE THE INPUT X AND Y.
480                                      ! & PRINT X AND Y ON SPECIFIED PRINTER.
490  PRINT USING "2(K,XMD.2D),2/";"X=",X/PI," PI,   Y=",Y 
500  GOTO Looop                          ! DO IT AGAIN
510  END
520  !
530  !
540  ! ***** LABEL AXES ROUTINES
550 Lxaxis: !
560  CSIZE 3                             ! SPECIFY CHARACTER SIZE.
570  LDIR -(PI/2)                        ! SPECIFY LABEL DIRECTION, DOWN.
580  LORG 2                              ! SPECIFY LABEL ORIGIN.
590  FOR X1=Xmin TO Xmax STEP PI
600  MOVE X1,Ymin                        ! MOVE TO LABEL POSITION.
610  LABEL USING "M4DX,K";X1/PI,"PI"     ! DRAW LABEL.
620  NEXT X1                             ! DO IT AGAIN.
630  RETURN
640  !
650 Lyaxis: !
660  CSIZE 3                             ! SPECIFY CHARACTER SIZE.
670  LDIR 0                              ! SPECIFY LABEL DIRECTION, L TO R.
680  LORG 8                              ! SPECIFY LABEL ORIGIN.
690  FOR Y1=Ymin TO Ymax STEP 0.5
700  MOVE Xmin,Y1                        ! MOVE TO LABEL POSITION.
710  LABEL USING "MD.DDX";Y1             ! DRAW LABEL.
720  NEXT Y1                             ! DO IT AGAIN.
730  RETURN
