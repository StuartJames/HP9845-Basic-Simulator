#include <winsvc.h>

constexpr auto NAME_TEMP_SIZE	 = 50;

UINT PrintThreadProc(LPVOID lpParam);

extern TCHAR  szSettings[];
static TCHAR  szPrinterPortFormat[]			= _T("%d,%d,%d,%d,%d,%d,%ld,%d,%4s");
static TCHAR  szPrinterPortVariables[]	= _T("PrinterPortVariables");

//////////////////////////////////////////////////////////////////////////////////////////////

void CS45BasicDoc::GetCommsSpecs(void)
{
char temp[NAME_TEMP_SIZE];
int nRead;

	CString strBuffer = AfxGetApp()->GetProfileString(szSettings, szPrinterPortVariables);
	if(!strBuffer.IsEmpty()){
		nRead = sscanf_s(strBuffer, szPrinterPortFormat, &m_DirectPrintComms.bXONXOFF, &m_DirectPrintComms.bRTSCTS,
			&m_DirectPrintComms.nDataBits, &m_DirectPrintComms.bDTRDSR,
			&m_DirectPrintComms.nParity, &m_DirectPrintComms.nStopBits,
			&m_DirectPrintComms.nBaud, &m_DirectPrintComms.nType, &temp, NAME_TEMP_SIZE);
		if(nRead == 9){
			temp[NAME_TEMP_SIZE - 1] = 0;
			m_DirectPrintComms.sPort = temp;
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////

void CS45BasicDoc::SaveCommsSpecs(void)
{
char szBuffer[100];

	sprintf_s(szBuffer, szPrinterPortFormat, m_DirectPrintComms.bXONXOFF, m_DirectPrintComms.bRTSCTS,
		m_DirectPrintComms.nDataBits, m_DirectPrintComms.bDTRDSR,
		m_DirectPrintComms.nParity, m_DirectPrintComms.nStopBits,
		m_DirectPrintComms.nBaud, m_DirectPrintComms.nType, (const char *)m_DirectPrintComms.sPort);
	AfxGetApp()->WriteProfileString(szSettings, szPrinterPortVariables, szBuffer);
}

//////////////////////////////////////////////////////////////////////////////////////////////

void CS45BasicDoc::SetPrinterDes(LPDEVNAMES lpDevNames)
{
	m_PrintDriver = (LPCTSTR)lpDevNames + lpDevNames->wDriverOffset;
	m_PrintDevice = (LPCTSTR)lpDevNames + lpDevNames->wDeviceOffset;
	m_PrintPort = (LPCTSTR)lpDevNames + lpDevNames->wOutputOffset;
}

//////////////////////////////////////////////////////////////////////////////////////////////

HGLOBAL CS45BasicDoc::GetPrinterDes(HGLOBAL hDevNames)
{
LONG cb;

	if(!IsPrinterDesSet()) return hDevNames;
	MiniGlobalFree(hDevNames);																																										// free the structure
	cb = sizeof(DEVNAMES) + m_PrintDriver.GetLength() + m_PrintDevice.GetLength() + m_PrintPort.GetLength() + 3;
	hDevNames = GlobalAlloc(GMEM_MOVEABLE, cb);																																		// create new structure from heap
	LPDEVNAMES lpDevNames = (LPDEVNAMES)::GlobalLock(hDevNames);																									// get a pointer to it
	lpDevNames->wDriverOffset = sizeof(DEVNAMES);																																	// and fill it in
	memmove((char *)lpDevNames + lpDevNames->wDriverOffset,(LPCSTR)m_PrintDriver,m_PrintDriver.GetLength() + 1);
	lpDevNames->wDeviceOffset = lpDevNames->wDriverOffset + m_PrintDriver.GetLength()+1;
	memmove((char *)lpDevNames + lpDevNames->wDeviceOffset,(LPCSTR)m_PrintDevice,m_PrintDevice.GetLength() + 1);
	lpDevNames->wOutputOffset = lpDevNames->wDeviceOffset + m_PrintDevice.GetLength()+1;
	memmove((char *)lpDevNames + lpDevNames->wOutputOffset,(LPCSTR)m_PrintPort,m_PrintPort.GetLength() + 1);
	lpDevNames->wDefault = FALSE;																																									// dont use default printer
	return hDevNames;
}

/////////////////////////////////////////////////////////////////////////////

void CS45BasicDoc::OnPrinterSetup()
{
CPrintSetup	dlgPrintSetup;

	dlgPrintSetup.m_pDoc = this;
  if((dlgPrintSetup.DoModal() == IDOK) && dlgPrintSetup.m_Update){
		if(m_DirectPrintComms.bConnected) OpenDirectPrint();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////

void CS45BasicDoc::EndPrint(void)
{
	if(m_DirectPrintComms.nType == CT_INTERNAL) m_pMainView->EndPrinting(false);
	else CloseDirectPrint();
}

//////////////////////////////////////////////////////////////////////////////////////////////

bool CS45BasicDoc::OpenDirectPrint()
{
COMMTIMEOUTS CommTimeOuts;
CString sPort;

	if(m_DirectPrintComms.bConnected) CloseDirectPrint();																// close to change settings
	sPort = _T("\\\\.\\") + m_DirectPrintComms.sPort;
	if((m_idPrintDev = CreateFile(sPort, GENERIC_WRITE | GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED, NULL)) == (HANDLE)-1) goto error;
	if(m_DirectPrintComms.nType == CT_SERIAL){																					// set up serial comm's
		int WriteTimeout = (int)(1.0F / (m_DirectPrintComms.nBaud / 12) * 1000) + 1;			// one character period, roughly speaking
		CommTimeOuts.ReadIntervalTimeout = MAXDWORD;
		CommTimeOuts.ReadTotalTimeoutMultiplier = 0;
		CommTimeOuts.ReadTotalTimeoutConstant = 0;
		CommTimeOuts.WriteTotalTimeoutMultiplier = 0;
		CommTimeOuts.WriteTotalTimeoutConstant = 2000;
		if(SetCommTimeouts(m_idPrintDev, &CommTimeOuts) == FALSE) goto error;
		if(SetupPrintConnection() == FALSE) goto error;
	}
	m_DirectPrintComms.bConnected = TRUE;
	if((m_pPrintThread = AfxBeginThread(PrintThreadProc, this)) == NULL){
		sPort.Format(_T("Failed to start direct print thread!\r\n"));
		g_pSysPrinter->PrintText(eDispArea::COMMENT, sPort.GetBuffer());
		m_DirectPrintComms.bConnected = false;
		CloseHandle(m_idPrintDev);
		m_idPrintDev = NULL;
		return false;
	}
	else{
		m_pPrintThread->SetThreadPriority(THREAD_PRIORITY_BELOW_NORMAL);
		m_pPrintThread->ResumeThread();
	}
	m_DirectPrintComms.nConnectedType = m_DirectPrintComms.nType;
	return true;
error:
	sPort.Format(_T("Failed to connect to control adaptor via port %s\n"), m_DirectPrintComms.sPort);
	g_pSysPrinter->PrintText(eDispArea::COMMENT, sPort.GetBuffer());
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////

bool CS45BasicDoc::SetupPrintConnection(void)
{
bool fRetVal;
DCB dcb;

	dcb.DCBlength = sizeof(DCB);
	GetCommState(m_idPrintDev, &dcb);
	dcb.BaudRate = m_DirectPrintComms.nBaud;
	dcb.ByteSize = m_DirectPrintComms.nDataBits;
	dcb.fParity = TRUE;
	switch(m_DirectPrintComms.nParity){
		case 0:
			dcb.Parity = NOPARITY;
			dcb.fParity = FALSE;
			break;
		case 1: dcb.Parity = EVENPARITY; break;
		case 2: dcb.Parity = ODDPARITY; break;
		case 3: dcb.Parity = MARKPARITY; break;
		case 4: dcb.Parity = SPACEPARITY; break;
		default: ASSERT(FALSE);
	}
	switch(m_DirectPrintComms.nStopBits){
		case 0: dcb.StopBits = ONESTOPBIT; break;
		case 1: dcb.StopBits = ONE5STOPBITS; break;
		case 2: dcb.StopBits = TWOSTOPBITS; break;
		default: ASSERT(FALSE);
	}
	dcb.fOutxDsrFlow = FALSE;
	dcb.fDsrSensitivity = m_DirectPrintComms.bDTRDSR;
	dcb.fDtrControl = m_DirectPrintComms.bDTRDSR ? DTR_CONTROL_HANDSHAKE : DTR_CONTROL_ENABLE;
	dcb.fOutxCtsFlow = m_DirectPrintComms.bRTSCTS;
	dcb.fRtsControl = m_DirectPrintComms.bRTSCTS ? RTS_CONTROL_TOGGLE : RTS_CONTROL_ENABLE;
	dcb.fInX = dcb.fOutX = m_DirectPrintComms.bXONXOFF;
	dcb.XonChar = ASCII_XON;
	dcb.XoffChar = ASCII_XOFF;
	dcb.XonLim = 100;
	dcb.XoffLim = 100;
	dcb.fBinary = TRUE;
	dcb.EvtChar = '\n';		// generate an event when this character arrives
	fRetVal = SetCommState(m_idPrintDev, &dcb);
	return fRetVal;
}

//////////////////////////////////////////////////////////////////////////////////////////////

void CS45BasicDoc::CloseDirectPrint()
{
	if(!m_DirectPrintComms.bConnected) return;
	m_DirectPrintComms.bConnected = FALSE;
	SetEvent(m_hDirectPrintEvent);
	WaitForSingleObject(m_pPrintThread->m_hThread, INFINITE);
	m_pPrintThread = nullptr;
	CloseHandle(m_idPrintDev);
	m_idPrintDev = NULL;
}

//////////////////////////////////////////////////////////////////////////////////////////////

UINT PrintThreadProc(LPVOID lpParam)
{
CS45BasicDoc *pDoc = (CS45BasicDoc *)lpParam;
OVERLAPPED os;
DWORD dwBytesWritten, fWriteStat;
CString cstr;
int Len;

	ASSERT(pDoc != NULL);
	ZeroMemory(&os, sizeof(OVERLAPPED));
	os.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	if(os.hEvent == NULL){
		AfxMessageBox(IDS_NOTHREAD, MB_OK | MB_ICONEXCLAMATION);
		return FALSE;
	}
	while(pDoc->m_DirectPrintComms.bConnected){																								// Loop until user requests disconnect
		if(!pDoc->m_DirectPrintList->IsEmpty()){
			cstr = pDoc->m_DirectPrintList->RemoveHead();
			Len = cstr.GetLength();
			fWriteStat = WriteFile(pDoc->m_idPrintDev, cstr, Len, &dwBytesWritten, &os);
			if(!fWriteStat && (GetLastError() == ERROR_IO_PENDING)){															// if still pending wait for 10s
				if(WaitForSingleObject(os.hEvent, 10000) != WAIT_OBJECT_0){
					fWriteStat = GetLastError();
				}
				ResetEvent(os.hEvent);
			}
		}
		else WaitForSingleObject(pDoc->m_hDirectPrintEvent, INFINITE);
		ResetEvent(pDoc->m_hDirectPrintEvent);
	}
	CloseHandle(os.hEvent);
	return TRUE;
}

//////////////////////////////////////////////////////////////////////////////////////////////

void CS45BasicDoc::PrintHardcopy(ExTextRow *pRow)
{
CString Line;
USHORT *pBuf = pRow->GetBuffer();
ExtChar Char;

	if((m_idPrintDev == nullptr) && !OpenDirectPrint()) return;
	while(*pBuf != 0){																																														// serialise the text data
		Char.Code = *(pBuf++);
		if(Char.b.Flags & 0x80) Line += Char.b.Flags;																																// check for control flags
 		Line += Char.b.Char;
	}
	if(m_DirectPrintComms.nType == CT_INTERNAL) m_pMainView->DoSpooledPrint(Line);
	else DoDirectPrint(Line);
}

//////////////////////////////////////////////////////////////////////////////////////////////

bool CS45BasicDoc::DoDirectPrint(CString Line)
{
CString cstr, PrintBuff;
bool Error = false; 																																														// begin page printing loop
int lfPos;		     

	PrintBuff += Line;																																														// add to existing data
	while((lfPos = PrintBuff.Find('\n')) >= 0){																																		// break down data into lines
		Line = PrintBuff.Left(++lfPos);																																							// only print text before line feed
		m_DirectPrintList->AddTail((LPCTSTR)Line);
		if(lfPos < PrintBuff.GetLength())	PrintBuff = PrintBuff.Right(PrintBuff.GetLength() - lfPos);								// recover rest of string
		else PrintBuff.Empty();
		SetEvent(m_hDirectPrintEvent);
	}
	if((lfPos = PrintBuff.Find('\f')) >= 0){																																			// check for single ff
		m_DirectPrintList->AddTail((LPCTSTR)PrintBuff);
		PrintBuff.Empty();
		SetEvent(m_hDirectPrintEvent);
	}
	return Error;
}

//////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////

IMPLEMENT_DYNAMIC(CPrintSetup, CDialogEx)

CPrintSetup::CPrintSetup(CWnd* pParent /*=nullptr*/) : CDialogEx(IDD_PRINT_COMMS, pParent)
{
	m_pDoc = nullptr;
	m_CommType = -1;
	m_Update = false;
}

//////////////////////////////////////////////////////////////////////////////////////////////

CPrintSetup::~CPrintSetup()
{
}

//////////////////////////////////////////////////////////////////////////////////////////////

void CPrintSetup::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Radio(pDX, IDC_COMMTYPE_SYSTEM, m_CommType);
}

//////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_MESSAGE_MAP(CPrintSetup, CDialogEx)
	ON_BN_CLICKED(IDC_PROPERTIES, OnCommsProperties)
	ON_BN_CLICKED(IDC_COMMTYPE_SYSTEM, OnCommtype)
	ON_BN_CLICKED(IDC_COMMTYPE_PARALLEL, OnCommtype)
	ON_BN_CLICKED(IDC_COMMTYPE_SERIAL, OnCommtype)
	ON_WM_DESTROY()
END_MESSAGE_MAP()

//////////////////////////////////////////////////////////////////////////////////////////////

BOOL CPrintSetup::OnInitDialog()
{
	m_CommType = m_pDoc->m_DirectPrintComms.nType;
	CDialogEx::OnInitDialog();
	return TRUE;
}

//////////////////////////////////////////////////////////////////////////////////////////////

void CPrintSetup::OnDestroy()
{
	if(!UpdateData(TRUE)) return;
	m_pDoc->m_DirectPrintComms.nType = m_CommType;
	CDialogEx::OnDestroy();
}

//////////////////////////////////////////////////////////////////////////////////////////////

void CPrintSetup::OnChange()
{
	m_Update = true;
}

//////////////////////////////////////////////////////////////////////////////////////////////

void CPrintSetup::OnCommtype()
{
	UpdateData(TRUE);
	m_Update = true;
}

//////////////////////////////////////////////////////////////////////////////////////////////

void CPrintSetup::OnSelchangePortcb()
{
	UpdateData(TRUE);
	OnChange();
}

//////////////////////////////////////////////////////////////////////////////////////////////

void CPrintSetup::OnCommsProperties()
{
	if(!UpdateData(TRUE)) return;
	switch(m_CommType){
		case CT_INTERNAL:{
			((CS45BasicApp*)AfxGetApp())->OnFilePrintSetup();
			break;
		}
		case CT_PARALLEL:{
			CParallelProps commsDlg;
			commsDlg.m_CommParams = &m_pDoc->m_DirectPrintComms;
			if((commsDlg.DoModal() == IDOK) && commsDlg.m_Update) m_Update = true;
			break;
		}
		case CT_SERIAL:{
			CSerialProps commsDlg;
			commsDlg.m_CommParams = &m_pDoc->m_DirectPrintComms;
			if((commsDlg.DoModal() == IDOK) && commsDlg.m_Update) m_Update = true;
			break;
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////

CSerialProps::CSerialProps(CWnd* pParent /*=NULL*/)
	: CDialogEx(CSerialProps::IDD, pParent)
{
	m_sPort = _T("");
	m_sBaud = _T("");
	m_sDataBits = _T("");
	m_nParity = -1;
	m_nStopBits = -1;
	m_bDTRDSR = FALSE;
	m_bRTSCTS = FALSE;
	m_Update = FALSE;
	m_Enable = 0xFF;
}

//////////////////////////////////////////////////////////////////////////////////////////////

void CSerialProps::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_CBString(pDX, IDC_BAUDCB, m_sBaud);
	DDX_CBString(pDX, IDC_DATABITSCB, m_sDataBits);
	DDX_CBIndex(pDX, IDC_PARITYCB, m_nParity);
	DDX_CBString(pDX, IDC_PORTCB, m_sPort);
	DDX_CBIndex(pDX, IDC_STOPBITSCB, m_nStopBits);
	DDX_Check(pDX, IDC_DTRDSR, m_bDTRDSR);
	DDX_Check(pDX, IDC_RTSCTS, m_bRTSCTS);
	DDX_Check(pDX, IDC_XONXOFF, m_bXONXOFF);
}

//////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_MESSAGE_MAP(CSerialProps, CDialogEx)
	ON_CBN_SELCHANGE(IDC_BAUDCB, OnChange)
	ON_CBN_SELCHANGE(IDC_DATABITSCB, OnChange)
	ON_BN_CLICKED(IDC_DTRDSR, OnChange)
	ON_CBN_SELCHANGE(IDC_PARITYCB, OnChange)
	ON_CBN_SELCHANGE(IDC_PORTCB, OnChange)
	ON_BN_CLICKED(IDC_RTSCTS, OnChange)
	ON_CBN_SELCHANGE(IDC_STOPBITSCB, OnChange)
	ON_BN_CLICKED(IDC_XONXOFF, OnChange)
	ON_BN_CLICKED(IDC_RTSCTS, OnHardwareFlow)
	ON_BN_CLICKED(IDC_DTRDSR, OnHardwareFlow)
	ON_BN_CLICKED(IDC_XONXOFF, OnXonxoff)
END_MESSAGE_MAP()

//////////////////////////////////////////////////////////////////////////////////////////////

BOOL CSerialProps::OnInitDialog()
{
char buf[40];

	m_sPort = m_CommParams->sPort;
	_itoa_s(m_CommParams->nBaud, buf, 40, 10);
	m_sBaud = buf;
	_itoa_s(m_CommParams->nDataBits, buf, 40, 10);
	m_sDataBits = buf;
	switch(m_CommParams->nParity){
		default:
			m_CommParams->nParity = 0;
		case 0:
		case 1:
		case 2:
		case 3:
		case 4:
			m_nParity = m_CommParams->nParity;
			break;
	}
	switch(m_CommParams->nStopBits){
		default:
			m_CommParams->nStopBits = 0;
		case 0:
		case 1:
		case 2:
			m_nStopBits = m_CommParams->nStopBits;
			break;
	}
	m_bDTRDSR = m_CommParams->bDTRDSR;
	m_bRTSCTS = m_CommParams->bRTSCTS;
	m_bXONXOFF = m_CommParams->bXONXOFF;
	CDialog::OnInitDialog();
	if(((CComboBox *)GetDlgItem(IDC_PORTCB))->FindString(0, m_sPort) == CB_ERR){
		m_sPort = "COM1";
		m_Update = true;
	}
	if(((CComboBox *)GetDlgItem(IDC_BAUDCB))->FindString(0, m_sBaud) == CB_ERR){
		m_sBaud = "9600";
		m_Update = true;
	}
	if(((CComboBox *)GetDlgItem(IDC_DATABITSCB))->FindString(0, m_sDataBits) == CB_ERR){
		m_sDataBits = "8";
		m_Update = true;
	}
	if(((CComboBox *)GetDlgItem(IDC_PARITYCB))->SetCurSel(m_nParity) == CB_ERR){
		m_nParity = 0;
		m_Update = true;
	}
	if(((CComboBox *)GetDlgItem(IDC_STOPBITSCB))->SetCurSel(m_nStopBits) == CB_ERR){
		m_nStopBits = 2;
		m_Update = true;
	}
	if(m_Update) UpdateData(FALSE);
	((CComboBox *)GetDlgItem(IDC_PORTCB))->EnableWindow(m_Enable & SP_PORTBIT);
	((CComboBox *)GetDlgItem(IDC_BAUDCB))->EnableWindow(m_Enable & SP_BAUDBIT);
	((CComboBox *)GetDlgItem(IDC_DATABITSCB))->EnableWindow(m_Enable & SP_DATABIT);
	((CComboBox *)GetDlgItem(IDC_PARITYCB))->EnableWindow(m_Enable & SP_PARITYBIT);
	((CComboBox *)GetDlgItem(IDC_STOPBITSCB))->EnableWindow(m_Enable & SP_STOPBIT);
	((CButton *)GetDlgItem(IDC_RTSCTS))->EnableWindow(m_Enable & SP_RTSBIT);
	((CButton *)GetDlgItem(IDC_DTRDSR))->EnableWindow(m_Enable & SP_DTRBIT);
	((CButton *)GetDlgItem(IDC_XONXOFF))->EnableWindow(m_Enable & SP_XONXOFFBIT);
	return TRUE;
}

//////////////////////////////////////////////////////////////////////////////////////////////

void CSerialProps::OnOK()
{
	if(!UpdateData(TRUE)) return;
	if(m_Update){
		m_CommParams->sPort = m_sPort;
		m_CommParams->nBaud = atoi(m_sBaud);
		m_CommParams->nDataBits = atoi(m_sDataBits);
		m_CommParams->nParity = m_nParity;
		m_CommParams->nStopBits = m_nStopBits;
		m_CommParams->bDTRDSR = m_bDTRDSR;
		m_CommParams->bRTSCTS = m_bRTSCTS;
		m_CommParams->bXONXOFF = m_bXONXOFF;
	}
	CDialogEx::OnOK();
}

//////////////////////////////////////////////////////////////////////////////////////////////

void CSerialProps::OnChange()
{
	m_Update = true;
}

//////////////////////////////////////////////////////////////////////////////////////////////

void CSerialProps::OnXonxoff()
{
	if(((CButton *)GetDlgItem(IDC_XONXOFF))->GetCheck()){
		((CButton *)GetDlgItem(IDC_RTSCTS))->SetCheck(0);
		((CButton *)GetDlgItem(IDC_DTRDSR))->SetCheck(0);
	}
	OnChange();
}

//////////////////////////////////////////////////////////////////////////////////////////////

void CSerialProps::OnHardwareFlow()
{
	if(((CButton *)GetDlgItem(IDC_DTRDSR))->GetCheck() || ((CButton *)GetDlgItem(IDC_RTSCTS))->GetCheck()){
		((CButton *)GetDlgItem(IDC_XONXOFF))->SetCheck(0);
	}
	OnChange();
}

//////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////

CParallelProps::CParallelProps(CWnd* pParent /*=NULL*/)
	: CDialogEx(CParallelProps::IDD, pParent)
{
	m_sPort = _T("");
	m_nTimeout = 0;
	m_Enable = true;
}

//////////////////////////////////////////////////////////////////////////////////////////////

void CParallelProps::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_CBString(pDX, IDC_PORTCB, m_sPort);
	DDX_Text(pDX, IDC_TIMEOUT, m_nTimeout);
}

//////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_MESSAGE_MAP(CParallelProps, CDialogEx)
	ON_CBN_SELCHANGE(IDC_PORTCB, OnChange)
	ON_EN_CHANGE(IDC_TIMEOUT, OnChange)
END_MESSAGE_MAP()

//////////////////////////////////////////////////////////////////////////////////////////////

BOOL CParallelProps::OnInitDialog()
{
	m_sPort = m_CommParams->sPort;
	CDialogEx::OnInitDialog();
	if(((CComboBox *)GetDlgItem(IDC_PORTCB))->FindString(0, m_sPort) == CB_ERR){
		m_sPort = "LPT1";
		m_Update = true;
	}
	if(m_Update) UpdateData(FALSE);
	((CComboBox *)GetDlgItem(IDC_PORTCB))->EnableWindow(m_Enable);
	((CEdit *)GetDlgItem(IDC_TIMEOUT))->EnableWindow(m_Enable);
	return TRUE;
}

//////////////////////////////////////////////////////////////////////////////////////////////

void CParallelProps::OnOK()
{
	if(!UpdateData(TRUE)) return;
	if(m_Update){
		m_CommParams->sPort = m_sPort;
	}
	CDialogEx::OnOK();
}

//////////////////////////////////////////////////////////////////////////////////////////////

void CParallelProps::OnChange()
{
	m_Update = true;
}

//////////////////////////////////////////////////////////////////////////////////////////////
