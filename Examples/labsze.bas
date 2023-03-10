10   ! THIS PROGRAM DEMONSTRATES SOME OF THE DIFFERENT CHARACTER SIZES
20   ! AND ASPECT RATIOS.
30   !
40   PLOTTER IS 13, "GRAPHICS"           ! SPECIFY CRT AS THE PLOTTER.
50   GRAPHICS                            ! SET CRT GRAPHICS MODE.
60   FRAME                               ! FRAME PLOTTING AREA.
70   SCALE -0.5,10,-10,0                 ! SPECIFY USER UNITS.
80   !
90   ! ***** LABEL PLOTTING AREA WITH TWO COLOMNS OF EXAMPLE CHARACTER SIZES.
100  FOR Column=1 TO 2                   ! SPECIFY ONE OF TWO COLUMNS.
110  FOR Size=2 TO 8                     ! SPECIFY ONE OF EIGHT CHAR SIZE.
120  CSIZE Size,Column/2                 ! SPECIFY CHAR SIZE AND ASPECT RATIO.
130  MOVE (Column-1)*4,-Size             ! MOVE TO LABEL POSITION.
140  LABEL USING "K";"CSIZE ",Size,",",Column/2    ! LABEL PLOTTING AREA.
150  NEXT Size                           ! GET NEXT SIZE			
160  NEXT Column                         ! GET NEXT COLUMN.
170  END