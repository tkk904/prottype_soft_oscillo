//
//
//

#include "stdafx.h"
#include "Primado2Monitor.h"

CPrimado2Monitor::CPrimado2Monitor()
{
}
CPrimado2Monitor::~CPrimado2Monitor()
{
}
CPrimado2Monitor& CPrimado2Monitor::Instance()
{
	static CPrimado2Monitor c_instance;
	return c_instance;
}
int  CPrimado2Monitor::GetError()
{
	return 0;
}
bool CPrimado2Monitor::Initialize()
{
	return true;
}
void CPrimado2Monitor::Finalize()
{
	Stop();
}

bool CPrimado2Monitor::Start(const char* primado2, const char* tsnd121)
{
	Stop();
	if (!m_primado2.Start(primado2)) {
		return false;
	}
	if (!m_tsnd121.Start(tsnd121)) {
		m_primado2.Stop();
		return false;
	}
	return true;
}
void CPrimado2Monitor::Stop()
{
	m_primado2.Stop();
	m_tsnd121.Stop();
}

int CPrimado2Monitor::GetData(double* ptr, int count)
{
	double data[2+3+3];
	data[0] = static_cast<double>(m_primado2.RPM());
	data[1] = m_primado2.Ampere();
	for (int i = 0; i < 3; i++) {
		data[2 + i] = m_tsnd121.Accelerate(i);
		data[2 + 3 + i] = m_tsnd121.AngularVelocity(i);
	}
	//
	if (sizeof(data)/sizeof(data[0]) < count) {
		count = sizeof(data) / sizeof(data[0]);
	}
	for (int i = 0; i < count; i++) {
		ptr[i] = data[i];
	}
	return count;
}
