//
//
//

#ifndef PRIMADO2_H_
#define	PRIMADO2_H_

#include "SerialPort.h"
#include "MMTimer.h"

class CPrimado2
{
	unsigned int m_rpm;
	double m_ampere;
	long m_mota_ofs, m_mota_3a;
	long m_adval;
	//
	char m_rxbuffer[49];
	char* m_rxptr;
	CSerialPort m_port;
	CMultiMediaTimer m_timer;
public:
	unsigned int RPM() const;
	double Ampere() const;
	bool Start(LPCTSTR portname);
	void Stop();
	//
	CPrimado2();
	virtual ~CPrimado2();
private:
	void Receive(int c);
	static void Receive(void*,const void*,unsigned int);
	void Transmit();
	static void Transmit(void*);
};

#endif // !PRIMADO2_H_
