/*
 *        Copyright 1996 Coherent Research Inc.
 *
 *      Author:Randy More
 *
 *			Revision: 5 $
 *
 * Edit Date/Ver   Edit Description
 * ==============  ===================================================
 * RM   11/06/1996  v8.00  Original
 * RM		06/10/1999	v2.32p Added this header block and comment blocks
 *
 */

#include "stdafx.h"
#include "resource.h"
#include <math.h>
#include "HersheyFont.h"
#include "HersheyFunctions.h"

const char szFontFile[] = _T(".\\HersheyFonts\\Hershey.smf");
const char szXtbFile[] =	_T(".\\HersheyFonts\\Hershey.xtb");

constexpr auto BASE_CHARACTER_WIDTH			= 16.0;							 // 16 points wide
constexpr auto BASE_CHARACTER_HEIGHT		= 22.0;							 // 22 points high
constexpr auto TEXT_DROP								= 2;
constexpr auto WADJUST									= 2.0;
constexpr auto HADJUST									= 1.0;



//////////////////////////////////////////////////////////////////////

CHersheyFunctions::CHersheyFunctions()
{
	m_CharCount = 0;
	for(int Type = 0; Type < TRANSLATION_TYPES; Type++){
		for(int Count = 0; Count < TRANSLATION_SIZE; Count++){
			m_XlationTable[Type][Count] = -1;
		}
	}
	LoadFontData();
	LoadXlationData();
}

//////////////////////////////////////////////////////////////////////

CHersheyFunctions::CHersheyFunctions(bool ForceReload)
{
	m_CharCount = 0;
	for(int Type = 0; Type < TRANSLATION_TYPES; Type++){
		for(int Count = 0; Count < TRANSLATION_SIZE; Count++){
			m_XlationTable[Type][Count] = -1;
		}
	}
	if(!ForceReload){
		LoadFontData();
		LoadXlationData();
	}
	else{
		LoadOldStyleFile();
		LoadOldStyleXlation();
	}
}

//////////////////////////////////////////////////////////////////////

CHersheyFunctions::~CHersheyFunctions()
{
CHersheyFont *pChar;

	for(UINT i = 0; i < m_CharCount; i++){
		pChar = m_FontCharList[i];
		delete pChar;
	}
	m_FontCharList.RemoveAll();
}

//////////////////////////////////////////////////////////////////////

void CHersheyFunctions::LoadFontData(void)
{
FILE *pFile;
CHersheyFont *pChar;
UINT Count;

	if(fopen_s(&pFile, szFontFile, "rb")){
		MessageBox(NULL, "Font stroke file was not found(\\HersheyFonts\\Hershey.smf)\nSpecify font stroke .CSV data file.", "Load Font Data", MB_OK);
		LoadOldStyleFile();
		SaveFontData();
	}
	else{
		fread(&Count, sizeof(unsigned int), 1, pFile);
		for(m_CharCount = 0; m_CharCount < Count; m_CharCount++){
			pChar = new CHersheyFont;
			pChar->Read(pFile);
			m_FontCharList.Add(pChar);
		}
		m_CharCount = Count;
		fclose(pFile);
	}
}

//////////////////////////////////////////////////////////////////////

void CHersheyFunctions::SaveFontData(void)
{
FILE *pFile;

	fopen_s(&pFile, szFontFile, "wb");
	UINT Count = m_CharCount;
	fwrite(&Count, sizeof(unsigned int), 1, pFile);
	for(Count = 0; Count < m_CharCount; Count++){
		m_FontCharList[Count]->Write(pFile);
	}
	fclose(pFile);
}

//////////////////////////////////////////////////////////////////////

