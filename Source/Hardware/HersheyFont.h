/*
 *        Copyright 1996 Coherent Research Inc.
 *
 *      Author:Randy More
 *
 *			Revision: 5 $
 *
 * Edit Date/Ver   Edit Description
 * ==============  ===================================================
 * RM   11/06/1996  v5.00  Original
 * RM		01/11/1999	v9.04a Added this header block and comment blocks
 *
 */

#pragma once

#include "stdafx.h"

//////////////////////////////////////////////////////////////////////

typedef struct CharVertex_s
{
	double X;
	double Y;
} CharVertex_s;

//////////////////////////////////////////////////////////////////////

class CHersheyFont
{
public:
										CHersheyFont();
										CHersheyFont(CHersheyFont *pChar);
	virtual						~CHersheyFont();
	void							Read(FILE *pFile);
	void							Write(FILE *pFile);
	int								ReadOldStyle(FILE *pFile);
	double						GetMinX(void){	return(m_pVertex[0].X);	};
	double						GetMaxX(void){	return(m_pVertex[0].Y);	};

	enum CHARVERTEX_TYPE
	{
		MOVE_TO,				//Pen up
		DRAW_TO,				//Pen down
		TERMINATE				//End of data
	};

	CHARVERTEX_TYPE		GetFirstVertex(CharVertex_s &Vertex,	int &Index);
	CHARVERTEX_TYPE		GetNextVertex(CharVertex_s &Vertex, int &Index);

private:
	int								m_VertexCount;
	CharVertex_s			*m_pVertex;
};
