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

#include "resource.h"
#include "MainFrm.h"
#include "S45BasicDefs.h"
#include "S45BasicView.h"
#include "./Basic/BasicDefs.h"
#include "CDib08.h"

class CMainFrame;
class CS45BasicDoc;
class CS45BasicView;
class StringList;

//////////////////////////////////////////////////////////////////////////////////////////////

class CRecallList
{
public:
													CRecallList() noexcept;
	virtual									~CRecallList();
	void										Append(Message_s *pMsg);
	BString*								Recall(bool IsShift);

protected: 
	StringList							m_RecallList;
	StringListNode 				 *m_pCurrentNode;
	bool										m_NewNode;
};

//////////////////////////////////////////////////////////////////////////

class CSoftKeyToken : public CObject
{
public:
													CSoftKeyToken();
													CSoftKeyToken(const CSoftKeyToken &Src);
													CSoftKeyToken(int tok);
													CSoftKeyToken(CString text);
	virtual									~CSoftKeyToken();
	bool 										IsType(int Token);
	int											GetToken(void){ return m_Token; };
	CString									GetString(void);
	void										SetString(char *pBuffer, int Count);
	void										Clone(const CSoftKeyToken *pSrc);																															// clone another string object

	void 										operator=(const CSoftKeyToken &Src) throw();

  enum KEY_TOKENS_e {
		KT_UNDEFINED = 0,
		KT_CLEAR,
		KT_CLEAR_LINE,
		KT_LEFT_ARROW,
		KT_RIGHT_ARROW,
		KT_UP_ARROW,
		KT_DOWN_ARROW,
		KT_INS_CHAR,
		KT_DEL_CHAR,
		KT_HOME,
		KT_EXECUTE,
		KT_TEXT,
		KT_MAX
	};


#ifdef _DEBUG
	virtual void						AssertValid() const;
  void										Dump(CDumpContext& dc) const;
#endif

private:
	static char							m_TokenNames[KT_MAX][20];

protected: 
	int											m_Token;
	CString									m_Text;

	DECLARE_DYNCREATE(CSoftKeyToken)

};

//////////////////////////////////////////////////////////////////////////////////////////////

class CS45BasicDoc : public CDocument
{
protected: // create from serialization only
													CS45BasicDoc() noexcept;
	virtual									~CS45BasicDoc();
	DECLARE_DYNCREATE(CS45BasicDoc)

public:
	void										InitialiseDocument();
	CMainFrame							*GetMainFrame(){ return m_pWnd; };
	CS45BasicView						*GetSystemView(){ return m_pMainView; };
	void                    UpdateViews(CView* pSender, LPARAM lHint=0L, CObject* pHint=NULL);
	int											ChangeDisplayMode(void);
	COLORREF								GetBackColour(){ return RGB(32, 32, 32);};
	COLORREF								GetForeColour(){ return RGB(0, 200, 0);};
	void										ClearScrollArea(void);
	void										ClearAllRgns(void);
  void										PrintFormatAt(int m_PrintRgn, LPCSTR lpStrFmt, ...);
  void										PrintRgnAt(int m_PrintRgn, CString Str);
	void										PrintScrollRgn(ExTextRow *pRow);
	int											GetScrollLineCount(void){ return m_ScrollLines.GetCount(); };
	int											NextScrollLine(ExTextRow *pRow);
	int											GetRowAt(int RowIndex, ExTextRow *pRow);
	int											SetRowIndex(int RowIndex, ExTextRow *pRow = nullptr);
	int											GetLinePos(void){ return m_RowWriteIndex; };
	void										SetRowVisible(int RowIndex);
	void										ClearLine(int Rgn);
  void										KeyEntry(char nChar);
  void										SpecialKeyEntry(char nChar, bool ShiftState, bool CtrlState);
	Message_s								*GetCommandStr(){ return &m_UserInputStr; };
	bool										BasicNotify(WPARAM wParam, LPARAM lParam); 
	int											FindSysCommand(char *in);
	int											GetEntryLinePosition(){ return m_EntryLinePosition[m_DisplayMode]; };
	UINT										CopyFromEditBuffer(Message_s *pMsg);
	UINT										CopyToEditBuffer(char *pBuffer);
	void										EditBufferPrint(void);
	void										ClearEditBuffer(void);
	void										SetPrinterDes(LPDEVNAMES lpDevNames);
	HGLOBAL									GetPrinterDes(HGLOBAL hDevNames);
	bool										IsPrinterDesSet(void){ if(!m_PrintDriver.IsEmpty() && !m_PrintDevice.IsEmpty() && !m_PrintPort.IsEmpty()) return true; else return false; };
	bool										OpenDirectPrint(void);
	void										CloseDirectPrint(void);
	void										EndPrint(void);
	void										PrintHardcopy(ExTextRow *pRow);
	bool										DoDirectPrint(CString Line);
	PrintRgn_t							*GetRegion(int Rgn){ if((Rgn < DISP_AREA_COUNT) && (Rgn >= 0)) return &m_PrintRgn[Rgn]; return nullptr;};
	int 										GetDisplayMode(){ return m_DisplayMode;};
	void										SetSystemMode(int NewMode);
	int 										GetSystemMode(void){ return m_SystemMode; };
	bool										SetGraphicsMode(bool Mode);
	bool									  GetGraphicsMode(void){ return m_GraphicsOn; };
	void										ResetSoftKey(UINT KeyNumber);
	void										ResetSoftKeys(void);
	int											GetVKCode(void){ return m_VKCode; };
	void										InitialiseSoftKey(UINT KeyNumber);

