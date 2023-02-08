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

#include "stdafx.h"
#include "BasicDefs.h"
#include <fcntl.h>      /* Needed only for _O_RDWR definition */
#include <io.h>

constexpr auto DEF_FCB_COUNT						= 11;               // actualy 10 (base index 1, 0 is for system use)
constexpr auto FS_MAXIMUM_FILES					= 10;

constexpr auto FS_ACCESS_NONE						= 0;
constexpr auto FS_ACCESS_READ						= 1;
constexpr auto FS_ACCESS_WRITE					= 2;
constexpr auto FS_ACCESS_READWRITE			= 3;

constexpr auto FS_LOCK_NONE							= 0;
constexpr auto FS_LOCK_SHARED						= 1;
constexpr auto FS_LOCK_EXCLUSIVE				= 2;
constexpr auto FS_BUFFER_SIZE						= 256;
constexpr auto FMSG_BUFFER_SIZE					= 128;

constexpr auto MD_INTEGER_VALUE					= 0x000A;
constexpr auto MD_FULLFLOAT_VALUE				= 0x001A;
constexpr auto MD_STRING_MIDDLE_PART		= 0x000C;
constexpr auto MD_STRING_FIRST_PART			= 0x001C;
constexpr auto MD_STRING_LAST_PART			= 0x002C;
constexpr auto MD_STRING_FULL_PART			= 0x003C;
constexpr auto MD_END_OF_RECORD					= 0x001E;
constexpr auto MD_END_OF_FILE						= 0x003E;

enum MassStorageType_e{
	MT_PROG = 0,
	MT_DATA,
	MT_KEYS,
	MT_ALL,
	MT_BAS,
	MT_MAXTYPE
};

class CMassStorage : public CDevice
{
public:
										CMassStorage();
										CMassStorage(CString Name, CString Dir);
  virtual						~CMassStorage();
 	virtual void			Initialise(void);
	virtual void			Destroy(void);

	inline FileTable_s** NewFCB(void);
	inline void				DestroyFCB(FileTable_s **ppFileTable);
 	bool							IsEOF(int FileIndex){ return _tell(m_ppFileTable[FileIndex]->Handle) >= _filelength(m_ppFileTable[FileIndex]->Handle); };
	bool							IsAssigned(int FileIndex){ return ((m_ppFileTable[FileIndex] != nullptr) && (m_ppFileTable[FileIndex]->Handle != -1)); };
	bool							IsValid(int FileIndex){ return ((FileIndex >= 1) && (FileIndex < DEF_FCB_COUNT)); };

	void							SetRootDir(CString dir){ m_RootDir = dir;};
	CString						GetRootDir(void){ return m_RootDir; };
	int 							Close(int FileIndex);
	void 							CloseAll(void);
	int								SetVerify(int FileIndex);
	void							VerifyAll(void);
	int								Create(const char *name, int RecordCount, int RecordLength);
	int 							Assign(int DevIndex, const char *pFileName, bool Buffered);
	int 							Lock(int FileIndex, off_t offset, off_t length, int mode, int w);
	int 							Truncate(int FileIndex);

	int 							WriteOpen(const char *name, const int FileType, bool Buffered);
	int								SetRecord(int FileIndex, int Record);
	int 							Flush(int FileIndex);
	int 							GetFillPos(int FileIndex);
	int 							Put(int FileIndex, char Chr);
	int								PutBytes(int FileIndex, const char *pNum, int Count);
	int 							PrintChars(int FileIndex, const char *pChars);
	int 							PrintString(int FileIndex, BString *pString);
	int 							PrintRealData(int FileIndex, bool Random, double X);
	int 							PrintStringData(int FileIndex, bool Random, BString *pString);
	int 							PrintArrayData(int FileIndex, bool Random, Var_s *pArray);
	int 							PrintIntegerData(int FileIndex, bool Random, long X);

	int 							ReadOpen(const char *name, const int FileType, bool Buffered);
	int								ReadRecord(int FileIndex, int Record);
	int 							GetDrainPos(int FileIndex);
	int 							Get(int FileIndex, char *pChr);
	int								GetBytes(int FileIndex, char *pNum, int Count);
	int 							ReadString(int FileIndex, BString *s, char stop);
	int 							ReadStringData(int FileIndex, bool Random, BString *pString);
	int 							ReadIntegerData(int FileIndex, bool Random, long *pX);
	int 							ReadRealData(int FileIndex, bool Random, double *pX);
	int 							ReadArrayData(int FileIndex, bool Random, Var_s *pArray);

	int 							Copy(const char *pSrcName, const char *pDestName);
	int								Cat(void);
  int								Rename(const char *pFrom, const char *pTo);

	int								PrintTokens(int FileIndex, Token_s *pToken);
	int								ReadTokens(int FileIndex, Token_s **pBase);

	CString						m_RootDir;

private:
	inline void					ClearBuffer(FileTable_s *pFile);
	int									GetFCB(int FileIndex);
	int									Refill(int FileIndex);
	int									BuildFileName(BString *pFilePath, const char *pName, const char *pFileType);
	int									IsOpen(int FileIndex, eOpenType Mode);
	bool								IsBuffered(int FileIndex){ return m_ppFileTable[FileIndex]->IsBuffered; };

 	static FileTable_s	**m_ppFileTable;

};

