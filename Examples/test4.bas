10   ! "BLOCKADE" GAME20   ! TOM LANE   10/14/7730   ! MODIFIED FOR SELFPLAY & BOARD-FILL PERCENTAGE BY BOB HALLISSY40   COM Cos(3),Sin(3),Xypos$(1:80,1:20)[10],Win$(2)[40],Blank$[80]         ! CONSTANTS50   INTEGER Field(1:80,1:20),Rturn(3,2)60   READ Rturn(*)70   DATA -1,0,1,1,0,-1,0,1,-1,0,-1,180   Waittime=10090   Awins=Bwins=0100  RANDOMIZE 110  DEG120  FIXED 0150  PRINT PAGE160  DISP "WAIT, PLEASE"170  FOR I=0 TO 3180    Cos(I)=COS(I*90)           ! HEADINGS ARE STORED AS MULTIPLES OF 90 DEG190    Sin(I)=-SIN(I*90)200    NEXT I210  Cac$=CHR$(27)&"&a"           ! ESC & a  = CURSOR ADDRESS CMD220  FOR I=1 TO 20230    Yp$=Cac$&VAL$(I-1)&"r"240    FOR J=1 TO 80250      Xypos$(J,I)=Yp$&VAL$(J-1)&"C"260    NEXT J270  NEXT I280  Win$(0)="WINNER! "290  Win$(1)=CHR$(128)&"        "&CHR$(128)300  Win$(2)="BLOCKADE"310  Blank$=RPT$(" ",80)330  Sq$=CHR$(127)340  E$=CHR$(27)350  Selfplay=1      ! INITIALIZE GAME TO SELFPLAY385  PAUSE410  Awins=Bwins=0530  Selfplay=0550  MAT Field=ZER560  PRINT E$&"H";570  FOR I=1 TO 20580    PRINT USING "K,K";CHR$(129),Blank$590  NEXT I610  Ahed=INT(4*RND)620  Ax=INT(30*RND)+6630  Ay=INT(10*RND)+6640  Bhed=INT(4*RND)650  Bx=INT(30*RND)+46660  By=INT(10*RND)+6670  A2=B2=0                      ! BOTH SLOW TO START680  DISP Win$(1+Selfplay),"          ";Awins,"          ";Bwins,RPT$("PRESS AN SFK TO PLAY",Selfplay)690  ! START GAME LOOP700 Cycle:Field(Ax,Ay)=Field(Bx,By)=1710  IF NOT Selfplay THEN 920720  R=2*(RND<.9)+(RND<.5)730  FOR I=0 TO 2740    Newhed=(Rturn(R,I)+Ahed) MOD 4750    Nx=Ax+Cos(Newhed)760    Ny=Ay+Sin(Newhed)770    IF (Nx<1) OR (Nx>80) OR (Ny<1) OR (Ny>20) THEN 790780    IF Field(Ax+Cos(Newhed),Ay+Sin(Newhed))=0 THEN 810790  NEXT I800  GOTO 820810  Ahed=Newhed820  R=2*(RND<.9)+(RND<.5)830  FOR I=0 TO 2840    Newhed=(Rturn(R,I)+Bhed) MOD 4850    Nx=Bx+Cos(Newhed)860    Ny=By+Sin(Newhed)870    IF (Nx<1) OR (Nx>80) OR (Ny<1) OR (Ny>20) THEN 890880    IF NOT Field(Nx,Ny) THEN 910890  NEXT I900  GOTO 920910  Bhed=Newhed920  GOSUB Adva930  GOSUB Advb940  IF (Ax=Bx) AND (Ay=By) THEN Astop=Bstop=1950  IF Astop OR Bstop THEN Game_over960  Field(Ax,Ay)=Field(Bx,By)=1970  IF A2 THEN GOSUB Adva980  IF B2 THEN GOSUB Advb990  IF (Ax=Bx) AND (Ay=By) THEN Astop=Bstop=11000 IF Astop OR Bstop THEN Game_over1010 WAIT Waittime*(NOT Selfplay)1020 GOTO Cycle1030 Adva:PRINT USING "K,K";Xypos$(Ax,Ay),Sq$1050 Ax=Ax+Cos(Ahed)1060 Ay=Ay+Sin(Ahed)1080 Astop=(Ax<1) OR (Ax>80) OR (Ay<1) OR (Ay>20)1090 IF NOT Astop THEN Astop=Field(Ax,Ay)1100 IF NOT Astop THEN PRINT USING "#,K,K";Xypos$(Ax,Ay),"*"1110 RETURN 1120 Advb:PRINT USING "K,K";Xypos$(Bx,By),Sq$1140 Bx=Bx+Cos(Bhed)1150 By=By+Sin(Bhed)1170 Bstop=(Bx<1) OR (Bx>80) OR (By<1) OR (By>20)1180 IF NOT Bstop THEN Bstop=Field(Bx,By)1190 IF NOT Bstop THEN PRINT USING "#,K,K";Xypos$(Bx,By),"+"1200 RETURN 1210 Game_over:IF NOT Selfplay THEN BEEP1220 IF Bstop AND NOT Astop THEN Awins=Awins+11230 IF Astop AND NOT Bstop THEN Bwins=Bwins+11240 T$=VAL$(SUM(Field(*))/(20*80)*100)&"%"1250 T$=T$&RPT$(" ",10-LEN(T$))1260 DISP Win$(Astop),"          ";Awins,T$;Bwins,Win$(Bstop)1270 IF Astop AND Bstop THEN DISP Win$(1),,"TIE."1280 WAIT 15001290 IF Selfplay OR (MAX(Awins,Bwins)<15) OR (ABS(Awins-Bwins)<2) THEN 1601300 DISP "GAME OVER...PLAYER "&RPT$("A",Awins>Bwins)&RPT$("B",Bwins>Awins)&" IS THE WINNER BY A SCORE OF ";MAX(Awins,Bwins);" TO ";MIN(Awins,Bwins);"."1310 PAUSE 1320 GOTO 1601500 END