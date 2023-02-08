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
#include "./Basic/BasicDefs.h"
#include <sys/locking.h>
#include <share.h>
#include <stdlib.h>
#include <stdio.h>

const int					m_OpenMode[4] = { 0, O_RDONLY, O_WRONLY, O_RDWR };

const int					BinTxt[MT_MAXTYPE] = {_O_BINARY, _O_BINARY, _O_TEXT, _O_BINARY, _O_TEXT };
const char				FileTypes[MT_MAXTYPE][6] = { _T(".prog"), _T(".data"), _T(".keys"), _T(".all"), _T(".bas")};  

// These are common to all instances of CMassStorage
FileTable_s				**CMassStorage::m_ppFileTable = nullptr;

//////////////////////////////////////////////////////////////////////

CMassStorage::CMassStorage(void) : CDevice()
{
	m_IsMultiUse = true;
}

//////////////////////////////////////////////////////////////////////

CMassStorage::CMassStorage(CString Name, CString Dir) : CDevice(Name)
{
	m_IsMultiUse = true;
	m_RootDir = Dir;
}

//////////////////////////////////////////////////////////////////////

CMassStorage::~CMassStorage(void)
{
}

//////////////////////////////////////////////////////////////////////

void CMassStorage::Initialise(void)
{
	m_pDoc = static_cast<CS45BasicDoc*>(g_pBasicDoc);
	if(m_ppFileTable == nullptr) NewFCB();
}

//////////////////////////////////////////////////////////////////////

void CMassStorage::Destroy(void)
{
	if(m_ppFileTable != nullptr) DestroyFCB(nullptr);
}

//////////////////////////////////////////////////////////////////////

inline FileTable_s** CMassStorage::NewFCB(void)
{
FileTable_s **ppFileTable = m_ppFileTable;

	m_ppFileTable = (FileTable_s**)malloc(sizeof(FileTable_s*) * DEF_FCB_COUNT);			// create a FCB table for the subcontext
	for(int i = 0; i < DEF_FCB_COUNT; ++i) m_ppFileTable[i] = nullptr;								// mark as all unused
	return ppFileTable;																																// return old FCB, can be null
}

//////////////////////////////////////////////////////////////////////

inline void CMassStorage::DestroyFCB(FileTable_s **ppFileTable)
{
	CloseAll();																																			 // make sure no files are open
	free(m_ppFileTable);																														 // free up existing FCB
	m_ppFileTable = ppFileTable;																										 // restore old FCB, can be null
}

//////////////////////////////////////////////////////////////////////

inline void CMassStorage::	ClearBuffer(FileTable_s *pFile)
{
	pFile->DrainPos = 0;
	pFile->FillPos = 0;
	pFile->Count = 0;
	memset(pFile->pBuffer, 0, pFile->RecordLength);
}

//////////////////////////////////////////////////////////////////////

int CMassStorage::GetFCB(int FileIndex)
{
	if((FileIndex < DEF_FCB_COUNT) && (m_ppFileTable[FileIndex] != nullptr)) Close(FileIndex);								// assume the programmer has finished with the previous assignment
	m_ppFileTable[FileIndex] = (FileTable_s*)malloc(sizeof(FileTable_s));		// allocate file control block
	memset(m_ppFileTable[FileIndex], 0, sizeof(FileTable_s));
	return DEV_OK;
}

//////////////////////////////////////////////////////////////////////

int CMassStorage::IsOpen(int FileIndex, eOpenType Mode)
{
BOOL Error = FALSE;

	if((FileIndex >= DEF_FCB_COUNT) || (m_ppFileTable[FileIndex] == nullptr) || (m_ppFileTable[FileIndex]->Handle == -1)){
		g_SufixErrorMsg.AppendPrintf(_T("file #%d not open"), FileIndex);
		return DEV_ERROR;
	}
	switch(Mode){
		case eOpenType::READ:{
			break;
		}
		case eOpenType::RANDOM:
		case eOpenType::WRITE:{
			if((m_ppFileTable[FileIndex]->OpenType != eOpenType::WRITE) && (m_ppFileTable[FileIndex]->OpenType != eOpenType::RANDOM)){
				g_SufixErrorMsg.AppendPrintf(_T("file #%d not opened for writing"), FileIndex);
				return DEV_ERROR;
			}
			break;
		}
		default:
			return DEV_ERROR;
	}
	return DEV_OK;
}

//////////////////////////////////////////////////////////////////////

int CMassStorage::Close(int FileIndex)
{
	if(m_ppFileTable[FileIndex] == nullptr)	return SetErrorMessage(_T("file not open"));
	switch(m_ppFileTable[FileIndex]->OpenType){
		case eOpenType::READ:{														
			_close(m_ppFileTable[FileIndex]->Handle);
			break;
		}
		case eOpenType::WRITE:{														
			Flush(FileIndex);
			_close(m_ppFileTable[FileIndex]->Handle);
			break;
		}
		case eOpenType::RANDOM:{													
			_close(m_ppFileTable[FileIndex]->Handle);
			break;
		}
		default:
			return DEV_ERROR;
	}
	if(m_ppFileTable[FileIndex]->IsBuffered) free(m_ppFileTable[FileIndex]->pBuffer);		
	free(m_ppFileTable[FileIndex]);
	m_ppFileTable[FileIndex] = nullptr;
	g_SufixErrorMsg.Clear();
	return DEV_OK;
}

//////////////////////////////////////////////////////////////////////

void CMassStorage::CloseAll(void)		// close all active streams 
{
	for(int i = 0; i < DEF_FCB_COUNT; ++i) if(m_ppFileTable[i] != nullptr) Close(i);
}

//////////////////////////////////////////////////////////////////////

int CMassStorage::SetVerify(int FileIndex)
{
	if(m_ppFileTable[FileIndex] == nullptr) return SetErrorMessage(_T("file not open"));
	m_ppFileTable[FileIndex]->Verify = true;		
	return DEV_OK;
}

//////////////////////////////////////////////////////////////////////

void CMassStorage::VerifyAll(void)
{
	for(int i = 0; i < DEF_FCB_COUNT; ++i) if(m_ppFileTable[i] != nullptr) SetVerify(i);
}

////////////////////////////////////////////////////////////////////////////////////

int CMassStorage::BuildFileName(BString *pFilePath, const char *pName, const char *pFileType)
{
char *pChr;

	pFilePath->AppendChars(GetRootDir());
	if(pFilePath != nullptr) pFilePath->AppendChars(pName);
	if(pFilePath->GetLength() && ((pChr = strchr(pFilePath->GetBuffer(), '.')) != nullptr)){
		int Length = pFilePath->GetBuffer() - pChr;
		if(Length) if(pFilePath->Resize(Length) == -1) return DEV_ERROR;
	}
	pFilePath->AppendChars(pFileType);
	return DEV_OK;
}

//////////////////////////////////////////////////////////////////////

int CMassStorage::Create(const char *pName, int RecordCount, int RecordLength)
{
int Handle;
BString FilePath;

	if(BuildFileName(&FilePath, pName, FileTypes[MT_DATA]) == -1) GetErrorMessage(FALSE);
	_sopen_s(&Handle, FilePath.GetBuffer(), _O_WRONLY |_O_TRUNC | _O_CREAT | BinTxt[MT_DATA], _SH_DENYNO, _S_IWRITE);
	if(Handle == -1) return GetErrorMessage(FALSE);
	if(_write(Handle, &RecordCount, sizeof(int)) != sizeof(int)) return GetErrorMessage(FALSE);
	if(_write(Handle, &RecordLength, sizeof(int)) !=  sizeof(int)) return GetErrorMessage(FALSE);
	_close(Handle);
	return DEV_OK;
}

