//
//
//

#include "stdafx.h"
#include "MMTimer.h"

#pragma comment( lib, "winmm.lib" )

CMultiMediaTimer::CMultiMediaTimer()
{
	m_timerID = NULL;
	m_handler = NULL;
}
CMultiMediaTimer::~CMultiMediaTimer()
{
	Stop();
}
bool CMultiMediaTimer::IsRunning() const
{
	return m_timerID != NULL;
}
void CMultiMediaTimer::Period(unsigned long msec)
{
	m_msec = msec;
	if (IsRunning()) {
		Start();
	}
}
void CMultiMediaTimer::Handler(void(*handler)(void*), void* parameter)
{
	m_handler = NULL;
	m_parameter = parameter;
	m_handler = handler;
}
void CMultiMediaTimer::Start(unsigned long msec, void(*handler)(void*), void* parameter)
{
	Stop();
	Handler(handler, parameter);
	Period(msec);
	Start();
}
void CMultiMediaTimer::Start()
{
	Stop();
	m_timerID = timeSetEvent(m_msec, 20, TimerProc, reinterpret_cast<DWORD_PTR>(this), TIME_PERIODIC);
}
void CMultiMediaTimer::Stop()
{
	if (IsRunning()) {
		timeKillEvent(m_timerID);
		m_timerID = NULL;
	}
}
void CMultiMediaTimer::TimerHandler()
{
	if (m_handler) {
		m_handler(m_parameter);
	}
}
void CALLBACK CMultiMediaTimer::TimerProc(UINT uTimerID, UINT uMsg, DWORD_PTR dwUser, DWORD_PTR dw1, DWORD_PTR dw2)
{
	reinterpret_cast<CMultiMediaTimer*>(dwUser)->TimerHandler();
}

/*
timeSetEvent ‚É CreateWaitableTimer ‚Æ SetWaitableTimer ‚ğ‘g‚İ‡‚í‚¹‚Ä usec ‚Ì¸“x‚ª‚¾‚¹‚é‚©‚à
timeSetEvent ‚Í­‚µ’Z‚ß‚Éİ’è‚µ TimerProc ‚Å WaitForSingleObject ‚Å‘Ò‚Â‚Æ‚©

*/