void CHersheyFunctions::LoadXlationData(void)
{
FILE *pFile;
int k, m = TRANSLATION_SIZE / 2;

	if(fopen_s(&pFile, szXtbFile,"rb")){
		MessageBox(NULL, "Font translation file was not found in the system directory\nSpecify translation data file (TranslationChart.CSV).", "Load Xlation Data", MB_OK);
		LoadOldStyleXlation();
		SaveXlationData();
	}
	else{
		for(int i = 0; i < TRANSLATION_TYPES; i++){
			for(int j = 0; j < m; j++){
				if(!feof(pFile)){
					fread(&k, sizeof(int), 1, pFile);
					m_XlationTable[i][j] = k;
				}
			}
		}
		for(int i = 0; i < TRANSLATION_TYPES; i++){
			for(int j = m; j < TRANSLATION_SIZE; j++){
				if(!feof(pFile)){
					fread(&k, sizeof(int), 1, pFile);
					m_XlationTable[i][j] = k;
				}
			}
		}
		fclose(pFile);
	}
}

//////////////////////////////////////////////////////////////////////

void CHersheyFunctions::SaveXlationData(void)
{
FILE *pFile;
int m = TRANSLATION_SIZE / 2;

	fopen_s(&pFile, szXtbFile, "wb");
	for(int i = 0; i < TRANSLATION_TYPES; i++){
		for(int j = 0; j < m; j++){
			int k = m_XlationTable[i][j];
			fwrite(&k, sizeof(int), 1, pFile);
		}
	}
	for(int i = 0; i < TRANSLATION_TYPES; i++){
		for(int j = m; j < TRANSLATION_SIZE; j++){
			int k = m_XlationTable[i][j];
			fwrite(&k, sizeof(int), 1, pFile);
		}
	}
	fclose(pFile);
}

//////////////////////////////////////////////////////////////////////

void CHersheyFunctions::LoadOldStyleXlation(void)
{
FILE * pFile;
char Buffer[4096];
char *pChar;
int i, j, k;

	m_CharCount = 0;
	CFileDialog open(TRUE, NULL, "*.csv",	OFN_HIDEREADONLY | OFN_FILEMUSTEXIST,	"Font Translation File (*.csv)|*.csv||",NULL);
	open.m_ofn.lpstrInitialDir = "\\";
	open.m_ofn.lpstrTitle = "Select Font Translation File";
	open.DoModal();
	if(fopen_s(&pFile, open.GetPathName(), "rb")){
		MessageBox(NULL, "Translation file did not open\nLoadOldStyleXlationFile","Error", MB_OK);
		_exit(0);
	}
	fgets((char *)Buffer, 4096, pFile);
	for(i = 0; i < TRANSLATION_SIZE; i++){
		fgets((char *)Buffer, 4096, pFile);
		pChar = Buffer;
		while(*pChar != ',') pChar++;
		pChar++;
		for(k = 0; k < TRANSLATION_TYPES; k++){
			while(*pChar != ',') pChar++;
			pChar++;
			sscanf_s(pChar, "%d", &j);
			m_XlationTable[k][i] = j;
		}
	}
	fclose(pFile);

}

//////////////////////////////////////////////////////////////////////

void CHersheyFunctions::LoadOldStyleFile(void)
{
FILE * pFile;
CHersheyFont *pChar;

	m_CharCount = 0;
	CFileDialog open(TRUE, nullptr, nullptr,	OFN_HIDEREADONLY | OFN_FILEMUSTEXIST,	_T("Font Stroke File (*.csv)|*.csv||"), NULL);
	open.m_ofn.lpstrInitialDir = "\\";
	open.m_ofn.lpstrTitle = "Select Font Stroke File";
	open.DoModal();
	if(fopen_s(&pFile, open.GetPathName(), "rb")){
		MessageBox(NULL, "Font file did not open\nLoadOldStyleFile", "Error", MB_OK);
		_exit(0);
	}
	for(;;){
		pChar = new CHersheyFont;
		if(pChar->ReadOldStyle(pFile)){
			m_FontCharList.Add(pChar);
			m_CharCount++;
		}
		else{
			delete pChar;
			break;
		}
	}
	fclose(pFile);
}