//////////////////////////////////////////////////////////////////////

int CMassStorage::ReadOpen(const char *pName, const int FileType, bool Buffered)											
{
int FileIndex = 0;			// system FCB
int Handle;
BString FilePath;

	if(BuildFileName(&FilePath, pName, FileTypes[FileType]) == -1) GetErrorMessage(FALSE);
	_sopen_s(&Handle, FilePath.GetBuffer(), _O_RDONLY | BinTxt[FileType], _SH_DENYNO, _S_IREAD);
	if(Handle == -1) return GetErrorMessage(FALSE);
	if(GetFCB(FileIndex) == -1) return DEV_ERROR;
	m_ppFileTable[FileIndex]->Handle = Handle;
	m_ppFileTable[FileIndex]->OpenType = eOpenType::READ;
	m_ppFileTable[FileIndex]->Verify = false;
	if(Buffered){
		m_ppFileTable[FileIndex]->IsBuffered = true;
		m_ppFileTable[FileIndex]->RecordLength = FS_BUFFER_SIZE;
		m_ppFileTable[FileIndex]->pBuffer = (char*)malloc(FS_BUFFER_SIZE);
		ClearBuffer(m_ppFileTable[FileIndex]);
	}
	else m_ppFileTable[FileIndex]->IsBuffered = false;
	g_SufixErrorMsg.Clear();
	return FileIndex;
}

//////////////////////////////////////////////////////////////////////

int CMassStorage::WriteOpen(const char *pName, const int FileType, bool Buffered)	
{
int FileIndex = 0;			// system FCB
int Handle;
BString FilePath;
  
	if(BuildFileName(&FilePath, pName, FileTypes[FileType]) == -1) GetErrorMessage(FALSE);
	_sopen_s(&Handle, FilePath.GetBuffer(), _O_WRONLY |_O_TRUNC | _O_CREAT | BinTxt[FileType], _SH_DENYNO, _S_IWRITE);
	if(Handle == -1) return GetErrorMessage(FALSE);
	for(FileIndex = 1; FileIndex < DEF_FCB_COUNT; ++FileIndex) if(m_ppFileTable[FileIndex] == nullptr) break;
	if(GetFCB(FileIndex) == -1) return DEV_ERROR;
	m_ppFileTable[FileIndex]->Handle = Handle;
	m_ppFileTable[FileIndex]->OpenType = eOpenType::WRITE;
	m_ppFileTable[FileIndex]->Verify = false;
	m_ppFileTable[FileIndex]->IsBuffered = false;
	if(Buffered){
		m_ppFileTable[FileIndex]->IsBuffered = true;
		m_ppFileTable[FileIndex]->RecordLength = FS_BUFFER_SIZE;
		m_ppFileTable[FileIndex]->pBuffer = (char*)malloc(FS_BUFFER_SIZE);
		ClearBuffer(m_ppFileTable[FileIndex]);
	}
	else m_ppFileTable[FileIndex]->IsBuffered = false;
	g_SufixErrorMsg.Clear();
	return FileIndex;
}

//////////////////////////////////////////////////////////////////////

int CMassStorage::Assign(int FileIndex, const char *pName, bool Buffered)		
{
int Handle, RecordCount = 4, RecordLength = FS_BUFFER_SIZE;
BString FilePath;

	assert((FileIndex > 0) && (FileIndex < DEF_FCB_COUNT) && (pName != nullptr));
	if(GetFCB(FileIndex) == -1) return DEV_ERROR;
	if(BuildFileName(&FilePath, pName, FileTypes[MT_DATA]) == -1) return GetErrorMessage(FALSE);
	int FileStat = _sopen_s(&Handle, FilePath.GetBuffer(), _O_RDWR/* | _O_CREAT*/, _SH_DENYNO, _S_IWRITE);
	if(Handle == -1) return GetErrorMessage(FALSE);
	m_ppFileTable[FileIndex]->Handle = Handle;
	m_ppFileTable[FileIndex]->OpenType = eOpenType::RANDOM;
	m_ppFileTable[FileIndex]->Verify = false;
	if(Buffered){
		m_ppFileTable[FileIndex]->IsBuffered = true;
		m_ppFileTable[FileIndex]->RecordLength = RecordLength;
		m_ppFileTable[FileIndex]->pBuffer = (char*)malloc(RecordLength);
		ClearBuffer(m_ppFileTable[FileIndex]);
	}
	else m_ppFileTable[FileIndex]->IsBuffered = false;
	g_SufixErrorMsg.Clear();
	return FileIndex;
}

//////////////////////////////////////////////////////////////////////

int CMassStorage::ReadRecord(int FileIndex, int Record)
{
FileTable_s *pFile = m_ppFileTable[FileIndex];

	if(IsOpen(FileIndex, eOpenType::RANDOM) == DEV_ERROR) return DEV_ERROR;
	if(_lseek(pFile->Handle, pFile->RecordLength * Record, SEEK_SET)) return DEV_ERROR;
	return Refill(FileIndex);
}

//////////////////////////////////////////////////////////////////////

int CMassStorage::SetRecord(int FileIndex, int Record)
{
FileTable_s *pFile = m_ppFileTable[FileIndex];

	if(IsOpen(FileIndex, eOpenType::RANDOM) == DEV_ERROR) return DEV_ERROR;
	if(_lseek(pFile->Handle, pFile->RecordLength * Record, SEEK_SET)) return DEV_ERROR;
	ClearBuffer(pFile);
	return DEV_OK;
}

//////////////////////////////////////////////////////////////////////

int CMassStorage::Lock(int FileIndex, off_t offset, off_t length, int mode, int w)
{
int Handle;
int lockmode = _LK_UNLCK;

	if(m_ppFileTable[FileIndex] == nullptr) return SetErrorMessage(_T("file not open"));
	if((Handle = m_ppFileTable[FileIndex]->Handle) == -1) assert(0);
	switch(mode){
		case FS_LOCK_SHARED:    lockmode = _LK_RLCK; break;
		case FS_LOCK_EXCLUSIVE: lockmode = _LK_LOCK; break;
		case FS_LOCK_NONE:      lockmode = _LK_UNLCK; break;
		default: assert(0);
	}
	if((_lseek(Handle, offset, SEEK_SET) != offset) || (_locking(Handle, lockmode, length) == -1)) return GetErrorMessage(FALSE);
	return DEV_OK;
}

//////////////////////////////////////////////////////////////////////

int CMassStorage::Truncate(int FileIndex)
{
int Handle;
off_t Offset;

	if(m_ppFileTable[FileIndex] == nullptr) return SetErrorMessage(_T("file not open"));
	if((Handle = m_ppFileTable[FileIndex]->Handle) == -1) assert(0);
	if((Offset = _lseek(Handle, 0, SEEK_CUR)) != 0 || _chsize(Handle, Offset + 1) == -1) return GetErrorMessage(FALSE);
	return DEV_OK;
}

//////////////////////////////////////////////////////////////////////

int CMassStorage::GetFillPos(int FileIndex)
{
	if(m_ppFileTable[FileIndex] == nullptr) return SetErrorMessage(_T("file not open"));
	return m_ppFileTable[FileIndex]->FillPos;
}

//////////////////////////////////////////////////////////////////////

