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

#include "stdafx.h"
#include "cdib08.h"

IMPLEMENT_SERIAL(CDib, CObject, 0);

CDib::CDib()
{
	m_hFile = NULL;
	m_hBitmap = NULL;
	m_hPalette = NULL;
  m_ppRows = NULL;
	m_nBmihAlloc = m_nImageAlloc = noAlloc;
	Empty();
}

CDib::CDib(CSize size, int nBitCount)
{
	m_hFile = NULL;
	m_hBitmap = NULL;
	m_hPalette = NULL;
  m_ppRows = NULL;
	m_nBmihAlloc = m_nImageAlloc = noAlloc;
	Empty();
	ComputePaletteSize(nBitCount);
	m_lpBMIH = (LPBITMAPINFOHEADER) new char[sizeof(BITMAPINFOHEADER) + sizeof(RGBQUAD) * m_nColorTableEntries];
	m_nBmihAlloc = crtAlloc;
	m_lpBMIH->biSize = sizeof(BITMAPINFOHEADER);
	m_lpBMIH->biWidth = size.cx;
	m_lpBMIH->biHeight = size.cy;
	m_lpBMIH->biPlanes = 1;
	m_lpBMIH->biBitCount = nBitCount;
	m_lpBMIH->biCompression = BI_RGB;
	m_lpBMIH->biSizeImage = 0;
	m_lpBMIH->biXPelsPerMeter = 0;
	m_lpBMIH->biYPelsPerMeter = 0;
	m_lpBMIH->biClrUsed = m_nColorTableEntries;
	m_lpBMIH->biClrImportant = m_nColorTableEntries;
	ComputeMetrics();
	memset(m_lpvColorTable, 0, sizeof(RGBQUAD) * m_nColorTableEntries);
	m_lpImage = NULL;  // no data yet
  m_RowLength = 0;
  m_HalfRowLength = 0;
}

CDib::~CDib()
{
	Empty();
}

CSize CDib::GetDimensions()
{	
	if(m_lpBMIH == NULL) return CSize(0, 0);
	return CSize((int) m_lpBMIH->biWidth, (int) m_lpBMIH->biHeight);
}

