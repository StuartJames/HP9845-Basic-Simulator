/*
 *        Copyright (c) 2020-2023 HydraSystems.
 *
 *  This program is free software; you can redistribute it and/or   
 *  modify it under the terms of the GNU General Public License as  
 *  published by the Free Software Foundation; either version 2 of  
 *  the License, or (at your option) any later version.             
 *                                                                  
 *  This program is distributed in the hope that it will be useful, 
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of  
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the   
 *  GNU General Public License for more details.                    
 *                                                                  
 *
 *  Based on a design by Michael Haardt
 *
 * Edit Date/Ver   Edit Description
 * ==============  ===================================================
 * SJ   19/08/2020  Original
 *
 */

#pragma once

// row position definitions
#define RP_LEFT       0
#define RP_MIDDLEOF3  1
#define RP_RIGHT      2
#define RP_MIDDLEOF2  3

class CDib : public CObject
{
    enum Alloc{noAlloc, crtAlloc, heapAlloc}; // applies to BITMAPINFOHEADER
    DECLARE_SERIAL(CDib)
public:
    LPVOID                m_lpvColorTable;
    HBITMAP               m_hBitmap;
    LPBYTE                m_lpImage;    // starting address of DIB bits
    LPBITMAPINFOHEADER    m_lpBMIH;     // buffer containing the BITMAPINFOHEADER

private:
    HGLOBAL               m_hGlobal;    // for external windows we need to free could be allocated by this class or allocated externally
    Alloc                 m_nBmihAlloc;
    Alloc                 m_nImageAlloc;

public:
    DWORD                 m_dwSizeImage; // of bits -- not BITMAPINFOHEADER or BITMAPFILEHEADER

private:
    int                   m_nColorTableEntries;
    PBYTE                 *m_ppRows;

    HANDLE                m_hFile;
    HANDLE                m_hMap;
    LPVOID                m_lpvFile;
    HPALETTE              m_hPalette;
    int                   m_RowLength;
    int                   m_HalfRowLength;

public:
    CDib();
    CDib(CSize size, int nBitCount);    // builds BITMAPINFOHEADER
    ~CDib();
    int                   GetSizeImage(){ return m_dwSizeImage; }
    int                   GetSizeHeader(){ return sizeof(BITMAPINFOHEADER) + sizeof(RGBQUAD) * m_nColorTableEntries; }
    CSize                 GetDimensions();
    BYTE*                 GetRowPtr(int y, int Position = RP_LEFT);
    int                   GetRowLength(){ return m_RowLength; }
    BYTE*                 GetPixelPtr(int x, int y);
    BOOL                  AttachMapFile(LPCSTR strPathname, BOOL bShare = FALSE);
    BOOL                  CopyToMapFile(LPCSTR strPathname);
    BOOL                  AttachMemory(LPVOID lpvMem, BOOL bMustDelete = FALSE, HGLOBAL hGlobal = NULL);
    BOOL                  Draw(CDC* pDC, CRect rcDest, CPoint ptSrc, DWORD rop);
    BOOL                  DrawGrayScale(CDC *pDC, CRect rcDest, CPoint ptSrc, DWORD rop);
    HBITMAP               CreateSection(CDC* pDC = NULL);
    UINT                  UsePalette(CDC* pDC, BOOL bBackground = FALSE);
    BOOL                  MakePalette();
    BOOL                  SetSystemPalette(CDC* pDC);
    BOOL                  Compress(CDC* pDC, BOOL bCompress = TRUE); // FALSE means decompress
    HBITMAP               CreateBitmap(CDC* pDC);
    BOOL                  Read(CFile* pFile);
    BOOL                  Write(CFile* pFile);
    void                  Serialize(CArchive& ar);
    void                  Empty();
    void                  EraseDib();
    void                  ScrollDib(bool MoveUp = false);
    void                  ScrollDib(int LinesToScroll);

private:
    BOOL                  MakeRows();
    void                  DetachMapFile();
    void                  ComputePaletteSize(int nBitCount);
    void                  ComputeMetrics();
};
