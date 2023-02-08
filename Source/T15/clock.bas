10  ! THIS PROGRAM SHOWS HOW THE PDIR STATEMENT CAN BE USED TO ROTATE
20  ! A SET OF FIXED POINTS. IN THIS CASE THE HANDS OF A CLOCK.
30  !
40  PLOTTER IS 13,"GRAPHICS"            ! SPECIFY THE CRT AS PLOTTER.
50  GRAPHICS                            ! SET CRT GRAPHICS MODE.
60  SHOW -6,6,-6,6                      ! SPECIFY USER UNITS.
70  FRAME                               ! FRAME THE PLOTTING AREA.
80  DEG                                 ! SET DEGREES MODE.
90  !
100 ! ***** DRAW THE FACE OF THE CLOCK *****
110 CSIZE 10                            ! SPECIFY CHARACTER SIZE.
120 LORG 5                              ! SPECIFY TO CENTER NUMBERS.
130 FOR Minute=1 TO 60                  ! FOR...NEXT TO SPECIFY MINUTE MARK.
140 MOVE 0,0                            ! MOVE TO CENTER OF CLOCK.
150 PDIR 360*(1-Minute/60)              ! PLOT DIRECTION FOR MINUTE.
160 IPLOT 0,4.5,-2                      ! MOVE TO RIM OF CLOCK.
170 IPLOT 0,.1,-1                       ! DRAW MINUTE MARK.
180 IF Minute MOD 5 THEN GOTO 220       ! IF NOT ON 5 MINUTE MARK, SKIP TO NEXT.
190 IPLOT 0,.2                          ! DRAW LONGER MINUTE MARK.
200 IPLOT 0,.7,-2                       ! MOVE TO CENTER OF NUMBER.
210 LABEL USING "K";Minute/5            ! LABEL WITH NUMBER.
220 NEXT Minute                         ! DO IT AGAIN FOR NEXT MINUTE.
230 !
240 ! ***** ENTER THE CURRENT TIME *****
250 EXIT GRAPHICS                       ! EXIT GRAPHICS FOR INPUT.
260 INPUT "ENTER THE HOUR",Hour         ! ENTER HOUR.
270 Hour=INT(Hour) MOD 12               ! ADJUST FOR TWELVE HOUR CLOCK.
280 INPUT "ENTER THE MINUTES",Minute    ! ENTER MINUTE.
290 Minute=INT(Minute) MOD 60           ! ADJUST MINUTE TO 60 MINUTE HOUR.
300 Minute=Minute+60*Hour               ! COMBINE MINUTES AND HOURS.
310 GRAPHICS                            ! GO BACK TO GRAPHICS.
320 !
330 ! ***** DRAW, ERASE AND ROTATE A BOX USING PDIR AND IPLOT STATEMENTS.
340 SecondLoop: !
350 PEN -1                              ! SPECIFY ERASE PEN.
360 GOSUB Second_hand                   ! ERASE SECOND HAND.
370 Second=(Second+1) MOD 60            ! INCREMENT SECONDS.
380 PEN 1                               ! SPECIFY TO DRAW.
390 GOSUB Second_hand                   ! DRAW SECOND HAND.
400 IF NOT Second THEN PEN -1           ! SPECIFY ERASE IF SECOND = 0.
410 GOSUB Minute_hand                   ! DRAW OR ERASE MINUTE HAND.
420 GOSUB Hour_hand                     ! DRAW OR ERASE HOUR HAND.
430 IF NOT Second THEN Minute=Minute+1  ! IF SECOND = 0, INCREMENT MINUTES.
440 PEN 1                               ! SPECIFY DRAW.
450 GOSUB Minute_hand                   ! DRAW MINUTE HAND.
460 GOSUB Hour_hand                     ! DRAW HOUR HAND.
470 WAIT 300                            ! WAIT TO ADJUST CLOCK.
480 GOTO SecondLoop                     ! DO IT AGAIN FOR NEXT SECOND.
490 !
500 ! ***** DRAW ROUTINES FOR THE SECOND, MINUTE AND HOUR HANDS.
510 !
520 !  DATA FOR SECOND HAND.
530 Second_hand: !
540 PDIR -Second*6
550 MOVE 0,0
560 IPLOT 0,4.4,-2
570 IPLOT -.2,-.4,-1
580 IPLOT .4,0
590 IPLOT -.2,.4
600 RETURN
610 !
620 !  DATA FOR MINUTE HAND.
630 Minute_hand: !
640 PDIR -Minute*6
650 MOVE 0,0
660 IPLOT -.25,3,-1
670 IPLOT .25,1
680 IPLOT .25,-1
690 IPLOT -.25,-3
700 RETURN
710 !
720 !  DATA FOR HOUR HAND.
730 Hour_hand: !
740 PDIR -Minute*6/12
750 MOVE 0,0
760 IPLOT -.4,2,-1
770 IPLOT .4,1
780 IPLOT .4,-1
790 IPLOT -.4,-2
800 RETURN