	CObArray								m_ScrollLines;
	CStringArray						m_EditLines;
	CObArray								m_SoftKeys[SOFT_KEY_COUNT * 2] ;
 	int 										m_RowWriteIndex;					// current scroll text line to write to
 	int											m_ScrollRowOffset;
	CSize										m_charSize;
	bool										m_bLoaded;
	volatile HANDLE					m_hLoopEvent;							// To hold loop in check when idle
	volatile HANDLE					m_hClosedEvent;						// To hold loop in check when idle
	int											m_EventType;	
	long 										m_CurrentEditIndex;
	int 										m_InsertEndNumber;
	int  										m_NumberWidth;
	CRecallList							m_RecallList;
	CStringList*						m_DirectPrintList;	
	CommParam_t							m_DirectPrintComms;
  HANDLE									m_hDirectPrintEvent;
	volatile HANDLE					m_idPrintDev;
	bool										m_CtrlKeyState;

	enum EVENT_TYPES {
		EVENT_NONE,
		EVENT_EXIT,
		EVENT_NEW_MESSAGE,
		EVENT_KEY_ACTION,
		EVENT_NULL_MESSAGE
	};

	
  enum KEY_COMMANDS {
		KC_EDIT = 0,
		KC_LIST,
		KC_STOP,
		KC_PAUSE,
		KC_CONT,
		KC_STEP,
		KC_RESULT,
		KC_CLEAR,
		KC_LINE,
		KC_KEY,
	};

	enum FUNCTION_KEYS{
		FUNCTION_KEY1 = 0,
		FUNCTION_KEY2,
		FUNCTION_KEY3,
		FUNCTION_KEY4,
		FUNCTION_KEY5,
		FUNCTION_KEY6,
		FUNCTION_KEY7,
		FUNCTION_KEY8,
		FUNCTION_KEY9,
		FUNCTION_KEY10,
	};

	enum TYPING_MODE_e{
		TM_INSERT = 0,
		TM_OVERWRITE
	};
	  

private:
	void										DoEnter(bool SaveToHistory = false);
	void										DoInsertLine(void);
	void										DoDeleteLine(void);
	void										EditLine(long LineNumber);
	void										EditExit();
	void										FormatEditScreen(long LineIndex);
	void										NextEditLine(bool SaveLine);
	void										PreviousEditLine(void);
	void										EditFillCntrl(Token_s *pToken);
	bool										EditInsertLine(void);
	int											EditNewLine(void);
	void										ProcessEditKey(int VKCode);
	void										ExitInsertLine(void);
	void										ScrollEditPage(bool UpDown = true);
	void										DoEscape(void);
	void										DoRecall(BOOL	IsShift);
	void										ProcessControlKey(int Key);
	void										SetInsertMode(enum TYPING_MODE_e Mode);
	void										ScrollLine(bool UpDown = true);
	void										ScrollPage(bool UpDown = true);
	void										InputPrepend(CString cstr);
	void										StoreLine(void);
	void										DestroyScrollLines(void);
	bool										SetupPrintConnection(void);
	void										GetCommsSpecs(void);
	void										SaveCommsSpecs(void);
 
	void										CopyTokens(CObArray& Dest, CObArray& Srce);
	void										DeleteSoftKeys(void);
	void										DestroySoftToken(CObArray& KeyTokens);
	void										EditSoftKey(int KeyNumber);
	void										FormatEditKeyScreen(int TokenIndex);
	void										EditSoftKeyCursorMove(bool left);
	void										EditSoftKeyGetPreviousToken(void);
	void										EditSoftKeyGetNextToken(void);
	void										EditSoftKeyBack(void);
	void										EditSoftKeyDelete(void);
	void										EditSoftKeyDeleteToken(void);
	void										EditSoftKeyExit(bool Stop);
	void										CheckForTextToken(void);
	void										ProcessEditSoftKey(int VKCode, bool CtrlState);
	bool										ListSoftKey(long OutDev);
	void										ProcessSoftKey(int Key);