//////////////////////////////////////////////////////////////////////

int CHersheyFunctions::GetCharID(unsigned char Char,	FONT_TYPE FontType)
{
	int Type = (int)FontType;
	int Index = Char;
	if(Type < 0)	return(-1);
	if(Type > (int)GOTHIC)	return(-1);
	return(m_XlationTable[Type][Index]);
}

//////////////////////////////////////////////////////////////////////

CPoint CHersheyFunctions::DrawString(CDC *pDC, CPoint Location, double Rotation, double CharHeight, double CharRatio, int Origin, FONT_TYPE pFontType, CString String)
{
CPoint Pos = Location;

	int StrLen = String.GetLength();
	if(StrLen < 1) return Pos;
	double CharWidth = CharHeight * CharRatio;
	double Kerning = CharWidth / WADJUST + 2;
	double ScaleX = CharWidth / BASE_CHARACTER_WIDTH;
	double ScaleY = CharHeight / BASE_CHARACTER_HEIGHT;
	double sin_val = -sin(Rotation);																			// anticlockwise rotation
	double cos_val = cos(Rotation);
	double KerningX = cos_val * Kerning;
	double KerningY = sin_val * Kerning;
	Pos.Offset((int)(CharWidth / 3), 5);
	double StrExtX = Kerning * ((double)StrLen);
	switch(Origin){																												// calculate origin offset
		case 1:{ Pos.Offset(0, (int)-BASE_CHARACTER_HEIGHT); break; }
		case 2:{ Pos.Offset(0, (int)-(BASE_CHARACTER_HEIGHT / 2)); break; }
		case 3: break; 
		case 4:{ Pos.Offset((int)-(StrExtX / 2), (int)-BASE_CHARACTER_HEIGHT); break; }
		case 5:{ Pos.Offset((int)-(StrExtX / 2), (int)-(BASE_CHARACTER_HEIGHT / 2)); break; }
		case 6:{ Pos.Offset((int)-(StrExtX / 2), 0); break; }
		case 7:{ Pos.Offset((int)-StrExtX, (int)-BASE_CHARACTER_HEIGHT); break; }
		case 8:{ Pos.Offset((int)-StrExtX, (int)-(BASE_CHARACTER_HEIGHT / 2)); break; }
		case 9:{ Pos.Offset((int)-StrExtX, 0); break; }
	}
	for(int CharPos = 0; CharPos < StrLen; CharPos++){
		int CharID = GetCharID(String.GetAt(CharPos),pFontType);
		if(CharID >= 0){
			CharVertex_s Vertex;
			int VertexIndex = 0;
			CHersheyFont *pChr = GetCharacter(CharID);
			CHersheyFont::CHARVERTEX_TYPE VertexType = pChr->GetFirstVertex(Vertex, VertexIndex);
			while(VertexType != CHersheyFont::TERMINATE){
				Vertex.X = Vertex.X * ScaleX;
				Vertex.Y = (Vertex.Y + TEXT_DROP) * ScaleY;
				if(VertexType == CHersheyFont::MOVE_TO){
					pDC->MoveTo((int)(Pos.x + (cos_val * Vertex.X - sin_val * Vertex.Y)), (int)(Pos.y + (sin_val * Vertex.X + cos_val * Vertex.Y)));
				}
				else{
					pDC->LineTo((int)(Pos.x + (cos_val * Vertex.X - sin_val * Vertex.Y)), (int)(Pos.y + (sin_val * Vertex.X +  cos_val * Vertex.Y)));
				}
				VertexType = pChr->GetNextVertex(Vertex, VertexIndex);
			}
		}
		Pos.x += (int)KerningX;
		Pos.y += (int)KerningY;
		Location.x +=	(int)KerningX;
		Location.y +=	(int)KerningY;
	}
	return Location;
}
