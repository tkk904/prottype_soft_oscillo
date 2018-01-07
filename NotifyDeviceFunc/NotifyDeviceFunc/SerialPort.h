//
//
//

#ifndef	_SERIALPORT_H_
#define	_SERIALPORT_H_

class CSerialPort
{
	HANDLE m_hComDev;
	bool m_bConnected;
	DWORD m_nThreadID;
	HANDLE m_hWatchThread;
	OVERLAPPED m_osRead, m_osWrite;
public:
	enum {
		NONE = 0,
		EVEN = 0x01, ODD, MARK, SPACE,
		DATA8 = 0x10, DATA7 = 0x20,
		STOP1 = 0x100, STOP2 = 0x200,
		XONOFF = 0x1000, HARDWIRE = 0x2000,
		N81 = NONE|DATA8|STOP1,
	};
	bool Open(LPCTSTR szPort, DWORD nBps, int nMode);
	bool IsConnected() const { return m_bConnected; }
	//
	virtual void Close();
	virtual long Read(void* pData,long nBytes);
	virtual long Write(const void* pData, long nBytes);
//	static void EnumCommPorts(CStringArray& array);
	//
	virtual ~CSerialPort();
	CSerialPort();
protected:
	long ReadComm(void* pData,long nBytes);
	virtual bool Receive();
private:
	bool SetupConnection(DWORD nBps, int nMode);
	static DWORD WatchProc(void* ptr);
	bool WriteComm(const void* pData, DWORD nBytes);
};

#endif	// _SERIALPORT_H_

