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

#include "stdafx.h"
#include "HersheyFont.h"

//////////////////////////////////////////////////////////////////////

CHersheyFont::CHersheyFont()
{
	m_VertexCount = 0;
	m_pVertex = NULL;
}

//////////////////////////////////////////////////////////////////////

CHersheyFont::CHersheyFont(CHersheyFont *pChar)
{
	m_VertexCount = pChar->m_VertexCount;
	m_pVertex = new CharVertex_s[m_VertexCount];
	for(int i = 0; i < m_VertexCount; i++){
		m_pVertex[i] = pChar->m_pVertex[i];
	}
}

//////////////////////////////////////////////////////////////////////

CHersheyFont::~CHersheyFont()
{
	if(m_pVertex!=NULL){
		delete m_pVertex;
	}
}

//////////////////////////////////////////////////////////////////////

void CHersheyFont::Read(FILE *pFile)
{
	fread(&m_VertexCount, sizeof(int), 1, pFile);
	m_pVertex = new CharVertex_s[m_VertexCount];
	fread(m_pVertex, sizeof(CharVertex_s), m_VertexCount, pFile);		
}

//////////////////////////////////////////////////////////////////////

void CHersheyFont::Write(FILE *pFile)
{
	fwrite(&m_VertexCount, sizeof(int), 1, pFile);
	fwrite(m_pVertex, sizeof(CharVertex_s), m_VertexCount, pFile);		
}

//////////////////////////////////////////////////////////////////////

int CHersheyFont::ReadOldStyle(FILE *pFile)
{
unsigned char Buffer[4096];
char *pChar;
int i;
		
	if(feof(pFile))	return(FALSE);
	fgets((char *)Buffer, 2048, pFile);				//read the data from the file
	pChar = (char *)Buffer;									//Get the data size
	sscanf_s(pChar, "%d", &m_VertexCount);
	m_VertexCount = (m_VertexCount / 2);
	if(m_VertexCount == 0)	return(FALSE);
	m_pVertex = new CharVertex_s[m_VertexCount];			//Build the data structure
 	for(i=0; i < m_VertexCount; i++){
		while(*pChar != ',') pChar++;
		pChar++;
		sscanf_s(pChar, "%lf,%lf", &(m_pVertex[i].X), &(m_pVertex[i].Y));
		while(*pChar != ',') pChar++;
		pChar++;
	}
	return(TRUE);
}

//////////////////////////////////////////////////////////////////////

CHersheyFont::CHARVERTEX_TYPE CHersheyFont::GetFirstVertex(CharVertex_s &Vertex, int &Index)
{
	if(m_pVertex == NULL)	return(TERMINATE);
	Index = 1;
	Vertex = m_pVertex[Index];
	return(MOVE_TO);
}

//////////////////////////////////////////////////////////////////////

CHersheyFont::CHARVERTEX_TYPE CHersheyFont::GetNextVertex(CharVertex_s &Vertex, int &Index)
{
	Index++;
	if(Index < m_VertexCount - 1){
		Vertex = m_pVertex[Index];
		if(Vertex.X < -63.5){
			Index++;
			Vertex = m_pVertex[Index];
			return(MOVE_TO);
		}
		return(DRAW_TO);
	}
	return(TERMINATE);
}


