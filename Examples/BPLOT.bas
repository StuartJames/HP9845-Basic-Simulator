10 Abort=0
20   CALL Bplot(Restart_flag,Abort)
30   IF Abort THEN GET "MENU",10,10
40   END
50   SUB Bplot(Restart_flag,Abort)
60 Recover:   ON KEY #7 GOTO Outt
70   ON KEY #15 GOTO Exit_demo
80   ON KEY #23 GOTO Restart
90   ON KEY #13 CALL Dump_graphics
100  PLOTTER IS 13,"GRAPHICS"
110  GCLEAR 
120  DIM R(17,2),A$[30],B$[20]
130  PRINT PAGE,LIN(2),SPA(25),"  BIRTHDAY PLOT  ",LIN(3)
140  PRINT SPA(5),"This program will design your very own Birthday Plot,"
150  PRINT SPA(5),"either on the CRT or the 9872 Plotter.",LIN(1)
160  LINPUT "WOULD YOU LIKE TO USE THE 9872 PLOTTER (Y/N)?",C$
170  IF C$="Y" THEN P9872
180  IF C$="N" THEN Crt
190  GOTO 160
200 P9872:  INPUT "SELECT CODE, BUS ADDRESS OF PLOTTER?",S_code,Bus_add
210  PLOTTER IS S_code,Bus_add,"9872A"
220  GOTO 240
230 Crt:  PLOTTER IS 13,"GRAPHICS"
240  PRINT SPA(5),"Simply enter your name as you want it printed on the plot.",LIN(1)
250  INPUT "WHAT IS YOUR NAME?",A$
260  PRINT SPA(5),"Next, enter your birthdate with two digits for the month, two for"
270  PRINT SPA(5),"the day and two for the year.  For those of us with a more mature"
280  PRINT SPA(5),"outlook on life, simply enter the month and day.   I   understand !"
290  INPUT "ENTER YOUR BIRTHDAY (101543).",A
300  IF A<10000 THEN A=A*100+77
310  B=INT(LOG(A)-7)*.01
320  RANDOMIZE -A*.6125423371*.0000001
330  C=INT(RND*10)+7
340  E=D=0
350  G=F=1
360  FOR H=1 TO C
370  R(H,1)=RND
380  IF R(H,1)<=D THEN 400
390  D=R(H,1)
400  IF R(H,1)>=F THEN 420
410  F=R(H,1)
420  R(H,2)=RND
430  IF R(H,2)<=E THEN 450
440  E=R(H,2)
450  IF R(H,2)>=G THEN 470
460  G=R(H,2)
470  NEXT H
480  SCALE F,D,G-.1*(E-G),E
490  GRAPHICS
500  FOR H=1 TO 60
510  IF H>50 THEN PEN 1
520  IF (H>35) AND (H<=50) THEN PEN 2
530  IF (H>20) AND (H<=35) THEN PEN 3
540  IF (H>10) AND (H<=20) THEN PEN 4
550  R(C+1,1)=R(1,1)
560  R(C+1,2)=R(1,2)
570  FOR I=1 TO C+1
580  PLOT R(I,1),R(I,2)
590  IF I>C THEN 620
600  R(I,1)=B*(R(I+1,1)-R(I,1))+R(I,1)
610  R(I,2)=B*(R(I+1,2)-R(I,2))+R(I,2)
620  NEXT I
630  NEXT H
640  PLOT D,E,-2
650  CSIZE 3
660  MOVE (D-F)*.1+F,G-.1*(E-G)
670  LABEL A$;"'S BIRTHDAY PLOT"
680  PAUSE 
690 Exit_demo:  SUBEXIT
700 Outt:  Abort=1
710  SUBEXIT
720 Restart:  Restart_flag=1
730  SUBEND
740  SUB Dump_graphics
750  GRAPHICS
760  DUMP GRAPHICS 
770  SUBEND
