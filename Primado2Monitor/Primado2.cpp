//
//
//

#include "stdafx.h"
#include "Primado2.h"

CPrimado2::CPrimado2()
{
	m_rpm = 0;
	m_ampere = 0.0;
	m_mota_ofs = m_mota_3a = 0;
}
CPrimado2::~CPrimado2()
{
}

unsigned int CPrimado2::RPM() const
{
	return m_rpm;
}
double CPrimado2::Ampere() const
{
	return m_ampere;
}
bool CPrimado2::Start(LPCTSTR portname)
{
	m_timer.Stop();
	if (m_port.IsConnected()) {
		m_port.Close();
		Sleep(200);
	}
	m_rxptr = m_rxbuffer;
	m_port.ReceiveHandler(Receive, this);
	if (!m_port.Open(portname, 38400, CSerialPort::N81)) {
		return false;
	}
	// 電流センサ回路補正値取得
	m_port.Write("@0000000000000000000000000000R001D0002*4F", 41);
	//	定期的に回転数と電流値を取得
	m_timer.Start(125, Transmit, this);
	return true;
}
void CPrimado2::Stop()
{
	m_timer.Stop();
	m_port.Close();
}

void CPrimado2::Transmit()
{
	if (m_port.IsConnected()) {
		// 回転数/電流ＡＤ値取得
		m_port.Write("@0000000000000000000000000000R08FD0002*30", 41);
	}
}
void CPrimado2::Transmit(void* ptr)
{
	static_cast<CPrimado2*>(ptr)->Transmit();
}

static long x2i(const char* s, int n)
{
	long l = 0;
	while (0 < n--) {
		l <<= 4;
		int c = *s++;
		if ('0' <= c && c <= '9') {
			l += c - '0';
		} else if ('a' <= c && c <= 'f') {
			l += c - 'a' + 10;
		} else if ('A' <= c && c <= 'F') {
			l += c - 'A' + 10;
		} else {
			ASSERT(FALSE);
		}
	}
	return l;
}
void CPrimado2::Receive(int c)
{
	if (m_rxptr == m_rxbuffer) {
		// 先頭を見つける
		if (c == '@') {
			*m_rxptr++ = c;
		}
		return;
	}
	// バッファに格納
	*m_rxptr++ = c;
	if (m_rxptr < &m_rxbuffer[48]) {
		return;
	}
	// データ取得完了。
	m_rxptr = m_rxbuffer;
	// データ解析
	if (memcmp(m_rxbuffer + 29,"R001D0002", 9) == 0) {
		m_mota_ofs = x2i(m_rxbuffer + 38, 4);
		m_mota_3a = x2i(m_rxbuffer + 42, 4);
	} else if (memcmp(m_rxbuffer + 29, "R08FD0002", 9) == 0) {
		m_rpm = x2i(m_rxbuffer + 38, 4) * 10;
		m_adval = x2i(m_rxbuffer + 42, 4);
		m_ampere = (double)MulDiv(m_adval - m_mota_ofs, 3000, m_mota_3a - m_mota_ofs) / 1000.0;
	}
}
void CPrimado2::Receive(void* ptr, const void* buffer, unsigned int bytes)
{
	const char* rptr = static_cast<const char*>(buffer);
	while (0 < bytes--) {
		static_cast<CPrimado2*>(ptr)->Receive(*rptr++);
	}
}