	static char							m_SysCommands[10][20];
	static int							m_EntryLinePosition[DISP_MODE_COUNT];
	char										*m_pKeyEntryBuffer;
	Message_s								m_UserInputStr;
	PrintRgn_t							m_PrintRgn[DISP_AREA_COUNT];
	CWinThread							*m_pPrintThread;

public:
	virtual void						OnCloseDocument();
	virtual BOOL						OnNewDocument();
	virtual BOOL            OnSaveDocument(LPCTSTR lpszPathName);
	virtual BOOL            OnOpenDocument(LPCTSTR lpszPathName);
	virtual void						Serialize(CArchive& ar);

public:
#ifdef _DEBUG
	virtual void						AssertValid() const;
	virtual void						Dump(CDumpContext& dc) const;
#endif

protected:

	CMainFrame*             m_pWnd;
	CS45BasicView*					m_pMainView;
	int											m_KeyColumn;
	int 										m_KeyLineLength;
	CString									m_PrintDriver;
	CString									m_PrintDevice;
	CString									m_PrintPort;
	bool										m_ShiftMode;
	bool										m_InsertLineMode;
	int											m_InsertLineInc;
	int											m_SystemMode;
	int											m_DisplayMode;
	bool										m_GraphicsOn;
	enum TYPING_MODE_e			m_TypingMode;
	int											m_VKCode;

	CObArray								m_SoftKey;
	CSoftKeyToken						*m_pCurrentToken;
	int											m_EditSoftKeyNumber;
	int											m_CurrentTokenIndex;

	DECLARE_MESSAGE_MAP()

public:
	afx_msg void			OnSoftKey(UINT nID);
	afx_msg void			OnUpdateSoftKeyUI(CCmdUI* pCmdUI);
	afx_msg void			OnControlKey(UINT nID);
	afx_msg void			OnUpdateControlUI(CCmdUI* pCmdUI);
	afx_msg void			OnPrinterSetup();
};

//////////////////////////////////////////////////////////////////////////////////////////////

class CPrintSetup : public CDialogEx
{
	DECLARE_DYNAMIC(CPrintSetup)

public:
	CPrintSetup(CWnd* pParent = nullptr);   // standard constructor
	virtual ~CPrintSetup();
	CS45BasicDoc*		m_pDoc;
//  bool						SetInitItem(int item);
	int							m_CommType;
	bool						m_Update;

	enum { IDD = IDD_PRINT_COMMS };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnInitDialog();

	afx_msg void OnChange();
	afx_msg void OnSelchangePortcb();
	afx_msg void OnCommsProperties();
	afx_msg void OnCommtype();
	afx_msg void OnDestroy();

	DECLARE_MESSAGE_MAP()
};

//////////////////////////////////////////////////////////////////////////////////////////////

class CSerialProps : public CDialogEx
{
public:
	CSerialProps(CWnd* pParent = nullptr);   // standard constructor

	enum { IDD = IDD_SERIAL_PROPS };
 	CString				m_sBaud;
	CString				m_sDataBits;
	int						m_nParity;
	CString				m_sPort;
	int						m_nStopBits;
	BOOL					m_bDTRDSR;
	BOOL					m_bRTSCTS;
	BOOL					m_bXONXOFF;
	CommParam_t		*m_CommParams;
	bool					m_Update;
	UCHAR					m_Enable;

protected:
	virtual BOOL OnInitDialog();
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual void OnOK();

	afx_msg void OnChange();
	afx_msg void OnHardwareFlow();
	afx_msg void OnXonxoff();
	DECLARE_MESSAGE_MAP()
};

//////////////////////////////////////////////////////////////////////////////////////////////

class CParallelProps : public CDialogEx
{
public:
	CParallelProps(CWnd* pParent = nullptr);   // standard constructor

	enum { IDD = IDD_PARALLEL_PROPS };
	CString					m_sPort;
	int							m_nTimeout;
	CommParam_t			*m_CommParams;
	bool						m_Update;
	bool						m_Enable;

protected:
	virtual BOOL OnInitDialog();
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual void OnOK();

	afx_msg void OnChange();
	DECLARE_MESSAGE_MAP()
};