int CMassStorage::Flush(int FileIndex)
{
FileTable_s *pFile;
UINT Written;
size_t Offset;

	pFile = m_ppFileTable[FileIndex];
	if(pFile == nullptr) return SetErrorMessage(_T("file not open"));
	if(!IsBuffered(FileIndex)) return SetErrorMessage(_T("Invalid file operation"));
	Offset = 0;
	while(Offset < pFile->FillPos){
		Written = _write(pFile->Handle, pFile->pBuffer + Offset,  pFile->FillPos - Offset);
		if(Written == 0) return GetErrorMessage(FALSE);
		else Offset += Written;
	}
	ClearBuffer(pFile);
	g_SufixErrorMsg.Clear();
	return DEV_OK;
}

//////////////////////////////////////////////////////////////////////

int CMassStorage::Put(int FileIndex, char Chr)
{
	if((IsOpen(FileIndex, eOpenType::RANDOM) == DEV_ERROR) || !IsBuffered(FileIndex)) return DEV_ERROR;
	FileTable_s *pFile = m_ppFileTable[FileIndex];
	pFile->pBuffer[pFile->FillPos++] = Chr;
	if(pFile->FillPos >= pFile->RecordLength)	return (Flush(FileIndex) == DEV_ERROR) ? DEV_ERROR : DEV_EOR;
	g_SufixErrorMsg.Clear();
	return DEV_OK;
}

//////////////////////////////////////////////////////////////////////

int CMassStorage::PutBytes(int FileIndex, const char *pNum, int Count)
{
	if(IsOpen(FileIndex, eOpenType::WRITE) == DEV_ERROR) return DEV_ERROR;
	FileTable_s *pFile = m_ppFileTable[FileIndex];
	if(pFile->FillPos + Count	>= pFile->RecordLength) return DEV_EOR;
	for(int i = Count - 1; i >= 0; i--){
		if(Put(FileIndex, *(pNum + i)) == DEV_ERROR) return DEV_ERROR;
	}
	return DEV_OK;
}

//////////////////////////////////////////////////////////////////////

int CMassStorage::PrintChars(int FileIndex, const char *pChars)
{
	if(IsOpen(FileIndex, eOpenType::WRITE) == DEV_ERROR) return DEV_ERROR;
	while(*pChars) if(Put(FileIndex, *pChars++) == DEV_ERROR) return DEV_ERROR;
	return DEV_OK;
}

//////////////////////////////////////////////////////////////////////

int CMassStorage::PrintString(int FileIndex, BString *pStr)
{
UINT Len = pStr->GetLength();
const char *c = pStr->GetBuffer();

	if(IsOpen(FileIndex, eOpenType::WRITE) == DEV_ERROR) return DEV_ERROR;
	while(Len){
		if(Put(FileIndex, *c++) == DEV_ERROR) return DEV_ERROR;
		else --Len;
	}
	return DEV_OK;
}

//////////////////////////////////////////////////////////////////////

int CMassStorage::PrintStringData(int FileIndex, bool Random, BString *pStr)
{
FileTable_s *pFile = m_ppFileTable[FileIndex];
USHORT Len = 0, Type, Count = 0;
bool FirstPart = true;

	if(IsOpen(FileIndex, eOpenType::RANDOM) == DEV_ERROR) return DEV_ERROR;
	Len = pStr->GetLength();
	if(Random){
		if((pFile->FillPos + pStr->GetLength() + 4) >= pFile->RecordLength) return SetErrorMessage(_T("End of record")); // make sure it'll fit
		Type = MD_STRING_FULL_PART;	
		if(PutBytes(FileIndex, (char*)&Type, 2) == DEV_ERROR) return DEV_ERROR;													// specify the type as full string
		if(PutBytes(FileIndex, (char*)&Len, 2) == DEV_ERROR) return DEV_ERROR;													// specify the string length
		if(PutBytes(FileIndex, pStr->GetBuffer(), pStr->GetLength()) == DEV_ERROR) return DEV_ERROR;		// write the string
		return DEV_OK;
	}
	do{
		if((pFile->FillPos + Len + 4) < pFile->RecordLength){																					// select the appropriate substring type
			if(FirstPart)	Type = MD_STRING_FULL_PART;	
			else Type = MD_STRING_LAST_PART;	
			if(PutBytes(FileIndex, (char*)&Type, 2) == DEV_ERROR) return DEV_ERROR;												// specify the type as full string
			if(PutBytes(FileIndex, (char*)&Len, 2) == DEV_ERROR) return DEV_ERROR;												// specify the string length
			if(PutBytes(FileIndex, pStr->GetBuffer(), Len) == DEV_ERROR) return DEV_ERROR;										// write the string
			return DEV_OK;
		}
		if(FirstPart)	Type = MD_STRING_FIRST_PART;	
		else Type = MD_STRING_MIDDLE_PART;
		Count = pFile->RecordLength - (pFile->FillPos + 4);																						// write as much as possible
		if(PutBytes(FileIndex, (char*)&Type, 2) == DEV_ERROR) return DEV_ERROR;													// specify the type as full string
		if(PutBytes(FileIndex, (char*)&Len, 2) == DEV_ERROR) return DEV_ERROR;													// specify the total length left
		if(PutBytes(FileIndex, pStr->GetBuffer(), Count) == DEV_ERROR) return DEV_ERROR;										// write the string
		Len -= Count;
	}
	while(1);
	return DEV_OK;
}

//////////////////////////////////////////////////////////////////////

int CMassStorage::PrintIntegerData(int FileIndex, bool Random, long Value)
{
FileTable_s *pFile = m_ppFileTable[FileIndex];
char *pVal = (char*)&Value;
USHORT Type = MD_INTEGER_VALUE;

	if(IsOpen(FileIndex, eOpenType::RANDOM) == DEV_ERROR) return DEV_ERROR;
	if(Random){
		if((pFile->FillPos + 4) >= pFile->RecordLength) return SetErrorMessage(_T("End of record"));	// make sure it'll fit
		if(PutBytes(FileIndex, (char*)&Type, 2) == DEV_ERROR) return DEV_ERROR;													// specify the type as integer
		if(PutBytes(FileIndex, pVal, 2) == DEV_ERROR) return DEV_ERROR;																	// specify the string length
		return DEV_OK;
	}
	if((pFile->FillPos + 4) >= pFile->RecordLength) if(Flush(FileIndex) == DEV_ERROR) return DEV_ERROR;	// make sure it'll fit, else next record
	if(PutBytes(FileIndex, (char*)&Type, 2) == DEV_ERROR) return DEV_ERROR;														// specify the type as integer
	if(PutBytes(FileIndex, pVal, 2) == DEV_ERROR) return DEV_ERROR;																		// specify the string length
	return DEV_OK;
}

//////////////////////////////////////////////////////////////////////

int CMassStorage::PrintRealData(int FileIndex, bool Random, double Value)
{
FileTable_s *pFile = m_ppFileTable[FileIndex];
char *pVal = (char*)&Value;
USHORT Type = MD_INTEGER_VALUE;

	if(IsOpen(FileIndex, eOpenType::RANDOM) == DEV_ERROR) return DEV_ERROR;
	if(Random){
		if((pFile->FillPos + 6) >= pFile->RecordLength) return SetErrorMessage(_T("End of record"));	// make sure it'll fit
		if(PutBytes(FileIndex, (char*)&Type, 2) == DEV_ERROR) return DEV_ERROR;													// specify the type as integer
		if(PutBytes(FileIndex, pVal, 4) == DEV_ERROR) return DEV_ERROR;																	// specify the string length
		return DEV_OK;
	}
	if((pFile->FillPos + 6) >= pFile->RecordLength) if(Flush(FileIndex) == DEV_ERROR) return DEV_ERROR;	// make sure it'll fit, else next record
	if(PutBytes(FileIndex, (char*)&Type, 2) == DEV_ERROR) return DEV_ERROR;														// specify the type as integer
	if(PutBytes(FileIndex, pVal, 4) == DEV_ERROR) return DEV_ERROR;																		// specify the string length
	return DEV_OK;
}

