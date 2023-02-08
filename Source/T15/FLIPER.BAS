10   DIM S$(19)[80],X1(8),X2(8),Y1(8),Y2(8),V(8),Score(10)
20   INTEGER I1(19,79)
30   READ X1(*),X2(*)
40   DATA 20,35,35,45,45,45,64,70,70
50   DATA 29,36,36,60,60,60,66,74,74
60   READ Y1(*),Y2(*)
70   DATA 9,0,14,3,9,14,8,5,12
80   DATA 11,5,19,5,11,16,12,8,15
90   READ V(*)
100  DATA 100,10,10,100,100,100,200,500,500
110  FOR I=0 TO 8
120   FOR J=X1(I) TO X2(I)
130    FOR K=Y1(I) TO Y2(I)
140     I1(K,J)=V(I)
150    NEXT K
160   NEXT J
170  NEXT I
180  FOR I=0 TO 79
190   FOR J=0 TO 19 STEP 19
200    I1(J,I)=1
210   NEXT J
220  NEXT I
230  FOR I=1 TO 18
240   I1(I,0)=1
250   I1(I,79)=1
260  NEXT I
270  C$=" "&CHR$(127)
280  FOR I=0 TO 19
290   S$(I)=" "
300   IF I<9 THEN S$(I)="L"
310   IF I>10 THEN S$(I)="R"
320   FOR J=1 TO 79
330    S$(I)[J+1;1]=C$[MIN(1,I1(I,J))+1;1]
340   NEXT J
350  NEXT I
360 Start: PRINT PAGE;
370  PRINT S$(*);"&a0r0C"
375  INPUT "Number of players ?",N_players
380  FOR I=1 TO N_players
390   Score(I)=0
400  NEXT I
420  FOR Ball=1 TO 5
430   FOR Player=1 TO N_players
440    DISP "PLAYER #";Player;" UP NEXT.  BALL #";Ball;".     Hit CONTINUE to start"
450    PAUSE 
460    X=Y=X_old=Y_old=5
470    Dy=.8*((.5-RND)*.1+1)
480    Dx=1.45*((.5-RND)*.1+1)
490    ON KEY #0 GOSUB Left
500    ON KEY #5 GOSUB Right
510    ON KEY #9 GOSUB Left
520 Move: ! 
530    X=MIN(79,MAX(0,X+Dx))
540    Y=MIN(19,MAX(0,Y+Dy))
550    IF I1(Y,X) THEN GOTO Wall
560 CrsImg: IMAGE #,"&a",DD,"r",DD,"C",K
570    PRINT USING CrsImg;Y_old,X_old," "
580    PRINT USING CrsImg;Y,X,"*"
590    X_old=X
600    Y_old=Y
605 WAIT 15
610    GOTO Move
611 Wall: IF I1(Y,X)<2 THEN GOTO No_score
620    Score(Player)=Score(Player)+I1(Y,X)
621    BEEP
630 No_score: IF X<.5 THEN GOTO Miss
650    X=X_old
660    Y=Y_old
670    DISP "score: ";Score(Player)
680    Dx_flag=I1(Y,MIN(79,MAX(0,X+Dx)))
681    Dy_flag=I1(MIN(79,MAX(0,Y+Dy)),X)
700    IF Dy_flag THEN Dy=-Dy-.01
710    IF Dx_flag THEN Dx=-Dx
720    IF Dy_flag OR Dx_flag THEN GOTO Move
730    Dx=-Dx
740    Dy=-Dy
750    GOTO Move
760 Miss: DISP "MISS !         Player #";Player;"  - -  ";Score(Player)
770    WAIT 2000
771    PRINT USING CrsImg;Y_old,X_old," "
780   NEXT Player
790  NEXT Ball
791  DISP 
800  PRINT PAGE;LIN(5)
810  FOR I=1 TO N_players
820   PRINT USING I;I,Score(I)
830 I: IMAGE 25X,"Player #",K,"  - -  ",K
840  NEXT I
841  DISP "Press CONTINUE to restart game."
850  PAUSE 
860  GOTO Start
870 Left: !
875  IF (Y>8) OR (X>10) THEN RETURN 
880  Dx=-Dx
890  RETURN 
900 Right: !
905  IF (Y<11) OR (X>10) THEN RETURN
910  Dx=-Dx
920  RETURN 
