/*
 * Module ID: minmaxlogic.h
 * Title    : CMinMaxLogic class definition.
 *
 * For details on CMinMaxLogic class, go to:
 * http://www3.sympatico.ca/olanglois/minmaxdemo.html
 *
 * Author   : Olivier Langlois <olanglois@sympatico.ca>
 * Date     : February 03, 2006
 */

#pragma once

#include <windows.h> /* For LONG */

/*
 * TBPos values
 */
constexpr auto TBNOTCREATED = -1;
constexpr auto TBTOP				= 0;
constexpr auto TBBOTTOM			= 1;
constexpr auto TBLEFT				= 2;
constexpr auto TBRIGHT			= 3;
constexpr auto TBFLOAT			= 4;

/* This is used with the class CMinMaxFrame. Its purpose is to isolate
 * everything that is not related to MFC to ease an eventual porting
 * to another framework (ie.: WTL).
 *
 * Note: This class assumes that the associated frame has a menu and the
 * following Window Styles:
 *
 * - WS_BORDER
 * - WS_CAPTION
 * - WS_THICKFRAME
 *
 * This condition should always be met since the MFC AppWizard
 * generated code is using WS_OVERLAPPEDWINDOW that includes all 3 styles
 * to create the frame window.  */
class CMinMaxLogic
{
public:
	CMinMaxLogic(LONG x, LONG y);
	~CMinMaxLogic(void);

	/******************************************************************************
 *
 * Purpose   : Compute the minimum frame size from the provided minimum client
 *             area size. It is called at construction and can be recalled anytime
 *             by the user.
 *
 * Parameters:
 *     x       (LONG) Minimum client horizontal size.
 *     y       (LONG) Minumum client vertical size.
 *
 * Return value : None.		*/
 
	void SetClientMin(LONG x, LONG y);

	/******************************************************************************
 * Purpose   : Set the minimum size to the minimum frame size and make
 *             adjustments based on the toolbar and status bar visibility
 *             state and their sizes.
 *
 * Parameters:
 *     lpMMI  (MINMAXINFO FAR*) MinMax info structure pointer.
 *
 * Return value : None.		 */

	void OnGetMinMaxInfo(MINMAXINFO FAR *lpMMI);

	BOOL			m_sbVisible; /* Status bar visibility      */
	LONG			m_sbHeight;  /* Status bar height          */
	BOOL			m_tbVisible; /* Toolbar visibility         */
	int				m_tbPos;      /* Toolbar position (left, right, top, bottom) */
	int				m_tbSize;     /* Toolbar size               */

private:
	LONG			m_cxMin; /* Minimum client size        */
	LONG			m_cyMin;
	LONG			m_fyMin; /* Minimum frame size that includes borders, the frame, the toolbar */
	LONG			m_fxMin; /* and the status bar to have a client area of m_cxMin*m_cyMin      */
};