//////////////////////////////////////////////////////////////////////

int CMassStorage::PrintArrayData(int FileIndex, bool Random, Var_s *pVar)
{
int Result = -1;

	if(IsOpen(FileIndex, eOpenType::RANDOM) == DEV_ERROR) return DEV_ERROR;
	for(int i = 0; i < pVar->Size; ++i){
		switch(pVar->pValue[i].Type){
			case eVType::V_INT: Result = PrintIntegerData(FileIndex, Random, pVar->pValue[i].uData.Integer); break;
			case eVType::V_REAL: Result = PrintRealData(FileIndex, Random, pVar->pValue[i].uData.Real); break;
			case eVType::V_STRING: Result = PrintStringData(FileIndex, Random, pVar->pValue[i].uData.pString); break;
			default: assert(0);
		}
		if(Result == -1) return DEV_ERROR;																				
	}
	return DEV_OK;
}

//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////

int CMassStorage::GetDrainPos(int FileIndex)
{
	if(m_ppFileTable[FileIndex] == nullptr) return SetErrorMessage(_T("file not open"));
	return m_ppFileTable[FileIndex]->DrainPos;
}

//////////////////////////////////////////////////////////////////////

int CMassStorage::Refill(int FileIndex)
{
FileTable_s *pFile;
UINT len;

	pFile = m_ppFileTable[FileIndex];
	if(pFile == nullptr) return SetErrorMessage(_T("file not open"));
	if(!IsBuffered(FileIndex)) return SetErrorMessage(_T("Invalid file operation"));
	pFile->DrainPos = 0;
	len = _read(pFile->Handle, pFile->pBuffer, pFile->RecordLength);
	if(len <= 0){
		pFile->Count = 0;
		return GetErrorMessage(len != -1);
	}
	pFile->Count = len;
	return DEV_OK;
}

//////////////////////////////////////////////////////////////////////

int CMassStorage::Get(int FileIndex, char *pChr)
{
	if((IsOpen(FileIndex, eOpenType::READ) == DEV_ERROR) || !IsBuffered(FileIndex)) return DEV_ERROR;
	FileTable_s *pFile = m_ppFileTable[FileIndex];
	g_SufixErrorMsg.Clear();
	if(pFile->Count == 0) if(Refill(FileIndex) == DEV_ERROR) return DEV_ERROR;									// get the first fill
	*pChr = pFile->pBuffer[pFile->DrainPos++];
	if(pFile->DrainPos >= pFile->Count) return (Refill(FileIndex) == DEV_ERROR) ? DEV_ERROR : DEV_EOR;	// refill if all of buffer read
	return DEV_OK;
}	

//////////////////////////////////////////////////////////////////////

int CMassStorage::GetBytes(int FileIndex, char *pNum, int Count)
{
  for(int i = Count - 1; i >= 0; i--){
		if(Get(FileIndex, (pNum + i)) == DEV_ERROR) return DEV_ERROR;															// file format is little-endian
	}
	return DEV_OK;
}

//////////////////////////////////////////////////////////////////////

int CMassStorage::ReadString(int FileIndex, BString *pStr, char StopChar)										 // read a string looking for a terminating character (eg. source file)
{
UINT CopyLen, Pos;
char Chr;
FileTable_s *pFile = m_ppFileTable[FileIndex];

	do{
		Pos = pFile->DrainPos;
		while(1){
			if(Pos == pFile->Count) break;																																	// at end of buffer
			Chr = pFile->pBuffer[Pos++];																																		// get the character
			if(Chr == '\n' || Chr == StopChar){																															// is it a termination character
				pFile->pBuffer[Pos - 1]	= '\n';																																// make sure it's a LF character
				break;
			}
		}
		CopyLen = Pos - pFile->DrainPos;																																	// calculate the number of characters to copy
		if(CopyLen){
			size_t offset = pStr->GetLength();																															// get the number of characters in the string already
			if(pStr->Resize(offset + CopyLen) == -1)	return GetErrorMessage(FALSE);												// resize the string to accomodate the new characters
			memcpy(pStr->GetBuffer() + offset, pFile->pBuffer + pFile->DrainPos, CopyLen);									// append the new characters
			pStr->SetLength(pStr->GetLength() + CopyLen);																										// get the number of characters in the string already
			pFile->DrainPos += CopyLen;																																			// adjust the read position of the source buffer
			if(pFile->pBuffer[Pos - 1] == '\n' || pFile->pBuffer[Pos - 1] == StopChar) return DEV_OK;				// have we got to the end of the source line? No... 
		}
		if(Refill(FileIndex) == DEV_ERROR) return (g_SufixErrorMsg.IsEmpty() ? DEV_EOF : DEV_ERROR);			// ...then we need to get more
	}
	while(1);
	return DEV_ERROR;
}	

//////////////////////////////////////////////////////////////////////

int CMassStorage::ReadStringData(int FileIndex, bool Random, BString *pStr)
{
FileTable_s *pFile = m_ppFileTable[FileIndex];
UINT Pos = pFile->DrainPos, Res;
USHORT Length = 0, Type, Lenchk;
char Chr;

	if(IsOpen(FileIndex, eOpenType::RANDOM) == DEV_ERROR) return DEV_ERROR;
	do{
		if(GetBytes(FileIndex, (char*)&Type, 2) == DEV_ERROR) return DEV_ERROR;
		if(Random && (Type != MD_STRING_FULL_PART)) return SetErrorMessage(_T("End of record"));			// the string must fit into 1 record for random reads
		switch(Type){
			case MD_STRING_MIDDLE_PART:{																																// get the next part of a string that stradles logical records
				if(GetBytes(FileIndex, (char*)&Lenchk, 2) == DEV_ERROR) return DEV_ERROR;										// get the block length of the string
				if((Lenchk + pStr->GetLength() ) > Length) return DEV_ERROR;																// test for abnormal block length
				while(pStr->GetLength() < Length){																												// get the bytes until an error or EOR marker
					Res = Get(FileIndex, &Chr);																															// read the data
					if((Res != DEV_OK) && (Res != DEV_EOR)){																									// check for errors or EOF
						if(Res == DEV_ERROR) return GetErrorMessage(FALSE);
						return SetErrorMessage(_T("End of file"));
					}
					pStr->Add(Chr);
					if(Res == DEV_EOR) break;
				}
				if(Length % 2) Get(FileIndex, &Chr);																											// file is word aligned
				if(pStr->GetLength() >= Length) return DEV_ERROR;																					// loop to get rest of string
				break;
			}
			case MD_STRING_FIRST_PART:{																																	// get the first part of a string that stradles logical records
				if(GetBytes(FileIndex, (char*)&Length, 2) == DEV_ERROR) return DEV_ERROR;										// get the total length of the string
				if(Length > pStr->GetMaxSize()) return DEV_ERROR;																					// test for abnormal length
				pStr->Clear();
				if(pStr->Resize(Length) == -1)	return GetErrorMessage(FALSE);														// resize the string to accomodate the new characters
				while(pStr->GetLength() < Length){																												// get the bytes until an error or EOR marker
					Res = Get(FileIndex, &Chr);																															// read the data
					if((Res != DEV_OK) && (Res != DEV_EOR)){																									// check for errors or EOF
						if(Res == DEV_ERROR) return GetErrorMessage(FALSE);
						return SetErrorMessage(_T("End of file"));
					}
					pStr->Add(Chr);
					if(Res == DEV_EOR) break;
				}
				if(Length % 2) Get(FileIndex, &Chr);																											// file is word aligned
				if(pStr->GetLength() >= Length) return DEV_ERROR;																					// loop to get rest of string
				break;
			}
			case MD_STRING_LAST_PART:{																																	// get the last part of a string that stradles logical records
				if(GetBytes(FileIndex, (char*)&Lenchk, 2) == DEV_ERROR) return DEV_ERROR;										// get the block length of the string
				if((Lenchk + pStr->GetLength()) > Length) return DEV_ERROR;																// test for abnormal block length
				while(pStr->GetLength()  < Length){																												// get the bytes until an error or EOR marker
					Res = Get(FileIndex, &Chr);																															// read the data
					if((Res != DEV_OK) && (Res != DEV_EOR)){																									// check for errors or EOF
						if(Res == DEV_ERROR) return GetErrorMessage(FALSE);
						return SetErrorMessage(_T("End of file"));
					}
					pStr->Add(Chr);
  				if(Res == DEV_EOR) break;
				}
				if(Length % 2) Get(FileIndex, &Chr);																											// file is word aligned
				return DEV_OK;
			}
			case MD_STRING_FULL_PART:{																																	// get a string that is contain within a logical record
				if(GetBytes(FileIndex, (char*)&Length, 2) == DEV_ERROR) return DEV_ERROR;										// get the total length of the string
				if(Length > pStr->GetMaxSize()) return DEV_ERROR;																					// test for abnormal length
				pStr->Clear();
				if(pStr->Resize(Length) == -1)	return GetErrorMessage(FALSE);														// resize the string to accomodate the new characters
				while(pStr->GetLength() < Length){																												// get the bytes until an error or EOR marker
					Res = Get(FileIndex, &Chr);																															// read the data
					if((Res != DEV_OK) && (Res != DEV_EOR)){																									// check for errors or EOF
						if(Res == DEV_ERROR) return GetErrorMessage(FALSE);																		// get the bytes until an error or EOR marker. An EOR is an error
						return SetErrorMessage(_T("End of file"));
					}
					pStr->Add(Chr);
				}
				if(Length % 2) Get(FileIndex, &Chr);																											// file is word aligned
				return DEV_OK;
			}
			default:{
				break;
			}
		}
	}
	while(1);
	return DEV_ERROR;
}