BOOL CDib::AttachMapFile(LPCSTR strPathname, BOOL bShare) // for reading
{
	// if we open the same file twice, Windows treats it as 2 separate files
	HANDLE hFile = ::CreateFile(strPathname, GENERIC_READ, bShare ? FILE_SHARE_READ : 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	ASSERT(hFile);
	DWORD dwFileSize = ::GetFileSize(hFile, NULL);
	HANDLE hMap = ::CreateFileMapping(hFile, NULL, PAGE_READONLY, 0, 0, NULL);
	DWORD dwErr = ::GetLastError();
	if(hMap == NULL){
		AfxMessageBox(_T("Empty bitmap file"));
		return FALSE;
	}
	LPVOID lpvFile = ::MapViewOfFile(hMap, FILE_MAP_READ, 0, 0, 0); // map whole file
	if(((LPBITMAPFILEHEADER) lpvFile)->bfType != 0x4d42){
		AfxMessageBox(_T("Invalid bitmap file"));
		return FALSE;
	}
	AttachMemory((LPBYTE) lpvFile + sizeof(BITMAPFILEHEADER));
	m_lpvFile = lpvFile;
	m_hFile = hFile;
	m_hMap = hMap;
  MakeRows();
	return TRUE;
}

void CDib::DetachMapFile()
{
	if(m_hFile == NULL) return;
	::UnmapViewOfFile(m_lpvFile);
	::CloseHandle(m_hMap);
	::CloseHandle(m_hFile);
	m_hFile = NULL;
}

BOOL CDib::CopyToMapFile(LPCSTR strPathname)
{
	// copies DIB to a new file, releases prior pointers
	// if you previously used CreateSection, the HBITMAP will be NULL (and unusable)
	BITMAPFILEHEADER bmfh;
	bmfh.bfType = 0x4d42;  // 'BM'
	bmfh.bfSize = m_dwSizeImage + sizeof(BITMAPINFOHEADER) + sizeof(RGBQUAD) * m_nColorTableEntries + sizeof(BITMAPFILEHEADER);
	// meaning of bfSize open to interpretation
	bmfh.bfReserved1 = bmfh.bfReserved2 = 0;
	bmfh.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + sizeof(RGBQUAD) * m_nColorTableEntries;	
	HANDLE hFile = ::CreateFile(strPathname, GENERIC_WRITE | GENERIC_READ, 0, NULL,	CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	ASSERT(hFile);
	int nSize =  sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + sizeof(RGBQUAD) * m_nColorTableEntries +  m_dwSizeImage;
	HANDLE hMap = ::CreateFileMapping(hFile, NULL, PAGE_READWRITE, 0, nSize, NULL);
	DWORD dwErr = ::GetLastError();
	ASSERT(hMap);
	LPVOID lpvFile = ::MapViewOfFile(hMap, FILE_MAP_WRITE, 0, 0, 0); // map whole file
	ASSERT(lpvFile);
	LPBYTE lpbCurrent = (LPBYTE) lpvFile;
	memcpy(lpbCurrent, &bmfh, sizeof(BITMAPFILEHEADER)); // file header
	lpbCurrent += sizeof(BITMAPFILEHEADER);
	LPBITMAPINFOHEADER lpBMIH = (LPBITMAPINFOHEADER) lpbCurrent;
	memcpy(lpbCurrent, m_lpBMIH, sizeof(BITMAPINFOHEADER) + sizeof(RGBQUAD) * m_nColorTableEntries); // info
	lpbCurrent += sizeof(BITMAPINFOHEADER) + sizeof(RGBQUAD) * m_nColorTableEntries;
	memcpy(lpbCurrent, m_lpImage, m_dwSizeImage); // bit image
	DWORD dwSizeImage = m_dwSizeImage;
	Empty();
	m_dwSizeImage = dwSizeImage;
	m_nBmihAlloc = m_nImageAlloc = noAlloc;
	m_lpBMIH = lpBMIH;
	m_lpImage = lpbCurrent;
	m_hFile = hFile;
	m_hMap = hMap;
	m_lpvFile = lpvFile;
	ComputePaletteSize(m_lpBMIH->biBitCount);
	ComputeMetrics();
	MakePalette();
  MakeRows();
	return TRUE;
}

BOOL CDib::AttachMemory(LPVOID lpvMem, BOOL bMustDelete, HGLOBAL hGlobal)
{
	// assumes contiguous BITMAPINFOHEADER, color table, image
	// color table could be zero length
	Empty();
	m_hGlobal = hGlobal;
	if(bMustDelete == FALSE) m_nBmihAlloc = noAlloc;
	else m_nBmihAlloc = ((hGlobal == NULL) ? crtAlloc : heapAlloc);
	m_lpBMIH = (LPBITMAPINFOHEADER) lpvMem;
	ComputePaletteSize(m_lpBMIH->biBitCount);
	ComputeMetrics();
	m_lpImage = (LPBYTE) m_lpvColorTable + sizeof(RGBQUAD) * m_nColorTableEntries;
	MakePalette();
	return TRUE;
}

UINT CDib::UsePalette(CDC* pDC, BOOL bBackground /* = FALSE*/ )
{
	if(m_hPalette == NULL) return 0;
	HDC hdc = pDC->GetSafeHdc();
	HPALETTE hOldPalette = ::SelectPalette(hdc, m_hPalette, bBackground);
	return ::RealizePalette(hdc);
}

BOOL CDib::Draw(CDC* pDC, CRect rcDest, CPoint ptSrc, DWORD rop)
{
HPALETTE hOldPal = NULL; //previous palette (our DIB's palette is m_hPalette)
BOOL bSuccess = FALSE; //success/fail flag
CSize dibsize = GetDimensions(); //get DIB's dimensions

  if(m_lpBMIH == NULL) return FALSE;
  if(m_hPalette != NULL) hOldPal = ::SelectPalette(pDC->GetSafeHdc(), m_hPalette, TRUE);
	::SetStretchBltMode(pDC->GetSafeHdc(), COLORONCOLOR);
	if(dibsize == rcDest.Size())	bSuccess = ::SetDIBitsToDevice(	pDC->GetSafeHdc(),rcDest.left,rcDest.top,	rcDest.Width(),rcDest.Height(),	ptSrc.x,ptSrc.y,0,rcDest.Height(),m_lpImage,(LPBITMAPINFO)m_lpBMIH,	DIB_RGB_COLORS);
	else bSuccess = ::StretchDIBits(pDC->GetSafeHdc(),rcDest.left,rcDest.top,rcDest.Width(),rcDest.Height(),ptSrc.x,ptSrc.y,m_lpBMIH->biWidth,m_lpBMIH->biHeight,m_lpImage,(LPBITMAPINFO)m_lpBMIH,DIB_RGB_COLORS,	rop);
	if(hOldPal != NULL)	::SelectPalette(pDC->GetSafeHdc(),hOldPal,TRUE);
  return bSuccess;
}

BOOL CDib::DrawGrayScale(CDC *pDC, CRect rcDest, CPoint ptSrc, DWORD rop)
{
BOOL bSuccess = FALSE; //success/fail flag
CPalette pal;
CPalette *pOldPalette = nullptr;
CSize dibsize = GetDimensions(); //get DIB's dimensions

  if(m_lpBMIH == NULL) return FALSE;
	int nColors = m_lpBMIH->biClrUsed ? m_lpBMIH->biClrUsed :	1 << m_lpBMIH->biBitCount;
	if( pDC->GetDeviceCaps(RASTERCAPS) & RC_PALETTE && nColors <= 256 ){// The device supports a palette and bitmap has color table
		UINT nSize = sizeof(LOGPALETTE) + (sizeof(PALETTEENTRY) * nColors);		// Allocate memory for a palette
		LOGPALETTE *pLP = (LOGPALETTE *) new BYTE[nSize];
		pLP->palVersion = 0x300;
		pLP->palNumEntries = nColors;
  	LPRGBQUAD pDibQuad = (LPRGBQUAD) m_lpvColorTable;
		for(int i = 0; i < nColors; i++){
			long lSquareSum = pDibQuad->rgbRed * pDibQuad->rgbRed
						+ pDibQuad->rgbGreen * pDibQuad->rgbGreen
						+ pDibQuad->rgbBlue * pDibQuad->rgbBlue;
			int nGray = (int)sqrt(((double)lSquareSum) / 3);
			pLP->palPalEntry[i].peRed = nGray;
			pLP->palPalEntry[i].peGreen = nGray;
			pLP->palPalEntry[i].peBlue = nGray;
			pLP->palPalEntry[i].peFlags = 0;
		}
		pal.CreatePalette( pLP );
		delete[] pLP;
		pOldPalette = pDC->SelectPalette(&pal, FALSE);		// Select the palette
		pDC->RealizePalette();
	}
	else if((pDC->GetDeviceCaps(RASTERCAPS) & RC_PALETTE) == 0 && nColors <= 256 ){
		// Device does not supports palettes but bitmap has a color table Modify the bitmaps color table directly
		// Note : This ends up changing the DIB. If that is not acceptable then copy the DIB and then change the copy rather than the original
  	LPRGBQUAD pDibQuad = (LPRGBQUAD) m_lpvColorTable;
		for(int i = 0; i < nColors; i++){
			long lSquareSum = pDibQuad->rgbRed * pDibQuad->rgbRed
						+ pDibQuad->rgbGreen * pDibQuad->rgbGreen
						+ pDibQuad->rgbBlue	* pDibQuad->rgbBlue;
			int nGray = (int)sqrt(((double)lSquareSum) / 3);
			pDibQuad->rgbRed = nGray;
			pDibQuad->rgbGreen = nGray;
			pDibQuad->rgbBlue = nGray;
		}
	}
	pDC->SetStretchBltMode(COLORONCOLOR);
	if(dibsize == rcDest.Size())	bSuccess = ::SetDIBitsToDevice(	pDC->GetSafeHdc(),rcDest.left,rcDest.top,	rcDest.Width(),rcDest.Height(),	ptSrc.x,ptSrc.y,0,rcDest.Height(),m_lpImage,(LPBITMAPINFO)m_lpBMIH,	DIB_RGB_COLORS);
	else bSuccess = ::StretchDIBits(pDC->GetSafeHdc(),rcDest.left,rcDest.top,rcDest.Width(),rcDest.Height(),ptSrc.x,ptSrc.y,m_lpBMIH->biWidth,m_lpBMIH->biHeight,m_lpImage,(LPBITMAPINFO)m_lpBMIH,DIB_RGB_COLORS,	rop);
	if(pOldPalette != nullptr) pDC->SelectPalette(pOldPalette, FALSE);
  return bSuccess;
}

HBITMAP CDib::CreateSection(CDC* pDC)
{
	if(m_lpBMIH == NULL) return NULL;
	if(m_lpImage != NULL) return NULL; // can only do this if image doesn't exist
	m_hBitmap = ::CreateDIBSection(pDC->GetSafeHdc(), (LPBITMAPINFO) m_lpBMIH, DIB_RGB_COLORS,	(LPVOID*) &m_lpImage, NULL, 0);
	ASSERT(m_lpImage != NULL);
  MakeRows();
	return m_hBitmap;
}

void CDib::EraseDib()
{
  int iRowLength = 4 * ((m_lpBMIH->biWidth * m_lpBMIH->biBitCount + 31) / 32);
  ZeroMemory(m_lpImage, m_lpBMIH->biHeight * iRowLength);
}

void CDib::ScrollDib(bool MoveUp/*=false*/)
{
  if(MoveUp) MoveMemory(m_ppRows[m_lpBMIH->biHeight - 1], m_ppRows[m_lpBMIH->biHeight - 2], m_RowLength * (m_lpBMIH->biHeight - 1));
  else MoveMemory(m_ppRows[m_lpBMIH->biHeight - 2], m_ppRows[m_lpBMIH->biHeight - 1], m_RowLength * (m_lpBMIH->biHeight - 1));
}

void CDib::ScrollDib(int LinesToScroll)
{
  if(LinesToScroll > 0) MoveMemory(m_ppRows[m_lpBMIH->biHeight - LinesToScroll - 1], m_ppRows[m_lpBMIH->biHeight - 1], m_RowLength * (m_lpBMIH->biHeight - LinesToScroll));
  else if(LinesToScroll < 0) MoveMemory(m_ppRows[m_lpBMIH->biHeight - 1], m_ppRows[m_lpBMIH->biHeight + LinesToScroll - 1], m_RowLength * (m_lpBMIH->biHeight + LinesToScroll));
}

BOOL CDib::MakeRows()
{ // Allocate an array of pointers to each row in the DIB
int y, cy;

  if(m_ppRows != NULL) delete m_ppRows;
  cy = m_lpBMIH->biHeight;
  m_ppRows = (PBYTE*)malloc(sizeof(BYTE*) * cy); 
  if(!m_ppRows) return FALSE;
    // Initialize them.
  m_RowLength = 4 * ((m_lpBMIH->biWidth * m_lpBMIH->biBitCount + 31) / 32); 
  if(m_lpBMIH->biHeight > 0) for(y = 0 ; y < cy ; y++) m_ppRows[y] = m_lpImage + (cy - y - 1) * m_RowLength;  // ie, bottom up
  else for(y = 0 ; y < cy ; y++) m_ppRows[y] = m_lpImage + y * m_RowLength ;  // top down
  m_HalfRowLength = m_RowLength / 2;
  return TRUE;
}

BYTE* CDib::GetRowPtr(int y, int Position/*=RP_START*/)
{
  if(y < 0 || y >= m_lpBMIH->biHeight) return NULL;
  switch(Position){
    default:
    case RP_LEFT:
      return m_ppRows[y];
    case RP_MIDDLEOF3:
      return m_ppRows[y] + m_RowLength / 3 - 3;
    case RP_MIDDLEOF2:
      return m_ppRows[y] + m_RowLength / 2;
    case RP_RIGHT:
      return m_ppRows[y] + m_RowLength - 1;
  }
}

BYTE* CDib::GetPixelPtr(int x, int y)
{
  if(x < 0 || x >= m_lpBMIH->biWidth || y < 0 || y >= m_lpBMIH->biHeight) return 0;
  return m_ppRows[y] + (x * m_lpBMIH->biBitCount >> 3);
}

BOOL CDib::MakePalette()
{
	// makes a logical palette (m_hPalette) from the DIB's color table
	// this palette will be selected and realized prior to drawing the DIB
	if(m_nColorTableEntries == 0) return FALSE;
	if(m_hPalette != NULL) ::DeleteObject(m_hPalette);
	TRACE("CDib::MakePalette -- m_nColorTableEntries = %d\n", m_nColorTableEntries);
	LPLOGPALETTE pLogPal = (LPLOGPALETTE) new char[2 * sizeof(WORD) +	m_nColorTableEntries * sizeof(PALETTEENTRY)];
	pLogPal->palVersion = 0x300;
	pLogPal->palNumEntries = m_nColorTableEntries;
	LPRGBQUAD pDibQuad = (LPRGBQUAD) m_lpvColorTable;
	for(int i = 0; i < m_nColorTableEntries; i++){
		pLogPal->palPalEntry[i].peRed = pDibQuad->rgbRed;
		pLogPal->palPalEntry[i].peGreen = pDibQuad->rgbGreen;
		pLogPal->palPalEntry[i].peBlue = pDibQuad->rgbBlue;
		pLogPal->palPalEntry[i].peFlags = 0;
		pDibQuad++;
	}
	m_hPalette = ::CreatePalette(pLogPal);
	delete pLogPal;
	return TRUE;
}	

BOOL CDib::SetSystemPalette(CDC* pDC)
{
	// if the DIB doesn't have a color table, we can use the system palette
	if(m_nColorTableEntries != 0) return FALSE;
	HDC hDC = pDC->GetSafeHdc();
	if(!(::GetDeviceCaps(hDC, RASTERCAPS) & RC_PALETTE)) return FALSE;
	int nSysColors = ::GetDeviceCaps(hDC, NUMCOLORS);
	int nPalEntries = ::GetDeviceCaps(hDC, SIZEPALETTE);
	ASSERT(nPalEntries <= 256);
	int nEntries = (nPalEntries == 0) ? nSysColors : nPalEntries;
	LPLOGPALETTE pLogPal = (LPLOGPALETTE) new char[2 * sizeof(WORD) +	nEntries * sizeof(PALETTEENTRY)];
	pLogPal->palVersion = 0x300;
	pLogPal->palNumEntries = nEntries;
	::GetSystemPaletteEntries(hDC, 0, nEntries,	(LPPALETTEENTRY) ((LPBYTE) pLogPal + 2 * sizeof(WORD)));
	m_hPalette = ::CreatePalette(pLogPal);
	delete pLogPal;
	return TRUE;
}

HBITMAP CDib::CreateBitmap(CDC* pDC)
{
  if (m_dwSizeImage == 0) return NULL;
  HBITMAP hBitmap = ::CreateDIBitmap(pDC->GetSafeHdc(), m_lpBMIH, CBM_INIT, m_lpImage, (LPBITMAPINFO) m_lpBMIH, DIB_RGB_COLORS);
  ASSERT(hBitmap);
  return hBitmap;
}

BOOL CDib::Compress(CDC* pDC, BOOL bCompress /* = TRUE*/ )
{
	// 1. makes GDI bitmap from existing DIB
	// 2. makes a new DIB from GDI bitmap with compression
	// 3. cleans up the original DIB
	// 4. puts the new DIB in the object
	if((m_lpBMIH->biBitCount != 4) && (m_lpBMIH->biBitCount != 8)) return FALSE;
		// compression supported only for 4 bpp and 8 bpp DIBs
	if(m_hBitmap) return FALSE; // can't compress a DIB Section!
	TRACE("Compress: original palette size = %d\n", m_nColorTableEntries); 
	HDC hdc = pDC->GetSafeHdc();
	HPALETTE hOldPalette = ::SelectPalette(hdc, m_hPalette, FALSE);
	HBITMAP hBitmap;  // temporary
	if((hBitmap = CreateBitmap(pDC)) == NULL) return FALSE;
	int nSize = sizeof(BITMAPINFOHEADER) + sizeof(RGBQUAD) * m_nColorTableEntries;
	LPBITMAPINFOHEADER lpBMIH = (LPBITMAPINFOHEADER) new char[nSize];
	memcpy(lpBMIH, m_lpBMIH, nSize);  // new header
	if(bCompress){
		switch(lpBMIH->biBitCount){
		  case 4:
			  lpBMIH->biCompression = BI_RLE4;
			  break;
		  case 8:
			  lpBMIH->biCompression = BI_RLE8;
			  break;
		  default:
			  ASSERT(FALSE);
		}
		// calls GetDIBits with null data pointer to get size of compressed DIB
		if(!::GetDIBits(pDC->GetSafeHdc(), hBitmap, 0, (UINT) lpBMIH->biHeight,	NULL, (LPBITMAPINFO) lpBMIH, DIB_RGB_COLORS)){
			AfxMessageBox(_T("Unable to compress this DIB"));
			// probably a problem with the color table
	 		::DeleteObject(hBitmap);
			delete []lpBMIH;
			::SelectPalette(hdc, hOldPalette, FALSE);
			return FALSE; 
		}
		if(lpBMIH->biSizeImage == 0){
			AfxMessageBox(_T("Driver can't do compression"));
	 		::DeleteObject(hBitmap);
			delete [] lpBMIH;
			::SelectPalette(hdc, hOldPalette, FALSE);
			return FALSE; 
		}
		else m_dwSizeImage = lpBMIH->biSizeImage;
	}
	else{
		lpBMIH->biCompression = BI_RGB; // decompress
		// figure the image size from the bitmap width and height
		DWORD dwBytes = ((DWORD) lpBMIH->biWidth * lpBMIH->biBitCount) / 32;
		if(((DWORD) lpBMIH->biWidth * lpBMIH->biBitCount) % 32) dwBytes++;
		dwBytes *= 4;
		m_dwSizeImage = dwBytes * lpBMIH->biHeight; // no compression
		lpBMIH->biSizeImage = m_dwSizeImage;
	} 
	// second GetDIBits call to make DIB
	LPBYTE lpImage = (LPBYTE) new char[m_dwSizeImage];
	VERIFY(::GetDIBits(pDC->GetSafeHdc(), hBitmap, 0, (UINT) lpBMIH->biHeight, lpImage, (LPBITMAPINFO) lpBMIH, DIB_RGB_COLORS));
  TRACE("dib successfully created - height = %d\n", lpBMIH->biHeight);
	::DeleteObject(hBitmap);
	Empty();
	m_nBmihAlloc = m_nImageAlloc = crtAlloc;
	m_lpBMIH = lpBMIH;
	m_lpImage = lpImage;
	ComputePaletteSize(m_lpBMIH->biBitCount);
	ComputeMetrics();
	MakePalette();
	::SelectPalette(hdc, hOldPalette, FALSE);
	TRACE("Compress: new palette size = %d\n", m_nColorTableEntries); 
	return TRUE;
}

BOOL CDib::Read(CFile* pFile)
{
	// 1. read file header to get size of info hdr + color table
	// 2. read info hdr (to get image size) and color table
	// 3. read image
	// can't use bfSize in file header
	Empty();
	int nCount, nSize;
	BITMAPFILEHEADER bmfh;
	TRY{
		nCount = pFile->Read((LPVOID) &bmfh, sizeof(BITMAPFILEHEADER));
		if(nCount != sizeof(BITMAPFILEHEADER)){
			AfxMessageBox(_T("read error 1"));
			return FALSE;
		}
		if(bmfh.bfType != 0x4d42){
			AfxMessageBox(_T("Invalid bitmap file"));
			return FALSE;
		}
		nSize = bmfh.bfOffBits - sizeof(BITMAPFILEHEADER);
		m_lpBMIH = (LPBITMAPINFOHEADER) new char[nSize];
		m_nBmihAlloc = m_nImageAlloc = crtAlloc;
		nCount = pFile->Read(m_lpBMIH, nSize); // info hdr & color table
		ComputeMetrics();
		m_lpImage = (LPBYTE) new char[m_dwSizeImage];
		nCount = pFile->Read(m_lpImage, m_dwSizeImage); // image only
	}
	CATCH(CException, e){
		AfxMessageBox(_T("Read error 1"));
		return FALSE;
	}
	END_CATCH
	ComputePaletteSize(m_lpBMIH->biBitCount);
	MakePalette();
  MakeRows();
	return TRUE;
}

BOOL CDib::Write(CFile* pFile)
{
	BITMAPFILEHEADER bmfh;
	bmfh.bfType = 0x4d42;  // 'BM'
	int nSize =  sizeof(BITMAPINFOHEADER) + sizeof(RGBQUAD) * m_nColorTableEntries;
	bmfh.bfSize = nSize + sizeof(BITMAPFILEHEADER);	// meaning of bfSize open to interpretation (bytes, words, dwords?) -- we won't use it
	bmfh.bfReserved1 = bmfh.bfReserved2 = 0;
	bmfh.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + sizeof(RGBQUAD) * m_nColorTableEntries;	
	TRY{
		pFile->Write((LPVOID) &bmfh, sizeof(BITMAPFILEHEADER));
		pFile->Write((LPVOID) m_lpBMIH, nSize);
		pFile->Write((LPVOID) m_lpImage, m_dwSizeImage);
	}
	CATCH(CException, e){
		AfxMessageBox(_T("write error"));
		return FALSE;
	}
	END_CATCH
	return TRUE;
}

void CDib::Serialize(CArchive& ar)
{
	ar.Flush();
	if(ar.IsStoring()) Write(ar.GetFile());
	else Read(ar.GetFile());
}

// helper functions
void CDib::ComputePaletteSize(int nBitCount)
{
	if((m_lpBMIH == NULL) || (m_lpBMIH->biClrUsed == 0)){
		switch(nBitCount){
			case 1:
				m_nColorTableEntries = 2;
				break;
			case 4:
				m_nColorTableEntries = 16;
				break;
			case 8:
				m_nColorTableEntries = 256;
				break;
			case 16:
			case 24:
			case 32:
				m_nColorTableEntries = 0;
				break;
			default:
				ASSERT(FALSE);
		}
	}
  else	m_nColorTableEntries = m_lpBMIH->biClrUsed;
	ASSERT(m_nColorTableEntries <= 256); 
}

void CDib::ComputeMetrics()
{
	m_dwSizeImage = m_lpBMIH->biSizeImage;
	if(m_dwSizeImage == 0){
		DWORD dwBytes = ((DWORD) m_lpBMIH->biWidth * m_lpBMIH->biBitCount) / 32;
		if(((DWORD) m_lpBMIH->biWidth * m_lpBMIH->biBitCount) % 32) dwBytes++;
		dwBytes *= 4;
		m_dwSizeImage = dwBytes * m_lpBMIH->biHeight; // no compression
	}
	m_lpvColorTable = (LPBYTE) m_lpBMIH + sizeof(BITMAPINFOHEADER);
}

void CDib::Empty()
{
	// this is supposed to clean up whatever is in the DIB
	DetachMapFile();
  if(m_ppRows != NULL) delete m_ppRows;
  m_ppRows = NULL;
	if(m_nBmihAlloc == crtAlloc) delete [] m_lpBMIH;
	else if(m_nBmihAlloc == heapAlloc){
		::GlobalUnlock(m_hGlobal);
		::GlobalFree(m_hGlobal);
	}
	if(m_nImageAlloc == crtAlloc) delete [] m_lpImage;
	if(m_hPalette != NULL) ::DeleteObject(m_hPalette);
	if(m_hBitmap != NULL) ::DeleteObject(m_hBitmap);
	m_nBmihAlloc = m_nImageAlloc = noAlloc;
	m_hGlobal = NULL;
	m_lpBMIH = NULL;
	m_lpImage = NULL;
	m_lpvColorTable = NULL;
	m_nColorTableEntries = 0;
	m_dwSizeImage = 0;
	m_lpvFile = NULL;
	m_hMap = NULL;
	m_hFile = NULL;
	m_hBitmap = NULL;
	m_hPalette = NULL;
  m_RowLength = 0;
  m_HalfRowLength = 0;
}
