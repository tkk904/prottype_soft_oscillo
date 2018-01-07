//
//
//

#include "stdafx.h"
#include "serialport.h"
//#include <afxcoll.h>
#include <atlstr.h>


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


CSerialPort::CSerialPort()
{
	m_hComDev = INVALID_HANDLE_VALUE;
	m_bConnected = false;
	m_nThreadID = 0;
	m_osRead.hEvent = NULL;
	m_osRead.Offset = 0;
	m_osRead.OffsetHigh = 0;
	m_osWrite = m_osRead;
}
CSerialPort::~CSerialPort()
{
	Close();
	if( m_osRead.hEvent != NULL ){
		CloseHandle(m_osRead.hEvent);
	}
	if( m_osWrite.hEvent != NULL ){
		CloseHandle(m_osWrite.hEvent);
	}
}
void CSerialPort::Close()
{
	if( m_bConnected ){
		m_bConnected = false;
		// スレッドが停止するまで待ちます
		while(m_nThreadID!=0){
			SetCommMask(m_hComDev,0);
		}
		// drop DTR
		EscapeCommFunction(m_hComDev,CLRDTR);
		PurgeComm(m_hComDev, PURGE_TXABORT|PURGE_RXABORT|PURGE_TXCLEAR|PURGE_RXCLEAR);
		CloseHandle(m_hComDev);
	}
}
bool CSerialPort::Open(LPCTSTR szPort,DWORD nBps,int nMode)
{
	if( m_osRead.hEvent == NULL ){
		m_osRead.hEvent = CreateEvent(NULL,TRUE,FALSE,NULL);
		if( m_osRead.hEvent == NULL ){
			return false;
		}
	}
	if( m_osWrite.hEvent == NULL ){
		m_osWrite.hEvent = CreateEvent(NULL,TRUE,FALSE,NULL);
		if( m_osWrite.hEvent == NULL ){
			return false;
		}
	}
	//
	// デバイス名の頭に "\\.\"を付けなければなりません。
	// これがないとCOM10以上は開けません。
	CString sPortName = _T("\\\\.\\");
	sPortName += szPort;
	m_hComDev = CreateFile(sPortName,GENERIC_READ | GENERIC_WRITE,
					0,
					NULL,
					OPEN_EXISTING,
					FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED,
					NULL );
	if( m_hComDev == INVALID_HANDLE_VALUE ){
		return false;
	}
	SetCommMask( m_hComDev,EV_RXCHAR );
	SetupComm( m_hComDev,4096,4096 );
	PurgeComm( m_hComDev,PURGE_TXABORT|PURGE_RXABORT|PURGE_TXCLEAR|PURGE_RXCLEAR);
	//
	COMMTIMEOUTS timeouts;
	timeouts.ReadIntervalTimeout = 0xFFFFFFFF ;
	timeouts.ReadTotalTimeoutMultiplier = 0 ;
	timeouts.ReadTotalTimeoutConstant = 1000 ;
	// CBR_9600 is approximately 1byte/ms. For our purposes, allow
	// double the expected time per character for a fudge factor.
	timeouts.WriteTotalTimeoutMultiplier = 0;//2*CBR_9600/nBps;
	timeouts.WriteTotalTimeoutConstant = 1000;
	SetCommTimeouts( m_hComDev, &timeouts );

	m_bConnected = SetupConnection(nBps,nMode);
	if( !m_bConnected ){
		m_bConnected = false;
		CloseHandle( m_hComDev );
		return false;
	}
	// create thread to watch for an event.
	m_hWatchThread = CreateThread( (LPSECURITY_ATTRIBUTES)NULL,
							0,
							(LPTHREAD_START_ROUTINE)WatchProc,
							this,
							0, &m_nThreadID );
	if( m_hWatchThread == NULL ){
		m_nThreadID = 0;
		m_bConnected = false;
		CloseHandle( m_hComDev );
		return false;
	}
	EscapeCommFunction(m_hComDev,SETDTR);
	return true;
}
bool CSerialPort::SetupConnection(DWORD nBps, int nMode)
{
	DCB dcb;
	dcb.DCBlength = sizeof(DCB);
	GetCommState(m_hComDev,&dcb);
 	switch( nBps ){
	case 110 : dcb.BaudRate = CBR_110; break;
	case 300 : dcb.BaudRate = CBR_300; break;
	case 600 : dcb.BaudRate = CBR_600; break;
	case 1200 : dcb.BaudRate = CBR_1200; break;
	case 2400 : dcb.BaudRate = CBR_2400; break;
	case 4800 : dcb.BaudRate = CBR_4800; break;
	case 9600 : dcb.BaudRate = CBR_9600; break;
	case 14400 : dcb.BaudRate = CBR_14400; break;
	case 19200 : dcb.BaudRate = CBR_19200; break;
	case 38400 : dcb.BaudRate = CBR_38400; break;
	case 56000 : dcb.BaudRate = CBR_56000; break;
	case 57600 : dcb.BaudRate = CBR_57600; break;
	case 115200 : dcb.BaudRate = CBR_115200; break;
	case 128000 : dcb.BaudRate = CBR_128000; break;
	case 256000 : dcb.BaudRate = CBR_256000; break;
	default :
		return false;
	}
	switch( nMode & (NONE|EVEN|ODD|SPACE|MARK) ){
	case NONE :	dcb.fParity = NOPARITY; break;
	case EVEN :	dcb.fParity = EVENPARITY; break;
	case ODD :	dcb.fParity = ODDPARITY; break;
	case SPACE : dcb.fParity = SPACEPARITY; break;
	case MARK :	dcb.fParity = MARKPARITY; break;
	default :
		return false;
	}
	switch( nMode & (DATA7|DATA8) ){
	case DATA7 : dcb.ByteSize = 7; break;
	case DATA8 : dcb.ByteSize = 8; break;
	default :
		return false;
	}
	switch( nMode & (STOP1|STOP2) ){
	case STOP1 : dcb.StopBits = ONESTOPBIT; break;
	case STOP2 : dcb.StopBits = TWOSTOPBITS; break;
	default :
		return false;
	}
	dcb.fDtrControl = DTR_CONTROL_ENABLE;
	dcb.fRtsControl = RTS_CONTROL_ENABLE;
	dcb.fOutxCtsFlow = dcb.fOutxDsrFlow = FALSE;
	dcb.fInX = dcb.fOutX = FALSE;
	dcb.XonChar = 0x11;		// ascii xon 0x11
	dcb.XoffChar = 0x13;	// ascii xoff 0x13
	dcb.XonLim = 100;
	dcb.XoffLim = 100;
	switch( nMode & (XONOFF|HARDWIRE) ){
	case XONOFF :
		dcb.fInX = dcb.fOutX = TRUE;
		break;
	case HARDWIRE :
		dcb.fOutxCtsFlow = dcb.fOutxDsrFlow = TRUE;
		dcb.fDtrControl = DTR_CONTROL_HANDSHAKE;
		dcb.fRtsControl = RTS_CONTROL_HANDSHAKE;
		break;
	case NONE :
		break;
	default :
		return false;
	}
	dcb.fBinary = TRUE;
	dcb.fParity = TRUE;

	return SetCommState( m_hComDev,&dcb ) == TRUE;
}
DWORD CSerialPort::WatchProc(void* ptr)
{
	CSerialPort* pObj = (CSerialPort*)ptr;
	OVERLAPPED os;
	memset(&os,0,sizeof(os));
	// create I/O event used for overlapped read
	os.hEvent = CreateEvent(NULL,TRUE,FALSE,NULL);
	if( os.hEvent == NULL ){
		return 0;
	}
	if( !SetCommMask(pObj->m_hComDev,EV_RXCHAR) ){
		return 0;
	}
	//
	DWORD nEventMask;
	while( pObj->IsConnected() ){
		nEventMask = 0;
		WaitCommEvent(pObj->m_hComDev,&nEventMask,NULL );
		if( (nEventMask&EV_RXCHAR) == EV_RXCHAR){
			while( !pObj->Receive() ){
				Sleep(100);
				if( !pObj->IsConnected() ){
					goto abort;
				}
			}
		}
	}
abort:;
	CloseHandle(os.hEvent);
	pObj->m_nThreadID = 0;
	pObj->m_hWatchThread = NULL;

	return 1;
}
bool CSerialPort::WriteComm(const void* pData, DWORD nBytes)
{
	DWORD nLength = 0;
	if( WriteFile( m_hComDev,pData,nBytes,&nLength,&m_osWrite) ){
		return true;
	}
	// error
	DWORD nErr;
	COMSTAT status;
	if( GetLastError() == ERROR_IO_PENDING ){
		DWORD nBytesSent = 0;
		while(!GetOverlappedResult(m_hComDev,&m_osWrite,&nLength,TRUE)){
			nErr = GetLastError();
			if( nErr != ERROR_IO_INCOMPLETE ){
				nBytesSent += nLength;
			} else {
//				TRACE( "comm write error %u\n",nErr );
				ClearCommError(m_hComDev,&nErr,&status);
				break;
			}
		}
		nBytesSent += nLength;
		if( nBytesSent != nBytes ){
//			TRACE( "多分タイムアウト %ld bytes\n",nBytesSent );
		} else {
//			TRACE( "%ld bytes 書き込み\n", nBytesSent );
			return true;
		}
	} else {
		ClearCommError(m_hComDev,&nErr,&status);
		if( 0 < nErr ){
//			TRACE( "comm write error %u\n", nErr );
		}
	}
	return false;
}
long CSerialPort::ReadComm(void* pData,long nBytes)
{
	COMSTAT status;
	DWORD nErr;
	ClearCommError(m_hComDev,&nErr,&status);
	DWORD nLength = min( (DWORD)nBytes, status.cbInQue );
	if( nLength == 0 ) return 0;
	//
	if( ReadFile(m_hComDev,pData,nLength,&nLength,&m_osRead) ){
		return nLength;
	}
	// error
	if( GetLastError() == ERROR_IO_PENDING ){
//		TRACE( "comm read io pending\n" );
		while( !GetOverlappedResult(m_hComDev,&m_osRead,&nLength,TRUE) ){
			nErr = GetLastError();
			if( nErr == ERROR_IO_INCOMPLETE ){
			} else {
//				TRACE( "comm read error %u\n", nErr );
				ClearCommError(m_hComDev,&nErr,&status);
				return -1;
			}
		}
		return nLength;
	} else {
		ClearCommError( m_hComDev, &nErr, &status );
//		TRACE( "error %u\n", nErr );
	}
	return -1;
}
long CSerialPort::Write(const void* pData, long nBytes)
{
	return WriteComm(pData,nBytes) ? nBytes : -1;
}
long CSerialPort::Read(void* pData, long nBytes)
{
	return ReadComm(pData,nBytes);
}
bool CSerialPort::Receive()
{
	return true;
}