//////////////////////////////////////////////////////////////////////

int CMassStorage::ReadIntegerData(int FileIndex, bool Random, long *pValue)
{
FileTable_s *pFile = m_ppFileTable[FileIndex];
char *pVal = (char*)pValue;
USHORT Type;

	if(IsOpen(FileIndex, eOpenType::RANDOM) == DEV_ERROR) return DEV_ERROR;
	if(GetBytes(FileIndex, (char*)&Type, 2) == DEV_ERROR) return DEV_ERROR;
	if(Type != MD_INTEGER_VALUE) return SetErrorMessage(_T("Invalid type"));
	if(GetBytes(FileIndex, pVal, 2) == DEV_ERROR) return DEV_ERROR;											// get the value
	return DEV_OK;
}

//////////////////////////////////////////////////////////////////////

int CMassStorage::ReadRealData(int FileIndex, bool Random, double *pValue)
{
FileTable_s *pFile = m_ppFileTable[FileIndex];
char *pVal = (char*)pValue;
USHORT Type;

	if(IsOpen(FileIndex, eOpenType::RANDOM) == DEV_ERROR) return DEV_ERROR;
	if(GetBytes(FileIndex, (char*)&Type, 2) == DEV_ERROR) return DEV_ERROR;
	if(Type != MD_FULLFLOAT_VALUE) return SetErrorMessage(_T("Invalid type"));
	if(GetBytes(FileIndex, pVal, 4) == DEV_ERROR) return DEV_ERROR;											// get the value
	return DEV_OK;
}

//////////////////////////////////////////////////////////////////////

int CMassStorage::ReadArrayData(int FileIndex, bool Random, Var_s *pVar)
{
UINT Result = -1;

	if(IsOpen(FileIndex, eOpenType::RANDOM) == DEV_ERROR) return DEV_ERROR;
	for(int i = 0; i < pVar->Size; ++i){
		switch(pVar->pValue[i].Type){
			case eVType::V_INT: Result = ReadIntegerData(FileIndex, Random, &pVar->pValue[i].uData.Integer); break;
			case eVType::V_REAL: Result = ReadRealData(FileIndex, Random, &pVar->pValue[i].uData.Real); break;
			case eVType::V_STRING: Result = ReadStringData(FileIndex, Random, pVar->pValue[i].uData.pString); break;
			default: assert(0);
		}
		if(Result == -1) return DEV_ERROR;																				
	}
	return DEV_OK;
}

//////////////////////////////////////////////////////////////////////

int CMassStorage::Copy(const char *pSrcName, const char *pDestName)
{
int InHandle, OutHandle;
char buf[4096];
UINT inlen, outlen = -1;

	_sopen_s(&InHandle, pSrcName, _O_RDONLY, _SH_DENYNO, _S_IWRITE);
	if(InHandle == -1) return GetErrorMessage(FALSE);
	_sopen_s(&OutHandle, pDestName, _O_CREAT | _O_WRONLY | _O_TRUNC, _SH_DENYNO, _S_IWRITE);
	if(OutHandle == -1)	return GetErrorMessage(FALSE);
	while((inlen = _read(InHandle, &buf, sizeof(buf))) > 0){
		UINT off = 0;
		while(inlen && (outlen = _write(OutHandle, &buf + off, inlen)) > 0){
			off += outlen;
			inlen -= outlen;
		}
		if(outlen == -1){
			_close(InHandle);
			_close(OutHandle);
			return GetErrorMessage(FALSE);
		}
	}
	if(inlen == -1){
		_close(InHandle);
		_close(OutHandle);
		return GetErrorMessage(FALSE);
	}
	if(_close(InHandle) == -1){
		_close(OutHandle);
		return GetErrorMessage(FALSE);
	}
	if(_close(OutHandle) == -1)	return GetErrorMessage(FALSE);
	return DEV_OK;
}

//////////////////////////////////////////////////////////////////////

int CMassStorage::Cat(void)
{
WIN32_FIND_DATA ffd;
LARGE_INTEGER filesize;
CString cstr;
HANDLE hFind = INVALID_HANDLE_VALUE;
DWORD dwError=0;
int cp;

	cstr = GetRootDir();
	cstr += _T("\\*");
	hFind = FindFirstFile(cstr, &ffd);
	if(INVALID_HANDLE_VALUE == hFind)	return DEV_ERROR;
	g_pPrinter->PrintChars(_T("   NAME        TYPE      SIZE\r\n"));
	g_pPrinter->PrintFormat(_T("  %s\r\n\n"), GetDevName());
	do{
		if(ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY){
			g_pPrinter->PrintFormat(_T("  %s"), ffd.cFileName);													// there were no directories on type drives
			g_pPrinter->SetCursorColumn(15);																						// however for clarity we show them here
			g_pPrinter->PrintFormat(_T("<DIR>\r\n"));																		
		}
		else{
			filesize.LowPart = ffd.nFileSizeLow;
			filesize.HighPart = ffd.nFileSizeHigh;
			cstr = ffd.cFileName;
			cstr.MakeUpper();
			if((cp = cstr.Find('.')) != -1){
				g_pPrinter->PrintFormat(_T("  %s"), cstr.Left(cp++));											// extract the file name...
				g_pPrinter->SetCursorColumn(15);
				g_pPrinter->PrintFormat(_T("%s"), cstr.Right(cstr.GetLength() - cp));			// ...and then the type
			}
			else g_pPrinter->PrintFormat(_T("  %s"), ffd.cFileName);
			g_pPrinter->SetCursorColumn(25);
			g_pPrinter->PrintFormat(_T("%ld\r\n"), filesize.QuadPart);						// lastly print the size
		}
	}
  while(FindNextFile(hFind, &ffd) != 0);
	return DEV_OK;
}

