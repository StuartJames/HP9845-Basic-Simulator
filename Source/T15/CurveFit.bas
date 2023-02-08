10   ! Curve fit program
20   OPTION BASE 1
30   INTEGER X(100),Y(100),Domain(350)
40   DIM C(100),Func(350),A$[15]
50   SelAddr=706                             !Set default select code.
60   INPUT "ENTER SELECT CODE OF DIGITIZER (e.g. 706)",SelAddr
70   A=INT(SelAddr/100)
80   B=INT(SelAddr-A*100)
90   IF (A<0) OR (A>16) OR (B<0) OR (B>31) THEN 60
100  PLOTTER IS 13, "GRAPHICS"                    !Set up graphics.
110  SCALE 0,17500,0,14200
120  GCLEAR 
130  CLIP 0,17500,0,12700
140  FRAME
150  UNCLIP
160  I=0
170  LORG 5
180  GRAPHICS
190  P=100                             !Max number of points allowed.
200  OUTPUT SelAddr;"DF;SG;AT;SK1"             !Set up digitizer.
210  A$="DIG, PRESS FA"                       !Display instructions.
220  CALL Display(SelAddr,A$)
230  OUTPUT SelAddr;"BP"
240  OUTPUT SelAddr;"OS"                       !Check status byte.
250  ENTER SelAddr;Status
260  IF NOT BIT(Status,2) THEN 400         !If no point dig, go back.
270  OUTPUT SelAddr;"OD"
280  ENTER SelAddr;X,Y,Z,W                     !Input a point.
290  IF I=0 THEN 330
300  IF X>X(I) THEN 330
310  BEEP
320  GOTO 240
330  I=I+1
340  X(I)=X
350  Y(I)=Y
360  MOVE X,Y
370  LABEL USING "K";"+"                      !Plot that point.
380  IF I>=P THEN 750                !Check for max number of points.
390  GOTO 240
400  OUTPUT SelAddr;"OK"                   !See if key fa was pressed.
410  ENTER SelAddr;Key
420  IF Key<>0 THEN 240
430  OUTPUT SelAddr;"SK0"                      !Clear keys.
440  A$="               "                     !Clear display.
450  CALL Display(SelAddr,A$)
460  IF Flag=1 THEN 580                   !If already printed, go on.
470  PRINTER IS 0                             !Print key definitions.
480  PRINT "       Fa - SPLINE FIT"
490  PRINT "Prefix Fa - ERASE SPLINE FIT"
500  PRINT "       Fb - CHEBYSCHEV FIT"
510  PRINT "Prefix Fb - ERASE CHEBYSCHEV FIT"
520  PRINT "       Fc - ENTER NEW SET OF POINTS WITHOUT CLEARING SCREEN"
530  PRINT "Prefix Fc - ENTER NEW SET OF POINTS AND CLEAR SCREEN"
540  PRINT "       Fd - DUMP GRAPHICS"
550  PRINT "       Fe - STOP"
560  PRINT LIN(2)
570  Flag=1
580  A$="CHOOSE OPTION"                       !Display choose option.
590  CALL Display(SelAddr,A$)
600  OUTPUT SelAddr;"BP"
610  OUTPUT SelAddr;"OK"                   !See which key was pressed.
620  ENTER SelAddr;Key
630  IF Key=0 THEN 610                   !If no key pressed, go back.
640  IF (Key=1) OR (Key=32) THEN Optn=1    !Fa - Optn for spline fit.
650  IF (Key=2) OR (Key=64) THEN Optn=2     !Fb - Optn for cheby fit.
660  IF Key=4 THEN Optn=3                     !Fc - Optn to add pts.
670  IF Key=128 THEN Optn=4        !Prefix Fc - Optn clear & add pts.
680  IF Key=8 THEN DUMP GRAPHICS              !Fd - Dump graphics.
690  IF Key=8 THEN 430
700  IF Key=16 THEN Optn=5                    !Fe - Optn to stop.
710  A$="               "                     !Clear display.
720  CALL Display(SelAddr,A$)
730  ON Optn GOTO Spline,Cheby,160,100,1410   !Go to appropiate line.
740  GOTO 610                             !If undefined key, go back.
750 Cheby: M=I                                !Set number of points. 
760  OUTPUT SelAddr;"BP"
770  CALL Display(SelAddr,"ENTER DEGREE") !Enter degree of polynomial.
780  OUTPUT SelAddr;"OS"
790  ENTER SelAddr;Status
800  IF NOT BIT(Status,0) THEN 780
810  OUTPUT SelAddr;"ON"
820  ENTER SelAddr;N
830  IF (N<=0) OR (N>M-2) THEN 770           !Check for valid degree.
840  GRAPHICS
850  IF Key=64 THEN 910            !If erasing, don't re-plot points.
860  PEN 1
870  FOR K=1 TO M                             !Re-plot the points.
880  MOVE X(K),Y(K)
890  LABEL USING "K";"+"
900  NEXT K
910  MOVE 3000,500                            !Plot message.
920  LABEL USING "K";"COMPUTING POLYNOMIAL"
930  CALL Cheby(M,N,C(*),X(*),Y(*))           !Call cheby subprogram.
940  PEN -1                                   !Erase message.
950  MOVE 3000,500
960  LABEL USING "K";"COMPUTING POLYNOMIAL"
970  LINE TYPE 1
980  PEN 1
990  IF Key=64 THEN PEN -1           !Erase, if shift fb was pressed.
1000 GOSUB Write
1010 GOTO 430                           !Return out of cheby routine.
1020 Write:FOR X=X(1) TO X(M) STEP 50   !Routine to draw polynomial.
1030 Sum=0
1040 FOR J=0 TO N                    !Compute the poly at each point.
1050 Sum=Sum+C(J)*X^J
1060 NEXT J
1070 IF X=X(1) THEN MOVE X,Sum
1080 PLOT X,Sum                               !Draw the line.
1090 NEXT X
1100 RETURN 
1110 Spline:MOVE 3000,500                     !Spline routine.
1120 LINE TYPE 1                              !Plot message.
1130 LABEL USING "K";"COMPUTING SPLINE"
1140 N=I                                      !Set number of points.
1150 LORG 5
1160 Narg=0
1170 FOR J=X(1) TO X(N) STEP 50        !Domain pts for interpolation.
1180 Narg=Narg+1
1190 Domain(Narg)=J
1200 NEXT J
1210 FOR K=1 TO N                             !Re-plot the points.
1220 MOVE X(K),Y(K)
1230 LABEL USING "K";"+"
1240 NEXT K
1250 Eps=.001                                 !Error tolerance.
1260 CALL Spline(N,Narg,X(*),Y(*),Domain(*),Func(*),Int,Eps)
1270 PEN -1                                   !Erase message.
1280 MOVE 3000,500
1290 LABEL USING "K";"COMPUTING SPLINE"
1300 LINE TYPE 2
1310 PEN 1
1320 IF Key=32 THEN PEN -1            !Erase if shift fa was pressed.
1330 GOSUB Draw
1340 LINE TYPE 1
1350 GOTO 430                            !Return from spline routine.
1360 Draw:FOR X=1 TO Narg              !Plot each interpolated point.
1370 PLOT Domain(X),Func(X)
1380 NEXT X
1390 PENUP
1400 RETURN 
1410 OUTPUT SelAddr;"DF"                       !Stop routine.
1420 GCLEAR 
1430 EXIT GRAPHICS
1440 A$="PROG DONE"
1450 CALL Display(SelAddr,A$)
1460 END
1470 SUB Spline(N,Narg,INTEGER X(*),Y(*),Domain(*),REAL Func(*),Int,Eps)
1480 Baddta=(Eps<=0)
1490 IF Baddta=0 THEN 1540
1500 PRINT LIN(2),"ERROR IN SUBPROGRAM Spline."
1510 PRINT "Eps=";Eps,LIN(2)
1520 PAUSE 
1530 GOTO 1480
1540 OPTION BASE 1
1550 DIM S(N),G(N-1),Work(N-1)
1560 FOR I=2 TO N-1
1570    Xi=X(I)
1580    Xim1=X(I-1)
1590    Xip1=X(I+1)
1600    Yi=Y(I)
1610    Yim1=Y(I-1)
1620    Yip1=Y(I+1)
1630    X=Xi-Xim1
1640    H=Xip1-Xim1
1650    Work(I)=.5*X/H
1660    T=((Yip1-Yi)/(Xip1-Xi)-(Yi-Yim1)/X)/H
1670    S(I)=2*T
1680    G(I)=3*T
1690 NEXT I
1700 S(1)=S(N)=0
1710 W=8-4*SQR(3)                    !W is the relaxations factor for
1720 U=0                                 !successive over-relaxation.
1730 FOR I=2 TO N-1
1740    T=W*(-S(I)-Work(I)*S(I-1)-(.5-Work(I))*S(I+1)+G(I))
1750    H=ABS(T)
1760    IF H>U THEN U=H
1770    S(I)=S(I)+T
1780 NEXT I
1790 IF U>=Eps THEN 1720
1800 FOR I=1 TO N-1
1810    G(I)=(S(I+1)-S(I))/(X(I+1)-X(I))
1820 NEXT I
1830 IF Narg=0 THEN 2050
1840 FOR J=1 TO Narg                   !Calculate function values and
1850 Corrector:    I=1                        !derivatives.
1860    T=Domain(J)
1870    IF T>=X(1) THEN 1920
1880    PRINT LIN(2),"ERROR IN SUBPROGRAM Spline."
1890    PRINT "ARGUMENT OUT OF BOUNDS."
1900    PRINT "X(1)=";X(1),"X(N)=";X(N),"Domain(";J;")=";Domain(J),LIN(2)
1910    PAUSE 
1920    I=I+1
1930    IF I>N THEN 1880
1940    IF T>X(I) THEN 1920
1950    I=I-1
1960    H=Domain(J)-X(I)
1970    T=Domain(J)-X(I+1)
1980    X=H*T
1990    S=S(I)+H*G(I)
2000    Z=1/6
2010    U=Z*(S(I)+S(I+1)+S)
2020    W=(Y(I+1)-Y(I))/(X(I+1)-X(I))
2030    Func(J)=W*H+Y(I)+X*U
2040 NEXT J
2050 Int=0                              !Calculate integral from X(1)
2060 FOR I=1 TO N-1                           !to X(N).
2070    H=X(I+1)-X(I)
2080    Int=Int+.5*H*(Y(I)+Y(I+1))-1/24*H^3*(S(I)+S(I+1))
2090 NEXT I
2100 SUBEND
2110 SUB Cheby(M,N,C(*),INTEGER X(*),Y(*))    !Chebyschev curve fit.
2120 Baddta=(M<=N+1)                          !Check that M>N+1.
2130 IF Baddta=0 THEN 2180
2140 PRINT LIN(2),"ERROR IN SUBPROGRAM Cheby."
2150 PRINT "# OF DATA POINTS MUST BE GREATER THAN DEG. OF POLY. +1.",LIN(2)
2160 PAUSE 
2170 GOTO 2120
2180 DIM T(1:M),Ax(1:N+2),Ay(1:N+2),Ah(1:N+2),By(1:N+2),Bh(1:N+2)
2190 INTEGER In(1:N+2)
2200 K=(M-1)/(N+1)
2210 FOR I=1 TO N+1
2220    In(I)=(I-1)*K+1
2230 NEXT I
2240 In(N+2)=M
2250 Start: Sign=1                            !Begin iteration.
2260 FOR I=1 TO N+2
2270    Ax(I)=X(In(I))
2280    Ay(I)=Y(In(I))
2290    Ah(I)=Sign
2300    Sign=-Sign
2310 NEXT I
2320 Difference: FOR I=2 TO N+2               !Divided differences.
2330    FOR J=I-1 TO N+2
2340       By(J)=Ay(J)
2350       Bh(J)=Ah(J)
2360    NEXT J
2370    FOR J=I TO N+2
2380       Diff=Ax(J)-Ax(J-I+1)
2390       Ay(J)=(By(J)-By(J-1))/Diff
2400       Ah(J)=(Bh(J)-Bh(J-1))/Diff
2410    NEXT J
2420 NEXT I
2430 H=-Ay(N+2)/Ah(N+2)
2440 Poly: FOR I=0 TO N                     !Polynomial coefficients.
2450    C(I)=Ay(I+1)+Ah(I+1)*H
2460    By(I+1)=0
2470 NEXT I
2480 By(1)=1
2490 Tmax=ABS(H)
2500 Imax=In(1)
2510 FOR I=1 TO N
2520    FOR J=0 TO I-1
2530       By(I+1-J)=By(I+1-J)-By(I-J)*X(In(I))
2540       C(J)=C(J)+C(I)*By(I+1-J)
2550    NEXT J
2560 NEXT I
2570 Error: FOR I=1 TO M                      !Compute deviations.
2580    T(I)=C(N)
2590    FOR J=0 TO N
2600       T(I)=T(I)*X(I)+C(N-J)
2610    NEXT J
2620    T(I)=T(I)-Y(I)
2630    IF ABS(T(I))<=Tmax THEN L1
2640    Tmax=ABS(T(I))
2650    Imax=I
2660 L1: NEXT I
2670 FOR I=1 TO N+2
2680    IF Imax<In(I) THEN L2
2690    IF Imax=In(I) THEN Fit
2700 NEXT I
2710 L2: IF T(Imax)*T(In(I))<0 THEN L3
2720 In(I)=Imax
2730 GOTO Start
2740 L3: IF In(1)<Imax THEN L4
2750 FOR I=1 TO N+1
2760    In(N+3-I)=In(N+2-I)
2770 NEXT I
2780 In(I)=Imax
2790 GOTO Start
2800 L4: IF In(N+2)<=Imax THEN L5
2810 In(I-2)=Imax
2820 GOTO Start
2830 L5: FOR I=1 TO N+1
2840    In(I)=In(I+1)
2850 NEXT I
2860 In(N+2)=Imax
2870 GOTO Start
2880 Fit:SUBEND
2890 SUB Display(SelAddr,A$)                   !Display subprogram.
2900 OPTION BASE 1
2910 DATA 238,254,156,252,158,142,190,110,96,112,0,28,0,42,252,206,230,10,182,30,124,0,0,0,118,0
2920 DATA 58,,62,26,122,222,142,246,46,32,112,0,96,0,42,58,206,230,10,182,30,56,0,0,0,118,0
2930 DATA 0,96,218,242,102,182,62,224,254,230,252,1,156,240,130,2,32,68,64,202
2940 INTEGER A(72)
2950 DIM D$[240],Alpha$[72]
2960 READ A(*)                                !Read character data.
2970 D$=""
2980 Display: IF LEN(A$)>15 THEN A$="LINE TOO LONG" !Ck line too long.
2990 Alpha$="ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz 1234567890.[]=-, '?" !List of accepted chars.  
3000 Alpha$[70,70]=CHR$(34)
3010 FOR I=1 TO LEN(A$)                       !For each character,
3020 P=POS(Alpha$,A$[I,I])                  !find position in string.
3030 IF P=0 THEN P=53              !If undefined, then display blank.
3040 D$=D$&";DD"&VAL$(I)&","&VAL$(A(P)) !Add char to instruct string.
3050 NEXT I
3060 OUTPUT SelAddr USING "K";D$            !Print instruction string.
3070 SUBEND

