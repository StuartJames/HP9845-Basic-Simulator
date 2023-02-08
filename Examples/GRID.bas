10   ! THIS PROGRAM DRAWS A GRID AND FRAMES THE PLOTTING AREA.
20   !
30   PLOTTER IS 13, "GRAPHICS"           ! SPECIFY THE CRT AS PLOTTER.
40   GRAPHICS                            ! SET GRAPHICS MODE.
50   SCALE -8,8,-8,8                     ! SPECIFY USER UNITS.
60   !
70   GRID 1,1,0,0,4,4,1                  ! DRAW GRID, SPECIFYING:
80                                       ! ONE MINOR TIC PER UNIT IN X,
90                                       ! ONE MINOR TIC PER UNIT IN Y,
100                                      ! INTERSECT AT 0,0,
110                                      ! ONE MAJOR TIC PER 4 MINOR TICS IN X,
120                                      ! ONE MAJOR TIC PER 4 MINOR TICS IN Y,
130                                      ! MINOR TIC SIZE IS 1 GDU.
140  !
150  FRAME                               ! FRAME PLOTTING AREA.
160  PAUSE
170  !
180  GCLEAR
190  SCALE -20,180,0,160
200  !CLIP 0,160,0,160
210  GRID 20,20,0,0
220  PAUSE
230  END