//////////////////////////////////////////////////////////////////////

int CMassStorage::Rename(const char *pFrom, const char *pTo)
{
	if(rename(pFrom, pTo) == -1) return GetErrorMessage(FALSE);
  return DEV_OK;
}

//////////////////////////////////////////////////////////////////////

int CMassStorage::PrintTokens(int FileIndex, Token_s *pToken)
{
Token_s *pTok;
USHORT Len;

	pTok = pToken;
	if(IsOpen(FileIndex, eOpenType::WRITE) == DEV_ERROR) return DEV_ERROR;
	do{
		if(_write(m_ppFileTable[FileIndex]->Handle, pTok, sizeof(Token_s)) != sizeof(Token_s)) return DEV_ERROR;
		switch(pTok->Type){
			case T_DATAINPUT:{
				Len = (USHORT)strlen(pTok->Obj.pDataInput) + 1;
				if(_write(m_ppFileTable[FileIndex]->Handle, &Len, 2) != 2) return DEV_ERROR;
				if(_write(m_ppFileTable[FileIndex]->Handle, pTok->Obj.pDataInput, Len) != Len) return DEV_ERROR;
				break;
			}
			case T_IDENTIFIER:{
				Len = (USHORT)strlen(pTok->Obj.pIdentifier->Name) + 1;
				if(_write(m_ppFileTable[FileIndex]->Handle, &Len, 2) != 2) return DEV_ERROR;
				if(_write(m_ppFileTable[FileIndex]->Handle, &pTok->Obj.pIdentifier->Name, Len) != Len) return DEV_ERROR;
				break;
			}
			case T_LABEL:{
				Len = (USHORT)strlen(pTok->Obj.pLabel->pName) + 1;
				if(_write(m_ppFileTable[FileIndex]->Handle, &Len, 2) != 2) return DEV_ERROR;
				if(_write(m_ppFileTable[FileIndex]->Handle, pTok->Obj.pLabel->pName, Len) != Len) return DEV_ERROR;
				break;
			}
			case T_REM:{
				Len = (USHORT)strlen(pTok->Obj.pRem) + 1;
				if(_write(m_ppFileTable[FileIndex]->Handle, &Len, 2) != 2) return DEV_ERROR;
				if(_write(m_ppFileTable[FileIndex]->Handle, pTok->Obj.pRem, Len) != Len) return DEV_ERROR;
				break;
			}
			case T_STRING:{
				Len = (USHORT)strlen(pTok->Obj.pString->GetBuffer()) + 1;
				if(_write(m_ppFileTable[FileIndex]->Handle, &Len, 2) != 2) return DEV_ERROR;
				if(_write(m_ppFileTable[FileIndex]->Handle, pTok->Obj.pString->GetBuffer(), Len) != Len) return DEV_ERROR;
				break;
			}
		}
	}
	while(pTok++->Type != T_EOL);
	return DEV_OK;
}

//////////////////////////////////////////////////////////////////////