#include <setupapi.h>
#include <Ntddser.h>
extern "C" {
#include "hidsdi.h"
}

#pragma comment(lib,"hid.lib")
#pragma comment(lib,"setupapi.lib")
/*
void CSerialPort::EnumCommPorts(CStringArray& list)
{
	HDEVINFO hDevInfo;
	DWORD MemberIndex = 0;
	SP_DEVINFO_DATA Data = { sizeof(SP_DEVINFO_DATA) };

	hDevInfo = SetupDiGetClassDevs(&GUID_DEVINTERFACE_COMPORT, 0, 0, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);	// デバイス情報セットを取得
	if (hDevInfo == 0) {	//	デバイス情報セットが取得できなかった場合
		return;
	}
	Data.cbSize = sizeof(Data);
	DWORD index = 0;
	while (SetupDiEnumDeviceInfo(hDevInfo, index, &Data)) {	// デバイスインターフェイスの取得
		DWORD dataT;
		DWORD size;
		LPTSTR buf;
		//	COM ポート名の取得
		HKEY key = SetupDiOpenDevRegKey(hDevInfo, &Data, DICS_FLAG_GLOBAL, 0, DIREG_DEV, KEY_QUERY_VALUE);
		if (key) {
			TCHAR name[256];
			DWORD type = 0;
			size = sizeof(name);
			RegQueryValueEx(key, _T("PortName"), NULL, &type, (LPBYTE)name, &size);
			list.Add(name);
			TRACE(L"%s", name);
		}
		//	デバイスの説明を取得
		size = 0;
		buf = NULL;
		while (!SetupDiGetDeviceRegistryProperty(hDevInfo, &Data, SPDRP_DEVICEDESC, &dataT, (PBYTE)buf, size, &size)) {
			if (GetLastError() != ERROR_INSUFFICIENT_BUFFER) {
				break;
			}
			if (buf) {
				LocalFree(buf);
			}
			buf = (LPTSTR)LocalAlloc(LPTR, size * 2);
		}
		TRACE(L"(%s)\n", buf);
		if (buf) {
			LocalFree(buf);
		}
		index++;
	}
	SetupDiDestroyDeviceInfoList(hDevInfo);	//	デバイス情報セットを解放
}
*/
