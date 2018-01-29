//
//
//

#ifndef TSND121_H_
#define	TSND121_H_

#include "SerialPort.h"
#include "MMTimer.h"

class CTSND121
{
	double m_accelerate[3];
	double m_angular_velocity[3];
	unsigned long m_ticktime;
	union {
		int data[6];
		struct {
			int accelerate[3];
			int angular[3];
		};
	} m_rawdata;
	//
	struct {
		bool(*handler)(void*);
		void* parameter;
	} m_logger;
	//
	unsigned char m_rxbuffer[256];
	unsigned char* m_rxptr;
	unsigned int m_parameterbytes;
	unsigned char m_checksum;
	bool m_bError;
	CSerialPort m_port;
public:
	double Accelerate(int id) const;
	double AngularVelocity(int id) const;
	unsigned long TickTime() const;
	//
	bool Start(LPCTSTR portname, bool(*handler)(void*) = NULL, void* parameter = NULL);
	void Stop();
	//
	const int* RawData() const;
	//
	CTSND121();
	virtual ~CTSND121();
private:
	long getint32(const unsigned char* ptr) const;
	long getint24(const unsigned char* ptr) const;
	double accelerate(int val) const;
	double angularveocity(int val) const;
	double accelerate(const unsigned char* ptr) const;
	double angularveocity(const unsigned char* ptr) const;
	void Receive(int c);
	static void Receive(void*, const void*, unsigned int);
	void Transmit();
	static void Transmit(void*);
};
/////////////////////////////////////////////////////////////////////////////
// inlines
inline const int* CTSND121::RawData() const
{
	return m_rawdata.data;
}

#endif // !TSND121_H_