int CMassStorage::ReadTokens(int FileIndex, Token_s **pBase)
{
Token_s Token, *pToken;
UINT TokCount = 0, RecOrgn, Count;
USHORT Len;

	if(IsOpen(FileIndex, eOpenType::READ) == DEV_ERROR) return DEV_ERROR;
	RecOrgn = _tell(m_ppFileTable[FileIndex]->Handle);
	do{
		if((Count = _read(m_ppFileTable[FileIndex]->Handle, &Token, sizeof(Token_s))) == -1) return DEV_ERROR;
		if(Count == 0) break;
		switch(Token.Type){
			case T_DATAINPUT:
			case T_IDENTIFIER:
			case T_LABEL:
			case T_REM:
			case T_STRING:{
				if(_read(m_ppFileTable[FileIndex]->Handle, &Len, 2) != 2) return DEV_ERROR;
				if(_lseek(m_ppFileTable[FileIndex]->Handle, Len, SEEK_CUR) == -1) return DEV_ERROR;
				break;
			}
		}
		++TokCount;
	}
	while(Token.Type != T_EOL);
 	if(_lseek(m_ppFileTable[FileIndex]->Handle, RecOrgn, SEEK_SET) == -1) return DEV_ERROR;
	pToken = *pBase = (Token_s*)malloc(sizeof(struct Token_s) * TokCount);
	for(UINT i = 0; i < TokCount; ++i){
		if(_read(m_ppFileTable[FileIndex]->Handle, pToken, sizeof(Token_s)) != sizeof(Token_s)) return DEV_ERROR;
		switch(pToken->Type){
			case T_ASSIGN:				pToken->pStatement = stmt_ASSIGN; break;
			case T_AXES:					pToken->pStatement = stmt_AXES; break;
			case T_BEEP:					pToken->pStatement = stmt_BEEP; break;
			case T_CALL:					pToken->pStatement = stmt_CALL; break;
			case T_CASEELSE:
			case T_CASE:{
				pToken->pStatement = stmt_CASE;
				pToken->Obj.pCaseValue = (Casevalue_s*)malloc(sizeof(Casevalue_s));
				break;
			}
			case T_CAT:
			case T_CATHASH:
			case T_CAT_TO:				pToken->pStatement = stmt_CAT; break;
			case T_CHDIR:					pToken->pStatement = stmt_FOLDER; break;
			case T_CHECK_READ:		pToken->pStatement = stmt_CHECK_READ; break;
			case T_CLEAR:					pToken->pStatement = stmt_CLEAR; break;
			case T_CLIP:					pToken->pStatement = stmt_CLIP; break;
			case T_CLOSE:					pToken->pStatement = stmt_CLOSE; break;
			case T_CLS:						pToken->pStatement = stmt_CLS; break;
			case T_COLOUR:				pToken->pStatement = stmt_COLOUR; break;
			case T_COM:						pToken->pStatement = stmt_COM; break;
			case T_COPY:					pToken->pStatement = stmt_COPY_RENAME; break;
			case T_CREATE: 				pToken->pStatement = stmt_CREATE; break;
			case T_CREATEDIR: 		pToken->pStatement = stmt_FOLDER; break;
			case T_CSIZE:					pToken->pStatement = stmt_CSIZE; break;
			case T_CURSOR:				pToken->pStatement = stmt_CURSOR; break;
			case T_DATA:					pToken->pStatement = stmt_DATA; break;
			case T_DATAINPUT:{
				if(_read(m_ppFileTable[FileIndex]->Handle, &Len, 2) != 2) return DEV_ERROR;
				if((pToken->Obj.pDataInput = (char*)malloc(Len)) == nullptr) return DEV_ERROR;
				if(_read(m_ppFileTable[FileIndex]->Handle, pToken->Obj.pDataInput, Len) != Len) return DEV_ERROR;
				break;
			}
			case T_DEFAULT_OFF:		pToken->pStatement = stmt_DEFAULT; break;
			case T_DEFAULT_ON:		pToken->pStatement = stmt_DEFAULT; break;
			case T_DEFDBL:				pToken->pStatement = stmt_DEFINT_DEFDBL_DEFSTR; break;
			case T_DEFINT:				pToken->pStatement = stmt_DEFINT_DEFDBL_DEFSTR; break;
			case T_DEFSTR:				pToken->pStatement = stmt_DEFINT_DEFDBL_DEFSTR; break;
			case T_DEFFN:{
				pToken->pStatement = stmt_DEFFN; break;
				pToken->Obj.pLocalSyms = nullptr;
				break;
			}
			case T_DEG:						pToken->pStatement = stmt_TRIGMODE; break;
			case T_DELETE:				pToken->pStatement = stmt_DELETE; break;
			case T_DELAY:					pToken->pStatement = stmt_DELAY; break;
			case T_DIM:						pToken->pStatement = stmt_DIM; break;
			case T_DIGITIZE:			pToken->pStatement = stmt_DIGITIZE; break;
			case T_DISABLE:				pToken->pStatement = stmt_ENABLE_DISABLE; break;
			case T_DISP:					pToken->pStatement = stmt_PRINT_LPRINT; break;
			case T_DO:						pToken->pStatement = stmt_DO; break;
			case T_DOUNTIL:				pToken->pStatement = stmt_DO_CONDITION; break;
			case T_DOWHILE:				pToken->pStatement = stmt_DO_CONDITION; break;
			case T_DRAW:					pToken->pStatement = stmt_PLOT; break;
			case T_DUMP:					pToken->pStatement = stmt_DUMP; break;
			case T_ELSE:					pToken->pStatement = stmt_ELSE_ELSEIFELSE; break;
			case T_ELSEIFELSE: 		pToken->pStatement = stmt_ELSE_ELSEIFELSE; break;
			case T_ELSEIFIF:			pToken->pStatement = stmt_IF_ELSEIFIF; break;
			case T_ENABLE:				pToken->pStatement = stmt_ENABLE_DISABLE; break;
			case T_ENDIF:					pToken->pStatement = stmt_ENDIF; break;
			case T_ENDPROC:				pToken->pStatement = stmt_SUBEND; break;
			case T_ENDSELECT:			pToken->pStatement = stmt_ENDSELECT; break;
			case T_END:						pToken->pStatement = stmt_END; break;
			case T_EOL:						pToken->pStatement = stmt_COLON_EOL; break;
			case T_EQ:						pToken->pStatement = stmt_EQ_FNEND; break;
			case T_EXECUTE:				pToken->pStatement = stmt_EXECUTE; break;
			case T_EXITDO:				pToken->pStatement = stmt_EXITDO; break;
			case T_EXITFOR:				pToken->pStatement = stmt_EXITFOR; break;
			case T_EXITGRAPH: 		pToken->pStatement = stmt_GRAPHICS; break;
			case T_FIXED:					pToken->pStatement = stmt_FIXED; break;
			case T_FLOAT:					pToken->pStatement = stmt_FLOAT; break;
			case T_FNEND:					pToken->pStatement = stmt_EQ_FNEND; break;
			case T_FOR:						pToken->pStatement = stmt_FOR; break;
			case T_FRAME:					pToken->pStatement = stmt_FRAME; break;
			case T_GCHARSET:			pToken->pStatement = stmt_GCHARSET; break;
			case T_GCLEAR:				pToken->pStatement = stmt_GCLEAR; break;
			case T_GET:						pToken->pStatement = stmt_GET; break;
			case T_GLABEL:				pToken->pStatement = stmt_PRINT_LPRINT; break;
			case T_GLOAD:					pToken->pStatement = stmt_GLOAD; break;
			case T_GOSUB:					pToken->pStatement = stmt_GOSUB; break;
			case T_GOTO:					pToken->pStatement = stmt_GOTO_RESUME; break;
			case T_GPLOT:					pToken->pStatement = stmt_GPLOT; break;
			case T_GPRINT:	  		pToken->pStatement = stmt_GPRINT; break;
			case T_GRAD:					pToken->pStatement = stmt_TRIGMODE; break;
			case T_GRAPHICS:			pToken->pStatement = stmt_GRAPHICS; break;
			case T_GRID:					pToken->pStatement = stmt_GRID; break;
			case T_IDENTIFIER:{
				pToken->pStatement = stmt_IDENTIFIER;
				if(_read(m_ppFileTable[FileIndex]->Handle, &Len, 2) != 2) return DEV_ERROR;
				pToken->Obj.pIdentifier = (Identifier_s*)malloc(sizeof(Identifier_s) + Len - 2);
				if(pToken->Obj.pIdentifier == nullptr) return DEV_ERROR;
				if(_read(m_ppFileTable[FileIndex]->Handle, &pToken->Obj.pIdentifier->Name, Len) != Len) return DEV_ERROR;
				switch(pToken->Obj.pIdentifier->Name[Len - 1]){
					case '$': pToken->Obj.pIdentifier->DefType = eVType::V_STRING; break;
					case '%': pToken->Obj.pIdentifier->DefType = eVType::V_INT; break;
					default: pToken->Obj.pIdentifier->DefType = eVType::V_REAL; break;
				}
				break;
			}
			case T_IF:						pToken->pStatement = stmt_IF_ELSEIFIF; break;
			case T_IMAGE:					pToken->pStatement = stmt_IMAGE; break;
			case T_INPUT:					pToken->pStatement = stmt_INPUT; break;
			case T_INTEGER:				pToken->pStatement = stmt_NUMERIC; break;
 			case T_IPLOT:					pToken->pStatement = stmt_PLOT; break;
			case T_LABEL:{
				pToken->pStatement = stmt_LABEL;
				if(_read(m_ppFileTable[FileIndex]->Handle, &Len, 2) != 2) return DEV_ERROR;
				pToken->Obj.pLabel = (Label_s*)malloc(sizeof(Label_s));
				if(pToken->Obj.pLabel == nullptr) return DEV_ERROR;
				pToken->Obj.pLabel->pName = (char*)malloc(Len);
				if(pToken->Obj.pLabel->pName == nullptr) return DEV_ERROR;
				if(_read(m_ppFileTable[FileIndex]->Handle, pToken->Obj.pLabel->pName, Len) != Len) return DEV_ERROR;
				break;
			}
			case T_LDIR:					pToken->pStatement = stmt_LDIR; break;
			case T_LET:						pToken->pStatement = stmt_LET; break;
			case T_LETTER:				pToken->pStatement = stmt_LETTER; break;
			case T_LIMIT:					pToken->pStatement = stmt_LIMIT; break;
			case T_LINEINPUT:			pToken->pStatement = stmt_INPUT; break;
			case T_LINETYPE:			pToken->pStatement = stmt_LINETYPE; break;
			case T_LINK: 					pToken->pStatement = stmt_LINK; break;
			case T_LIST:					pToken->pStatement = stmt_LIST; break;
			case T_LOAD:					pToken->pStatement = stmt_LOAD; break;
			case T_LOCAL:					pToken->pStatement = stmt_LOCAL; break;
			case T_LOCATE:				pToken->pStatement = stmt_LOCATE; break;
			case T_LOCK:					pToken->pStatement = stmt_LOCK_UNLOCK; break;
			case T_LOOP:					pToken->pStatement = stmt_LOOP; break;
			case T_LOOPUNTIL:			pToken->pStatement = stmt_LOOPUNTIL; break;
			case T_LORG:					pToken->pStatement = stmt_LORG; break;
			case T_LPRINT:				pToken->pStatement = stmt_PRINT_LPRINT; break;
			case T_LSET:					pToken->pStatement = stmt_LSET_RSET; break;
			case T_MASSTORAGEIS:	pToken->pStatement = stmt_MASSTORAGEIS; break;
			case T_MATINPUT:			pToken->pStatement = stmt_MATINPUT; break;
			case T_MATPRINT:			pToken->pStatement = stmt_MATPRINT; break;
			case T_MATREAD:				pToken->pStatement = stmt_MATREAD; break;
			case T_MAT:						pToken->pStatement = stmt_MAT; break;
			case T_MOVE:					pToken->pStatement = stmt_MOVE; break;
		  case T_MSCALE:				pToken->pStatement = stmt_SCALE; break;
			case T_NAME:					pToken->pStatement = stmt_NAME; break;
			case T_NEXT:{
				pToken->pStatement = stmt_NEXT;
				pToken->Obj.pNext = (Next_s*)malloc(sizeof(Next_s));
				break;
			}
			case T_NORMAL:				pToken->pStatement = stmt_TRACE; break;
			case T_OFFKEY:				pToken->pStatement = stmt_OFFKEY; break;
			case T_ONERROROFF:		pToken->pStatement = stmt_ONERROROFF;	 break;
			case T_ONERROR:				pToken->pStatement = stmt_ONERROR; break;
			case T_ONKEY:					pToken->pStatement = stmt_ONKEY; break;
			case T_ON:{
				pToken->pStatement = stmt_ON;
				pToken->Obj.On.PcLength = 1;
				pToken->Obj.On.pPc = nullptr;
				break;
			}
			case T_OPTIONBASE:		pToken->pStatement = stmt_OPTIONBASE; break;
			case T_OUT:						pToken->pStatement = stmt_OUTPUT; break;
			case T_PAUSE:					pToken->pStatement = stmt_PAUSE; break;
			case T_PDIR:					pToken->pStatement = stmt_PDIR; break;
			case T_PEN:						pToken->pStatement = stmt_PEN; break;
			case T_PENUP:					pToken->pStatement = stmt_PENUP; break;
			case T_PLOT:					pToken->pStatement = stmt_PLOT; break;
			case T_PLOTTERIS:			pToken->pStatement = stmt_PLOTTERIS; break;
			case T_POINTER:				pToken->pStatement = stmt_POINTER; break;
			case T_PRINT:					pToken->pStatement = stmt_PRINT_LPRINT; break;
			case T_PRINTALLIS:		pToken->pStatement = stmt_PRINTERIS; break;
			case T_PRINTERIS:			pToken->pStatement = stmt_PRINTERIS; break;
			case T_RAD:						pToken->pStatement = stmt_TRIGMODE; break;
			case T_RANDOMIZE:			pToken->pStatement = stmt_RANDOMIZE; break;
			case T_RATIO:					pToken->pStatement = stmt_RATIO; break;
			case T_READ:					pToken->pStatement = stmt_READ; break;
			case T_REAL:					pToken->pStatement = stmt_NUMERIC; break;
			case T_REDIM:					pToken->pStatement = stmt_REDIM; break;
			case T_REM:{
				pToken->pStatement = stmt_QUOTE_REM;
				if(_read(m_ppFileTable[FileIndex]->Handle, &Len, 2) != 2) return DEV_ERROR;
				pToken->Obj.pRem = (char*)malloc(Len);
				if(pToken->Obj.pRem == nullptr) return DEV_ERROR;
				if(_read(m_ppFileTable[FileIndex]->Handle, pToken->Obj.pRem, Len) != Len) return DEV_ERROR;
				break;
			}
			case T_RENAME:				pToken->pStatement = stmt_COPY_RENAME; break;
			case T_RENUM:					pToken->pStatement = stmt_RENUM; break;
			case T_REPEAT:				pToken->pStatement = stmt_REPEAT; break;
			case T_RESTORE:				pToken->pStatement = stmt_RESTORE; break;
			case T_RESUME:				pToken->pStatement = stmt_GOTO_RESUME; break;
			case T_RETURN:				pToken->pStatement = stmt_RETURN; break;
			case T_RPLOT:					pToken->pStatement = stmt_PLOT; break;
			case T_RSET:					pToken->pStatement = stmt_LSET_RSET; break;
			case T_RUN:						pToken->pStatement = stmt_RUN; break;
			case T_SAVE:					pToken->pStatement = stmt_SAVE; break;
			case T_SCALE:					pToken->pStatement = stmt_SCALE; break;
			case T_SCRATCH:
		  case T_SCRATCH_A:
			case T_SCRATCH_C:			pToken->pStatement = stmt_SCRATCH; break;
			case T_SELECT:{
				pToken->pStatement = stmt_SELECT;
				pToken->Obj.pSelectCase = (Selectcase_s*)malloc(sizeof(Selectcase_s));
				break;
			}
			case T_SHORT:					pToken->pStatement = stmt_NUMERIC; break;
			case T_SHOW:					pToken->pStatement = stmt_SHOW; break;
			case T_STANDARD:			pToken->pStatement = stmt_STANDARD; break;
			case T_STOP:					pToken->pStatement = stmt_STOP; break;
			case T_STORE:					pToken->pStatement = stmt_STORE; break;
			case T_STRING:{
				if(_read(m_ppFileTable[FileIndex]->Handle, &Len, 2) != 2) return DEV_ERROR;
				pToken->Obj.pString = new BString;
				if(pToken->Obj.pString == nullptr) return DEV_ERROR;
				pToken->Obj.pString->Resize(Len);
				if(_read(m_ppFileTable[FileIndex]->Handle, pToken->Obj.pString->GetBuffer(), Len) != Len) return DEV_ERROR;
				break;
			}
			case T_SUBEND:				pToken->pStatement = stmt_SUBEND; break;
			case T_SUBEXIT:				pToken->pStatement = stmt_SUBEXIT; break;
			case T_SUB:{
				pToken->pStatement = stmt_SUBDEF;
				pToken->Obj.pLocalSyms = nullptr;
				break;
			}
			case T_SYSTEM:				pToken->pStatement = stmt_SYSTEM; break;
			case T_TRACE:					pToken->pStatement = stmt_TRACE; break;
			case T_TRALL:					pToken->pStatement = stmt_TRACE; break;
			case T_TRPAUSE:				pToken->pStatement = stmt_TRACE; break;
			case T_TRVARS:				pToken->pStatement = stmt_TRACE; break;
			case T_TRALLVARS:			pToken->pStatement = stmt_TRACE; break;
			case T_TRWAIT:				pToken->pStatement = stmt_TRACE; break;
			case T_TRUNCATE:			pToken->pStatement = stmt_TRUNCATE; break;
			case T_UNCLIP:				pToken->pStatement = stmt_UNCLIP; break;
			case T_UNLOCK:				pToken->pStatement = stmt_LOCK_UNLOCK; break;
			case T_UNNUM:					pToken->pStatement = stmt_UNNUM; break;
			case T_UNTIL:					pToken->pStatement = stmt_UNTIL; break;
			case T_WAIT:					pToken->pStatement = stmt_WAIT; break;
			case T_WEND:{
				pToken->pStatement = stmt_WEND;
				pToken->Obj.pWhilePc = (PC_s*)malloc(sizeof(PC_s));
				break;
			}
			case T_WHERE:					pToken->pStatement = stmt_WHERE; break;
			case T_WHILE:{
				pToken->pStatement = stmt_WHILE;
				pToken->Obj.pAfterWendPc = (PC_s*)malloc(sizeof(PC_s));
				break;
			}
			case T_WRITE:					pToken->pStatement = stmt_WRITE; break;
		}
		++pToken;
	}
	return DEV_OK;
}

