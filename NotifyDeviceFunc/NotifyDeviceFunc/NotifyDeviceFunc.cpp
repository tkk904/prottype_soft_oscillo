// NotifyDeviceFunc.cpp : DLL アプリケーション用にエクスポートされる関数を定義します。
//

#include "stdafx.h"

#include <atlstr.h>
#include "SerialPort.h"
#include "NotifyDeviceFunc.h"
#include <iostream>
#include <string>

using namespace std;

#ifdef UNICODE
static auto& tcout = ::std::wcout;
static auto& tcin = ::std::wcin;
#else
static auto& tcout = ::std::cout;
static auto& tcin = ::std::cin;
#endif

CSerialPort m_port;
string m_portname;

bool Start(char* PortName)
{
	if (m_port.IsConnected()) {
		m_port.Close();
	}
	//よりスマートなやり方があると思いますが、現状はこれで。
	//char* -> std::string -> CString -> LPCTSTR
	m_portname = PortName;
	CString hoge = m_portname.c_str();
	LPCTSTR ss = hoge;
	if (!m_port.Open(ss, 115200, CSerialPort::N81)) {
		return false;
	}
	return true;
}

void StopNotify()
{
	m_port.Close();
	return;
}

void Notify(int time) {
	if (m_port.IsConnected()) {
#define DO_ON	":7880010101000000000000000005\r\n"
			m_port.Write(DO_ON, sizeof(DO_ON));
			Sleep(time);
#define DO_OFF	":7880010001000000000000000006\r\n"
			m_port.Write(DO_OFF, sizeof(DO_OFF));
	}
}

